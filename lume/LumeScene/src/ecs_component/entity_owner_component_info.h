#ifndef SCENE_SRC_ECS_COMPONENT_ENTITY_OWNER_COMPONENT_INFO_H
#define SCENE_SRC_ECS_COMPONENT_ENTITY_OWNER_COMPONENT_INFO_H

#include <core/plugin/intf_plugin.h>

#include "entity_owner_component.h"

SCENE_BEGIN_NAMESPACE()

CORE_NS::IComponentManager* IEntityOwnerComponentManagerInstance(CORE_NS::IEcs&);
void IEntityOwnerComponentManagerDestroy(CORE_NS::IComponentManager*);

constexpr CORE_NS::ComponentManagerTypeInfo ENTITY_OWNER_COMPONENT_TYPE_INFO {
    { CORE_NS::ComponentManagerTypeInfo::UID }, IEntityOwnerComponentManager::UID,
    CORE_NS::GetName<IEntityOwnerComponentManager>().data(), IEntityOwnerComponentManagerInstance,
    IEntityOwnerComponentManagerDestroy
};

SCENE_END_NAMESPACE()

#endif
