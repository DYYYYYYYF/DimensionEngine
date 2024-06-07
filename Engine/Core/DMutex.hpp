#pragma once

#include "Defines.hpp"

/**
 * A mutex to be used for synchronization purposes. A mutex (or mutual exclusion)
 * is used to limit access to a resource when there are multiple threads of 
 * execution around that resource.
 */
class DAPI Mutex {
public:
	Mutex() : InternalData(nullptr) {}

	/**
	 * Creates a mutex.
	 */
	bool Create();

	/**
	 * Destroys the mutex.
	 */
	void Destroy();

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