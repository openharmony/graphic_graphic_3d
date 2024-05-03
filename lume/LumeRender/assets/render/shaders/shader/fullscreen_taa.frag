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
    return textureLod(sampler2D(uDepth, uSampler), uv, 0).x;
#endif
}

vec2 GetUnpackVelocity(const vec2 uv, const vec2 invSize)
{
#if (ENABLE_INPUT_ATTACHMENTS == 1)
    return subpassLoad(uVelocity).xy;
#else
    return textureLod(sampler2D(uVelocity, uSampler), uv, 0).xy * invSize;
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

        // NOTE: needs compile time constant offset and does not compile if e.g. passed to function
        vec2 batch1 = vec2(1.0, textureLodOffset(sampler2D(uDepth, uSampler), inUv.xy, 0.0, offsets[0]).x);
        vec2 batch2 = vec2(textureLodOffset(sampler2D(uDepth, uSampler), inUv.xy, 0.0, offsets[1]).x, textureLodOffset(sampler2D(uDepth, uSampler), inUv.xy, 0.0, offsets[2]).x);
        ivec2 index = mix(ivec2(2, 0), ivec2(1, 2), lessThan(batch1, batch2));
        batch1 = mix(batch1, batch2, lessThan(batch1, batch2));

        vec2 batch3 = vec2(textureLodOffset(sampler2D(uDepth, uSampler), inUv.xy, 0.0, offsets[3]).x, textureLodOffset(sampler2D(uDepth, uSampler), inUv.xy, 0.0, offsets[4]).x);
        index = mix(index, ivec2(3, 4), lessThan(batch1, batch3));
        batch1 = mix(batch1, batch3, lessThan(batch1, batch3));

        ivec2 offset = offsets[batch1.x < batch1.y?index.x:index.y];
        float depth = batch1.x <batch1.y ? batch1.x :  batch1.y;

        velUv += offset * (uPc.viewportSizeInvSize.zw);
    }
    // multiply velocity to correct uv offsets
    return textureLod(sampler2D(uVelocity, uSampler), velUv, 0).xy * uPc.viewportSizeInvSize.zw;
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

    // Box filter for history
    // diamond shape
    vec3 bc1 = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(0, -1)).rgb;
    vec3 bc3 = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(-1, 0)).rgb;
    // center sample
    vec3 bc4 = currColor.rgb;
    vec3 min13 = min(min(bc1, bc3), bc4);
    vec3 max13 = max(max(bc1, bc3), bc4);

    vec3 bc5 = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(1, 0)).rgb;
    vec3 bc7 = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(0, 1)).rgb;
    vec3 min57 = min(bc5, bc7);
    vec3 max57 = max(bc5, bc7);
    vec3 boxMin = min(min13, min57);
    vec3 boxMax = max(max13, max57);

    //
    vec3 bc0 = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(-1, -1)).rgb;
    vec3 bc2 = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(1, -1)).rgb;
    vec3 min02 = min(boxMin, min(bc0, bc2));
    vec3 max02 = max(boxMax, max(bc0, bc2));

    vec3 bc6 = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(-1, 1)).rgb;
    vec3 bc8 = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(1, 1)).rgb;
    vec3 min68 = min(bc6, bc8);
    vec3 max68 = max(bc6, bc8);

    // corners
    const vec3 boxMinCorner = min(min02, min68);
    const vec3 boxMaxCorner = max(max02, max68);

    // NOTE: add
    // filter with tonemapping and yCoCG

    boxMin = (boxMin + boxMinCorner) * 0.5;
    boxMax = (boxMax + boxMaxCorner) * 0.5;

    // NOTE: add option to use clipping
    history.rgb = clamp(history.rgb, boxMin, boxMax);

    // luma based tonemapping as suggested by Karis
    const float historyLuma = CalcLuma(history.rgb);
    const float colorLuma = CalcLuma(currColor.rgb);

    history.rgb *= 1.0 / (1.0 + historyLuma);
    currColor.rgb *= 1.0 / (1.0 + colorLuma);

    const float taaAlpha = uPc.factor.a;
    vec4 color = mix(history, currColor, taaAlpha);

    // inverse tonemap
    color.rgb *= 1.0 / (1.0 - CalcLuma(color.rgb));

    // safety for removing negative values
    outColor = max(color, vec4(0.0));
}
