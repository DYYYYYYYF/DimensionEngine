#include "Platform/Thread/DThread.hpp"

#if defined(DPLATFORM_WINDOWS)

#include "Core/EngineLogger.hpp"
#include <windows.h>

// NOTE: Begin Threads
bool Thread::Create(PFN_thread_start start_func, void* params, bool auto_detach) {
	if (!start_func) {
		return false;
	}

	InternalData = CreateThread(
		0,
		0,													// Default stack size
		(LPTHREAD_START_ROUTINE)start_func, params,			// Function ptr
		0,													// Params to pass to thread
		(DWORD*)&ThreadID);

	GLOG(Log::eDebug, "Starting process on thread id: %#x.", ThreadID);
	if (!InternalData) {
		return false;
	}

	if (auto_detach) {
		CloseHandle((HANDLE)InternalData);
	}

	return true;
}

void Thread::Destroy() {
	if (InternalData != nullptr) {
		DWORD ExitCode;
		GetExitCodeThread(InternalData, &ExitCode);
		//if (ExitCode == STILL_ACTIVE) {
		//	TerminateThread(InternalData, 0);	// 0 = failure
		//}
		CloseHandle((HANDLE)InternalData);
		InternalData = nullptr;
		ThreadID = 0;
	}
}

void Thread::Detach() {
	if (InternalData == nullptr) {
		return;
	}

	CloseHandle((HANDLE)InternalData);
	InternalData = nullptr;
}

void Thread::Cancel() {
	if (InternalData == nullptr) {
		return;
	}

	TerminateThread((HANDLE)InternalData, 0);
	InternalData = nullptr;
}

bool Thread::IsActive() const {
	if (InternalData == nullptr) {
		return false;
	}

	DWORD ExitCode = WaitForSingleObject((HANDLE)InternalData, 0);
	if (ExitCode == WAIT_TIMEOUT) {
		return true;
	}
	return false;
}

void Thread::Sleep(size_t ms) {
	Platform::PlatformSleep(ms);
}

size_t Thread::GetThreadID() {
	return (size_t)GetCurrentThreadId();
}
// NOTE: End Threads

#endif