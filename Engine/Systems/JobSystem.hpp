#pragma once

#include "Defines.hpp"
#include "Platform/Thread/DThread.hpp"
#include "Platform/Thread/DMutex.hpp"
#include "Platform/Thread/ConditionVariable.hpp"

#include <functional>

enum class JobType : uint32_t {
    eGeneral = 0,
    eResource_Load = 1,
    eGPU_Resource = 2,
    eCount = 3   // 队列数量，新增类型时同步修改
};

enum class JobPriority : int {
    eLow = 0,
    eNormal = 1,
    eHigh = 2
};

//
// 所有参数与结果通过闭包捕获，无需 void* 传参。
//
struct JobInfo {
    std::function<bool()> entry;
    std::function<void()> on_success = nullptr;
    std::function<void()> on_failed = nullptr;
    JobType               type = JobType::eGeneral;
    JobPriority           priority = JobPriority::eNormal;
};

class JobSystem {
public:
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    /**
     * @brief 初始化任务系统。
     * @param thread_count_per_type 每种 JobType 分配的线程数量数组，长度必须为 JobType::eCount。
     *                              nullptr 则每种类型各分配 1 个线程。
     */
    static DAPI bool Initialize(const uint32_t* thread_count_per_type = nullptr);
    static DAPI void Shutdown();

    /**
     * @brief 每帧在主线程调用，执行 on_success / on_failed 回调。
     */
    static DAPI void Update();

    /**
     * @brief 提交任务到对应 JobType 的队列（线程安全）。
     */
    static DAPI void Submit(JobInfo info);
};