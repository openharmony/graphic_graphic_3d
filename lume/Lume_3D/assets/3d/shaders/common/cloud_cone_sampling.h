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
#ifndef CLOUDS_CONESAMPLING_H
#define CLOUDS_CONESAMPLING_H

// calculate cone sampling similar to Meteoros
// with the last sample being 3 times the
// unit cone size
void ConeSampling(inout vec3 coneSamples[5])
{
    coneSamples[0] = vec3(0.1, 0.1, 0.0);
    coneSamples[1] = vec3(0.2, 0.0, 0.2);
    coneSamples[2] = vec3(0.4, -0.4, 0.0);
    coneSamples[3] = vec3(0.8, 0.0, -0.8);
    coneSamples[4] = vec3(3.0, 0.0, 0.0);
}

// creates a rotation matrix that rotates vector a onto
// vector b by performing a 2D rotation on the plane
// with normal a x b
// source: https://math.stackexchange.com/questions/180418/calculate-rotation-matrix-to-align-vector-a-to-vector-b-in-3d
mat3 ConeRotationMatrix(in vec3 coneDir, in vec3 lightDir)
{
    vec3 a = normalize(coneDir);
    vec3 b = normalize(lightDir);
    return mat3(dot(a, b), -length(cross(a, b)), 0.0, length(cross(a, b)), dot(a, b), 0.0, 0.0, 0.0, 1.0);
}

// take 5 samples in direction to the sun, where the last
// sample is further away than the rest to also capture
// clouds further away
float SampleConeDensity(in vec3 pos, in vec3 L, in float lightStep, in vec3 weatherData, float mipLevel,
    in vec3 coneSamples[6], in vec2 inCloudMinMax, bool baseOnly)
{
    // sample density along cone
    float cloudDensity = 0.0;
    float coneSpreadMultplier = lightStep;
    mat3 coneRot = ConeRotationMatrix(vec3(1, 0, 0), L);

    for (int i = 0; i < 6; i++) {
        pos += (L * lightStep) + coneRot * (coneSamples[i] * coneSpreadMultplier * (float(i) + 0.5));
        float mip_bias = float(int(i * 0.5));
        cloudDensity += SampleCloudDensity(pos, weatherData, mipLevel + mip_bias, inCloudMinMax, baseOnly);
    }

#if DEBUG_
    if (isinf(cloudDensity) || isnan(cloudDensity)) {
        debugPrintfEXT("sample cloud: [%f]", cloudDensity);
        return 0;
    }
#endif

    return cloudDensity;
}

float DensityToLight(in vec3 pos, in vec3 weatherData, float mipmap, in vec3 L, float stepSize, in int samples,
    int farSamples, vec2 inCloudMinMax, bool base)
{
    float x = 1.0 / float(samples);
    float accum = 0;
    vec3 p = pos;
    for (int i = 0; i < samples + farSamples; i++) {
        p += L * stepSize;
        if (i >= samples)
            p += L * stepSize;
        accum += max(0, SampleCloudDensity(p, weatherData, mipmap, inCloudMinMax, base));
    }

    return accum;
}

#endif // CLOUDS_CONESAMPLING_H
