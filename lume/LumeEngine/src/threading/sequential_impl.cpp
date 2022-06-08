/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "threading/sequential_impl.h"

#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::vector;

SequentialImpl::~SequentialImpl() = default;

SequentialImpl::SequentialImpl(const IThreadPool::Ptr& threads) : queue_(threads) {}

void SequentialImpl::Submit(uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    queue_.Submit(taskIdentifier, move(task));
}

void SequentialImpl::SubmitAfter(uint64_t afterIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    queue_.SubmitAfter(afterIdentifier, taskIdentifier, move(task));
}

void SequentialImpl::Execute()
{
    queue_.Execute();
}

void SequentialImpl::Clear()
{
    queue_.Clear();
}

void SequentialImpl::Destroy()
{
    delete this;
}
CORE_END_NAMESPACE()
