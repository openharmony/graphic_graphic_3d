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
    void SubmitAfter(BASE_NS::array_view<const uint64_t> afterIdentifiers, uint64_t taskIdentifier,
        IThreadPool::ITask::Ptr&& task) override;
    void Execute() override;
    void Clear() override;

protected:
    void Destroy() override;

private:
    DispatcherTaskQueue queue_;
};
CORE_END_NAMESPACE()

#endif // CORE_THREADING_DISPATHCER_IMPL_H
