#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_post_process_common.h"
#include "render/shaders/common/render_tonemap_common.h"

// sets

#include "render/shaders/common/render_post_process_layout_common.h"

layout(set = 1, binding = 0) uniform sampler2D uInput;
layout(set = 1, binding = 1) uniform sampler2D uDepth;
layout(set = 1, binding = 2) uniform sampler2D uVelocity;
layout(set = 1, binding = 3) uniform sampler2D uTileVelocity;

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

// NOTE: cannot be used (remove if not used for any input)
#define ENABLE_INPUT_ATTACHMENTS 0

#define QUALITY_LOW 0
#define QUALITY_MED 1
#define QUALITY_HIGH 2

#define SHARPNESS_LOW 0
#define SHARPNESS_MED 1
#define SHARPNESS_HIGH 2

#define SAMPLE_COUNT_LOW 7.0
#define SAMPLE_COUNT_MEDIUM 11.0
#define SAMPLE_COUNT_HIGH 15.0

#define MOTION_BLUR_EPSILON 0.0001

#define MOTION_BLUR_TILE_SIZE 8
#define MOTION_BLUR_FLOAT_TILE_SIZE 8.0

vec2 GetUnpackVelocity(const vec2 uv, const vec2 invSize)
{
    return textureLod(uVelocity, uv, 0).xy * invSize;
}

vec2 GetUnpackTileVelocity(const vec2 uv, const vec2 invSize)
{
    return textureLod(uTileVelocity, uv, 0).xy * invSize;
}

vec4 GetFactor()
{
    return uLocalData.factors[0U];
}

bool IsVelocityZero(const vec2 velocity)
{
    return ((abs(velocity.x) < MOTION_BLUR_EPSILON) || (abs(velocity.y) < MOTION_BLUR_EPSILON));
}

float CompareCone(const float localVelLength, const float velLength)
{
    return clamp(1.0 - localVelLength / velLength, 0.0, 1.0);
}

float CompareCylinder(const float localVelLength, const float velLength)
{
    return 1.0 - smoothstep(0.95 * velLength, 1.05 * velLength, localVelLength);
}

float SoftDepthCompare(const float za, const float zb)
{
    const float softExtent = 0.01;
    return clamp(1.0 - (za - zb) / softExtent, 0.0, 1.0);
}

// mostly based on "A Reconstruction Filter for Plausible Motion Blur"
float SampleWeight(const float baseDepth, const float sampleDepth, const float baseTileVelLength, const vec2 baseVel,
    const vec2 sampleVel)
{
    const float f = SoftDepthCompare(baseDepth, sampleDepth);
    const float b = SoftDepthCompare(sampleDepth, baseDepth);

    const float xVelLength = length(baseVel);
    const float yVelLength = length(sampleVel);
    const float xyVelLength = length(baseVel - sampleVel);
    const float yxVelLength = length(sampleVel - baseVel);

    return (f * CompareCone(yxVelLength, baseTileVelLength)) + (b * CompareCone(xyVelLength, baseTileVelLength)) +
           (CompareCylinder(yxVelLength, baseTileVelLength) * CompareCylinder(yxVelLength, baseTileVelLength) * 2.0);
    //return (f * CompareCone(yxVelLength, yVelLength)) + (b * CompareCone(xyVelLength, xVelLength)) +
    //       (CompareCylinder(yxVelLength, yVelLength) * CompareCylinder(yxVelLength, xVelLength) * 2.0);
}

/*
 * basic motion blur
 */
