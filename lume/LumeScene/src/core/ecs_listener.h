#ifndef SCENE_SRC_CORE_ECS_LISTENER_H
#define SCENE_SRC_CORE_ECS_LISTENER_H

#include <scene/base/types.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_event_listener.h>

#include <base/containers/unordered_map.h>

SCENE_BEGIN_NAMESPACE()

class EcsListener : CORE_NS::IEcs::EntityListener, CORE_NS::IEcs::ComponentListener {
public:
    bool Initialize(IEcsContext& context);
    void Uninitialize();

    void RegisterEcsObject(const IEcsObject::Ptr&);
    void DeregisterEcsObject(const IEcsObject::ConstPtr&);
    void DeregisterEcsObject(CORE_NS::Entity);

    IEcsObject::Ptr FindEcsObject(CORE_NS::Entity ent) const;

private:
    void OnEntityEvent(
        CORE_NS::IEcs::EntityListener::EventType type, BASE_NS::array_view<const CORE_NS::Entity> entities) override;
    void OnComponentEvent(CORE_NS::IEcs::ComponentListener::EventType type, const CORE_NS::IComponentManager& manager,
        BASE_NS::array_view<const CORE_NS::Entity> entities) override;

private:
    IEcsContext* ecs_ {};
    BASE_NS::unordered_map<CORE_NS::Entity, IEcsObject::WeakPtr> objects_;
};

SCENE_END_NAMESPACE()

#endif