#include "JobSystem.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

bool JobSystem::IsRunning = false;
unsigned char JobSystem::ThreadCount;
JobThread JobSystem::JobThreads[32];
std::queue<JobInfo> JobSystem::LowPriorityQueue;
std::queue<JobInfo> JobSystem::NormalPriorityQueue;
std::queue<JobInfo> JobSystem::HighPriorityQueue;
Mutex JobSystem::LowPriQueueMutex;
Mutex JobSystem::NormalPriQueueMutex;
Mutex JobSystem::HighPriQueueMutex;
JobResultEntry JobSystem::PendingResults[MAX_JOB_RESULTS];
Mutex JobSystem::ResultMutex;

void JobSystem::StoreResult(PFN_OnJobComplete callback, void* params, size_t param_size) {
	// Create the new entry.
	JobResultEntry Entry;
	Entry.id = INVALID_ID_U16;
	Entry.param_size = (uint32_t)param_size;
	Entry.callback = std::move(callback);

	if (Entry.param_size > 0) {
		// Take a copy, as the job is destroyed after this.
		Entry.params = Memory::Allocate(param_size, MemoryType::eMemory_Type_Job);
		Memory::Copy(Entry.params, params, param_size);
	}
	else {
		Entry.params = nullptr;
	}

	// Lock, find a free space, store, unlock.
	if (!ResultMutex.Lock()) {
		LOG_ERROR("Failed to obtain mutex lock for storing a result! Result storage may be corrupted.");
	}

	for (unsigned short i = 0; i < MAX_JOB_RESULTS; ++i) {
		if (PendingResults[i].id == INVALID_ID_U16) {
			PendingResults[i] = Entry;
			PendingResults[i].id = i;
			break;
		}
	}

	if (!ResultMutex.UnLock()) {
		LOG_ERROR("Failed to release mutex lock for result storage, storage may be corrupted.");
	}
}

uint32_t JobSystem::RunJobThread(void* param) {
	uint32_t index = *(uint32_t*)param;
	JobThread* Thr = &JobThreads[index];
	size_t ThreadID = Thr->thread.ThreadID;
	LOG_INFO("Starting job thread #%i (id=%#x, type=%#x).", Thr->index, ThreadID, Thr->type_mask);

	// A mutex to lock info for this thread.
	if (!Thr->info_mutex.Create()) {
		LOG_ERROR("Failed to create job thread mutex! Aborting thread.");
		return 0;
	}

	// Run forever, waiting for jobs,
	while (true) {
		if (!IsRunning || !Thr) {
			break;
		}

		// Lock and grab a copy of info.
 		if (!Thr->info_mutex.Lock()) {
			LOG_ERROR("Failed to obtain lock on job thread mutex!");
			break;
		}

		JobInfo info = Thr->info;
		if (!Thr->info_mutex.UnLock()) {
			LOG_ERROR("Failed to release lock on job thread mutex!");
			break;
		}

		if (info.entry_point) {
			bool Result = info.entry_point(info.param_data, info.result_data);

			// Store the result to be executed on the main thread later.
			// Note that store_result takes a copy of the result_data so it does
			// not have to be held onto by this thread any longer.
			if (Result && info.on_success) {
				StoreResult(info.on_success, info.result_data, info.result_data_size);
			}
			else if (!Result && info.on_failed) {
				StoreResult(info.on_failed, info.result_data, info.result_data_size);
			}

			// Clear the param data and result data.
			if (info.param_data) {
				Memory::Free(info.param_data, info.param_data_size, MemoryType::eMemory_Type_Job);
			}
			if (info.result_data) {
				Memory::Free(info.result_data, info.result_data_size, MemoryType::eMemory_Type_Job);
			}

			// Lock and reset the thread's info object.
			if (!Thr->info_mutex.Lock()) {
				LOG_ERROR("Failed to obtain lock on job thread mutex!");
				break;
			}
			Thr->info.Release();
			Memory::Zero(&Thr->info, sizeof(JobInfo));
			if (!Thr->info_mutex.UnLock()) {
				LOG_ERROR("Failed to release lock on job thread mutex!");
				break;
			}
		}

		if (IsRunning) {
			// TODO: Should probably find a better way to do this. such as sleeping until
			// a request comes through for a new job.
			Thr->thread.Sleep(10);
		}
		else {
			break;
		}
	}

	// Destroy the mutex for this thread.
	Thr->info_mutex.Destroy();
	return 1;
}

