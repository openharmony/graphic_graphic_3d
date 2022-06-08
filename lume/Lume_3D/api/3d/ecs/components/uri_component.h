/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(URI_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define URI_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/string.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** URI component is used for indicating e.g. where the entity originiated.
 */
BEGIN_COMPONENT(IUriComponentManager, UriComponent)

    DEFINE_PROPERTY(BASE_NS::string, uri, "Uri", 0,)

END_COMPONENT(IUriComponentManager, UriComponent, "8588db9a-1012-400a-95db-84414bb5ec65")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
