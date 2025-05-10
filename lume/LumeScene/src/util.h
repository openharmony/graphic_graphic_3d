
#ifndef SCENE_SRC_UTIL_H
#define SCENE_SRC_UTIL_H

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>

#include <3d/ecs/components/render_handle_component.h>
#include <core/property/scoped_handle.h>

SCENE_BEGIN_NAMESPACE()

inline CORE_NS::IEcs* GetNativeEcs(const IEcsObject::Ptr& object)
{
    if (auto s = object->GetScene()) {
        return s->GetEcsContext().GetNativeEcs().get();
    }
    return nullptr;
}

inline IEcsContext* GetEcs(const IEcsObject::Ptr& object)
{
    if (auto s = object->GetScene()) {
        return &s->GetEcsContext();
    }
    return nullptr;
}

template<typename ComponentType>
CORE_NS::ScopedHandle<ComponentType> GetScopedHandle(const IEcsObject::Ptr& object)
{
    if (auto ecs = GetEcs(object)) {
        if (auto m = ecs->FindComponent<ComponentType>()) {
            return CORE_NS::ScopedHandle<ComponentType> { m->GetData(object->GetEntity()) };
        }
    }
    return CORE_NS::ScopedHandle<ComponentType> {};
}

template<typename Interface>
typename Interface::Ptr ObjectWithRenderHandle(
    const IInternalScene::Ptr& scene, CORE_NS::EntityReference entRef, typename Interface::Ptr p, META_NS::ObjectId id)
{
    if (auto rhman = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
            scene->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>())) {
        if (!p && id.IsValid()) {
            p = interface_pointer_cast<Interface>(scene->CreateObject(id));
        }
        if (auto i = interface_cast<IRenderResource>(p)) {
            i->SetRenderHandle(rhman->GetRenderHandleReference(entRef));
        }
    }
    return p;
}

inline CORE_NS::EntityReference HandleFromRenderResource(
    const IInternalScene::Ptr& scene, const RENDER_NS::RenderHandleReference& handle)
{
    return scene->GetEcsContext().GetRenderHandleEntity(handle);
}

template<typename Interface>
CORE_NS::EntityReference HandleFromRenderResource(const IInternalScene::Ptr& scene, const typename Interface::Ptr& p)
{
    CORE_NS::EntityReference ent;
    if (auto i = interface_cast<IRenderResource>(p)) {
        ent = HandleFromRenderResource(scene, i->GetRenderHandle());
    }
    return ent;
}

SCENE_END_NAMESPACE()

#endif