bool JobSystem::Initialize(unsigned char job_thread_count, unsigned int type_masks[]) {
	IsRunning = true;
	ThreadCount = job_thread_count;

	// Invalidate all result slots.
	for (unsigned short i = 0; i < MAX_JOB_RESULTS; ++i) {
		PendingResults[i].id = INVALID_ID_U16;
	}

	LOG_INFO("Main thread id is: %#x.", Thread::GetThreadID());
	LOG_DEBUG("Spawning %i job threads.", ThreadCount);

	for (unsigned char i = 0; i < ThreadCount; ++i) {
		JobThreads[i].index = i;
		JobThreads[i].type_mask = type_masks[i];
		if (!JobThreads[i].thread.Create(RunJobThread, &JobThreads[i].index, false)) {
			LOG_FATAL("OS Error in creating job thread. Application cannot continue.");
			return false;
		}

		Memory::Zero(&JobThreads[i].info, sizeof(JobInfo));
	}

	// Create needed mutexes.
	if (!ResultMutex.Create()) {
		LOG_ERROR("Failed to create result mutex!");
		return false;
	}
	if (!LowPriQueueMutex.Create()) {
		LOG_ERROR("Failed to create low priority queue mutex!");
		return false;
	}
	if (!NormalPriQueueMutex.Create()) {
		LOG_ERROR("Failed to create normal priority queue mutex!");
		return false;
	}
	if (!HighPriQueueMutex.Create()) {
		LOG_ERROR("Failed to create high priority queue mutex!");
		return false;
	}

	return true;
}

void JobSystem::Shutdown() {
	if (!IsRunning) {
		return;
	}

	IsRunning = false;

	// Check for a free thread first.
	for (unsigned char i = 0; i < ThreadCount; ++i) {
		JobThreads[i].thread.Destroy();
	}

	for (uint32_t i = 0; i < LowPriorityQueue.size(); ++i) {
		JobInfo* Temp = nullptr;
		LowPriorityQueue.pop();
        if (Temp != nullptr){
            Temp->Release();
            Temp = nullptr;
        }

		NormalPriorityQueue.pop();
        if (Temp != nullptr){
            Temp->Release();
            Temp = nullptr;
        }
        
		HighPriorityQueue.pop();
        if (Temp != nullptr){
            Temp->Release();
            Temp = nullptr;
        }
	}

	// Destroy mutexes
	ResultMutex.Destroy();
	LowPriQueueMutex.Destroy();
	NormalPriQueueMutex.Destroy();
	HighPriQueueMutex.Destroy();
}

void JobSystem::ProcessQueue(std::queue<JobInfo>& queue, Mutex* queue_mutex) {
	// Check for a free thread first.
	while (queue.size() > 0) {
		JobInfo info = queue.front();
		bool ThreadFound = false;
		for (unsigned char i = 0; i < ThreadCount; ++i) {
			JobThread* thread = &JobThreads[i];
			if ((thread->type_mask & (uint32_t)info.type) == 0) {
				continue;
			}

			// Check that the job thread can handle the job type.
			if (!thread->info_mutex.Lock()) {
				LOG_ERROR("Failed to obtain lock on job thread mutex!");
			}

			if (!thread->info.entry_point) {
				// Make sure to remove the entry from the queue.
				if (!queue_mutex->Lock()) {
					LOG_ERROR("Failed to obtain lock on queue mutex!");
				}
				queue.pop();
				if (!queue_mutex->UnLock()) {
					LOG_ERROR("Failed to release lock on queue mutex!");
				}

				thread->info = info;
				ThreadFound = true;
			}

			if (!thread->info_mutex.UnLock()) {
				LOG_ERROR("Failed to release lock on thread mutex!");
			}

			// Break after unlocking if an available thread was found.
			if (ThreadFound) {
				break;
			}
		}

		// This means all of the threads are currently handling a job.
		// So wait until the next update and try again.
		if (!ThreadFound) {
			break;
		}
	}
}

