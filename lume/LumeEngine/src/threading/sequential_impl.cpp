/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "threading/sequential_impl.h"

#include <base/containers/array_view.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
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

void SequentialImpl::SubmitAfter(
    array_view<const uint64_t> afterIdentifiers, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    queue_.SubmitAfter(afterIdentifiers, taskIdentifier, move(task));
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
