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

#ifndef META_INTERFACE_ITASKQUEUE_REGISTRY_H
#define META_INTERFACE_ITASKQUEUE_REGISTRY_H

#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/intf_meta_object_lib.h>
#include <meta/interface/intf_task_queue.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ITaskQueueRegistry, "9dd2907e-dc6c-4090-8873-fa84f367b87e")

/**
 * @brief The ITaskQueueRegistry interface can be used to register task queue objects which
 *        need to be used across the application.
 */
class ITaskQueueRegistry : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITaskQueueRegistry)
public:
    /**
     * @brief Get a task queue with a given queue id.
     * @param queueId The id of the queue to get.
     * @return The task queue with a given id or nullptr if no queue has been registered with the id.
     */
    virtual ITaskQueue::Ptr GetTaskQueue(const BASE_NS::Uid& queueId) const = 0;
    /**
     * @brief Register a task queue instance with a given id.
     * @param queue The task queue to register.
     *              If queue = {} this call is equivalent to calling UnregisterTaskQueue(queueId).
     * @param queueId Id to register the queue with.
     *                If a queue has already been registered with the given id, the previous registration
     *                is replaced with the new one.
     * @return True if registration was successful, false otherwise.
     */
    virtual bool RegisterTaskQueue(const ITaskQueue::Ptr& queue, const BASE_NS::Uid& queueId) = 0;
    /**
     * @brief Unregister a previous registered task queue.
     * @param queueId Id of the task queue to unregister.
     * @return True if a queue was unregistered, false otherwise.
     */
    virtual bool UnregisterTaskQueue(const BASE_NS::Uid& queueId) = 0;
    /**
     * @brief Check if a queue with a given id has been registered.
     * @param queueId Id of the queue to check.
     * @return True if a task queue with a given id has been registered, false otherwise.
     */
    virtual bool HasTaskQueue(const BASE_NS::Uid& queueId) const = 0;
    /**
     * @brief Unregisters all task queues.
     * @return True if task queue registry is empty after this call.
     */
    virtual bool UnregisterAllTaskQueues() = 0;
    /**
     * @brief If execution of current thread comes via task queue (i.e. executing a task), this returns that queue.
     * Note: Not all task queues have to implement this feature.
     * Note: This is single variable, if executing tasks in a task, one has to take care to reset it correctly.
     */
    virtual ITaskQueue::Ptr GetCurrentTaskQueue() const = 0;
    /**
     * @brief Sets the currently executing task queue. One should always set the queue to nullptr when not executing a
     * task.
     * @return The previously set task queue.
     */
    virtual ITaskQueue::WeakPtr SetCurrentTaskQueue(ITaskQueue::WeakPtr) = 0;
};

/** Returns a reference to the global ITaskQueueRegistry instance. */
inline META_NS::ITaskQueueRegistry& GetTaskQueueRegistry()
{
    return GetMetaObjectLib().GetTaskQueueRegistry();
}

META_END_NAMESPACE()

#endif // META_INTERFACE_ITASKQUEUE_REGISTRY_H
