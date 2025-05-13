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
#ifndef CLOUDS_LIGHTING_H
#define CLOUDS_LIGHTING_H

#include "common/cloud_math.h"

// the Henyey-Greenstein formula is used to approximate Mie scattering which is too
// computationally expensive
float HenyeyGreenstein(float cosAngle, float eccentricity)
{
    float eccentricity2 = eccentricity * eccentricity;
    return ((1.0 - eccentricity2) / pow(1.0 + eccentricity2 - 2.0 * eccentricity * cosAngle, 3.0 / 2.0)) / (4 * PI);
}

// calculates single scattering using henyey-sreenstein approximations
float HenyeyGreensteinPhase(float cosAngle, float eccintricity, float silverIntensity, float silverSpread)
{
    float HG1 = HenyeyGreenstein(cosAngle, eccintricity);
    float HG2 = HenyeyGreenstein(cosAngle, 0.99 - silverSpread);
    float hg = max(HG1, silverIntensity * HG2);
    return hg;
}

float GetLightEnergyA(vec3 p, float heightFraction, float dl, float dsLoded, float phaseProbability, float cosAngle,
    float stepSize, float brightness)
{
    dl = max(0, dl);

    // attenuation difference from slides reduce the secondary component when we look toward the sun.
    highp float primaryAttenuation = exp(-dl);
    highp float secondaryAttenuation = exp(-dl * 0.25) * 0.7;
    float attenuationProbability =
        max(remap(cosAngle, 0.7, 1.0, secondaryAttenuation, secondaryAttenuation * 0.25), primaryAttenuation);

#if DEBUG_
    if (isinf(attenuationProbability) || isnan(attenuationProbability)) {
        debugPrintfEXT(
            "GetLightEnergyA: [attenuation_probability %f %f primary_attenuation %f secondary_attenuation %f",
            attenuationProbability, dl, primaryAttenuation, secondaryAttenuation);
        return 0;
    }
#endif

    // in-scattering one difference from presentation slides we also reduce this effect once light has attenuated to
    // make it directional.

    float lightSize = clamp(dl / stepSize, 0, 1);
    float h = remap(heightFraction, 0.3, 0.85, 0.5, 2.0);
    float c = dsLoded > 0 ? pow(clamp(dsLoded, 0, 1), h) : 0;
    float depthProbability = mix(0.05 + c, 1.0, lightSize);

#if DEBUG_
    if (isinf(depthProbability) || isnan(depthProbability)) {
        debugPrintfEXT("GetLightEnergyA: [depth %f ds_loded %f  height %f]", c, dsLoded, h);
        return 0;
    }
#endif

    float verticalProbability = pow(max(0.1, remap(heightFraction, 0.07, 0.14, 0.1, 1.0)), 0.8);

#if DEBUG_
    if (isinf(verticalProbability) || isnan(verticalProbability)) {
        debugPrintfEXT("GetLightEnergyA: [depth %f ds_loded %f  height %f]", c, dsLoded, h);
        return 0;
    }
#endif

    float inScatterProbability = depthProbability * verticalProbability;

    float lightEnergy = attenuationProbability * inScatterProbability * phaseProbability * brightness;

#if DEBUG_
    if (isinf(lightEnergy) || isnan(lightEnergy)) {
        debugPrintfEXT("GetLightEnergyA: [depth %f ds_loded %f  height %f]", c, dsLoded, h);
        return 0;
    }
#endif

    return attenuationProbability * phaseProbability * inScatterProbability * brightness;
}

#endif // CLOUDS_LIGHTING_H