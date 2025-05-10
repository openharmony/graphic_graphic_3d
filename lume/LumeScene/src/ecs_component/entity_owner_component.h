

#if !defined(SCENE_SRC_ECS_COMPONENT_ENTITY_OWNER_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define SCENE_SRC_ECS_COMPONENT_ENTITY_OWNER_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <scene/base/namespace.h>

#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>

SCENE_BEGIN_NAMESPACE()
#endif

BEGIN_COMPONENT(IEntityOwnerComponentManager, EntityOwnerComponent)
    /** Owned entity. */
    DEFINE_PROPERTY(CORE_NS::EntityReference, entity, "Entity", 0, )
END_COMPONENT(IEntityOwnerComponentManager, EntityOwnerComponent, "aa74aebf-58d5-46bf-b6f2-bdcb55274e9e")
#if !defined(IMPLEMENT_MANAGER)
SCENE_END_NAMESPACE()
#endif

#endif
