/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef CORE3D_DM_FW_CLOUD_FRAG_H
#define CORE3D_DM_FW_CLOUD_FRAG_H

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_control_flow_attributes : enable

// includes
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "3d/shaders/common/3d_dm_target_packing_common.h"
#include "render/shaders/common/render_compatibility_common.h"

// sets
#include "3d/shaders/common/3d_dm_frag_layout_common.h"
#include "3d/shaders/common/3d_dm_inplace_sampling_common.h"

// in / out

layout(location = 0) in vec2 inUv;
layout(location = 1) in flat uint inIndices;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outVelocityNormal;

#define KERNEL_SIZE 1
// Values are temporarily boosted to help with artifacts
const float DEPTH_SIGMA = 0.30;  // was 0.1
const float SPATIAL_SIGMA = 1.7; // was 1.1
const float COLOR_SIGMA = 2.0;   // was 0.1

// In bindless the texture indices are offset by 1
#if (CORE3D_DM_BINDLESS_FRAG_LAYOUT == 1)
const uint texIndexAddition = 1;
#else
const uint texIndexAddition = 0;
#endif
vec4 GetTexelFetch(const ivec2 uv, const uint instanceIdx, const uint matTexIdx)
{
#if (CORE3D_DM_BINDLESS_FRAG_LAYOUT == 1)
    BindlessImageAndSamplerIndex bi = GetImageAndSamplerIndex(instanceIdx, matTexIdx);
    return texelFetch(sampler2D(uImages[nonuniformEXT(bi.imageIdx)], uSamplers[nonuniformEXT(bi.samplerIdx)]), uv, 0);
#else
    return texelFetch(uSampTextures[matTexIdx], uv, 0);
#endif
}

void CloudMain()
{
    const uint frameIndex = floatBitsToUint(uEnvironmentData.groundColor.w);
    const uint idx = CORE_MATERIAL_TEX_NORMAL_IDX + texIndexAddition + frameIndex;

    const vec2 uvFull = inUv * 0.5 + 0.5;
    const vec2 lowResTexelSize = 1.0 / GetTextureSize(0, idx, 0);
    const ivec2 lowResTextureSize = GetTextureSize(0, idx, 0);

    const vec2 uvLow = uvFull - 0.5 * lowResTexelSize;
    const vec2 lowUV_scaled = uvFull / lowResTexelSize;
    const ivec2 base = ivec2(floor(lowUV_scaled));

    const float twoDepthSigmaSq = 2.0 * DEPTH_SIGMA * DEPTH_SIGMA;
    const float twoSpatialSigmaSq = 2.0 * SPATIAL_SIGMA * SPATIAL_SIGMA;
    const float twoColorSigmaSq = 2.0 * COLOR_SIGMA * COLOR_SIGMA;

    vec4 next = GetTextureSample(uvFull, 0, idx);

    uint depthTexIdx = CORE_MATERIAL_TEX_EMISSIVE_IDX + texIndexAddition + frameIndex;
    float centerDepth = GetTextureSample(uvFull, 0, depthTexIdx).r;
    vec4 neighborColor;
    float neighborDepth;

    vec4 totalColor = vec4(0.0);
    float totalWeight = 0.0;
    for (int j = -KERNEL_SIZE; j <= KERNEL_SIZE; ++j) {
        for (int i = -KERNEL_SIZE; i <= KERNEL_SIZE; ++i) {
            ivec2 neighborCoord = clamp(base + ivec2(i, j), ivec2(0), lowResTextureSize - ivec2(1));
            neighborColor = GetTexelFetch(neighborCoord, 0, idx);
            neighborDepth = GetTexelFetch(neighborCoord, 0, depthTexIdx).r;

            float depthDiff = (neighborDepth - centerDepth);
            float depthWeight = exp(-(depthDiff * depthDiff) / twoDepthSigmaSq);

            vec2 neighborUV = (vec2(neighborCoord) + 0.5) * lowResTexelSize;
            vec2 uvDiff = (uvFull - neighborUV) / lowResTexelSize;

            float distSq = dot(uvDiff, uvDiff);
            float spatialWeight = exp(-distSq / twoSpatialSigmaSq);

            float colorDiff = length(neighborColor.rgb - next.rgb);
            float colorWeight = exp(-(colorDiff * colorDiff) / twoColorSigmaSq);

            float finalWeight = depthWeight * spatialWeight * colorWeight;

            totalColor += neighborColor * finalWeight;
            totalWeight += finalWeight;
        }
    }
    outVelocityNormal = GetPackVelocityAndNormal(vec2(0.0), vec3(0.0));
    gl_FragDepth = 1.0;

    if (totalWeight < 1e-4) {
        // Fallback: if weights are zero, just do a simple bilinear sample.
        outColor = next;
        return;
    }

    outColor = totalColor / totalWeight;
}

#endif // CORE3D_DM_FW_CLOUD_FRAG_H