#pragma once

#include "Defines.hpp"

#include "Core/DThread.hpp"
#include "Core/DMutex.hpp"
#include "Core/DMemory.hpp"

#include <queue>
#include <functional>

#define MAX_JOB_RESULTS 512

typedef std::function<bool(void*, void*)> PFN_OnJobStart;
typedef std::function<void(void*)> PFN_OnJobComplete;

//typedef bool(*PFN_OnJobStart)(void*, void*);
//typedef void(*PFN_OnJobComplete)(void*);

/**
 * @brief Describes a type of job.
 */
enum class JobType : char{
	/**
	* @brief A general job that does not have any specific thread requirements.
	* This means it matters little which job thread this job runs on.
	*/
	eGeneral = 0x02,

	/**
	* @brief A resource loading job. Resources should always load on the same thread
	* to avoid potential disk thrashing.
	*/
	eResource_Load = 0x04,

	/**
	* @brief Jobs using GPU resources should be bound to a thread using this job type.
	* Multithread renderers will use a specific job thread, and this type of job will
	* run on that thread.
	* For single-threaded renderers, this will be on the main thread.
	*/
	eGPU_Resource = 0x08
};

/**
 * @brief Determines which job queue a job uses. The high-priority queue is always exhausted
 * first before processing the normal-priority queue, which must also be exhausted before
 * processing the low-priority queue.
 */
enum class JobPriority : char{
	eLow,
	eNormal,
	eHigh
};

struct JobInfo {
public:
	JobInfo() : entry_point(nullptr), on_success(nullptr), on_failed(nullptr), param_data(nullptr), result_data(nullptr) {}
	JobInfo(const JobInfo& info) {
		entry_point = nullptr;
		on_success = nullptr;
		on_failed = nullptr;

		if (info.entry_point) {
			entry_point = info.entry_point;
		}
		if (info.on_success) {
			on_success = info.on_success;
		}
		if (info.on_failed) {
			on_failed = info.on_failed;
		}

		type = info.type;
		priority = info.priority;
		param_data_size = info.param_data_size;
		result_data_size = info.result_data_size;

		if (info.param_data) {
			param_data = Memory::Allocate(param_data_size, MemoryType::eMemory_Type_Job);
			Memory::Copy(param_data, info.param_data, param_data_size);
		}
		if (info.result_data) {
			result_data = Memory::Allocate(result_data_size, MemoryType::eMemory_Type_Job);
			Memory::Copy(result_data, info.result_data, result_data_size);
		}
	}

	void Release() {
		if (param_data) {
			Memory::Free(param_data, param_data_size, MemoryType::eMemory_Type_Job);
			param_data = nullptr;
            param_data_size = 0;
		}
		if (result_data) {
			Memory::Free(result_data, result_data_size, MemoryType::eMemory_Type_Job);
			result_data = nullptr;
            result_data_size = 0;
		}

		entry_point = nullptr;
		on_success = nullptr;
		on_failed = nullptr;
	}

	JobType type = JobType::eGeneral;
	JobPriority priority = JobPriority::eNormal;
	PFN_OnJobStart entry_point = nullptr;
	PFN_OnJobComplete on_success = nullptr;
	PFN_OnJobComplete on_failed = nullptr;

	void* param_data = nullptr;
	size_t param_data_size = 0;

	void* result_data = nullptr;
	size_t result_data_size = 0;
};

struct JobThread {
	unsigned char index;
	Thread thread;
	JobInfo info;
	// A mutex to guard access to this thread's info.
	Mutex info_mutex;
	// The types of jobs this thread can handle.
	uint32_t type_mask;
};

struct JobResultEntry {
	unsigned short id;
	PFN_OnJobComplete callback = nullptr;
	uint32_t param_size = 0;
	void* params = nullptr;
};

class JobSystem {
public:
	static bool Initialize(unsigned char job_thread_count, unsigned int type_masks[]);
	static void Shutdown();

	/**
	 * @brief Updates the job system. Should happen once an update cycle.
	 */
	static void Update();

	/**
	 * @brief Submits the provided job to the queued for execution.
	 * @param info The description of the job to be executed.
	 */
	static DAPI void Submit(JobInfo info);

	/**
	 * @brief Creates a new job with default type
	 */
	static DAPI JobInfo CreateJob(PFN_OnJobStart entry, PFN_OnJobComplete on_success, PFN_OnJobComplete on_failed, 
		void* param_data, size_t param_data_size, size_t result_data_size,
		JobType type = JobType::eGeneral, JobPriority priority = JobPriority::eNormal);

private:
	static void StoreResult(PFN_OnJobComplete callback, void* params, size_t param_size);
	static uint32_t RunJobThread(void* params);
	static void ProcessQueue(std::queue<JobInfo>& queue, Mutex* queue_mutex);

private:
	static bool IsRunning;
	static unsigned char ThreadCount;
	static JobThread JobThreads[32];
	
	static std::queue<JobInfo> LowPriorityQueue;
	static std::queue<JobInfo> NormalPriorityQueue;
	static std::queue<JobInfo> HighPriorityQueue;
	
	// Mutexes for each queue, since a job could be kicked off from another job (thread).
	static Mutex LowPriQueueMutex;
	static Mutex NormalPriQueueMutex;
	static Mutex HighPriQueueMutex;
	
	static JobResultEntry PendingResults[MAX_JOB_RESULTS];
	static Mutex ResultMutex;
};
