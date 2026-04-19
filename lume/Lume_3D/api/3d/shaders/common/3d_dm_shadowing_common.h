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

#ifndef SHADERS_COMMON_3D_DM_SHADOWING_COMMON_H
#define SHADERS_COMMON_3D_DM_SHADOWING_COMMON_H

#include "render/shaders/common/render_compatibility_common.h"

#extension GL_EXT_control_flow_attributes : require

/*
Basic shadowing functions
*/
// NOTE: has currently some issues
#define CORE_SHADOW_ENABLE_RECEIVER_PLANE_BIAS 1

vec2 ComputeReceiverPlaneDepthBias(const vec3 uvDdx, const vec3 uvDdy)
{
    vec2 uvBias;
    uvBias.x = uvDdy.y * uvDdx.z - uvDdx.y * uvDdy.z;
    uvBias.y = uvDdx.x * uvDdy.z - uvDdy.x * uvDdx.z;
    uvBias *= 1.0f / ((uvDdx.x * uvDdy.y) - (uvDdx.y * uvDdy.x));
    return uvBias;
}

float GetPcfSample(sampler2DShadow shadow, const vec2 baseUv, const vec2 offset, const float compareDepth,
    const vec2 texelSize, const vec2 receiverPlaneDepthBias)
{
    const vec2 uvOffset = offset * texelSize;
#if (CORE_SHADOW_ENABLE_RECEIVER_PLANE_BIAS == 1)
    const float compZ = compareDepth + dot(uvOffset, receiverPlaneDepthBias);
#else
    const float compZ = compareDepth;
#endif
    return texture(shadow, vec3(baseUv + uvOffset, compZ)).x;
}

float GetPcfSampleCmp(sampler2D shadow, const vec2 baseUv, vec2 offset, const float compareDepth,
    const vec2 texelSize, const vec2 receiverPlaneDepthBias)
{
    const vec2 uvOffset = offset * texelSize;
#if (CORE_SHADOW_ENABLE_RECEIVER_PLANE_BIAS == 1)
    const float compZ = compareDepth + dot(uvOffset, receiverPlaneDepthBias);
#else
    const float compZ = compareDepth;
#endif
    return float(int(texture(shadow, baseUV + uvOffset).x <= compareDepth));
}

float GetPcfSample(sampler2D shadow, const vec2 baseUv, const vec2 offset, const float compareDepth,
    const vec2 texelSize, const vec2 receiverPlaneDepthBias)
{
    const vec2 uvOffset = offset * texelSize;
#if (CORE_SHADOW_ENABLE_RECEIVER_PLANE_BIAS == 1)
    const float compZ = compareDepth + dot(uvOffset, receiverPlaneDepthBias);
#else
    const float compZ = compareDepth;
#endif
    return float(texture(shadow, baseUV + uvOffset).x);
}

float GetPcfSampleCmp(sampler2D shadow, const vec2 baseUv, const vec2 offset, const float compareDepth,
    const vec2 texelSize, const vec2 receiverPlaneDepthBias, const float bias, float dDepth_dx, float dDepth_dy,
    vec2 uvGradient_x, vec2 uvGradient_y)
{
    if (offset.x != 0 || offset.y != 0) {
        const vec2 uvOffset = offset * texelSize;
        float newDepth = 0;
        float det = uvGradient_x.x * uvGradient_y.y - uvGradient_x.y * uvGradient_y.x;
        if (abs(det) < 1e-6) {
            newDepth = compareDepth;
        } else {
            float px = (uvGradient_y.y * uvOffset.x - uvGradient_y.x * uvOffset.y) / det;
            float py = (-uvGradient_x.y * uvOffset.x + uvGradient_x.x * uvOffset.y) / det;
            newDepth = compareDepth + dDepth_dx * px + dDepth_dy * py;
        }
        newDepth = newDepth - bias;
        return float(int(texture(shadow, baseUV + uvOffset).x <= newDepth));
    } else {
        return 0;
    }
}

