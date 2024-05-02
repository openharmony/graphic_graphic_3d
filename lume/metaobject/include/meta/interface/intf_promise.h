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
#ifndef META_INTERFACE_IPROMISE_H
#define META_INTERFACE_IPROMISE_H

#include <meta/base/interface_macros.h>
#include <meta/base/time_span.h>
#include <meta/base/types.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_callable.h>
#include <meta/interface/intf_task_queue.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Promise to fulfill future
 */
class IPromise : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPromise);

public:
    /**
     * @brief This will release waiting on associated future and sets the state to COMPLETED
     */
    virtual void Set(const IAny::Ptr&) = 0;
    /**
     * @brief This will release waiting on associated future and sets the state to ABANDONED
     */
    virtual void SetAbandoned() = 0;
    /**
     * @brief Get associated future
     */
    virtual IFuture::Ptr GetFuture() = 0;
    /**
     * @brief Set Executing task token and queue
     */
    virtual void SetQueueInfo(const ITaskQueue::Ptr& queue, ITaskQueue::Token token) = 0;
};

/** Basic Promise implementations */
META_REGISTER_CLASS(Promise, "664797ae-14be-481a-a124-2da82593edcd", ObjectCategoryBits::NO_CATEGORY)

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IPromise)

#endif
