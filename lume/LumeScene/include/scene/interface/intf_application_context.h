
#ifndef SCENE_INTERFACE_IAPPLICATION_CONTEXT_H
#define SCENE_INTERFACE_IAPPLICATION_CONTEXT_H

#include <scene/base/types.h>
#include <scene/interface/intf_render_context.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/scene_options.h>

#include <meta/interface/intf_task_queue.h>

SCENE_BEGIN_NAMESPACE()

class IApplicationContext : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IApplicationContext, "52d1686d-8214-49ea-98f7-83c02a45f985")
public:
    struct ApplicationContextInfo {
        META_NS::ITaskQueue::Ptr engineTaskQueue;
        META_NS::ITaskQueue::Ptr applicationTaskQueue;
        BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext;
        CORE_NS::IResourceManager::Ptr resourceManager;
        SceneOptions defaultSceneOptions;
    };
    virtual bool Initialize(const ApplicationContextInfo& info) = 0;

    virtual IRenderContext::Ptr GetRenderContext() const = 0;
    virtual ISceneManager::Ptr GetSceneManager() const = 0;
    virtual SceneOptions GetDefaultSceneOptions() const = 0;
};

META_REGISTER_SINGLETON_CLASS(
    ApplicationContext, "424b9f97-6447-41c0-8225-1fb0e1bbd7e8", META_NS::ObjectCategoryBits::NO_CATEGORY)

inline IApplicationContext::ConstPtr GetDefaultApplicationContext()
{
    if (auto obj = META_NS::GetObjectRegistry().Create<IApplicationContext>(ClassId::ApplicationContext)) {
        return obj;
    }
    return nullptr;
}

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IApplicationContext)

#endif
