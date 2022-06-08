/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(SKIN_IBM_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define SKIN_IBM_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/vector.h>
#include <base/math/matrix.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** Contains the inverse bind matrices (IBM) of a skin. The transform each skin joint to the initial pose.
 */
BEGIN_COMPONENT(ISkinIbmComponentManager, SkinIbmComponent)

    DEFINE_PROPERTY(BASE_NS::vector<BASE_NS::Math::Mat4X4>, matrices, "", 0,)

END_COMPONENT(ISkinIbmComponentManager, SkinIbmComponent, "f62fc6d6-de51-42f6-86fd-bed8ce5f3d03")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
