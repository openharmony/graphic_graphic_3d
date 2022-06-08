/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "threading/parallel_impl.h"

#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::vector;

ParallelImpl::~ParallelImpl() = default;

ParallelImpl::ParallelImpl(const IThreadPool::Ptr& threads) : queue_(threads) {}

void ParallelImpl::Submit(uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    queue_.Submit(taskIdentifier, move(task));
}

void ParallelImpl::SubmitAfter(uint64_t afterIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    queue_.SubmitAfter(afterIdentifier, taskIdentifier, move(task));
}

void ParallelImpl::Execute()
{
    queue_.Execute();
}

void ParallelImpl::Clear()
{
    queue_.Clear();
}

void ParallelImpl::Destroy()
{
    delete this;
}
CORE_END_NAMESPACE()
