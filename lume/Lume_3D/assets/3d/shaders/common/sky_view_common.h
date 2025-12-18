/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SKY_VIEW_COMMON_H
#define SKY_VIEW_COMMON_H

#include "common/atmosphere_lut.h"

const int NUM_SCATTERING_STEPS = 32;

vec3 GetValFromTransmittanceLUT(ivec2 bufferRes, vec3 pos, vec3 sunDir)
{
    float height = length(pos);
    vec3 up = pos / height;
    float sunCosZenithAngle = dot(sunDir, up);

    // CRITICAL: If sun is below horizon from this position, return zero transmittance
    if (sunCosZenithAngle < 0.0 && RayIntersectSphere(pos, sunDir, GROUND_RADIUS_KM) > 0.0) {
        return vec3(0.0);
    }

    // Use Bruneton 2017 mapping for proper sampling
    vec2 uv = GetTransmittanceTextureUvFromRMu(height, sunCosZenithAngle);

    return texture(transmittanceLut, uv).rgb;
}

vec3 GetValFromMultipleScatteringLUT(ivec2 bufferRes, vec3 pos, vec3 sunDir)
{
    float height = length(pos);
    vec3 up = pos / height;
    float sunCosZenithAngle = dot(sunDir, up);

    float normalizedHeight = clamp((height - GROUND_RADIUS_KM) / (ATMOSPHERE_RADIUS_KM - GROUND_RADIUS_KM), 0.0, 1.0);

    vec2 uv;
    uv.x = clamp(0.5 + 0.5 * sunCosZenithAngle, 0.0, 1.0); // 0.5: parm
    uv.y = normalizedHeight;

    return texture(multipleScatteringLut, uv).rgb;
}

float GetSkyExposure(vec3 sunDir, float skyViewBrightness)
{
    // Higher exposure when sun is low
    float exposureBase = 20.0 * clamp(skyViewBrightness, 0.1, 10.0);
    float elevationFactor = smoothstep(-0.1, 0.4, sunDir.y);
    float exposure = exposureBase * mix(2.0, 0.5, elevationFactor);

    return exposure;
}

// https://www.shadertoy.com/view/fd2fWc
// Allow to get outside of the atmosphere by changing the start position of ray marching
vec3 RaymarchScattering(vec3 viewPos, vec3 rayDir, vec3 sunDir, int numSteps, const AtmosphericConfig atmosphericConfig)
{
    // Calculate atmosphere and ground intersections
    vec2 atmosIntercept = RayIntersectSphere2D(viewPos, rayDir, ATMOSPHERE_RADIUS_KM);
    float terraIntercept = RayIntersectSphere(viewPos, rayDir, GROUND_RADIUS_KM);

    float mindist;
    float maxdist;
    bool hitGround = false;

    // Check if the ray intersects the atmosphere at all
    if (atmosIntercept.x < atmosIntercept.y) {
        // there is an atmosphere intercept!
        // start at the closest atmosphere intercept
        // trace the distance between the closest and farthest intercept
        mindist = atmosIntercept.x > 0.0 ? atmosIntercept.x : 0.0;
        maxdist = atmosIntercept.y > 0.0 ? atmosIntercept.y : 0.0;
    } else {
        // No atmosphere intercept means no atmosphere!
        return vec3(0.0);
    }

    // If we're already in the atmosphere, start from the camera position
    if (length(viewPos) < ATMOSPHERE_RADIUS_KM) {
        mindist = 0.0;
    }

    // if there's a terra intercept that's closer than the atmosphere one,
    // use that instead!
    if (terraIntercept > 0.0) {
        maxdist = terraIntercept; // confirm valid intercepts
        hitGround = true;
    }

    // Start marching at the minimum distance
    vec3 pos = viewPos + mindist * rayDir;

    float cosTheta = dot(rayDir, sunDir);

    float miePhaseValue = GetMiePhase(cosTheta);
    float rayleighPhaseValue = GetRayleighPhase(-cosTheta);
    ivec2 transmittanceDim = textureSize(transmittanceLut, 0);
    ivec2 multipleScatteringDim = textureSize(multipleScatteringLut, 0);

    vec3 lum = vec3(0.0);
    vec3 transmittance = vec3(1.0);
    float t = 0.0;

    // Ray march through the valid segment of atmosphere
    float segmentLength = maxdist - mindist;
    for (uint i = 0; i < numSteps; i++) {
        float newT = ((i + 0.3) / float(numSteps)) * segmentLength;
        float dt = newT - t;
        t = newT;

        vec3 newPos = pos + t * rayDir;

        vec3 rayleighScattering = vec3(0.0);
        vec3 extinction = vec3(0.0);
        float mieScattering;
        GetScatteringValues(newPos, atmosphericConfig, rayleighScattering, mieScattering, extinction);
        extinction = max(extinction, 1e-6);

        vec3 sampleTransmittance = exp(-dt * extinction);
        vec3 sunTransmittance = GetValFromTransmittanceLUT(transmittanceDim, newPos, sunDir);
        vec3 psiMS = GetValFromMultipleScatteringLUT(multipleScatteringDim, newPos, sunDir);

        vec3 rayleighInScattering = rayleighScattering * (rayleighPhaseValue * sunTransmittance + psiMS);
        vec3 mieInScattering = mieScattering * (miePhaseValue * sunTransmittance + psiMS);
        vec3 inScattering = (rayleighInScattering + mieInScattering);
        vec3 scatteringIntegral = (inScattering - inScattering * sampleTransmittance) / extinction;

        lum += scatteringIntegral * transmittance;

        transmittance *= sampleTransmittance;
    }

    if (hitGround) {
        float blendFactor = max(0.0, -sunDir.y);
        lum = blendFactor * atmosphericConfig.groundColor;
    }

    lum *= GetSkyExposure(-sunDir, atmosphericConfig.skyViewBrightness);

    return lum;
}

#endif // SKY_VIEW_COMMON_H