bool ValidShadowRange(const vec3 shadowCoord, float stepSize, float shadowIdx, const float texelSizeX)
{
    const float xMin = stepSize * shadowIdx;
    const float xMax = xMin + stepSize;
    return ((shadowCoord.z > 0.0) && (shadowCoord.x > xMin) && (shadowCoord.x < xMax - texelSizeX * 2.0f));
}

float Hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void InitPoissonDiskSamples(vec2 randomSeed, out vec2[64] disk, float radius, int numSamples)
{
    float angle = 3.1415 * 3.1415;
    const float ANGLE_STEP = 3.883222077450933;
    float radiusStep = radius / numSamples;
    float temp = 0;
    for (int i = 0; i < numSamples; i++)
    {
        disk[i] = vec2(cos(angle), sin(angle)) * pow(temp / radius, 0.75) * radius;
        temp += radiusStep;
        angle += ANGLE_STEP;
    }
}

//Variable PCF implementation with adjustable radius and sample count (4-64)
float CalcVariablePcfShadow(
    sampler2D shadow, vec4 inShadowCoord, float NoL, vec4 shadowFactor, vec4 atlasSizeInvSize, uvec2 shadowFlags,
    float sampleRadius, int sampleCount)
{
    //divide for perspective (just in case if used)
    const vec3 shadowCoord = inShadowCoord.xyz / inShadowCoord.w;

    float light = 1.0;
    if (ValidShadowRange(shadowCoord, shadowFactor.w, float(shadowFlags.x), atlasSizeInvSize.z)) {
        const vec2 textureSize = atlasSizeInvSize.xy;
        const vec2 texelSize = atlasSizeInvSize.zw;
        const float normalBias = shadowFactor.z;
        const float depthBias = shadowFactor.y;
        float bias = max(normalBias * (1.0 - NoL), depthBias);

        const vec2 offset = vec2(0.5);
        const vec2 uv = (shadowCoord.xy * textureSize) + offset;
        vec2 baseUv = (floor(uv) - offset) * texelSize;

#if (CORE_SHADOW_ENABLE_RECEIVER_PLANE_BIAS == 1)
        const vec3 shadowDdx = dFdx(shadowCoord);
        const vec3 shadowDdy = dFdy(shadowCoord);
        const vec2 receiverPlaneDepthBias = ComputeReceiverPlaneDepthBias(shadowDdx, shadowDdy);

        //static depth biasing for incorrect fractional sampling
        const float fSamplingError = 2.0 * dot(vec2(1.0) * texelSize, abs(receiverPlaneDepthBias));

        const float compareDepth = shadowCoord.z - min(fSamplingError, 0.01);
#else
        // NOTE: not used for this purpose ATM
        const vec2 receiverPlaneDepthBias = vec2(shadowFactor.y, shadowFactor.y);
        const float compareDepth = shadowCoord.z - bias;
#endif

        float sum = 0.0;
        int validSamples = 0;

        //Clamp sample count to valid range
        int clampedSampleCount = clamp(sampleCount, 0, 64);

        vec2 possion[64];
        vec2 seed = shadowCoord.xy * gl_FragCoord.xy;
        InitPoissonDiskSamples(seed, possion, sampleRadius, clampedSampleCount);

        float dDepth_dx = dFdx(compareDepth);
        float dDepth_dy = dFdy(compareDepth);
        vec2 uvGradient_x = dFdx(baseUv);
        vec2 uvGradient_y = dFdy(baseUv);

        [[unroll]]for (int i = 0; i < 32; i++)
        {
            if (i >= clampedSampleCount) {
                break;
            }
            sum += GetPcfSampleCmp(shadow, baseUv, possion[i], compareDepth, texelSize, receiverPlaneDepthBias, bias,
                dDepth_dx, dDepth_dy, uvGradient_x, uvGradient_y);
            validSamples++;
        }

        if (validSamples > 0) {
            sum /= float(validSamples);
            //shadow strength
            light = 1.0 - (sum * shadowFactor.x);
        }
    }
    return light;
}