void JobSystem::Update() {
	if (!IsRunning) {
		return;
	}

	ProcessQueue(HighPriorityQueue, &HighPriQueueMutex);
	ProcessQueue(NormalPriorityQueue, &NormalPriQueueMutex);
	ProcessQueue(LowPriorityQueue, &LowPriQueueMutex);

	// Process pending results.
	for (unsigned short i = 0; i < MAX_JOB_RESULTS; ++i) {
		// Lock and take a copy, unlock.
		if (!ResultMutex.Lock()) {
			LOG_ERROR("Failed to obtain lock on result mutex!");
		}

		JobResultEntry Entry = PendingResults[i];
		if (!ResultMutex.UnLock()) {
			LOG_ERROR("Failed to release lock on result mutex!");
		}

		if (Entry.id != INVALID_ID_U16) {
			// Execute the callback.
			Entry.callback(Entry.params);

			if (Entry.params) {
				Memory::Free(Entry.params, Entry.param_size, MemoryType::eMemory_Type_Job);
			}

			// Lock actual entry, invalidate and clear it
			if (!ResultMutex.Lock()) {
				LOG_ERROR("Failed to obtain lock on result mutex!");
			}
			Memory::Zero(&PendingResults[i], sizeof(JobResultEntry));
			PendingResults[i].id = INVALID_ID_U16;
			if (!ResultMutex.UnLock()) {
				LOG_ERROR("Failed to release lock on result mutex!");
			}
		}
	}
}

void JobSystem::Submit(JobInfo info) {
	std::queue<JobInfo>* Queue = &NormalPriorityQueue;
	Mutex* QueueMutex = &NormalPriQueueMutex;

	// If the job is high priority, try to kick it off immediately.
	if (info.priority == JobPriority::eHigh) {
		Queue = &HighPriorityQueue;
		QueueMutex = &HighPriQueueMutex;

		// Check for a free thread that supports the job type first.
		for (unsigned char i = 0; i < ThreadCount; ++i) {
			JobThread* thread = &JobThreads[i];
			if (thread->type_mask & (uint32_t)info.type) {
				bool Found = false;
				if (!thread->info_mutex.Lock()) {
					LOG_ERROR("Failed to obtain lock on job thread mutex!");
				}
				if (!thread->info.entry_point) {
					LOG_INFO("Job immediately submitted on thread %i.", thread->index);
					thread->info = info;
					Found = true;
				}
				if (!thread->info_mutex.UnLock()) {
					LOG_ERROR("Failed to release lock on job thread mutex!");
				}
				if (Found) {
					return;
				}
			}
		}
	}

	// If this point is reached, all threads are busy.
	// Add to the queue and try again next cycle.
	if (info.priority == JobPriority::eLow) {
		Queue = &LowPriorityQueue;
		QueueMutex = &LowPriQueueMutex;
	}

	// NOTO: Locking here in case the job is submitted from another thread.
	if (!QueueMutex->Lock()) {
		LOG_ERROR("Failed to obtain lock on queue mutex!");
	}
	Queue->push(info);
	if (!QueueMutex->UnLock()) {
		LOG_ERROR("Failed to release lock on queue mutex!");
	}
}

JobInfo JobSystem::CreateJob(PFN_OnJobStart entry, PFN_OnJobComplete on_success, PFN_OnJobComplete on_failed,
	void* param_data, size_t param_data_size, size_t result_data_size, JobType type, JobPriority priority) {
	JobInfo Job;
	Job.entry_point = std::move(entry);
	Job.on_success = std::move(on_success);
	Job.on_failed = std::move(on_failed);
	Job.type = type;
	Job.priority = priority;

	Job.param_data_size = param_data_size;
	if (param_data_size > 0) {
		Job.param_data = Memory::Allocate(param_data_size, MemoryType::eMemory_Type_Job);
		Memory::Copy(Job.param_data, param_data, param_data_size);
	}
	else {
		Job.param_data = nullptr;
	}

	Job.result_data_size = result_data_size;
	if (result_data_size > 0) {
		Job.result_data = Memory::Allocate(result_data_size, MemoryType::eMemory_Type_Job);
	}
	else {
		Job.result_data = nullptr;
	}

	return Job;
}
