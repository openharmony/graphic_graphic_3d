#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_post_process_common.h"
#include "render/shaders/common/render_tonemap_common.h"

// sets

layout(set = 0, binding = 0) uniform texture2D uDepth;
layout(set = 0, binding = 1) uniform texture2D uColor;
layout(set = 0, binding = 2) uniform texture2D uVelocity;
layout(set = 0, binding = 3) uniform texture2D uHistory;
layout(set = 0, binding = 4) uniform sampler uSampler;

layout(push_constant, std430) uniform uPostProcessPushConstant
{
    LocalPostProcessPushConstantStruct uPc;
};

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

// NOTE: cannot be used (remove if not used for any input)
#define ENABLE_INPUT_ATTACHMENTS 0

#define QUALITY_LOW 0
#define QUALITY_MED 1
#define QUALITY_HIGH 2

// if the magnitude of the velocity vector is greater than
// the threshold, the pixel is considered to be in motion.
#define STATIONARY_VELOCITY_THRESHOLD 0.001
// used to detect if the current pixel is an edge
#define DEPTH_DIFF 0.0005

float GetUnpackDepthBuffer(const vec2 uv) {
#if (ENABLE_INPUT_ATTACHMENTS == 1)
    return subpassLoad(uDepth).x;
#else
    return textureLod(sampler2D(uDepth, uSampler), uv, 0).x;
#endif
}

vec2 GetUnpackVelocity(const vec2 uv, const vec2 invSize) {
#if (ENABLE_INPUT_ATTACHMENTS == 1)
    return subpassLoad(uVelocity).xy;
#else
    return textureLod(sampler2D(uVelocity, uSampler), uv, 0).xy * invSize;
#endif
}

vec3 RGBToYCoCg(const vec3 rgb) {
    const float co = rgb.r - rgb.b;
    const float tmp = rgb.b + co / 2.0;
    const float cg = rgb.g - tmp;
    const float y = tmp + cg / 2.0;
    return vec3(y, co, cg);
}

vec3 YCoCgToRGB(const vec3 ycocg) {
    const float tmp = ycocg.r - ycocg.b / 2.0;
    const float g = ycocg.b + tmp;
    const float b = tmp - ycocg.g / 2.0;
    const float r = ycocg.g + b;
    return vec3(r, g, b);
}

vec2 GetVelocity(out bool isEdge) {
    const uint quality = uint(uPc.factor.y + 0.5);
    vec2 velUv = inUv;

    if (quality == QUALITY_MED) {
        //  sample 5 values in a cross pattern

        const uint offsetCount = 5;
        const ivec2 offsets[offsetCount] = {
            ivec2(-1, -1),
            ivec2(1, -1),
            ivec2(0, 0),
            ivec2(-1, 1),
            ivec2(1, 1),
        };

        float depths[offsetCount];

        const ivec2 accessOffsets[4] = {
            ivec2(-1, -1),
            ivec2(1, -1),
            ivec2(0, 0),
            ivec2(-1, 1)
        };
        const vec4 depth0123 = textureGatherOffsets(sampler2D(uDepth, uSampler), inUv.xy, accessOffsets, 0);

        depths[0] = depth0123.x;
        depths[1] = depth0123.y;
        depths[2] = depth0123.z;
        depths[3] = depth0123.w;
        depths[4] = textureLodOffset(sampler2D(uDepth, uSampler), inUv.xy, 0.0, offsets[4]).x;

        const float currentDepth = depths[2];

        float minDepth = depths[0];
        float avgDepth = depths[0];
        int minDepthIndex = 0;
        for (int ii = 1; ii < offsetCount; ++ii) {
            if (depths[ii] < minDepth) {
                minDepth = depths[ii];
                minDepthIndex = ii;
            }

            avgDepth += depths[ii];
        }

        const ivec2 offset = offsets[minDepthIndex];
        velUv += offset * (uPc.viewportSizeInvSize.zw);

        avgDepth /= float(offsetCount);
        isEdge = abs(currentDepth - avgDepth) > DEPTH_DIFF ? true : false;
    }
    else if (quality >= QUALITY_HIGH) {
        // sample a full 3x3 grid

        const uint offsetCount = 9;
        const ivec2 offsets[offsetCount] = {
            // the first gather square
            ivec2(-1, -1),
            ivec2(-1, 0),
            ivec2(0, -1),
            ivec2(0, 0),

            // the second gather square
            // ivec2(0, 0),
            ivec2(0, 1),
            ivec2(1, 0),
            ivec2(1, 1),

            // the remaining corners
            ivec2(1, -1),
            ivec2(-1, 1)
        };

        float depths[offsetCount];

        // textureGather samples from the given uv to uv + ivec2(1, 1) in a 2x2 pattern
        const vec4 depth0123 = textureGather(sampler2D(uDepth, uSampler), inUv.xy - ivec2(-1, -1), 0);
        const vec4 depth3456 = textureGather(sampler2D(uDepth, uSampler), inUv.xy, 0);

        depths[0] = depth0123.x;
        depths[1] = depth0123.y;
        depths[2] = depth0123.z;
        depths[3] = depth0123.w;

        depths[4] = depth3456.y;
        depths[5] = depth3456.z;
        depths[6] = depth3456.w;
        
        depths[7] = textureLodOffset(sampler2D(uDepth, uSampler), inUv.xy, 0.0, offsets[7]).x;
        depths[8] = textureLodOffset(sampler2D(uDepth, uSampler), inUv.xy, 0.0, offsets[8]).x;

        const float currentDepth = depths[2];

        float minDepth = depths[0];
        float avgDepth = depths[0];
        int minDepthIndex = 0;
        for (int ii = 1; ii < offsetCount; ++ii) {
            if (depths[ii] < minDepth) {
                minDepth = depths[ii];
                minDepthIndex = ii;
            }

            avgDepth += depths[ii];
        }

        const ivec2 offset = offsets[minDepthIndex];
        velUv += offset * (uPc.viewportSizeInvSize.zw);

        avgDepth /= float(offsetCount);
        isEdge = abs(currentDepth - avgDepth) > DEPTH_DIFF ? true : false;
    }
    // multiply velocity to correct uv offsets
    return textureLod(sampler2D(uVelocity, uSampler), velUv, 0).xy * uPc.viewportSizeInvSize.zw;
}

