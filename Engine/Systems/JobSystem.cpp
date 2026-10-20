#include "JobSystem.hpp"
#include "Containers/TArray.hpp"
#include "Containers/TDeque.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"

#include <atomic>
#include <deque>
#include <vector>

// ─── 内部实现 ─────────────────────────────────────────────────────────────────

struct JobSystemImpl {

	// ── 单个类型的任务队列 ────────────────────────────────────────────────────

	struct QueueEntry {
		JobInfo job;
		bool operator<(const QueueEntry& o) const {
			return static_cast<int>(job.priority) < static_cast<int>(o.job.priority);
		}
	};

	struct TypedQueue {
		TypedQueue() = default;
		~TypedQueue() = default;

		TypedQueue(const TypedQueue&) = delete;
		TypedQueue& operator=(const TypedQueue&) = delete;

		// 单队列最大积压任务数：疯狂切场景/高频提交时，防止无上限堆积吃掉内存
		// （此前疯狂按 P 时 eResource_Load 队列 submit 187 vs 完成 105，积压持续增长）。
		static constexpr size_t kMaxPendingJobs = 128;

		// 按优先级从高到低排序的 pending 队列（同优先级保持插入序），
		// 使用 deque 以便在队列满时线性扫描并淘汰已取消的任务。
		TDeque<QueueEntry>  queue;
		Mutex                   mtx;
		ConditionVariable       cv;

		// 返回 true 表示已入队；false 表示队列已满（且无可淘汰的取消任务）被拒绝。
		// in_flight：入队成功时自增（去重合并不算新任务），由 worker 执行完/取消跳过时自减。
		bool Push(JobInfo job, std::atomic<uint32_t>& in_flight) {
			{
				MutexGuard guard(mtx);

				// 去重：同 dedup_key 且未取消的任务已在队列中时，本次提交合并（视为已受理），
				// 不重复入队。防止疯狂按 P 时同一纹理/资源被反复提交 load job 导致积压吃内存。
				if (!job.dedup_key.empty()) {
					for (const QueueEntry& it : queue) {
						if (it.job.dedup_key == job.dedup_key &&
							(!it.job.is_cancelled || !it.job.is_cancelled())) {
							GLOG(Log::eDebug, "JobSystem: dedup hit for key '%s', merged.", job.dedup_key.c_str());
							return true;
						}
					}
				}

				// 队列已满时：先扫描淘汰 is_cancelled 返回 true 的任务（如场景已切换、
				// Actor 已销毁的加载 job），为后续有效任务腾出空间。
				if (queue.size() >= kMaxPendingJobs) {
					EraseCancelledLocked();
				}

				if (queue.size() >= kMaxPendingJobs) {
					// 仍有空闲可腾、但队列确实满：拒绝本次提交，调用方优雅降级。
					GLOG(Log::eWarn, "JobSystem: queue(type=%u) full (%zu/%zu), submission rejected.",
						static_cast<uint32_t>(job.type), queue.size(), kMaxPendingJobs);
					return false;
				}

				QueueEntry entry{ std::move(job) };
				// 按优先级从高到低插入，保持同优先级 FIFO（稳定）。
				auto it = queue.begin();
				while (it != queue.end() &&
					static_cast<int>(it->job.priority) >= static_cast<int>(entry.job.priority)) {
					++it;
				}
				queue.insert(it, std::move(entry));
				in_flight.fetch_add(1, std::memory_order_release);
			}
			// 只唤醒一个监听该队列的线程
			cv.NotifyOne();
			return true;
		}

		// 阻塞直到取到任务或 shutdown
		// 返回 false = shutdown，线程应退出
		bool WaitAndPop(JobInfo& out, std::atomic<bool>& running) {
			mtx.Lock();

			while (running.load() && queue.empty()) {
				cv.Wait(mtx);
			}

			if (!running.load() && queue.empty()) {
				mtx.UnLock();
				return false;
			}

			out = std::move(queue.front().job);
			queue.pop_front();

			mtx.UnLock();
			return true;
		}

		void WakeAll() {
			cv.NotifyAll();
		}

	private:
		// 前提：持有 mtx。线性扫描并移除已取消任务，返回移除数量。
		size_t EraseCancelledLocked() {
			size_t removed = 0;
			for (auto it = queue.begin(); it != queue.end();) {
				if (it->job.is_cancelled && it->job.is_cancelled()) {
					it = queue.erase(it);
					++removed;
				}
				else {
					++it;
				}
			}
			if (removed > 0) {
				GLOG(Log::eInfo, "JobSystem: evicted %zu cancelled job(s) from queue (full).", removed);
			}
			return removed;
		}
	};

