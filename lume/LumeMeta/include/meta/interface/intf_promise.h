/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Simple future interface for synchronisation
 * Author: Mikael Kilpeläinen
 * Create: 2023-02-14
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
    META_INTERFACE(CORE_NS::IInterface, IPromise)

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
