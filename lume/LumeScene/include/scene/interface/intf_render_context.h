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

#ifndef SCENE_INTERFACE_RENDER_CONTEXT_H
#define SCENE_INTERFACE_RENDER_CONTEXT_H

#include <scene/base/types.h>

#include <core/resources/intf_resource_manager.h>
#include <render/intf_render_context.h>

#include <meta/api/make_callback.h>
#include <meta/api/task_queue.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_event.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/property/construct_property.h>

SCENE_BEGIN_NAMESPACE()

class IRenderContext : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IRenderContext, "12db3467-ecd5-42bf-bd37-3a6369742bf0")
public:
    virtual bool Initialize(BASE_NS::shared_ptr<RENDER_NS::IRenderContext>, META_NS::ITaskQueue::Ptr rq,
        META_NS::ITaskQueue::Ptr aq, CORE_NS::IResourceManager::Ptr) = 0;
    virtual BASE_NS::shared_ptr<RENDER_NS::IRenderContext> GetRenderer() const = 0;
    virtual META_NS::ITaskQueue::Ptr GetRenderQueue() const = 0;
    virtual META_NS::ITaskQueue::Ptr GetApplicationQueue() const = 0;
    virtual CORE_NS::IResourceManager::Ptr GetResources() const = 0;

    struct Counters {
        struct HandleCount {
            /// GPU buffer count
            uint32_t bufferHandleCount {};
            /// GPU image count
            uint32_t imageHandleCount {};
            /// GPU sampler count
            uint32_t samplerHandleCount {};
        };
        struct SceneInfo {
            /// Number of entities in the underlying ECS.
            uint32_t entityCount {};
            /// The number of nodes in the underlying ECS for the Scene.
            uint32_t nodeCount {};
            /// The number of Node objects accessed by the user from the Scene. This number can be smaller than
            /// nodeCount if the user has only accessed some Nodes from the Scene.
            uint32_t nodeObjectCount {};
        };
        /// Alive scenes in this render context
        BASE_NS::vector<SceneInfo> scenes;
        /// Scene count across all render contexts
        size_t totalSceneCount {};
        /// Global resource manager resource count
        size_t resourceCount {};
        /// GPU handle counts
        HandleCount handles;
    };

    /**
     * @brief Returns the global resource counters.
     */
    virtual Counters GetCounters() const = 0;

    template<typename Func>
    [[deprecated(
        "use correct variant instead AddTaskFuture,AddTaskOrRunDirectly,RunDirectlyOrInTask")]] [[nodiscard]] auto
    AddTask(Func&& func)
    {
        return AddTaskOrRunDirectly(BASE_NS::forward<Func>(func));
    }
    template<typename Func>
    [[deprecated(
        "use correct variant instead AddTaskFuture,AddTaskOrRunDirectly,RunDirectlyOrInTask")]] [[nodiscard]] auto
    AddTask(Func&& func, const META_NS::ITaskQueue::Ptr& queue)
    {
        return AddTaskOrRunDirectly(BASE_NS::forward<Func>(func), queue);
    }

    // AddTaskFuture adds a task to the renderqueue (or other specified queue) and returns a future
    template<typename Func>
    [[nodiscard]] auto AddTaskFuture(Func&& func) const
    {
        return AddTaskFuture(BASE_NS::forward<Func>(func), GetRenderQueue());
    }
    template<typename Func>
    [[nodiscard]] auto AddTaskFuture(Func&& func, const META_NS::ITaskQueue::Ptr& queue) const
    {
        return META_NS::Future<META_NS::PlainType_t<decltype(func())>>(
            queue->AddWaitableTask(META_NS::CreateWaitableTask(BASE_NS::move(func))));
    }

    // AddTaskOrRunDirectly adds a task to the renderqueue (or other specified queue) and returns a future.
    // POSSIBLY completing the future directly (if queue matches current, or the queue executes in the current thread)
    template<typename Func>
    [[nodiscard]] auto AddTaskOrRunDirectly(Func&& func) const
    {
        return AddTaskOrRunDirectly(BASE_NS::forward<Func>(func), GetRenderQueue());
    }

    template<typename Func>
    [[nodiscard]] auto AddTaskOrRunDirectly(Func&& func, const META_NS::ITaskQueue::Ptr& queue) const
    {
        return META_NS::AddFutureTaskOrRunDirectly(queue, BASE_NS::forward<Func>(func));
    }

    // RunDirectlyOrInTask runs the given function synchronously.
    // (either by waiting a future from the queue or directly. if the queue or execution thread matches)
    template<typename Func>
    auto RunDirectlyOrInTask(Func&& func) const
    {
        return RunDirectlyOrInTask<Func>(BASE_NS::forward<Func>(func), GetRenderQueue());
    }

    template<typename Func>
    auto RunDirectlyOrInTask(Func&& func, const META_NS::ITaskQueue::Ptr& queue) const
    {
        using namespace META_NS;
        auto& tr = GetTaskQueueRegistry();
        auto currentQueue = tr.GetCurrentTaskQueue();
        bool queueChange = (currentQueue != queue);
        bool direct = !queueChange;
        if (!direct) {
            if (auto ti = interface_cast<ITaskQueueThreadInfo>(queue)) {
                direct = ti->CurrentThreadIsExecutionThread();
            }
        }
        IFuture::Ptr future;
        using type = PlainType_t<decltype(func())>;
        if (direct) {
            if (queueChange) {
                tr.SetCurrentTaskQueue(queue);
            }
            if constexpr (BASE_NS::is_void_v<type>) {
                func();
                if (queueChange) {
                    tr.SetCurrentTaskQueue(currentQueue);
                }
                return;
            } else {
                auto ret = func();
                if (queueChange) {
                    tr.SetCurrentTaskQueue(currentQueue);
                }
                return ret;
            }
        } else {
            future = queue->AddWaitableTask(CreateWaitableTask(BASE_NS::move(func)));
        }
        if constexpr (BASE_NS::is_void_v<type>) {
            future->Wait();
        } else {
            return future->GetResultOr<type>({});
        }
    }

    template<typename Func>
    void PostUserNotification(Func&& func) const
    {
        if (auto q = GetApplicationQueue()) {
            q->AddTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>([f = BASE_NS::forward<Func>(func)] {
                f();
                return false;
            }));
        }
    }
    template<typename MyEvent, typename... Args>
    void InvokeUserNotification(const META_NS::IEvent::Ptr& event, Args... args) const
    {
        return PostUserNotification([=, ev = META_NS::IEvent::WeakPtr(event)] {
            if (auto e = ev.lock()) {
                META_NS::Invoke<MyEvent>(e, args...);
            }
        });
    }
};

META_REGISTER_CLASS(RenderContext, "a9080232-32e7-4d59-8498-753e639524e7", META_NS::ObjectCategoryBits::NO_CATEGORY)

inline META_NS::IMetadata::Ptr CreateRenderContextArg(const IRenderContext::Ptr& context)
{
    auto obj = META_NS::GetObjectRegistry().Create<META_NS::IMetadata>(META_NS::ClassId::Object);
    if (obj) {
        obj->AddProperty(META_NS::ConstructProperty<IRenderContext::Ptr>("RenderContext", context));
    }
    return obj;
}

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IRenderContext)

#endif
