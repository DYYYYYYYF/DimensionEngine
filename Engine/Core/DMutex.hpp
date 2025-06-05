#pragma once

#include "Defines.hpp"

/**
 * A mutex to be used for synchronization purposes. A mutex (or mutual exclusion)
 * is used to limit access to a resource when there are multiple threads of 
 * execution around that resource.
 */
class DAPI Mutex {
public:
	Mutex();
	virtual ~Mutex();

	/**
	 * Creates a mutex lock.
	 * @returns True if locked successfully.
	 */
	bool Lock();

	/**
	 * Unlock the mutex.
	 * @returns True if unlocked successfully.
	 */
	bool UnLock();

public:
	void* InternalData;
};

class MutexGuard {
public:
	explicit MutexGuard(Mutex& mutex)
		: _mutex(mutex), _locked(false)
	{
		_locked = _mutex.Lock();
	}

	~MutexGuard() {
		if (_locked) {
			_mutex.UnLock();
		}
	}

	// 禁止拷贝构造和赋值，防止资源重复释放
	MutexGuard(const MutexGuard&) = delete;
	MutexGuard& operator=(const MutexGuard&) = delete;

private:
	Mutex& _mutex;
	bool _locked;
};