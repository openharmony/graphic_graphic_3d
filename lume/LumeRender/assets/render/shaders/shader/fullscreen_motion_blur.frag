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

float CompareCone(const float dist, const float velocity)
{
    return clamp(1.0 - dist / max(velocity, MOTION_BLUR_EPSILON), 0.0, 1.0);
}

float CompareCylinder(const float dist, const float velocity)
{
    return 1.0 - smoothstep(0.95 * velocity, 1.05 * velocity, dist);
}

float SoftDepthCompare(const float za, const float zb)
{
    return clamp(1.0 - (za - zb) / max(min(za, zb), MOTION_BLUR_EPSILON), 0.0, 1.0);
}

float Rand(const vec2 co)
{
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233)) * 43758.5453));
}

/*
 * basic motion blur
 */
void main(void)
{
    const vec4 factor = GetFactor();
    const uint sharpness = uint(factor.x + 0.5);

    // NOTE: the sample count doesn't seem to have much of any impact
    //       on the performance, even on mobile.
    const uint quality = uint(factor.y + 0.5);
    float fSampleCount = SAMPLE_COUNT_MEDIUM;
    if (quality == QUALITY_HIGH) {
        fSampleCount = SAMPLE_COUNT_HIGH;
    }

    const vec2 X = inUv.xy;

    // blur scale defined in .shader new struct not defined here
    const float velocityCoefficient = factor.w;
    const vec2 maxNeighborVelocity = GetUnpackTileVelocity(X, uPc.viewportSizeInvSize.zw) * velocityCoefficient;

    const vec3 baseColor = texture(uInput, X).xyz;
    if (!IsVelocityZero(maxNeighborVelocity))
    {
        if (quality == QUALITY_LOW) {
            vec3 color = vec3(0.0);
            const vec2 baseVelocity = GetUnpackVelocity(X, uPc.viewportSizeInvSize.zw) * velocityCoefficient;

            // only color sampling
            // weight of the sample is based on the offset
            for (uint ii = 0; ii < uint(fSampleCount); ii++) {
                const vec2 offset = baseVelocity * (float(ii) / fSampleCount - 0.5);
                color += textureLod(uInput, X + offset, 0).xyz;
            }

            outColor = vec4(color / fSampleCount, 1.0);
        } else {
            // jittering improves ghosting but too much may add artifacts.
            const float jitterMagnitude = 0.5;
            const float jitter = (Rand(X) * 2.0 - 1.0) * jitterMagnitude;
            
            const vec2 xVelocity = GetUnpackVelocity(X, uPc.viewportSizeInvSize.zw) * velocityCoefficient;
            const float xVelocityLen = length(xVelocity);
            const float xDepth = textureLod(uDepth, X, 0).x;

            float weight = 1.0 / max(xVelocityLen, 0.5);
            vec3 color = baseColor.rgb * weight;
            
            for (int ii = 0; ii < int(fSampleCount); ii++)
            {
                const float tt = mix(-1.0, 1.0, (float(ii) + jitter + 1.0) / (fSampleCount + 1.0));
                const vec2 offset = maxNeighborVelocity * tt;
                const vec2 Y = X + offset;
                const vec2 yVelocity = GetUnpackVelocity(Y, uPc.viewportSizeInvSize.zw) * velocityCoefficient;
                const float yVelocityLen = length(yVelocity);
                const float offsetLen = length(offset);
                const float yDepth = textureLod(uDepth, Y, 0).x;

                const float ff = SoftDepthCompare(xDepth, yDepth);
                const float bb = SoftDepthCompare(yDepth, xDepth);

                // hard cutoff for preventing background blur
                // from bleeding into the foreground.
                const float yInFrontOfX = yDepth > xDepth ? 0.0 : 1.0;

                const float yWeight = 
                    yInFrontOfX * ff * CompareCone(offsetLen, yVelocityLen) +
                    bb * CompareCone(offsetLen, xVelocityLen) +
                    (CompareCylinder(offsetLen, yVelocityLen) * CompareCylinder(offsetLen, xVelocityLen) * 2.0);
                
                weight += yWeight;
                color += yWeight * textureLod(uInput, Y, 0).rgb;
            }

            outColor = vec4(color / weight, 1.0);
        }
    }
    else
    {
        outColor = vec4(baseColor, 1.0);
    }
}
