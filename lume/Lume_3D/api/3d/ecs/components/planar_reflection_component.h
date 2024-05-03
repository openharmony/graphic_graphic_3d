/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if !defined(API_3D_ECS_COMPONENTS_PLANAR_REFLECTION_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_PLANAR_REFLECTION_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/ecs/components/layer_defines.h>
#include <3d/namespace.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>

CORE3D_BEGIN_NAMESPACE()
#endif

/** Planar reflection is used to create planar reflections, like mirrors.
 */
BEGIN_COMPONENT(IPlanarReflectionComponentManager, PlanarReflectionComponent)
#if !defined(IMPLEMENT_MANAGER)
    enum FlagBits : uint8_t {
        /** Disables the reflection, does not affect the plane rendering. */
        ACTIVE_RENDER_BIT = (1 << 0),
        MSAA_BIT = (1 << 2)
    };
    /** Container for planar reflection flag bits */
    using Flags = uint8_t;
#endif

    /** Downsample percentage (relative to render resolution), affects reflection quality.
     */
    DEFINE_PROPERTY(float, screenPercentage, "Downsample Percentage", 0, 0.5f)

    /** Quick access to current render target resolution.
     */
    DEFINE_ARRAY_PROPERTY(uint32_t, 2, renderTargetResolution, "Render Target Resolution", 0, ARRAY_VALUE(0, 0))

    /** Reflection output render target.
     */
    DEFINE_PROPERTY(CORE_NS::EntityReference, colorRenderTarget, "Render Output Target", 0, )

    /** Reflection pass depth render target.
     */
    DEFINE_PROPERTY(CORE_NS::EntityReference, depthRenderTarget, "Depth Output Target", 0, )

    /** Clip offset to reflection plane.
     */
    DEFINE_PROPERTY(float, clipOffset, "Reflection Clip Offset", 0, VALUE(0.0f))

    /** Additional flags for reflection processing.
     */
    DEFINE_BITFIELD_PROPERTY(Flags, additionalFlags, "Flags", PropertyFlags::IS_BITFIELD,
        VALUE(FlagBits::ACTIVE_RENDER_BIT), PlanarReflectionComponent::FlagBits)

    /** Defines a layer mask which affects reflection camera's rendering. Default is all layer mask when the
     * reflection camera renders objects from all layers. */
    DEFINE_BITFIELD_PROPERTY(uint64_t, layerMask, "Layer Mask", PropertyFlags::IS_BITFIELD,
        VALUE(LayerConstants::ALL_LAYER_MASK), LayerFlagBits)

END_COMPONENT(IPlanarReflectionComponentManager, PlanarReflectionComponent, "5081ccf4-2013-43c1-b9bb-23041e73ac6d")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