// http://www.ludicon.com/castano/blog/articles/shadow-mapping-summary-part-1/
// MIT license: https://github.com/TheRealMJP/Shadows/blob/master/Shadows/Mesh.hlsl
float CalcPcfShadow(
    sampler2DShadow shadow, vec4 inShadowCoord, float NoL, vec4 shadowFactor, vec4 atlasSizeInvSize, uvec2 shadowFlags)
{
    // divide for perspective (just in case if used)
    const vec3 shadowCoord = inShadowCoord.xyz / inShadowCoord.w;

    CORE_RELAXEDP float light = 1.0;
    // Check if shadow coord is in correct range
    // We pass texelSize.x so that valid shadow range include 3x3 PCF kernel samples
    if (ValidShadowRange(shadowCoord, shadowFactor.w, float(shadowFlags.x), atlasSizeInvSize.z)) {
        const vec2 textureSize = atlasSizeInvSize.xy;
        const vec2 texelSize = atlasSizeInvSize.zw;
        const float normalBias = shadowFactor.z;
        const float depthBias = shadowFactor.y;
        const float bias = max(normalBias * (1.0 - NoL), depthBias);

        const vec2 offset = vec2(0.5);
        const vec2 uv = (shadowCoord.xy * textureSize) + offset;
        vec2 baseUv = (floor(uv) - offset) * texelSize;
        const float fracS = fract(uv.x);
        const float fracT = fract(uv.y);

#if (CORE_SHADOW_ENABLE_RECEIVER_PLANE_BIAS == 1)
        const vec3 shadowDdx = dFdx(shadowCoord);
        const vec3 shadowDdy = dFdy(shadowCoord);
        const vec2 receiverPlaneDepthBias = ComputeReceiverPlaneDepthBias(shadowDdx, shadowDdy);

        const float MAX_SAMPLING_ERROR = 0.0005f;
        const float fSamplingError = min(dot(abs(receiverPlaneDepthBias), texelSize), MAX_SAMPLING_ERROR);

        const float compareDepth = shadowCoord.z - bias - fSamplingError;
#else
        // NOTE: not used for this purpose ATM
        const vec2 receiverPlaneDepthBias = vec2(shadowFactor.y, shadowFactor.y);
        const float compareDepth = shadowCoord.z - bias;
#endif

        const float uw0 = 4.0 - 3.0 * fracS;
        const float uw1 = 7.0;
        const float uw2 = 1.0 + 3.0 * fracS;

        const float u0 = (3.0 - 2.0 * fracS) / uw0 - 2.0;
        const float u1 = (3.0 + fracS) / uw1;
        const float u2 = fracS / uw2 + 2.0;

        const float vw0 = 4.0 - 3.0 * fracT;
        const float vw1 = 7.0;
        const float vw2 = 1.0 + 3.0 * fracT;

        const float v0 = (3.0 - 2.0 * fracT) / vw0 - 2.0;
        const float v1 = (3.0 + fracT) / vw1;
        const float v2 = fracT / vw2 + 2.0;

        CORE_RELAXEDP float sum = 0;

        sum += uw0 * vw0 * GetPcfSample(shadow, baseUv, vec2(u0, v0), compareDepth, texelSize, receiverPlaneDepthBias);
        sum += uw1 * vw0 * GetPcfSample(shadow, baseUv, vec2(u1, v0), compareDepth, texelSize, receiverPlaneDepthBias);
        sum += uw2 * vw0 * GetPcfSample(shadow, baseUv, vec2(u2, v0), compareDepth, texelSize, receiverPlaneDepthBias);

        sum += uw0 * vw1 * GetPcfSample(shadow, baseUv, vec2(u0, v1), compareDepth, texelSize, receiverPlaneDepthBias);
        sum += uw1 * vw1 * GetPcfSample(shadow, baseUv, vec2(u1, v1), compareDepth, texelSize, receiverPlaneDepthBias);
        sum += uw2 * vw1 * GetPcfSample(shadow, baseUv, vec2(u2, v1), compareDepth, texelSize, receiverPlaneDepthBias);

        sum += uw0 * vw2 * GetPcfSample(shadow, baseUv, vec2(u0, v2), compareDepth, texelSize, receiverPlaneDepthBias);
        sum += uw1 * vw2 * GetPcfSample(shadow, baseUv, vec2(u1, v2), compareDepth, texelSize, receiverPlaneDepthBias);
        sum += uw2 * vw2 * GetPcfSample(shadow, baseUv, vec2(u2, v2), compareDepth, texelSize, receiverPlaneDepthBias);

        sum *= 1.0f / 144.0f;
        // shadow strenght
        light = 1.0 - (sum * shadowFactor.x);
    }

    return light;
}
float CalcPcfShadowMed(
    sampler2DShadow shadow, vec4 inShadowCoord, float NoL, vec4 shadowFactor, vec4 atlasSizeInvSize, uvec2 shadowFlags)
{
    // divide for perspective (just in case if used)
    const vec3 shadowCoord = inShadowCoord.xyz / inShadowCoord.w;

    CORE_RELAXEDP float light = 1.0;
    if (ValidShadowRange(shadowCoord, shadowFactor.w, float(shadowFlags.x), atlasSizeInvSize.z)) {
        const vec2 textureSize = vec2(textureSize(shadow, 0).xy);
        const vec2 texelSize = 1.0 / textureSize;
        const float normalBias = shadowFactor.z;
        const float depthBias = shadowFactor.y;
        const float bias = max(normalBias * (1.0 - NoL), depthBias);

        const vec2 offset = vec2(0.5);
        const vec2 uv = (shadowCoord.xy * textureSize) + offset;
        vec2 baseUv = (floor(uv) - offset) * texelSize;
        const float fracS = fract(uv.x);
        const float fracT = fract(uv.y);

#if (CORE_SHADOW_ENABLE_RECEIVER_PLANE_BIAS == 1)
        const vec3 shadowDdx = dFdx(shadowCoord);
        const vec3 shadowDdy = dFdy(shadowCoord);
        const vec2 receiverPlaneDepthBias = ComputeReceiverPlaneDepthBias(shadowDdx, shadowDdy);

        // static depth biasing for incorrect fractional sampling
        const float fSamplingError = 2.0 * dot(vec2(1.0) * texelSize, abs(receiverPlaneDepthBias));

        const float compareDepth = shadowCoord.z - min(fSamplingError, 0.01);
#else
        // NOTE: not used for this purpose ATM
        const vec2 receiverPlaneDepthBias = vec2(shadowFactor.y, shadowFactor.y);
        const float compareDepth = shadowCoord.z - bias;
#endif

        const float uw0 = (3 - 2 * fracS);
        const float uw1 = (1 + 2 * fracS);

        const float u0 = (2 - fracS) / uw0 - 1;
        const float u1 = fracS / uw1 + 1;

        const float vw0 = (3 - 2 * fracT);
        const float vw1 = (1 + 2 * fracT);

        float v0 = (2 - fracT) / vw0 - 1;
        float v1 = fracT / vw1 + 1;

        CORE_RELAXEDP float sum = 0;

        sum += uw0 * vw0 * GetPcfSample(shadow, baseUv, vec2(u0, v0), compareDepth, texelSize, receiverPlaneDepthBias);
        sum += uw1 * vw0 * GetPcfSample(shadow, baseUv, vec2(u1, v0), compareDepth, texelSize, receiverPlaneDepthBias);

        sum += uw0 * vw1 * GetPcfSample(shadow, baseUv, vec2(u0, v1), compareDepth, texelSize, receiverPlaneDepthBias);
        sum += uw1 * vw1 * GetPcfSample(shadow, baseUv, vec2(u1, v1), compareDepth, texelSize, receiverPlaneDepthBias);

        sum *= (1.0f / 16.0f);
        // shadow strenght
        light = 1.0 - (sum * shadowFactor.x);
    }

    return light;
}

