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
#ifndef CLOUDS_POSTPROCESS_H
#define CLOUDS_POSTPROCESS_H

#include "common/cloud_common.h"
#include "common/cloud_scattering.h"
#include "common/cloud_structs.h"

vec4 Postprocess(in Ray ray, in VolumeSample o, in int colorType)
{
    vec4 finalColor;

    // reconstruct the particle
    float d = o.depth <= 0.0 ? 1 : o.depth;
    vec3 p = ray.o + ray.dir * d;
    vec3 L = normalize(uSunPos() - p);
    vec3 V = normalize(p - ray.o);
    vec3 B = normalize(vec3(ray.dir.x, 0, ray.dir.z));
    float s = dot(L, normalize(B));

    vec3 planetHit =
        RaySphereIntersect(ray.o - DEFAULT_PLANET_POS, ray.dir, DEFAULT_PLANET_RADIUS - DEFAULT_PLANET_BIAS);
    float maxDepth =
        planetHit.z == 1.0 && planetHit.x >= 0 && planetHit.y >= 0 ? min(min(planetHit.x, planetHit.y), 1e12) : 1e12;
    vec3 normal = normalize(ray.o - DEFAULT_PLANET_POS + ray.dir * d);
    float NdotL = dot(normal, normalize(uSunPos()));

    float Cos_angle = dot(L, V);
    float phase = HenyeyGreensteinPhase(Cos_angle, 0.2, silverIntensity(), silverSpread());

    // calculate the skybox color, could do subpassLoads
    ScatteringResult scattSample;
    ScatteringResult scattLight;
    ScatteringResult scattCamera2;
    ScatteringResult scattReal;

    float max_atmos = DEFAULT_ATMOS_RADIUS * 3;

    float max_distance = 1e12;

    [[branch]] if (o.transmission == 1) {
        return vec4(0, 0, 0, 0);
    }
    [[branch]] if (o.depth <= 0) {
        return vec4(0, 0, 0, 0);
    }

    if (planetHit.z == 1.0 && planetHit.x > 0)
        max_distance = min(max_distance, max(planetHit.x, 0.0));
    if (planetHit.z == 1.0 && planetHit.y > 0)
        max_distance = min(max_distance, max(planetHit.y, 0.0));
    float max_cloud_depth = o.depth <= 0.0 ? max_distance : min(max_distance, o.depth / scatteringDistance());

    ScatteringParams params = DefaultParameters();
    params.numStepsPrimary = 2;
    params.numStepsLight = 4;

    CalculateScatteringComponents(Camera(ray.o,         // the position of the camera
                                      ray.dir,          // the camera vector (ray direction of this pixel)
                                      max_cloud_depth), // max dist, essentially the scene depth
        ScatteringLightSource(normalize(uSunPos()),     // light direction
            vec3(40.0)),
        params, scattSample);

    params.numStepsPrimary = 2;
    params.numStepsLight = 4;

    CalculateScatteringComponents(Camera(ray.o + ray.dir * max_cloud_depth, // the position of the camera
                                      ray.dir, // the camera vector (ray direction of this pixel)
                                      max_distance - max_cloud_depth), // max dist, essentially the scene depth
        ScatteringLightSource(normalize(uSunPos()),                    // light direction
            vec3(40.0)),
        params, scattCamera2);

    params.numStepsPrimary = 2;
    params.numStepsLight = 4;

    CalculateScatteringComponents(Camera(ray.o,             // the position of the camera
                                      ray.dir,              // the camera vector (ray direction of this pixel)
                                      max_distance - 1000), // max dist, essentially the scene depth
        ScatteringLightSource(normalize(uSunPos()),         // light direction
            vec3(40.0)),
        params, scattReal);

    params.numStepsPrimary = 1;
    params.numStepsLight = 8;

    CalculateScatteringComponents(Camera(ray.o + ray.dir * d, // the position of the camera
                                      ray.dir,                // the camera vector (ray direction of this pixel)
                                      100),                   // max dist, essentially the scene depth
        ScatteringLightSource(normalize(uSunPos()),           // light direction
            vec3(40.0)),
        params, scattLight);

    const float cloudFadeOutPoint = 0.06f;
    float blend = smoothstep(0.0, 1.0, min(1.0, remap(ray.dir.y, cloudFadeOutPoint, 0.1f, 0.0f, 1.0f)));

    if (colorType == 0) {
        [[branch]] if (false) {
        }
        else {

            float ambient_energy = 1;
            float direct_energy = 1;

            vec3 ambientColor = vec3(0, 0, 0);
            vec3 directColor = vec3(0, 0, 0);

            float ambient_term = 1;
            float direct_term = 1;

            float blend = mix(0.0, 0.08,
                clamp(Luminance(ScatterTonemapping(scattCamera2.mie + scattCamera2.ray + scattCamera2.ambient)), 0, 1));

            float int_ = 0.0;
            float x = Luminance(ScatterTonemapping(scattReal.mie + scattReal.ray + scattReal.ambient)) +
                      0.1 * Luminance(scattReal.opacity) + NdotL * 0.9 * Luminance(scattReal.opacity);
            x = clamp(x * 2.2, 0, 1);
            int_ = x * 0.02;

            directColor = scattReal.mie + scattReal.ray + scattReal.ambient;

            directColor = ScatterTonemapping(directColor * (1 - o.transmission));
            directColor +=
                ScatterTonemapping((scattLight.mie + scattLight.ray + scattLight.ambient) * (o.transmission) +
                                   max(0.0015, NdotL) * (scattLight.opacity) * (o.transmission));
            directColor = (scattCamera2.mie + scattCamera2.ray + scattCamera2.ambient) * (1 / 160) * int_ +
                          directColor * scattCamera2.opacity;
            directColor = ScatterTonemapping(directColor);

            ambientColor = vec3(1, 1, 1);
            ambientColor = ScatterTonemapping((scattLight.mie + scattLight.ray + scattLight.ambient) +
                                              max(vec3(0.01, 0.01, 0.015), NdotL) * Luminance(scattLight.opacity));
            ambientColor = (scattCamera2.mie + scattCamera2.ray + scattCamera2.ambient) * (1 / 160) * int_ +
                           ambientColor * scattCamera2.opacity;
            ambientColor = ScatterTonemapping(ambientColor);
            ambientColor = ambientColor * mix(0.2, 0.8, phase);

            finalColor = vec4(ambientColor * o.ambient * ambientStrength() * ambient_term +
                                  directColor * o.intensity * direct_term * directStrength(),
                mix(1 - blend, 0, o.transmission));

            // apply scattering from the sample to the camera
            finalColor = vec4(ScatterTonemapping(scattSample.mie + scattSample.ray + scattSample.ambient) * 0.5 +
                                  finalColor.rgb * scattSample.opacity,
                finalColor.a);

            // apply scattering tone mapping
            finalColor = vec4(ScatterTonemapping(finalColor.rgb), finalColor.a);
        }

        [[branch]] if (isnan(o.transmission) || isinf(finalColor.x) || isinf(finalColor.y) || isinf(finalColor.z) ||
                       isinf(finalColor.w) || isnan(finalColor.x) || isnan(finalColor.y) || isnan(finalColor.z) ||
                       isnan(finalColor.w)) {
            finalColor = vec4(1, 1, 0, 1);
            finalColor = vec4(1, 0, 1, 1 - o.transmission);
        }
    }

    return finalColor;
}

#endif // CLOUDS_POSTPROCESS_H