	// ── 工作线程 ──────────────────────────────────────────────────────────────

	struct Worker {
		Thread   thread;
		JobType  type = JobType::eGeneral;
		uint32_t index = 0;
	};

	// ── 完成回调条目（主线程执行）────────────────────────────────────────────

	struct ResultEntry {
		std::function<void()> callback;
	};

	// ── 成员 ──────────────────────────────────────────────────────────────────

	std::atomic<bool> running{ false };

	// 每种 JobType 独立队列，下标对应 JobType 的整数值
	TypedQueue queues[static_cast<uint32_t>(JobType::eCount)];

	// 每种 JobType 在途任务计数（已入队且尚未执行完/未取消跳过）：用于 WaitForIdle
	// 判断指定类型的异步加载是否真正收尾，避免只查队列空而漏掉 worker 正在执行的 job。
	std::atomic<uint32_t> in_flight[static_cast<uint32_t>(JobType::eCount)]{};

	TArray<Worker> workers;
	Mutex               result_mutex;
	TArray<ResultEntry> pending_results;

	TypedQueue& GetQueue(JobType type) {
		uint32_t idx = static_cast<uint32_t>(type);
		if (idx >= static_cast<uint32_t>(JobType::eCount)) {
			idx = static_cast<uint32_t>(JobType::eGeneral);
		}
		return queues[idx];
	}
};

static JobSystemImpl g_impl;

// ─── 工作线程函数 ─────────────────────────────────────────────────────────────

static uint32_t WorkerThreadFunc(void* param) {
	uint32_t worker_index = *reinterpret_cast<uint32_t*>(param);
	JobType  type = g_impl.workers[worker_index].type;
	uint32_t type_idx = static_cast<uint32_t>(type);

	GLOG(Log::eInfo, "Job thread #%u started (type=%u).", worker_index, type_idx);

	while (g_impl.running.load()) {
		JobInfo job;

		if (!g_impl.queues[type_idx].WaitAndPop(job, g_impl.running)) {
			break;
		}

		if (!job.entry) {
			continue;
		}

		// 取出后再次检查取消谓词：场景已切换/Actor 已销毁的加载 job 直接跳过，
		// 不执行 entry、不产生回调，避免白跑加载流程与无效回调。
		if (job.is_cancelled && job.is_cancelled()) {
			GLOG(Log::eDebug, "Job thread #%u: skipped cancelled job.", worker_index);
			g_impl.in_flight[type_idx].fetch_sub(1, std::memory_order_acq_rel);
			continue;
		}

		bool succeeded = false;
		try {
			succeeded = job.entry();
		}
		catch (...) {
			GLOG(Log::eError, "Job thread #%u: unhandled exception.", worker_index);
			succeeded = false;
		}

		auto& callback = succeeded ? job.on_success : job.on_failed;
		if (callback) {
			MutexGuard guard(g_impl.result_mutex);
			g_impl.pending_results.push_back({ std::move(callback) });
		}
		// 递减必须在回调入队之后，否则 WaitForIdle 可能在回调进入 pending_results 前
		// 误判空闲而提前返回（回调将在主线程 JobSystem::Update 中执行，此处只入队）。
		g_impl.in_flight[type_idx].fetch_sub(1, std::memory_order_acq_rel);
	}

	GLOG(Log::eInfo, "Job thread #%u exiting.", worker_index);
	return 0;
}

// ─── Initialize ───────────────────────────────────────────────────────────────

