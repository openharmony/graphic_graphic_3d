
#ifndef SCENE_EXT_ICOMPONENT_FACTORY_H
#define SCENE_EXT_ICOMPONENT_FACTORY_H

#include <scene/ext/intf_component.h>
#include <scene/ext/intf_ecs_object.h>

SCENE_BEGIN_NAMESPACE()

class IComponentFactory : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IComponentFactory, "512141cf-be70-40b2-9f37-e9a0670fff10")
public:
    virtual IComponent::Ptr CreateComponent(const IEcsObject::Ptr&) = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IComponentFactory)

#endif
