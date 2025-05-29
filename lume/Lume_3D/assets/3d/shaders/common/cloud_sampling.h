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
#ifndef CLOUDS_CLOUDSAMPLING_H
#define CLOUDS_CLOUDSAMPLING_H

float GetHeightFractionForPoint(vec3 inPosition, vec2 inCloudMinMax)
{
    vec3 d = inPosition - vec3(0, -DEFAULT_PLANET_RADIUS, 0);
    float l = length(d) - DEFAULT_PLANET_RADIUS - inCloudMinMax.x;
    return clamp(l / (inCloudMinMax.y - inCloudMinMax.x), 0, 1);
}

//   color(0.0, vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0)); = red
//   color(0.5, vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0)); = green
//   color(1.0, vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0)); = blue
//   color(0.25, vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0)); = red + geen / 2.0
//   color(0.75, vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0)); = green + blue / 2.0
vec3 color(float cloudType, vec3 stratus, vec3 stratocumulus, vec3 cumulus)
{
    float a = clamp(cloudType, 0.0, 0.5) * 2.0;
    float b = clamp(cloudType, 0.5, 1.0) * 2.0 - 1.0;
    return mix(mix(stratus, stratocumulus, a), mix(stratocumulus, cumulus, b), step(0.5, cloudType));
}

float GetDensityHeightGradientForPoint(in float h, in vec3 weatherData)
{
    float cloudType = weatherData.b;
    const vec4 cumulus = vec4(0.01, 0.3, 0.6, 0.95);
    const vec4 stratocumulus = vec4(0.0, 0.25, 0.3, 0.65);
    const vec4 stratus = vec4(0, 0.1, 0.2, 0.3);

    float a = clamp(cloudType, 0.0, 0.5) * 2.0;
    float b = clamp(cloudType, 0.5, 1.0) * 2.0 - 1.0;
    vec4 grad = mix(mix(stratus, stratocumulus, a), mix(stratocumulus, cumulus, b), step(0.5, cloudType));

    float grad_density = clamp(remap(h, grad.x, grad.y, 0.0, 1.0) * remap(h, grad.z, grad.w, 1.0, 0.0), 0, 1);
    return grad_density * exp(-h * 0.00008) * 1.25;
}

vec3 SampleCloudMap(in vec3 pos)
{
    return textureLod(sampler2D(uCloudMapTex, uSamplerWeather), WeathermapPosition(pos), 0).rgb;
}

float check(float val, float min_val, float max_val)
{
    return val < min_val || val > max_val ? -1 : val;
}

