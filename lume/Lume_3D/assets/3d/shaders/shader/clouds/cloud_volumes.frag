#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_control_flow_attributes : enable

// specialization

// includes
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "common/cloud_constants.h"
#include "common/cloud_math.h"
#include "common/cloud_structs.h"
#include "render/shaders/common/render_compatibility_common.h"

const int WINDFACTOR = 5000;

const int WEATHER_DOMAIN = 1000;

// sets
#include "common/cloud_layout.h"

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

struct PushConstantStruct {
    mat4 view; // 4x4x4 = 64
    vec4 params[4];
};

layout(push_constant, std430) uniform uPushConstant
{
    PushConstantStruct uPc;
};

#include "common/cloud_common.h"
#include "common/cloud_sampling.h"

float lodStep()
{
    return 1.0;
}

#include "common/cloud_cone_sampling.h"
#include "common/cloud_lighting.h"
#include "common/cloud_postprocess.h"
#include "common/cloud_scattering.h"

bool SampleVolume(in Ray ray, in VolumeParameters params, inout VolumeSample data, inout vec4 color)
{
    // float nativeDepth = texture(sampler2D(uDepth, uSampler), inUv.xy).r;
    // float depth = LinearizeDepth(nativeDepth, zmin(), zmax());

    // stop rendering clouds is occlusive geometry
    // if (nativeDepth != 1.0) {
    //    discard;
    //}
    const bool baseOnly = params.baseOnly;
    const int minSamples = params.minSamples;
    const int maxSamples = params.maxSamples;
    vec2 tIntersect;
    vec2 cloudMinMax;

    /*
    [[branch]]
    if( params.volumeType == 2 )
    {

        tIntersect = IntersectLayer(ray, params.center, params.radius, vec2(innerHeight, outerHeight), vec3(0,1,0));
        cloudMinMax = vec2( ray.o.y + ray.dir.y * tIntersect.x, ray.o.y + ray.dir.y * tIntersect.y );
        color = vec4(1, 1, 1, 0.7);
        return false;

    }
    else if( params.volumeType == 3 )
    {
        bool retval = FindAtmosphereIntersections( ray, DEFAULT_PLANET_POS, vec3(DEFAULT_PLANET_RADIUS -
    DEFAULT_PLANET_BIAS, DEFAULT_PLANET_RADIUS - DEFAULT_PLANET_BIAS + innerHeight, DEFAULT_PLANET_RADIUS -
    DEFAULT_PLANET_BIAS + outerHeight), tIntersect); cloudMinMax = vec2( innerHeight, outerHeight ); if( retval ==
    false ) discard;
    }
    else
    {
        tIntersect = vec2(1,2);
        cloudMinMax = vec2(1,2);
    }
    */

    bool retval = FindAtmosphereIntersections(ray, DEFAULT_PLANET_POS,
        vec3(DEFAULT_PLANET_RADIUS - DEFAULT_PLANET_BIAS, DEFAULT_PLANET_RADIUS - DEFAULT_PLANET_BIAS + innerHeight,
            DEFAULT_PLANET_RADIUS - DEFAULT_PLANET_BIAS + outerHeight),
        tIntersect);
    cloudMinMax = vec2(innerHeight, outerHeight);
    if (retval == false) {
        discard;
    }
    //   tIntersect = vec2(innerHeight, outerHeight);

    // when sampling between two hemispheres, the physical distance of the ray march increases when the ray is sloped.
    int sampleCount = int(mix(minSamples, maxSamples, ComputeSamples(ray, vec3(0, 1, 0))));
    float recipSamples = 1.0 / float(sampleCount);

    // setup ray marching variables
    highp float alpha = 0.0;

    int zeroDensitySampleCount = 0;
    float sampledDensityPrevious = -1.0;
    float cloudTest = 0.0;
    highp float ds = 0.0;

    highp float depthMean = 0.0;

    // number of samples of light
    int samples = 0;

    // Trace the ray to inner atmosphere to the outer atmosphere
    float dither = noise(inUv);
    float stepSize = abs(tIntersect.y - tIntersect.x) / float(sampleCount * 2);
    vec3 p = ray.o + ray.dir * min(tIntersect.x, tIntersect.y);
    vec3 step = ray.dir * stepSize;
    p = p + step + step * dither;

    vec3 coneSamples[6];
    coneSamples[0] = normalize(vec3(0.0, 0.0, 0.0));
    coneSamples[1] = normalize(vec3(0.0, 0.0, 0.0));
    coneSamples[2] = normalize(vec3(0.0, 0.0, 0.0));
    coneSamples[3] = normalize(vec3(0.0, 0.0, 0.0));
    coneSamples[4] = normalize(vec3(0.0, 0.0, 0.0));
    coneSamples[5] = normalize(vec3(0.0, 0.0, 0.0));

    float mipLevel = 0.0;

    float totalEnergy = 0.0;

    float scale = 1.4e-2;
    // float sigma_a = 0.00000000002 * scale;
    // float sigma_s = 0.99999999998 * scale;
    // float sigma_t = 1.0 * scale;

    float sigmaa = 0.001 * 1;
    float sigmas = 0.999 * 1;
    float sigmat = 1.0 * 1;

    const int consequetiveSamples = 5;

    [[loop]] for (int i = 0; i < sampleCount; i++) {
        float h = GetHeightFractionForPoint(p, cloudMinMax);

        // skip if there's no weather
        vec2 wuv = WeathermapPosition(p);

        // if there's no weather information, break raymarching.
        if (any(bvec4(lessThan(wuv, vec2(0, 0)), greaterThan(wuv, vec2(1, 1)))))
            break;

        // sample weather data, and use override controls
        vec3 weatherData = SampleCloudMap(p);
        weatherData.r = clamp(weatherData.r + overrideCoverage(), 0, 1);
        weatherData.g = clamp(weatherData.g + overridePercipitation(), 0, 1);
        weatherData.b = clamp(weatherData.b + overrideCloudType(), 0, 1);

        // precipation modifies cloud type and coverage
        weatherData.r = mix(weatherData.r, weatherData.r * 0.7, weatherData.g);
        weatherData.b = mix(weatherData.b, 1.0, weatherData.g);

        [[dont_flatten]]
        // cloud_test starts as zero so we always evaluate the second case from the beginning 
        if (cloudTest != 0.0) {
            // sample density the expensive way by setting the last parameter, to false, indicating a full sample.
            float sampledDensity = SampleCloudDensity(p, weatherData, mipLevel, cloudMinMax, baseOnly);
            // If we just sampled a zero and the previous sample was zero, increment the counter
            zeroDensitySampleCount += all(lessThan(vec2(sampledDensity, sampledDensityPrevious), vec2(0, 0))) ? 1 : 0;

            // if we are doing an expensive sample that is still potentially in the cloud...
            if (zeroDensitySampleCount < consequetiveSamples && sampledDensity > 0) {
                // compute transmission based on beers law and the extinction coefficient (asorption + scattering)
                float sampleAtt = exp(-sampledDensity * sigmat);
                data.transmission *= sampleAtt;

                [[branch]] if (data.transmission <= 1e-3) {
                    data.transmission = 0.0;
                    break;
                }

                ds += sampledDensity;
                samples += 1;

                // L is the vector of the particle towards the sun
                vec3 L = normalize(uSunPos() - p);

                // V is the vector of the particle towards the view
                vec3 V = normalize(p - ray.o);

                // Cos angle therefore is basically N dot L operation, in reverse order.
                float cosAngle = dot(L, V);

                float phase =
                    max(0, HenyeyGreensteinPhase(cosAngle, eccintricity(), silverIntensity(), silverSpread()));
                float lightStep = stepSize * 0.1 * lightStep();
                float dl = 0.0f;

                [[branch]] if (params.conesampler == true) {
                    dl = SampleConeDensity(
                        p, L, lightStep, weatherData, mipLevel, coneSamples, cloudMinMax, data.transmission > 0.3);
                    dl = clamp(dl, 0, 16);
                    dl = dl * remap(weatherData.g, 0, 1, 1, 9);
                    dl = max(0, dl);
                } else {
                    dl = DensityToLight(
                        p, weatherData, mipLevel, L, lightStep, 6, 1, cloudMinMax, data.transmission > 0.3);
                    dl = clamp(dl, 0, 16);
                    dl = dl * remap(weatherData.g, 0, 1, 1, 9);
                    dl = max(0, dl);
                }

                float dsLoded =
                    max(0, GetLodCloudDepth(p, weatherData, mipLevel, stepSize, cloudMinMax, data.transmission > 0.3));
                dsLoded = sampledDensity * lodStep();

                float scattered = (brightness() - brightness() * data.transmission) / sigmaa;
                float energy = max(0, GetLightEnergyA(p, h, dl, dsLoded, phase, cosAngle, lightStep, scattered));
                data.intensity += energy * 0.8 * data.transmission * sigmas * sampledDensity;
                data.ambient += scattered * exp(-dl) * data.transmission * sigmas * sampledDensity;
                depthMean += dot(p - ray.o, p - ray.o);
                zeroDensitySampleCount = 0;
            } else if (zeroDensitySampleCount < consequetiveSamples && sampledDensity <= 0) {
                zeroDensitySampleCount = 0;
            }
            // if not, then set cloudTest to zero so that we go back to the cheap sample case
            else {
                zeroDensitySampleCount = 0;
                cloudTest = 0.0;
                p += step;
            }

            p += step;
            sampledDensityPrevious = sampledDensity;
        } else {
            // binary search, sample the clouds in a cheap way until we find an altidude/density
            cloudTest = SampleCloudDensity(p, weatherData, mipLevel, cloudMinMax, true);
            p += cloudTest > 0.0 ? -step : step * 2;
        }
    }

    // energery conservation, during ray marching samples can be skipped
    data.ambient /= max(1, float(samples));
    data.intensity /= max(1, float(samples));
    data.depth = sqrt(depthMean / max(1, float(samples)));

    vec3 atmosphere2 = RaySphereIntersect(
        ray.o - DEFAULT_PLANET_POS, ray.dir, DEFAULT_PLANET_RADIUS - DEFAULT_PLANET_BIAS + outerHeight * 2);
    vec3 planet2 = RaySphereIntersect(ray.o - DEFAULT_PLANET_POS, ray.dir, DEFAULT_PLANET_RADIUS - DEFAULT_PLANET_BIAS);

    float maxDistances = 1e12;
    maxDistances = mix(maxDistances, atmosphere2.y, atmosphere2.y > 0);
    maxDistances = mix(maxDistances, atmosphere2.x, atmosphere2.x > 0);

    float weight = 1;
    if (planet2.y > 0 && planet2.y < maxDistances)
        weight = 0;
    if (planet2.x > 0 && planet2.x < maxDistances)
        weight = 0;
    if (maxDistances == 1e12)
        weight = 0;

    vec2 wuv = WeathermapPosition(ray.o + ray.dir * maxDistances);
    float cirrus = textureLod(sampler2D(uCirrusTex, uSamplerCube), wuv.xy * 4 + time() / 80.0f, 0).r;

    data.transmission *= mix(1, (0.25 + (1 - cirrus) * 0.75), weight);
    data.ambient += mix(0, data.transmission * cirrus * 5.85, weight);
    data.intensity += mix(0, data.transmission * cirrus * 5.85, weight);
    data.depth = mix(data.depth, (data.depth * samples * 3 + maxDistances) / (samples * 3 + 1), weight);

    color = vec4(cirrus, 0, 0, 1);
    return true;
}

