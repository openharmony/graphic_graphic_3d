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
