#pragma once

#include "Defines.hpp"

typedef unsigned int(*PFN_thread_start)(void*);

/**
 * Represents a process thread in the system to be used for work.
 * Generally should not be created directly in user code.
 * This calls to the platform-spectific thread implementation.
 */
class Thread {
public:
	Thread() : InternalData(nullptr), ThreadID(0) {}

	/**
	 * @brief Creates a new thread, Immediately calling the function pointed to.
	 * @param start_func The function to be invoked immediately. Required.
	 * @param params A pointer to any data to be passed to the start.
	 * @param auto_detach Indicates if the thread should immediately release its resources when the work is complete.
	 */
	bool Create(PFN_thread_start start_func, void* params, bool auto_detach);

	/**
	 * Destroys the thread.
	 */
	void Destroy();
	
	/**
	 * Detaches the thread, automatically releasing resources when work is complete.
	 */
	void Detach();

	/**
	 * Cancels work on the thread, if possible, and releases resources when possible.
	 */
	void Cancel();

	/**
	 * Indicates if the thread is currently active.
	 * @return True if active.
	 */
	bool IsActive() const;

	/**
	 * Sleeps on the thread for a given number of milliseconds. Should be called from the
	 * thread requiring the sleep.
	 * @param ms The sleep time in ms.
	 */
	void Sleep(size_t ms);

	static size_t GetThreadID();

public:
	size_t ThreadID;
	void* InternalData;
};