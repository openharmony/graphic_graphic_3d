/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef SHADERS__COMMON__CMAA2_COMMON_H
#define SHADERS__COMMON__CMAA2_COMMON_H

// loosely based on the original intel's reference implementation
// https://github.com/GameTechDev/CMAA2/blob/master/Projects/CMAA2/CMAA2/CMAA2.hlsl

#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"

#define CMAA2_EXTRA_SHARPNESS 0 // 1 to preserve more text clarity
#define CMAA2_SKIP_WEAK_EDGES 0 // 1 to skip weak edges, slight optimization

#define CMAA2_CS_INPUT_KERNEL_SIZE_X 16
#define CMAA2_CS_INPUT_KERNEL_SIZE_Y 16
#define CMAA2_CS_OUTPUT_KERNEL_SIZE_X (CMAA2_CS_INPUT_KERNEL_SIZE_X - 2)
#define CMAA2_CS_OUTPUT_KERNEL_SIZE_Y (CMAA2_CS_INPUT_KERNEL_SIZE_Y - 2)
#define CMAA2_PROCESS_CANDIDATES_NUM_THREADS 128
#define CMAA2_DEFERRED_APPLY_NUM_THREADS 32

#define CMAA2_MAX_LINE_LENGTH 86

#define CMAA2_BLEND_MAX_ITER 64u

#if CMAA2_EXTRA_SHARPNESS
#define CMAA2_LOCAL_CONTRAST_ADAPTATION_AMOUNT 0.15f
#define CMAA2_SIMPLE_SHAPE_BLUR_AMOUNT 0.07f
#else
#define CMAA2_LOCAL_CONTRAST_ADAPTATION_AMOUNT 0.10f
#define CMAA2_SIMPLE_SHAPE_BLUR_AMOUNT 0.10f
#endif

#define SYMMETRY_CORRECTION_OFFSET 0.22f
#if CMAA2_EXTRA_SHARPNESS
#define DAMPENING 0.11f
#else
#define DAMPENING 0.15f
#endif

struct ControlBufferStruct {
    uint itemCount;
    uint shapeCandidateCount;
    uint blendLocationCount;
    uint deferredBlendItemCount;
};

struct ShaderDataStruct {
    float resX;
    float resY;
    float edgeThreshold;
    float padding;
};

struct DispatchIndirectCommand {
    uint x;
    uint y;
    uint z;
};

uvec4 UnpackEdges(const uint value)
{
    return uvec4((value & 0x01u) != 0u ? 1u : 0u, (value & 0x02u) != 0u ? 1u : 0u, (value & 0x04u) != 0u ? 1u : 0u,
        (value & 0x08u) != 0u ? 1u : 0u);
}

uint PackEdges4(const vec4 edges)
{
    return packUnorm4x8(edges);
}

vec4 UnpackEdges4(const uint packed)
{
    return unpackUnorm4x8(packed);
}

uint PackEdgesBinary(const vec4 edges)
{
    return uint(edges.x) | (uint(edges.y) << 1u) | (uint(edges.z) << 2u) | (uint(edges.w) << 3u);
}

vec4 UnpackEdgesFloat(const uint value)
{
    return vec4(UnpackEdges(value));
}

uint PackColor(const vec3 color)
{
    // clamp to R11G11B10F
    const uvec3 f32bits = floatBitsToUint(clamp(color, 0.0f, 65504.0f));

    const uvec3 exponent = (f32bits >> 23u) & 0xFFu;
    const uvec3 mantissa = f32bits & 0x7FFFFFu;

    // convert to R11G11B10F
    const uvec3 newExp = clamp(exponent - 127u + 15u, 0u, 31u);

    const uint rr = ((newExp.r << 6u) | (mantissa.r >> 17u)) & 0x7FFu;
    const uint gg = ((newExp.g << 6u) | (mantissa.g >> 17u)) & 0x7FFu;
    const uint bb = ((newExp.b << 5u) | (mantissa.b >> 18u)) & 0x3FFu;

    return rr | (gg << 11u) | (bb << 22u);
}

vec3 UnpackColor(const uint packed)
{
    // extract from R11G11B10F
    const uint r11 = packed & 0x7FFu;
    const uint g11 = (packed >> 11u) & 0x7FFu;
    const uint b10 = (packed >> 22u) & 0x3FFu;

    const uvec3 exponent = uvec3(r11 >> 6u, g11 >> 6u, b10 >> 5u);
    const uvec3 mantissa = uvec3(r11 & 0x3Fu, g11 & 0x3Fu, b10 & 0x1Fu);

    const uvec3 f32exp = clamp(exponent + 127u - 15u, 0u, 255u);
    const uvec3 f32mant = uvec3(mantissa.r << 17u, mantissa.g << 17u, mantissa.b << 18u);

    const uvec3 f32bits = (f32exp << 23u) | f32mant;

    return vec3(uintBitsToFloat(f32bits.r), uintBitsToFloat(f32bits.g), uintBitsToFloat(f32bits.b));
}

float RgbToLuma(const vec3 rgb)
{
    const float luma = dot(sqrt(rgb.rgb), vec3(0.299f, 0.587f, 0.114f));
    return luma;
}

#endif // SHADERS__COMMON__CMAA2_COMMON_H
