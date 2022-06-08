/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(LAYER_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define LAYER_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/ecs/components/layer_defines.h>
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/namespace.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** Layer component can be used to limit the visibility of the entity in cameras and reflections.
 */
BEGIN_COMPONENT(ILayerComponentManager, LayerComponent)

    /** Defines a layer mask. */
    DEFINE_BITFIELD_PROPERTY(uint64_t, layerMask, "Layer mask", CORE_NS::PropertyFlags::IS_BITFIELD,
        VALUE(LayerConstants::DEFAULT_LAYER_MASK), LayerFlagBits)

END_COMPONENT(ILayerComponentManager, LayerComponent, "01507270-f290-4f71-82f4-58f1bddeadc5")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