bool JobSystem::Initialize(const uint32_t* thread_count_per_type) {
	if (g_impl.running.load()) {
		GLOG(Log::eWarn, "JobSystem::Initialize: already running.");
		return false;
	}

	// 每种类型的默认线程数
	const uint32_t type_count = static_cast<uint32_t>(JobType::eCount);

	uint32_t counts[static_cast<uint32_t>(JobType::eCount)];
	if (thread_count_per_type) {
		for (uint32_t i = 0; i < type_count; ++i) {
			counts[i] = thread_count_per_type[i] > 0 ? thread_count_per_type[i] : 1;
		}
	}
	else {
		// 默认：eGeneral = 核心数-1（最少1），其余各1个
		uint32_t hw = (uint32_t)Platform::GetProcessorCount();
		uint32_t general = (hw > type_count) ? (hw - type_count) : 1;

		for (uint32_t i = 0; i < type_count; ++i) {
			counts[i] = (i == static_cast<uint32_t>(JobType::eGeneral)) ? general : 1;
		}
	}

	// 计算总线程数并预分配，确保 resize 只做一次，之后内存不再移动
	uint32_t total = 0;
	for (uint32_t i = 0; i < type_count; ++i) total += counts[i];
	g_impl.workers.resize(total);

	// 配置所有 worker（不启动线程）
	uint32_t worker_idx = 0;
	for (uint32_t type_idx = 0; type_idx < type_count; ++type_idx) {
		for (uint32_t j = 0; j < counts[type_idx]; ++j) {
			g_impl.workers[worker_idx].index = worker_idx;
			g_impl.workers[worker_idx].type = static_cast<JobType>(type_idx);
			++worker_idx;
		}
	}

	// 配置完成后再设置 running，确保线程启动时状态正确
	g_impl.running.store(true);

	// 统一启动所有线程
	for (uint32_t i = 0; i < total; ++i) {
		if (!g_impl.workers[i].thread.Create(WorkerThreadFunc, &g_impl.workers[i].index, false)) {
			GLOG(Log::eFatal, "JobSystem: failed to create worker thread #%u.", i);
			Shutdown();
			return false;
		}
		GLOG(Log::eInfo, "Job thread #%u created (type=%u).",
			i, static_cast<uint32_t>(g_impl.workers[i].type));
	}

	GLOG(Log::eInfo, "JobSystem initialized: %u total threads.", total);
	for (uint32_t i = 0; i < type_count; ++i) {
		GLOG(Log::eInfo, "  Type %u: %u thread(s).", i, counts[i]);
	}

	return true;
}

// ─── Shutdown ─────────────────────────────────────────────────────────────────

void JobSystem::Shutdown() {
	if (!g_impl.running.load()) return;

	g_impl.running.store(false);

	// 唤醒所有队列的所有等待线程
	for (uint32_t i = 0; i < static_cast<uint32_t>(JobType::eCount); ++i) {
		g_impl.queues[i].WakeAll();
	}

	for (auto& w : g_impl.workers) {
		w.thread.Destroy();
	}

	g_impl.workers.clear();

	{
		MutexGuard guard(g_impl.result_mutex);
		g_impl.pending_results.clear();
	}

	// 清零在途计数：残留未执行的 job 已随队列丢弃，避免后续误判
	for (uint32_t i = 0; i < static_cast<uint32_t>(JobType::eCount); ++i) {
		g_impl.in_flight[i].store(0, std::memory_order_release);
	}

	GLOG(Log::eInfo, "JobSystem shut down.");
}

// ─── Update ───────────────────────────────────────────────────────────────────

void JobSystem::Update() {
	if (!g_impl.running.load()) return;

	TArray<JobSystemImpl::ResultEntry> frame_results;
	{
		MutexGuard guard(g_impl.result_mutex);
		frame_results.swap(g_impl.pending_results);
	}

	for (auto& entry : frame_results) {
		try {
			entry.callback();
		}
		catch (...) {
			GLOG(Log::eError, "JobSystem::Update: exception in result callback.");
		}
	}
}

// ─── WaitForIdle ──────────────────────────────────────────────────────────────

void JobSystem::WaitForIdle(JobType type) {
	if (!g_impl.running.load()) return;

	uint32_t type_idx = static_cast<uint32_t>(type);
	if (type_idx >= static_cast<uint32_t>(JobType::eCount)) {
		type_idx = static_cast<uint32_t>(JobType::eGeneral);
	}

	// 主线程自旋：等待指定类型的在途任务数归零，并同步执行已完成 job 的完成回调
	// （on_success/on_failed 必须在主线程执行，否则场景清理后回调悬挂）。
	// 退出条件：in_flight 为 0（队列为空 + worker 无正在执行）且 pending_results 全部执行完。
	for (;;) {
		if (g_impl.in_flight[type_idx].load(std::memory_order_acquire) == 0) {
			bool results_done = false;
			{
				MutexGuard guard(g_impl.result_mutex);
				results_done = g_impl.pending_results.empty();
			}
			if (results_done) break;
		}
		JobSystem::Update();
		Platform::PlatformSleep(1);
	}
}

// ─── Submit ───────────────────────────────────────────────────────────────────

bool JobSystem::Submit(JobInfo info) {
	if (!g_impl.running.load()) {
		GLOG(Log::eWarn, "JobSystem::Submit: system not running.");
		return false;
	}

	GLOG(Log::eInfo, "JobSystem::Submit: type=%u priority=%d",
		static_cast<uint32_t>(info.type), static_cast<int>(info.priority));

	uint32_t type_idx = static_cast<uint32_t>(info.type);
	if (type_idx >= static_cast<uint32_t>(JobType::eCount)) {
		type_idx = static_cast<uint32_t>(JobType::eGeneral);
	}

	return g_impl.GetQueue(info.type).Push(std::move(info), g_impl.in_flight[type_idx]);
}