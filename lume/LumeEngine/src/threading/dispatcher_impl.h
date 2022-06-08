/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_THREADING_DISPATHCER_IMPL_H
#define CORE_THREADING_DISPATHCER_IMPL_H

#include <base/containers/vector.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

#include "threading/dispatcher_task_queue.h"

CORE_BEGIN_NAMESPACE()
// Wrapper for DispatcherTaskQueue exposing a subset of the functionality.
class DispatcherImpl final : public IDispatcherTaskQueue {
public:
    ~DispatcherImpl();
    explicit DispatcherImpl(const IThreadPool::Ptr& threads);

    BASE_NS::vector<uint64_t> CollectFinishedTasks() override;

    void Submit(uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task) override;
    void SubmitAfter(uint64_t afterIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task) override;
    void Execute() override;
    void Clear() override;

protected:
    void Destroy() override;

private:
    DispatcherTaskQueue queue_;
};
CORE_END_NAMESPACE()

#endif // CORE_THREADING_DISPATHCER_IMPL_H
