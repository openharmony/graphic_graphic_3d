/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_THREADING_PARALLEL_IMPL_H
#define CORE_THREADING_PARALLEL_IMPL_H

#include <base/containers/vector.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

#include "threading/parallel_task_queue.h"

CORE_BEGIN_NAMESPACE()
// Wrapper for ParallelTaskQueue exposing a subset of the functionality.
class ParallelImpl final : public IParallelTaskQueue {
public:
    ~ParallelImpl();
    explicit ParallelImpl(const IThreadPool::Ptr& threads);

    void Submit(uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task) override;
    void SubmitAfter(uint64_t afterIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task) override;
    void Execute() override;
    void Clear() override;

protected:
    void Destroy() override;

private:
    ParallelTaskQueue queue_;
};
CORE_END_NAMESPACE()

#endif // CORE_THREADING_PARALLEL_IMPL_H
