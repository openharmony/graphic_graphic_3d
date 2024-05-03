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
#define POST_PROCESS_INDEX_DOF 12
#define POST_PROCESS_INDEX_MOTION_BLUR 13

// should be aligned to 512 (i.e. 14 x vec4 + 2 x vec4 + 16 x vec4)
#define POST_PROCESS_GLOBAL_VEC4_FACTOR_COUNT 14
#define POST_PROCESS_GLOBAL_USER_VEC4_FACTOR_COUNT 16
// aligned to 256
#define POST_PROCESS_LOCAL_VEC4_FACTOR_COUNT 16

#define CORE_POST_PROCESS_TONEMAP_ACES 0
#define CORE_POST_PROCESS_TONEMAP_ACES_2020 1
#define CORE_POST_PROCESS_TONEMAP_FILMIC 2

#define CORE_POST_PROCESS_COLOR_CONVERSION_SRGB 1

#else

// note global post process UBO struct alignment for 512
constexpr uint32_t POST_PROCESS_GLOBAL_VEC4_FACTOR_COUNT { 14u };
constexpr uint32_t POST_PROCESS_GLOBAL_USER_VEC4_FACTOR_COUNT { 16u };
// note UBO struct alignment for 256
constexpr uint32_t POST_PROCESS_LOCAL_VEC4_FACTOR_COUNT { 16u };

#endif

// the same data throughout the pipeline
// should be aligned to 512 (i.e. 32 x vec4)
// needs to match api/core/render/RenderDataStoreRenderPods.h
struct GlobalPostProcessStruct {
    // enable flags
    uvec4 flags;
    // .x = delta time (ms), .y = tick delta time (ms), .z = tick total time (s), .w = frame index (asuint)
    vec4 renderTimings;

    // all factors from defines
    vec4 factors[POST_PROCESS_GLOBAL_VEC4_FACTOR_COUNT];

    // all user factors that are automatically mapped
    vec4 userFactors[POST_PROCESS_GLOBAL_USER_VEC4_FACTOR_COUNT];
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
