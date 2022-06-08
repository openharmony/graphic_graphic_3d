/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(NAME_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define NAME_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/string.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** Name component is used for giving a name for an entity.
 */
BEGIN_COMPONENT(INameComponentManager, NameComponent)

    DEFINE_PROPERTY(BASE_NS::string, name, "Name", 0,)

END_COMPONENT(INameComponentManager, NameComponent, "6b432233-bac4-43ff-8c15-983f467052a6")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
