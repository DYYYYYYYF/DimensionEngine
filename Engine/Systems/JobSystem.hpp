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
    // 可选取消谓词：worker 取出任务后若返回 true 则直接跳过（不执行 entry、不产生回调）；
    // 队列已满时也优先扫描淘汰这些任务，为有效任务腾出空间。谓词必须线程安全（只读原子状态）。
    std::function<bool()> is_cancelled = nullptr;
    JobType               type = JobType::eGeneral;
    JobPriority           priority = JobPriority::eNormal;
    // 可选去重键：非空时，若队列中已存在相同键且未取消的任务，本次提交视为已受理（不重复入队）。
    // 用于防止高频场景切换时对同一资源（如纹理名）重复提交 load job 造成队列无限积压吃内存。
    std::string           dedup_key;
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
     * @brief 阻塞主线程直到指定类型的队列清空且所有完成回调执行完毕。
     *        用于场景切换等时机：先等上一批异步资源加载收尾，再创建新资源提交新 job，
     *        避免高频切换（如疯狂按 P）时 load job 积压吃内存导致分配器溢出崩溃。
     * @param type 要等待的 JobType。
     */
    static DAPI void WaitForIdle(JobType type);

    /**
     * @brief 提交任务到对应 JobType 的队列（线程安全）。
     * @return true 已入队；false 队列已满（已尽力淘汰无效任务后仍无空位）或系统未运行。
     */
    static DAPI bool Submit(JobInfo info);
};