// clip towards aabb center
// e.g. "temporal reprojection anti-aliasing in inside"
vec4 ClipAabb(const vec3 aabbMin, const vec3 aabbMax, const vec4 color, const vec4 history) {
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

// clip the color to be inside a box defined by the center (mean) and size (variance or stdev)
vec3 VarianceClipAABB(const vec3 history, const vec3 currColor, const vec3 center, const vec3 size) {
    if (all(lessThanEqual(abs(history - center), size))) {
        return history;
    }

    const vec3 dir = currColor - history;
    const vec3 near = center - sign(dir) * size;
    const vec3 tAll = (near - history) / dir;

    // just some sufficiently large value
    float t = 1e20;
    
    for (int ii = 0; ii < 3; ii++) {
        if (tAll[ii] >= 0.0 && tAll[ii] < t) {
            t = tAll[ii];
        }
    }

    if (t >= 1e20) {
        return history;
    }

    return history + dir * t;
}

// mean-variance clip history color to acceptable values within the current frame color
vec4 CalcVarianceClippedHistoryColor(const vec3 history, const vec3 currColor, const bool useyCoCG) {
    const uint quality = uint(uPc.factor.y + 0.5);

    vec3 colors[9];
    uint numSamples = 0;

    if (quality <= QUALITY_MED) {
        // sample only in a cross pattern
        numSamples = 5;
        
        colors[0] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(0, -1)).rgb;
        colors[1] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(-1, 0)).rgb;
        colors[2] = currColor;
        colors[3] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(1, 0)).rgb;
        colors[4] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(0, 1)).rgb;
    } else {
        // sample all 9 values within the 3x3 grid
        numSamples = 9;
        
        colors[0] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(-1, -1)).rgb;
        colors[1] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(0, -1)).rgb;
        colors[2] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(1, -1)).rgb;
        colors[3] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(-1, 0)).rgb;
        colors[4] = currColor;
        colors[5] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(1, 0)).rgb;
        colors[6] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(-1, 1)).rgb;
        colors[7] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(0, 1)).rgb;
        colors[8] = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(1, 1)).rgb;
    }

    vec3 sum = vec3(0);
    vec3 sumSq = vec3(0);

    for (int ii = 0; ii < numSamples; ii++) {
        vec3 value = colors[ii];
        if (useyCoCG) {
            value = RGBToYCoCg(value);
        }
        sum += value;
        sumSq += value * value;
    }

    const vec3 mean = sum / float(numSamples);
    const vec3 variance = sqrt((sumSq / float(numSamples)) - mean * mean);
    
    const vec3 minColor = mean - variance;
    const vec3 maxColor = mean + variance;

    vec3 clampedHistoryColor;
    if (useyCoCG) {
        clampedHistoryColor = YCoCgToRGB(VarianceClipAABB(RGBToYCoCg(history), RGBToYCoCg(currColor), mean, variance));
    } else {
        clampedHistoryColor = VarianceClipAABB(history, currColor, mean, variance);
    }

    return vec4(clampedHistoryColor, 1.0);
}

vec4 CalcMinMaxClippedHistoryColor(const vec3 history, const vec3 currColor) {
    // sample 3x3 grid
    // 0 1 2
    // 3 4 5
    // 6 7 8

    // Box filter for history
    // diamond shape
    vec3 bc1 = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(0, -1)).rgb;
    vec3 bc3 = textureLodOffset(sampler2D(uColor, uSampler), inUv.xy, 0.0, ivec2(-1, 0)).rgb;
    // center sample
    vec3 bc4 = currColor;
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

    boxMin = (boxMin + boxMinCorner) * 0.5;
    boxMax = (boxMax + boxMaxCorner) * 0.5;
    return vec4(clamp(history, boxMin, boxMax), 1.0);
}

