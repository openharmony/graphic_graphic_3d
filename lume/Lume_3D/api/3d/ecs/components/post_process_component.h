/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(POST_PROCESS_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define POST_PROCESS_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>
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
    };
    /** Container for post-processing flag bits */
    using Flags = uint32_t;
#endif

    /** The type of the background fill when rendering.
     */
    DEFINE_BITFIELD_PROPERTY(
        Flags, enableFlags, "Enable flags", PropertyFlags::IS_BITFIELD, VALUE(0), PostProcessComponent::FlagBits)

    /** Tonemap configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::TonemapConfiguration, tonemapConfiguration, "Tonemap configuration", 0, ARRAY_VALUE())

    /** Bloom configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::BloomConfiguration, bloomConfiguration, "Bloom configuration", 0, ARRAY_VALUE())

    /** Vignette configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::VignetteConfiguration, vignetteConfiguration, "Vignette configuration", 0, ARRAY_VALUE())

    /** Color fringe configuration.
     */
    DEFINE_PROPERTY(
        RENDER_NS::ColorFringeConfiguration, colorFringeConfiguration, "Color fringe configuration", 0, ARRAY_VALUE())

    /** Dither configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::DitherConfiguration, ditherConfiguration, "Dither configuration", 0, ARRAY_VALUE())

    /** Blur configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::BlurConfiguration, blurConfiguration, "Target blur configuration", 0, ARRAY_VALUE())

    /** Color conversion configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::ColorConversionConfiguration, colorConversionConfiguration,
        "Color conversion configuration", 0, ARRAY_VALUE())

    /** FX anti-aliasing configuration.
     */
    DEFINE_PROPERTY(
        RENDER_NS::FxaaConfiguration, fxaaConfiguration, "FXAA anti-aliasing configuration", 0, ARRAY_VALUE())

    /** Temporal anti-aliasing configuration.
     */
    DEFINE_PROPERTY(RENDER_NS::TaaConfiguration, taaConfiguration, "TAA anti-aliasing configuration", 0, ARRAY_VALUE())

END_COMPONENT(IPostProcessComponentManager, PostProcessComponent, "a2c647e7-9c66-4565-af3b-3acc75b3718f")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
