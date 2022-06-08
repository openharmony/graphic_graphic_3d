/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(MORPH_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define MORPH_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** Component for giving weights to morph targets. */
BEGIN_COMPONENT(IMorphComponentManager, MorphComponent)
    /** Name of each morph target. */
    DEFINE_PROPERTY(BASE_NS::vector<BASE_NS::string>, morphNames, "", PropertyFlags::IS_HIDDEN,)
    /** Weight of each morph target. The */
    DEFINE_PROPERTY(BASE_NS::vector<float>, morphWeights, "@morphNames[*]", PropertyFlags::IS_SLIDER,)
END_COMPONENT(IMorphComponentManager, MorphComponent, "46f1ddb9-053d-4f54-aaea-ddfca075294f")

#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