void main(void)
{
    const uint cameraIdx = 0;

    const int pxd = 800;
    const int pyd = 480;
    const bool debug = int(gl_FragCoord.x) > (pxd - 3) && int(gl_FragCoord.x) < (pxd + 3) &&
                       int(gl_FragCoord.y) > (pyd - 3) && int(gl_FragCoord.y) < (pyd + 3);

    Ray ray = CreateRay(cameraIdx);
    outColor = vec4(vec3(ray.dir) * 0.5 + 0.5, 1.0);

    VolumeParameters params;
    params.center = vec3(0, -DEFAULT_PLANET_RADIUS, 0);
    params.radius = DEFAULT_PLANET_RADIUS;
    params.baseOnly = false;
    params.minSamples = 24;
    params.maxSamples = 48;
    params.conesampler = false;
    params.nativeDepth = 1.0;
    params.volumeType = 3;

    VolumeSample o;
    o.depth = 0;
    o.intensity = 0;
    o.ambient = 0;
    o.transmission = 1;

    vec4 color = vec4(0, 0, 0, 0);

    if (!SampleVolume(ray, params, o, color)) {
        outColor = mix(color, vec4(1.0, 0.0, 0.0, 1.0), debug ? 1.0 : 0.0);
        return;
    }

    outColor = vec4(o.intensity, o.ambient, o.depth, 1 - o.transmission);

    // color = Postprocess(ray, o, 0);
    // outColor = mix(color, vec4(1.0, 0.0, 0.0, 1.0), debug ? 1.0 : 0.0);
}
