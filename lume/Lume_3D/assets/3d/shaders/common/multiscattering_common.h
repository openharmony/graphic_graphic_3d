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

#ifndef MULTIPLESCATTERING_COMMON_H
#define MULTIPLESCATTERING_COMMON_H

#include "common/atmosphere_lut.h"

const uint FLOAT_TO_UINT_SCALE = 1u << 24;
const float UINT_TO_FLOAT_SCALE = 1.0 / (1u << 24);

uint FloatToScaledUint(float value)
{
    return uint(clamp(value * float(FLOAT_TO_UINT_SCALE), 0.0, float(1u << 31))); // 31: offset
}

float ScaledUintToFloat(uint value)
{
    return float(value) * UINT_TO_FLOAT_SCALE;
}

uvec3 Vec3ToScaledUvec3(vec3 value)
{
    return uvec3(FloatToScaledUint(value.x), FloatToScaledUint(value.y), FloatToScaledUint(value.z));
}

vec3 ScaledUvec3ToVec3(uvec3 value)
{
    return vec3(ScaledUintToFloat(value.x), ScaledUintToFloat(value.y), ScaledUintToFloat(value.z));
}

vec3 GetValFromTransmittanceLUT(ivec2 bufferRes, vec3 pos, vec3 sunDir)
{
    float height = length(pos);
    vec3 up = pos / height;
    float sunCosZenithAngle = dot(sunDir, up);

    vec2 uv = GetTransmittanceTextureUvFromRMu(height, sunCosZenithAngle);

    return texture(transmittanceLut, uv).rgb;
}

void GetMulScattValues(vec3 pos, vec3 sunDir, inout vec3 currentThreadLum, inout vec3 currentThreadFms,
    const int totalSamples, const int numThreadsInWorkGroup, const int localIndex,
    const AtmosphericConfig atmosphericConfig)
{
    ivec2 transmittanceDim = textureSize(transmittanceLut, 0);

    // Distribute the SQRT_SAMPLES * SQRT_SAMPLES iterations among threads in the workgroup.
    // With local_size=8x8, each thread will perform exactly one iteration of this loop
    for (int k = localIndex; k < totalSamples; k += numThreadsInWorkGroup) {
        int i = k / SQRT_SAMPLES;
        int j = k % SQRT_SAMPLES;

        float theta = M_PI * (float(i) + 0.5) / float(SQRT_SAMPLES);
        float phi = Safeacos(1.0 - 2.0 * (float(j) + 0.5) / float(SQRT_SAMPLES));
        vec3 rayDir = GetSphericalDir(theta, phi);

        float atmoDist = RayIntersectSphere(pos, rayDir, ATMOSPHERE_RADIUS_KM);
        float groundDist = RayIntersectSphere(pos, rayDir, GROUND_RADIUS_KM);
        float tMax = atmoDist;
        if (groundDist > 0.0) {
            tMax = groundDist;
        }

        float cosTheta = dot(rayDir, sunDir);

        float miePhaseValue = GetMiePhase(cosTheta);
        float rayleighPhaseValue = GetRayleighPhase(-cosTheta);

        vec3 lum = vec3(0.0), lumFactor = vec3(0.0), transmittance = vec3(1.0);
        float t = 0.0;
        for (uint stepI = 0; stepI < MULTIPLE_SCATTERING_STEPS; stepI++) {
            float newT = ((stepI + 0.3) / float(MULTIPLE_SCATTERING_STEPS)) * tMax;
            float dt = newT - t;
            t = newT;

            vec3 newPos = pos + t * rayDir;

            vec3 rayleighScattering = vec3(0, 0, 0);
            vec3 extinction = vec3(0, 0, 0);
            float mieScattering;
            GetScatteringValues(newPos, atmosphericConfig, rayleighScattering, mieScattering, extinction);
            extinction = max(extinction, 1e-6);

            vec3 sampleTransmittance = exp(-dt * extinction);

            vec3 scatteringNoPhase = rayleighScattering + mieScattering;
            vec3 scatteringF = (scatteringNoPhase - scatteringNoPhase * sampleTransmittance) / extinction;
            lumFactor += transmittance * scatteringF;

            vec3 sunTransmittance = GetValFromTransmittanceLUT(transmittanceDim, newPos, sunDir);

            vec3 rayleighInScattering = rayleighScattering * rayleighPhaseValue;
            float mieInScattering = mieScattering * miePhaseValue;
            vec3 inScattering = (rayleighInScattering + mieInScattering) * sunTransmittance;

            vec3 scatteringIntegral = (inScattering - inScattering * sampleTransmittance) / extinction;

            lum += scatteringIntegral * transmittance;
            transmittance *= sampleTransmittance;
        }

        if (groundDist > 0.0) {
            vec3 hitPos = pos + groundDist * rayDir;
            hitPos = normalize(hitPos) * GROUND_RADIUS_KM;
            float blendFactor = max(0.0, sunDir.y);
            lum += blendFactor * transmittance * GROUND_ALBEDO *
                   GetValFromTransmittanceLUT(transmittanceDim, hitPos, sunDir);
        }

        currentThreadFms += lumFactor;
        currentThreadLum += lum;
    }
}

#endif // MULTIPLESCATTERING_COMMON_H
