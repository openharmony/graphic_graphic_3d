/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_RENDER_SHADERS_COMMON_POST_PROCESS_STRUCTS_COMMON_H
#define API_RENDER_SHADERS_COMMON_POST_PROCESS_STRUCTS_COMMON_H

#include "render/shaders/common/render_compatibility_common.h"

// defines
#ifdef VULKAN

// needs to match api/core/render/render_data_store_render_pods.h
#define POST_PROCESS_SPECIALIZATION_TONEMAP_BIT (1 << 0)
#define POST_PROCESS_SPECIALIZATION_VIGNETTE_BIT (1 << 1)
#define POST_PROCESS_SPECIALIZATION_DITHER_BIT (1 << 2)
#define POST_PROCESS_SPECIALIZATION_COLOR_CONVERSION_BIT (1 << 3)
#define POST_PROCESS_SPECIALIZATION_COLOR_FRINGE_BIT (1 << 4)

#define POST_PROCESS_SPECIALIZATION_BLUR_BIT (1 << 8)
#define POST_PROCESS_SPECIALIZATION_BLOOM_BIT (1 << 9)
#define POST_PROCESS_SPECIALIZATION_FXAA_BIT (1 << 10)
#define POST_PROCESS_SPECIALIZATION_TAA_BIT (1 << 11)

#define POST_PROCESS_INDEX_TONEMAP 0
#define POST_PROCESS_INDEX_VIGNETTE 1
#define POST_PROCESS_INDEX_DITHER 2
#define POST_PROCESS_INDEX_COLOR_CONVERSION 3
#define POST_PROCESS_INDEX_COLOR_FRINGE 4

#define POST_PROCESS_INDEX_BLUR 8
#define POST_PROCESS_INDEX_BLOOM 9
#define POST_PROCESS_INDEX_FXAA 10
#define POST_PROCESS_INDEX_TAA 11

// should be aligned to 256 (i.e. 14 x vec4 + 2 x vec4)
#define POST_PROCESS_GLOBAL_VEC4_FACTOR_COUNT 14
#define POST_PROCESS_LOCAL_VEC4_FACTOR_COUNT 16

#define CORE_POST_PROCESS_TONEMAP_ACES 0
#define CORE_POST_PROCESS_TONEMAP_ACES_2020 1
#define CORE_POST_PROCESS_TONEMAP_FILMIC 2

#define CORE_POST_PROCESS_COLOR_CONVERSION_SRGB 1

#else

// note UBO struct alignment for 256
constexpr uint32_t POST_PROCESS_GLOBAL_VEC4_FACTOR_COUNT { 14 };
constexpr uint32_t POST_PROCESS_LOCAL_VEC4_FACTOR_COUNT { 16 };

#endif

// the same data throughout the pipeline
// should be aligned to 256 (i.e. 16 x vec4)
// needs to match api/core/render/RenderDataStoreRenderPods.h
struct GlobalPostProcessStruct {
    // enable flags
    uvec4 flags;
    // .x = delta time (ms), .y = tick delta time (ms), .z = tick total time (s), .w = frame index (asuint)
    vec4 renderTimings;

    // all factors from defines
    vec4 factors[POST_PROCESS_GLOBAL_VEC4_FACTOR_COUNT];
};

// local data for a specific post process
// should be aligned to 256 (i.e. 16 x vec4)
// NOTE: one can create a new struct in their own shader, please note the alignment
struct LocalPostProcessStruct {
    // all factors for the specific post process material
    vec4 factors[POST_PROCESS_LOCAL_VEC4_FACTOR_COUNT];
};

struct LocalPostProcessPushConstantStruct {
    // viewport size and inverse viewport size for this draw
    vec4 viewportSizeInvSize;
    // fast factors for effect, might be direction for blur, some specific coefficients etc.
    vec4 factor;
};

struct PostProcessTonemapStruct {
    vec4 texSizeInvTexSize;

    uvec4 flags;

    vec4 tonemap;
    vec4 vignette;
    vec4 colorFringe;
    vec4 dither;
    // .x is thresholdHard, .y is thresholdSoft, .z is amountCoefficient, .w is dirtMaskCoefficient
    vec4 bloomParameters;
};

#endif // API_RENDER_SHADERS_COMMON_POST_PROCESS_STRUCTS_COMMON_H