float CalcPcfShadowSimpleSample(
    sampler2DShadow shadow, vec4 inShadowCoord, vec4 shadowFactor, vec4 atlasSizeInvSize, uvec2 shadowFlags)
{
    // divide for perspective (just in case if used)
    const vec3 shadowCoord = inShadowCoord.xyz / inShadowCoord.w;

    CORE_RELAXEDP float light = 1.0;
    if (ValidShadowRange(shadowCoord, shadowFactor.w, float(shadowFlags.x), 0.0f)) {
        const float bias = 0.002;
        const vec2 baseUv = shadowCoord.xy;
        const float compareDepth = shadowCoord.z - bias;
        const CORE_RELAXEDP float sum = texture(shadow, vec3(baseUv, compareDepth)).x;

        light = 1.0 - (sum * shadowFactor.x);
    }

    return light;
}

// https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch08.html
// reduce light bleeding
float LinstepVsm(float minVal, float maxVal, float v)
{
    return clamp((v - minVal) / (maxVal - minVal), 0.0, 1.0);
}
float ReduceLightBleedingVsm(float pMax, float amount)
{
    // Remove the [0, amount] tail and linearly rescale (amount, 1].
    return LinstepVsm(amount, 1.0, pMax);
}

float CalcVsmShadow(
    sampler2D shadow, vec4 inShadowCoord, float NoL, vec4 shadowFactors, vec4 atlasSizeInvSize, uvec2 shadowFlags)
{
    // divide for perspective (just in case if used)
    const vec3 shadowCoord = inShadowCoord.xyz / inShadowCoord.w;

    CORE_RELAXEDP float light = 1.0;
    if (ValidShadowRange(shadowCoord, shadowFactors.w, float(shadowFlags.x), 0.0f)) {
        const vec2 moments = texture(shadow, shadowCoord.xy).xy;
        if (shadowCoord.z > moments.x) {
            float variance = moments.y - (moments.x * moments.x);
            const float evaluatedMinValue = 0.00001;
            variance = max(variance, evaluatedMinValue);

            const float d = shadowCoord.z - moments.x;
            light = variance / (variance + d * d);
            const float evaluatedLightBleedingVal = 0.2;
            light = ReduceLightBleedingVsm(light, evaluatedLightBleedingVal);
            light = 1.0 - (1.0 - light) * shadowFactors.x;
        }
    }

    return light;
}

float CalcVsmShadowSimpleSample(
    sampler2D shadow, vec4 inShadowCoord, vec4 shadowFactors, vec4 atlasSizeInvSize, uvec2 shadowFlags)
{
    // divide for perspective (just in case if used)
    const vec3 shadowCoord = inShadowCoord.xyz / inShadowCoord.w;

    CORE_RELAXEDP float light = 1.0;
    if (ValidShadowRange(shadowCoord, shadowFactors.w, float(shadowFlags.x), 0.0f)) {
        const vec2 moments = texture(shadow, shadowCoord.xy).xy;
        if (shadowCoord.z > moments.x) {
            float variance = moments.y - (moments.x * moments.x);
            const float evaluatedMinValue = 0.00001;
            variance = max(variance, evaluatedMinValue);

            const float d = shadowCoord.z - moments.x;
            light = variance / (variance + d * d);
            const float evaluatedLightBleedingVal = 0.2;
            light = ReduceLightBleedingVsm(light, evaluatedLightBleedingVal);
            light = 1.0 - (1.0 - light) * shadowFactors.x;
        }
    }

    return light;
}

#endif // SHADERS_COMMON_3D_DM_SHADOWING_COMMON_H
