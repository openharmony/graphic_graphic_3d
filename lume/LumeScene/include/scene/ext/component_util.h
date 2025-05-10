
#ifndef SCENE_EXT_COMPONENT_UTIL_H
#define SCENE_EXT_COMPONENT_UTIL_H

#include <scene/ext/intf_component_factory.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_scene.h>

SCENE_BEGIN_NAMESPACE()

template<typename Manager>
bool AddComponent(INode::Ptr node)
{
    auto acc = interface_cast<IEcsObjectAccess>(node);
    if (!acc) {
        CORE_LOG_E("Invalid node, no ecs access");
        return false;
    }
    auto ecsObj = acc->GetEcsObject();
    if (!ecsObj) {
        CORE_LOG_E("Invalid node, ecs object is null");
        return false;
    }
    auto entity = ecsObj->GetEntity();

    auto is = node->GetScene()->GetInternalScene();
    auto m = CORE_NS::GetManager<Manager>(*is->GetEcsContext().GetNativeEcs());

    if (!m->HasComponent(entity)) {
        m->Create(entity);
    }

    auto fac = is->FindComponentFactory(Manager::UID);
    if (!fac) {
        CORE_LOG_E("Failed to create component factory");
        return false;
    }

    auto c = fac->CreateComponent(ecsObj);
    if (!c) {
        CORE_LOG_E("Failed to create component");
        return false;
    }

    auto obj = interface_cast<META_NS::IAttach>(node);
    if (!obj) {
        CORE_LOG_E("Failed to attach component to node");
        return false;
    }

    obj->Attach(c);
    return true;
}

SCENE_END_NAMESPACE()

#endif
