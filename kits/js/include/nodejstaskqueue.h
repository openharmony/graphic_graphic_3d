/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef NODEJSTASKQUEUE_H
#define NODEJSTASKQUEUE_H
#include <meta/interface/intf_task_queue.h>

#include "napi_api.h"

/*
 * NodeJSTaskQueue identifier
 */
static constexpr BASE_NS::Uid JS_THREAD_DEP { "b2e8cef3-453a-4651-b564-5190f8b5190d" };

/*
 * NodeJSTaskQueue executes it's tasks in the thread where it was first initialized, which MUST have a valid napi_env.
 * napi_env must be passed to the instance in a "Create" parameter block, in a parameter "env" of type "uintptr_t"
 *
 * See INodeJSTaskQueue for more information.
 */
META_REGISTER_CLASS(NodeJSTaskQueue, "66aabbba-ec01-47f8-ac68-2393cfc76a9a", META_NS::ObjectCategoryBits::NO_CATEGORY)

/*
 * INodeJSTaskQueue can be used by tasks in NodeJSTaskQueue to access the napi_env where the tasks run.
 *
 *   "GetNapiEnv" result is only valid in the thread where it was created.
 *   "Acquire" MUST be called on the creating thread.
 *   "Release" MUST be called on the creating thread.
 *
 * Before the task queue can schedule/run tasks, the queue must be "acquired".
 *
 * Acquire must be called atleast once in the creating thread.
 * Release must be called as many times as Acquire is done.
 *
 * after first acquired, the NodeJS message loop will be kept alive.
 * after fully released, then AFTER all tasks in queue have run to completion, the javascript side resources are
 * released, allowing NodeJS to terminate.
 *
 * while being fully released, the addtask methods will fail.
 * requiring a new acquire to work again.
 *
 * NodeJSQueue is created and acquired by the SceneJS and released by SceneJS.
 */
class INodeJSTaskQueue : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, INodeJSTaskQueue, "9d898eee-f592-410b-bf72-c8baeaa65614");

public:
    // provides access to the environment.
    // this is provided so tasks can do napi calls.
    // ONLY safe to use while executing tasks INodeJSTaskQueue (or the thread where napi_env is created)
    virtual napi_env GetNapiEnv() const = 0;

    // These are safe in the napi_env / js thread.

    // The following are counted.
    // When released to 0, the queue will run to termination, and release JS resources
    // Acquire can be used to allocate the said resources again.

    // Acquire makes sure that the queue can work
    virtual bool Acquire() = 0;
    // Release allows the resources to be released
    virtual bool Release() = 0;

    // Returns if it's safe to destroy the instance. (fully released with no tasks)
    virtual bool IsReleased() = 0;
};

// creates and registers the main nodetaskqueue instance if it doesn't exist yet. (uses provided napi_env)
// MUST BE CALLED IN THE THREAD OWNING THE NAPI_ENV!
META_NS::ITaskQueue::Ptr GetOrCreateNodeTaskQueue(napi_env e);

// unregisters the main nodetaskqueue instance.
// this SHOULD be called after no users left.
// and the queue being fully "Released".
// MUST BE CALLED IN THE THREAD OWNING THE NAPI_ENV!
bool DeinitNodeTaskQueue();
#endif
