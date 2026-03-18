#include "Core/EngineLogger.hpp"
#include "Platform/Thread/ConditionVariable.hpp"

#if defined(_WIN32) || defined(_WIN64)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

static_assert(sizeof(CONDITION_VARIABLE) <= sizeof(void*),
	"m_cv buffer too small for CONDITION_VARIABLE — increase buffer size in DConditionVariable.hpp");

// 内联取得平台类型指针，避免到处 reinterpret_cast
static CONDITION_VARIABLE* NativeCV(void* storage) {
	return reinterpret_cast<CONDITION_VARIABLE*>(storage);
}

ConditionVariable::ConditionVariable() {
	InitializeConditionVariable(NativeCV(&m_cv));
}

ConditionVariable::~ConditionVariable() {
	// CONDITION_VARIABLE 无需显式销毁
}

void ConditionVariable::Wait(Mutex& guard) {
	auto* cs = reinterpret_cast<CRITICAL_SECTION*>(guard.InternalData);
	// SleepConditionVariableCS 原子释放锁并等待，返回后重新持有锁
	// MutexGuard 的 _locked 状态不变，析构时正常 UnLock
	SleepConditionVariableCS(NativeCV(&m_cv), cs, INFINITE);
}

void ConditionVariable::NotifyOne() {
	WakeConditionVariable(NativeCV(&m_cv));
}

void ConditionVariable::NotifyAll() {
	WakeAllConditionVariable(NativeCV(&m_cv));
}

// ─────────────────────────────────────────────────────────────────────────────
// POSIX（Linux / macOS）
// ─────────────────────────────────────────────────────────────────────────────
#else

#include <pthread.h>

static_assert(sizeof(pthread_cond_t) <= 48,
	"m_cv buffer too small for pthread_cond_t — increase buffer size in DConditionVariable.hpp");

static pthread_cond_t* NativeCV(void* storage) {
	return reinterpret_cast<pthread_cond_t*>(storage);
}

ConditionVariable::ConditionVariable() {
	pthread_cond_init(NativeCV(&m_cv), nullptr);
}

ConditionVariable::~ConditionVariable() {
	pthread_cond_destroy(NativeCV(&m_cv));
}

void ConditionVariable::Wait(Mutex& mutex) {
	// 假设 Mutex::InternalData 是 pthread_mutex_t*（与现有 DMutex POSIX 实现一致）
	auto* mtx = reinterpret_cast<pthread_mutex_t*>(mutex.InternalData);
	pthread_cond_wait(NativeCV(&m_cv), mtx);
}

void ConditionVariable::NotifyOne() {
	pthread_cond_signal(NativeCV(&m_cv));
}

void ConditionVariable::NotifyAll() {
	pthread_cond_broadcast(NativeCV(&m_cv));
}

#endif