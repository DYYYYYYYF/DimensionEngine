#include "JobSystem.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"

#include <atomic>
#include <queue>
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

		std::priority_queue<QueueEntry> pq;
		Mutex                           mtx;
		ConditionVariable               cv;

		void Push(JobInfo job) {
			{
				MutexGuard guard(mtx);
				pq.push(QueueEntry{ std::move(job) });
			}
			// 只唤醒一个监听该队列的线程
			cv.NotifyOne();
		}

		// 阻塞直到取到任务或 shutdown
		// 返回 false = shutdown，线程应退出
		bool WaitAndPop(JobInfo& out, std::atomic<bool>& running) {
			mtx.Lock();

			while (running.load() && pq.empty()) {
				cv.Wait(mtx);
			}

			if (!running.load() && pq.empty()) {
				mtx.UnLock();
				return false;
			}

			out = std::move(const_cast<QueueEntry&>(pq.top()).job);
			pq.pop();

			mtx.UnLock();
			return true;
		}

		void WakeAll() {
			cv.NotifyAll();
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

	std::vector<Worker> workers;
	Mutex               result_mutex;
	std::vector<ResultEntry> pending_results;

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

	GLOG(Log::eInfo, "JobSystem shut down.");
}

// ─── Update ───────────────────────────────────────────────────────────────────

void JobSystem::Update() {
	if (!g_impl.running.load()) return;

	std::vector<JobSystemImpl::ResultEntry> frame_results;
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

// ─── Submit ───────────────────────────────────────────────────────────────────

void JobSystem::Submit(JobInfo info) {
	if (!g_impl.running.load()) {
		GLOG(Log::eWarn, "JobSystem::Submit: system not running.");
		return;
	}

	GLOG(Log::eInfo, "JobSystem::Submit: type=%u priority=%d",
		static_cast<uint32_t>(info.type), static_cast<int>(info.priority));

	g_impl.GetQueue(info.type).Push(std::move(info));
}