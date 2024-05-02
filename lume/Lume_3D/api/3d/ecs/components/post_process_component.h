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

#if !defined(API_3D_ECS_COMPONENTS_POST_PROCESS_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_POST_PROCESS_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <render/datastore/render_data_store_render_pods.h>

CORE3D_BEGIN_NAMESPACE()
#endif

/** Post-process component that can be used with cameras.
 * Uses the core configuration of PostProcessConfiguration, expect fog control not used/implemented.
 */
BEGIN_COMPONENT(IPostProcessComponentManager, PostProcessComponent)
#if !defined(IMPLEMENT_MANAGER)
    enum FlagBits : uint32_t {
        TONEMAP_BIT = (1 << 0),
        VIGNETTE_BIT = (1 << 1),
        DITHER_BIT = (1 << 2),
        COLOR_CONVERSION_BIT = (1 << 3),
        COLOR_FRINGE_BIT = (1 << 4),
        BLUR_BIT = (1 << 8),
        BLOOM_BIT = (1 << 9),
        FXAA_BIT = (1 << 10),
        TAA_BIT = (1 << 11),
        DOF_BIT = (1 << 12),
        MOTION_BLUR_BIT = (1 << 13),
    };
    /** Container for post-processing flag bits */
    using Flags = uint32_t;
#endif

    /** The type of the background fill when rendering.
     */
    DEFINE_BITFIELD_PROPERTY(
        Flags, enableFlags, "Enabled Effects", PropertyFlags::IS_BITFIELD, VALUE(0), PostProcessComponent::FlagBits)

    /** Tonemap configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::TonemapConfiguration, tonemapConfiguration, "Tonemap Configuration", 0, ARRAY_VALUE())

    /** Bloom configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::BloomConfiguration, bloomConfiguration, "Bloom Configuration", 0, ARRAY_VALUE())

    /** Vignette configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::VignetteConfiguration, vignetteConfiguration, "Vignette Configuration", 0, ARRAY_VALUE())

    /** Color fringe configuration.
     */
    DEFINE_PROPERTY(
        RENDER_NS::ColorFringeConfiguration, colorFringeConfiguration, "Color Fringe Configuration", 0, ARRAY_VALUE())

    /** Dither configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::DitherConfiguration, ditherConfiguration, "Dither Configuration", 0, ARRAY_VALUE())

    /** Blur configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::BlurConfiguration, blurConfiguration, "Target Blur Configuration", 0, ARRAY_VALUE())

    /** Color conversion configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::ColorConversionConfiguration, colorConversionConfiguration,
        "Color Conversion Configuration", 0, ARRAY_VALUE())

    /** FX anti-aliasing configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::FxaaConfiguration, fxaaConfiguration, "Fast Approximate Anti-Aliasing Configuration", 0,
        ARRAY_VALUE())

    /** Temporal anti-aliasing configuration.
     */
    DEFINE_PROPERTY(
        RENDER_NS::TaaConfiguration, taaConfiguration, "Temporal Anti-Aliasing Configuration", 0, ARRAY_VALUE())

    /** Depth of field configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::DofConfiguration, dofConfiguration, "Depth Of Field Configuration", 0, ARRAY_VALUE())

    /** Motion blur configuration.
     */
    DEFINE_PROPERTY(
        RENDER_NS::MotionBlurConfiguration, motionBlurConfiguration, "Motion Blur Configuration", 0, ARRAY_VALUE())

END_COMPONENT(IPostProcessComponentManager, PostProcessComponent, "a2c647e7-9c66-4565-af3b-3acc75b3718f")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
