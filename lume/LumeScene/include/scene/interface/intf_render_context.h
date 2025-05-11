
#ifndef SCENE_INTERFACE_RENDER_CONTEXT_H
#define SCENE_INTERFACE_RENDER_CONTEXT_H

#include <scene/base/types.h>

#include <core/resources/intf_resource_manager.h>
#include <render/intf_render_context.h>

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

    template<typename Func>
    auto AddTask(Func&& func) const
    {
        return AddTask(BASE_NS::forward<Func>(func), GetRenderQueue());
    }
    template<typename Func>
    auto AddTask(Func&& func, const META_NS::ITaskQueue::Ptr& queue) const
    {
        return META_NS::AddFutureTaskOrRunDirectly(queue, BASE_NS::forward<Func>(func));
    }
    template<typename Func>
    auto PostUserNotification(Func&& func) const
    {
        return AddTask(BASE_NS::forward<Func>(func), GetApplicationQueue());
    }
    template<typename MyEvent, typename... Args>
    auto InvokeUserNotification(const META_NS::IEvent::Ptr& event, Args... args) const
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
