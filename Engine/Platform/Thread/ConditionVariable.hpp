#pragma once

#include "Defines.hpp"
#include "DMutex.hpp"

/**
 * @brief 跨平台条件变量，值类型，与 Mutex/Thread 风格一致。
 * 内部通过 #ifdef 区分平台实现，对外接口统一。
 * 配合 Mutex 使用，Wait() 调用前必须已持有对应 mutex。
 */
class DAPI ConditionVariable {
public:
    ConditionVariable();
    ~ConditionVariable();

    // 禁止拷贝（与 Mutex/Thread 保持一致）
    ConditionVariable(const ConditionVariable&) = delete;
    ConditionVariable& operator=(const ConditionVariable&) = delete;

    /**
     * @brief 原子地释放 mutex 并阻塞等待，被唤醒后重新获取 mutex。
     * @param mutex 调用前必须已持有。
     */
    void Wait(Mutex& guard);

    /**
     * @brief 唤醒一个等待中的线程。
     */
    void NotifyOne();

    /**
     * @brief 唤醒所有等待中的线程。
     */
    void NotifyAll();

private:
#if defined(_WIN32) || defined(_WIN64)
    // CONDITION_VARIABLE 在 Win32 上等于一个指针大小，直接内嵌
    // 不引入 <Windows.h>，由 .cpp 的 static_assert 保证尺寸正确
    alignas(void*) unsigned char m_cv[sizeof(void*)];
#else
    // pthread_cond_t 在各平台最大约 48 字节
    // 由 .cpp 的 static_assert 保证尺寸正确
    alignas(void*) unsigned char m_cv[48];
#endif
};