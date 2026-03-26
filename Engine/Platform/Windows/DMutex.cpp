#include "Platform/Thread/DMutex.hpp"

#if defined(DPLATFORM_WINDOWS)
#include <windows.h>

Mutex::Mutex() {
	InternalData = new CRITICAL_SECTION();
	InitializeCriticalSection((CRITICAL_SECTION*)InternalData);
}

Mutex::~Mutex() {
	if (!InternalData) return;
	DeleteCriticalSection((CRITICAL_SECTION*)InternalData);
	delete (CRITICAL_SECTION*)InternalData;
	InternalData = nullptr;
}

bool Mutex::Lock() {
	if (!InternalData) return false;
	EnterCriticalSection((CRITICAL_SECTION*)InternalData);
	return true;
}

bool Mutex::UnLock() {
	if (!InternalData) return false;
	LeaveCriticalSection((CRITICAL_SECTION*)InternalData);
	return true;
}

#endif