void main(void)
{
    const vec4 factor = GetFactor();
    const uint sharpness = uint(factor.x + 0.5);
    float fSampleCount = SAMPLE_COUNT_MEDIUM;
    if (sharpness == SHARPNESS_LOW) {
        fSampleCount = SAMPLE_COUNT_LOW;
    } else if (sharpness == SHARPNESS_MED) {
        fSampleCount = SAMPLE_COUNT_MEDIUM;
    } else if (sharpness == SHARPNESS_HIGH) {
        fSampleCount = SAMPLE_COUNT_HIGH;
    }
    const uint quality = uint(factor.y + 0.5);

    const vec2 baseUv = inUv.xy;
    vec2 velUv = baseUv;
    const vec2 texIdx = baseUv * uPc.viewportSizeInvSize.xy;
    const vec2 roundVal = round(texIdx);
    const vec2 fracVal = fract(texIdx / MOTION_BLUR_FLOAT_TILE_SIZE);
    const float dThreshold = 1.0 / MOTION_BLUR_FLOAT_TILE_SIZE;
    const float uThreshold = 1.0 - dThreshold;

    // blur scale defined in .shader new struct not defined here
    const float velocityCoefficient = factor.w;
    vec2 baseTileVelocity = GetUnpackTileVelocity(velUv.xy, uPc.viewportSizeInvSize.zw) * velocityCoefficient;
    // scale for the largest velocity
    // const float initialBaseTileVelLength = length(baseTileVelocity);
    // if (initialBaseTileVelLength > 0.0) {
    //    const float minScale = min(8.0, initialBaseTileVelLength);
    //    baseTileVelocity = baseTileVelocity * (minScale / initialBaseTileVelLength);
    //}
    const vec2 baseVelocity = GetUnpackVelocity(velUv.xy, uPc.viewportSizeInvSize.zw) * velocityCoefficient;

    const vec3 baseColor = texture(uInput, baseUv).xyz;
    if (!IsVelocityZero(baseTileVelocity)) {
        vec3 color = vec3(0.0);
        float weightSum = fSampleCount;
        if (quality == QUALITY_LOW) {
            // only color sampling
            // weight of the sample is based on the offset
            for (uint idx = 0; idx < uint(fSampleCount); ++idx) {
                const vec2 offset = baseVelocity * (float(idx) / fSampleCount - 0.5);
                color += textureLod(uInput, baseUv + offset, 0).xyz;
            }
        } else if (quality == QUALITY_MED) {
            const float baseTileVelLength = length(baseTileVelocity);
            weightSum = 0.0;
            // included depth sampling
            // weight of the sample is based on if it's behind or in-front
            const float baseDepth = textureLod(uDepth, baseUv, 0).x;
            for (uint idx = 0; idx < uint(fSampleCount); ++idx) {
                const vec2 offset = baseTileVelocity * (float(idx) / fSampleCount - 0.5);
                const vec2 uv = baseUv + offset;
                const float depth = textureLod(uDepth, uv, 0).x;

                // const float weight = SampleWeight(baseDepth, depth, baseVelLength, length(offset));
                const float weight = 1.0;

                color += textureLod(uInput, uv, 0).xyz * weight;
                weightSum += weight;
            }
        } else if (quality == QUALITY_HIGH) {
            const float baseTileVelLength = length(baseTileVelocity);
            weightSum = 0.0;
            // included depth sampling
            // weight of the sample is based on if it's behind or infront
            const float baseDepth = textureLod(uDepth, baseUv, 0).x;
            for (uint idx = 0; idx < uint(fSampleCount); ++idx) {
                const vec2 offset = baseTileVelocity * (float(idx) / fSampleCount - 0.5);
                const vec2 uv = baseUv + offset;
                const float sampleDepth = textureLod(uDepth, uv, 0).x;
                const vec2 sampleVel = GetUnpackVelocity(uv, uPc.viewportSizeInvSize.zw) * velocityCoefficient;

                const float weight = SampleWeight(baseDepth, sampleDepth, baseTileVelLength, baseVelocity, sampleVel);

                color += textureLod(uInput, uv, 0).xyz * weight;
                weightSum += weight;
            }
        }
        weightSum = max(0.001, weightSum);
        color = color / weightSum;
        outColor = vec4(mix(baseColor, color, factor.z), 1.0);
    } else {
        outColor = vec4(baseColor, 1.0);
    }
}