float SampleCloudDensity(in vec3 p, in vec3 weatherData, in float mipLevel, in vec2 inCloudMinMax, in bool doCheaply)
{
    float cloud_top_offset = 500.0;

    float heightFraction = GetHeightFractionForPoint(p, inCloudMinMax);
    p += heightFraction * windDir() * cloud_top_offset;
    p += (windDir() + vec3(0.0, 0.1, 0.0)) * time() * windSpeed();
    if (heightFraction == 0 || heightFraction == 1.0)
        return 0;

    // range [0...1] fractal brownian motion for low frequency clouds
    vec4 lowFrequencyNoises =
        textureLod(sampler3D(uCloudBaseTex, uSamplerCube), p / vec3(CLOUD_DOMAIN_LOW_RES()), mipLevel);
    float lowFreqfBm = dot(lowFrequencyNoises.gba, vec3(0.625, 0.25, 0.125));

    // define the base cloud shape by dilating it with the low frequency fBm made of Worley noise.
    float baseCloud = remap(lowFrequencyNoises.r, -(1.0 - lowFreqfBm), 1.0, 0.0, 1.0);

    // Get the density-height gradient using the density-height function (not included)
    float densityHeightGradient = GetDensityHeightGradientForPoint(heightFraction, weatherData);

    // apply the height function to the base cloud shape
    baseCloud *= densityHeightGradient;

    // cloud coverage is stored in the weather_data�s red channel.
    float cloudCoverage = weatherData.r;

    // apply anvil deformations
    cloudCoverage = pow(cloudCoverage, remap(heightFraction, 0.7, 0.8, 1.0, mix(1.0, 0.5, AnvilBias())));

    // reduce the cloud coverage by an episilon because otherwise the remap function, has origional min/max 1 causing a
    // division by zero
    cloudCoverage = mix(0, 0.99999, cloudCoverage);

    // Use remapper to apply cloud coverage attribute
    float baseCloudWithCoverage = remap(baseCloud, cloudCoverage, 1.0, 0.0, 1.0);

    // Multiply result by cloud coverage so that smaller clouds are lighter and more aesthetically pleasing.
    baseCloudWithCoverage *= cloudCoverage;

    // define final cloud value
    float finalCloud = baseCloudWithCoverage;

    // only do detail work if we are taking expensive samples!
    [[dont_flatten]] if (!doCheaply) {
        // add some turbulence to bottoms of clouds using curl noise.  Ramp the effect down over height and scale it by
        // some value (200 in this example)
        vec2 curlNoise =
            textureLod(sampler2D(uTurbulenceTex, uSamplerCube), vec2(p.x, p.z) / CLOUD_DOMAIN_CURL_RES(), 0.0).rg;
        p.xz += curlNoise.rg * (1.0 - heightFraction) * CLOUD_DOMAIN_CURL_AMPLITUDE();

        // sample high-frequency noises
        vec3 highFrequencyNoises =
            textureLod(sampler3D(uCloudDetailTex, uSamplerCube), p / CLOUD_DOMAIN_HIGH_RES(), mipLevel).rgb;

        // build High frequency Worley noise fBm
        float highFreqfBm =
            (highFrequencyNoises.r * 0.625) + (highFrequencyNoises.g * 0.25) + (highFrequencyNoises.b * 0.125);

        // get the height_fraction for use with blending noise types over height
        float heightFraction = GetHeightFractionForPoint(p, inCloudMinMax);

        // transition from wispy shapes to billowy shapes over height
        float highFreqNoiseModifier = mix(highFreqfBm, 1.0 - highFreqfBm, clamp(heightFraction * 10.0, 0.0, 1.0));

        // erode the base cloud shape with the distorted high frequency Worley noises.
        finalCloud = remap(baseCloudWithCoverage, highFreqNoiseModifier * 0.2, 1.0, 0.0, 1.0);
    }

#if DEBUG_
    if (isinf(finalCloud) || isnan(finalCloud)) {
        debugPrintfEXT("sample cloud: [%f %f %f %f %f %f]", finalCloud, baseCloud, baseCloudWithCoverage,
            cloudCoverage, heightFraction, densityHeightGradient);
        return 0;
    }
#endif

    return clamp(finalCloud, 0, 1);
}

// returns the cloud depth around a position
float GetLodCloudDepth(in vec3 pos, in vec3 weatherData, float mipmap, float ds, vec2 inCloudMinMax, bool base)
{
    // get sample positions around the position
    vec3 p1 = pos + vec3(ds, 0, 0);
    vec3 p2 = pos - vec3(ds, 0, 0);
    vec3 p3 = pos + vec3(0, ds, 0);
    vec3 p4 = pos - vec3(0, ds, 0);
    vec3 p5 = pos + vec3(0, 0, ds);
    vec3 p6 = pos - vec3(0, 0, ds);

    // sum depth for all sampled positions
    float sum = 0;
    sum += SampleCloudDensity(p1, weatherData, mipmap, inCloudMinMax, base);
    sum += SampleCloudDensity(p2, weatherData, mipmap, inCloudMinMax, base);
    sum += SampleCloudDensity(p3, weatherData, mipmap, inCloudMinMax, base);
    sum += SampleCloudDensity(p4, weatherData, mipmap, inCloudMinMax, base);
    sum += SampleCloudDensity(p5, weatherData, mipmap, inCloudMinMax, base);
    sum += SampleCloudDensity(p6, weatherData, mipmap, inCloudMinMax, base);

#if DEBUG_
    if (isinf(sum) || isnan(sum)) {
        debugPrintfEXT("getLodCloudDepth: [%f]", sum);
        return 0;
    }
#endif

    sum *= 1.0 / 6.0;
    return sum;
}
#endif // CLOUDS_CLOUDSAMPLING_H
