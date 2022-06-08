/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_THREADING_TASK_QUEUE_FACTORY_H
#define CORE_THREADING_TASK_QUEUE_FACTORY_H

#include <core/threading/intf_thread_pool.h>

CORE_BEGIN_NAMESPACE()
class TaskQueueFactory final : public ITaskQueueFactory {
public:
    ~TaskQueueFactory() = default;

    uint32_t GetNumberOfCores() const override;

    IThreadPool::Ptr CreateThreadPool(const uint32_t threadCountconst) const override;

    IDispatcherTaskQueue::Ptr CreateDispatcherTaskQueue(const IThreadPool::Ptr& threadPool) const override;
    IParallelTaskQueue::Ptr CreateParallelTaskQueue(const IThreadPool::Ptr& threadPool) const override;
    ISequentialTaskQueue::Ptr CreateSequentialTaskQueue(const IThreadPool::Ptr& threadPool) const override;

    // IInterface
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;
};
CORE_END_NAMESPACE()

#endif // CORE_THREADING_TASK_QUEUE_FACTORY_H