vec4 cubic(const float value) {
    const vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - value;
    const vec4 s = n * n * n;
    const float x = s.x;
    const float y = s.y - 4.0 * s.x;
    const float z = s.z - 4.0 * s.y + 6.0 * s.x;
    const float w = 6.0 - x - y - z;
    return vec4(x, y, z, w) * (1.0 / 6.0);
}

vec4 GetHistory(const vec2 historyUv, const vec4 currColor, const bool bicubic,
                const bool useVarianceClipping, const bool useyCoCG) {
    vec4 history;

    if (bicubic) {
        vec2 texCoords = historyUv * uPc.viewportSizeInvSize.xy - 0.5;

        const vec2 fxy = fract(texCoords);
        texCoords -= fxy;

        const vec4 xcubic = cubic(fxy.x);
        const vec4 ycubic = cubic(fxy.y);

        const vec4 c = texCoords.xxyy + vec2(-0.5, 1.5).xyxy;
        const vec4 s = vec4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
        const vec4 offset = (c + vec4(xcubic.yw, ycubic.yw) / s) * uPc.viewportSizeInvSize.zw.xxyy;

        const vec4 aa = textureLod(sampler2D(uHistory, uSampler), offset.xz, 0);
        const vec4 bb = textureLod(sampler2D(uHistory, uSampler), offset.yz, 0);
        const vec4 cc = textureLod(sampler2D(uHistory, uSampler), offset.xw, 0);
        const vec4 dd = textureLod(sampler2D(uHistory, uSampler), offset.yw, 0);

        const float sx = s.x / (s.x + s.y);
        const float sy = s.z / (s.z + s.w);

        history = mix(mix(dd, cc, sx), mix(bb, aa, sx), sy);
    } else {
        history = textureLod(sampler2D(uHistory, uSampler), historyUv, 0.0);
    }

    if (useVarianceClipping) {
        history = CalcVarianceClippedHistoryColor(history.rgb, currColor.rgb, useyCoCG);
    } else {
        history = CalcMinMaxClippedHistoryColor(history.rgb, currColor.rgb);
    }

    return history;
}

void UnpackFeatureToggles(out bool useBicubic, out bool useVarianceClipping, out bool useyCoCG, out bool ignoreEdges) {
    // reflected in LumeRender/src/datastore/render_data_store_post_process.h:GetFactorTaa()
    const uint combined = floatBitsToUint(uPc.factor.z);

    useBicubic = (combined & (1u << TAA_USE_BICUBIC_BIT)) != 0u;
    useVarianceClipping = (combined & (1u << TAA_USE_VARIANCE_CLIPPING_BIT)) != 0u;
    useyCoCG = (combined & (1u << TAA_USE_YCOCG_BIT)) != 0u;
    ignoreEdges = (combined & (1u << TAA_IGNORE_EDGES_BIT)) != 0u;
}

void main(void) {
    bool isEdge;
    const vec2 velocity = GetVelocity(isEdge);
    const vec2 historyUv = inUv.xy - velocity;

    bool useBicubic;
    bool useVarianceClipping;
    bool useyCoCG;
    bool ignoreEdges;
    UnpackFeatureToggles(useBicubic, useVarianceClipping, useyCoCG, ignoreEdges);

    /**
     * This is a bit of a hack since variance clipping causes flickering
     * without specific mitigations. Some ideas are discussed here, but 
     * just disabling variance clipping for stationary elements is enough
     * to remove the flickering without affecting the results otherwise.
     * https://advances.realtimerendering.com/s2014/epic/TemporalAA.pptx
     */
    if (length(velocity) <= STATIONARY_VELOCITY_THRESHOLD) {
        useVarianceClipping = false;
    }

    // Bicubic filtering makes edges look blurry; disable if an edge.
    if (isEdge && ignoreEdges) {
        useBicubic = false;
    }

    vec4 currColor = textureLod(sampler2D(uColor, uSampler), inUv.xy, 0.0);
    vec4 history = GetHistory(historyUv, currColor, useBicubic, useVarianceClipping, useyCoCG);

    // NOTE: add filtered option for less blurred history

    const float blendWeight = uPc.factor.a;

    // luma based tonemapping as suggested by Karis
    const float historyLuma = CalcLuma(history.rgb);
    const float colorLuma = CalcLuma(currColor.rgb);

    history.rgb *= 1.0 / (1.0 + historyLuma);
    currColor.rgb *= 1.0 / (1.0 + colorLuma);

    vec4 color = mix(history, currColor, blendWeight);
    
    // inverse tonemap
    color.rgb *= 1.0 / (1.0 - CalcLuma(color.rgb));

    // safety for removing negative values
    outColor = max(color, vec4(0.0));
}
