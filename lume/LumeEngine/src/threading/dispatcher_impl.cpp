/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "threading/dispatcher_impl.h"

#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::vector;

DispatcherImpl::~DispatcherImpl() = default;

DispatcherImpl::DispatcherImpl(const IThreadPool::Ptr& threads) : queue_(threads) {}

vector<uint64_t> DispatcherImpl::CollectFinishedTasks()
{
    return queue_.CollectFinishedTasks();
}

void DispatcherImpl::Submit(uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    queue_.Submit(taskIdentifier, move(task));
}

void DispatcherImpl::SubmitAfter(uint64_t afterIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    queue_.SubmitAfter(afterIdentifier, taskIdentifier, move(task));
}

void DispatcherImpl::Execute()
{
    queue_.Execute();
}

void DispatcherImpl::Clear()
{
    queue_.Clear();
}

void DispatcherImpl::Destroy()
{
    delete this;
}
CORE_END_NAMESPACE()
