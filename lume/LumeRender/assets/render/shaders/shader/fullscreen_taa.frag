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
#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_post_process_common.h"
#include "render/shaders/common/render_tonemap_common.h"

// sets

#include "render/shaders/common/render_post_process_layout_common.h"

layout(set = 1, binding = 0) uniform texture2D uDepth;
layout(set = 1, binding = 1) uniform texture2D uColor;
layout(set = 1, binding = 2) uniform texture2D uVelocity;
layout(set = 1, binding = 3) uniform texture2D uHistory;
layout(set = 1, binding = 4) uniform sampler uSampler;

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

// NOTE: cannot be used (remove if not used for any input)
#define ENABLE_INPUT_ATTACHMENTS 0

#define QUALITY_LOW 0
#define QUALITY_MED 1
#define QUALITY_HIGH 2

float GetUnpackDepthBuffer(const vec2 uv)
{
#if (ENABLE_INPUT_ATTACHMENTS == 1)
    return subpassLoad(uDepth).x;
#else
    return texture(sampler2D(uDepth, uSampler), uv).x;
#endif
}

vec2 GetUnpackVelocity(const vec2 uv, const vec2 invSize)
{
#if (ENABLE_INPUT_ATTACHMENTS == 1)
    return subpassLoad(uVelocity).xy;
#else
    return texture(sampler2D(uVelocity, uSampler), uv).xy * invSize;
#endif
}

vec2 GetVelocity()
{
    const uint quality = uint(uPc.factor.x + 0.5);
    vec2 velUv = inUv;
    if (quality >= QUALITY_MED) {
        const uint offsetCount = 5;
        const ivec2 offsets[offsetCount] = {
            ivec2(-1, -1),
            ivec2(1, -1),
            ivec2(0, 0),
            ivec2(-1, 1),
            ivec2(1, 1),
        };
        ivec2 offset = ivec2(0, 0);
        float depth = 1.0f;
        // NOTE: needs compile time constant offset and does not compile if e.g. passed to function
        {
            const float currDepth = textureLodOffset(sampler2D(uDepth, uSampler), inUv.xy, 0.0, offsets[0]).x;
            if (currDepth < depth) {
                depth = currDepth;
                offset = offsets[0];
            }
        }
        {
            const float currDepth = textureLodOffset(sampler2D(uDepth, uSampler), inUv.xy, 0.0, offsets[1]).x;
            if (currDepth < depth) {
                depth = currDepth;
                offset = offsets[1];
            }
        }
        {
            const float currDepth = textureLodOffset(sampler2D(uDepth, uSampler), inUv.xy, 0.0, offsets[2]).x;
            if (currDepth < depth) {
                depth = currDepth;
                offset = offsets[2];
            }
        }
        {
            const float currDepth = textureLodOffset(sampler2D(uDepth, uSampler), inUv.xy, 0.0, offsets[3]).x;
            if (currDepth < depth) {
                depth = currDepth;
                offset = offsets[3];
            }
        }
        {
            const float currDepth = textureLodOffset(sampler2D(uDepth, uSampler), inUv.xy, 0.0, offsets[4]).x;
            if (currDepth < depth) {
                depth = currDepth;
                offset = offsets[4];
            }
        }

        velUv += vec2(offset) * (uPc.viewportSizeInvSize.zw);
    }
    // multiply velocity to correct uv offsets
    return texture(sampler2D(uVelocity, uSampler), velUv).xy * uPc.viewportSizeInvSize.zw;
}

// clip towards aabb center
// e.g. "temporal reprojection anti-aliasing in inside"
vec4 ClipAabb(const vec3 aabbMin, const vec3 aabbMax, const vec4 color, const vec4 history)
{
    const vec3 pClip = 0.5 * (aabbMax + aabbMin);
    const vec3 eClip = 0.5 * (aabbMax - aabbMin);

    const vec4 vClip = history - vec4(pClip, color.w);
    const vec3 vUnit = vClip.xyz - eClip;
    const vec3 aUnit = abs(vUnit);
    const float maUnit = max(aUnit.x, max(aUnit.y, aUnit.z));
    // if maUnit <= 1.0 the point is inside the aabb
    const vec4 res = (maUnit > 1.0) ? (vec4(pClip, color.w) + vClip / maUnit) : color;
    return res;
}

void main(void)
{
    // NOTE: could use closest depth 3x3 velocity
    const vec2 velocity = GetVelocity();
    const vec2 historyUv = inUv.xy - velocity;

    const vec4 colorSample = textureLod(sampler2D(uColor, uSampler), inUv.xy, 0.0);
    const float frameAlpha = colorSample.a;
    vec4 currColor = colorSample;

    // NOTE: add filtered option for less blurred history
    vec4 history = textureLod(sampler2D(uHistory, uSampler), historyUv, 0.0);
    // sample 3x3 grid
    // 0 1 2
    // 3 4 5
    // 6 7 8
    vec3 bc[9];
    bc[0] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(-1, -1)).rgb;
    bc[1] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(0, -1)).rgb;
    bc[2] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(1, -1)).rgb;
    bc[3] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(-1, 0)).rgb;
    // center sample
    bc[4] = currColor.rgb;
    //
    bc[5] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(1, 0)).rgb;
    bc[6] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(-1, 1)).rgb;
    bc[7] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(0, 1)).rgb;
    bc[8] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(1, 1)).rgb;

    // NOTE: add
    // filter with tonemapping and yCoCG

    // Box filter for history
    // diamond shape
    vec3 boxMin = min(bc[4], min(min(bc[1], bc[3]), min(bc[5], bc[7])));
    vec3 boxMax = max(bc[4], max(max(bc[1], bc[3]), max(bc[5], bc[7])));
    // corners
    const vec3 boxMinCorner = min(boxMin, min(min(bc[0], bc[2]), min(bc[6], bc[8])));
    const vec3 boxMaxCorner = max(boxMax, max(max(bc[0], bc[2]), max(bc[6], bc[8])));
    boxMin = (boxMin + boxMinCorner) * 0.5;
    boxMax = (boxMax + boxMaxCorner) * 0.5;

    // NOTE: add option to use clipping
    history.rgb = clamp(history.rgb, boxMin, boxMax);

    // luma based tonemapping as suggested by Karis
    const float historyLuma = CalcLuma(history.rgb);
    const float colorLuma = CalcLuma(currColor.rgb);

    history.rgb *= 1.0 / (1.0 + historyLuma);
    currColor.rgb *= 1.0 / (1.0 + colorLuma);

    const float taaAlpha = 0.1;
    vec4 color = mix(history, currColor, taaAlpha);

    // inverse tonemap
    color.rgb *= 1.0 / (1.0 - CalcLuma(color.rgb));

    // safety for removing negative values
    outColor = max(color, vec4(0.0));
}
