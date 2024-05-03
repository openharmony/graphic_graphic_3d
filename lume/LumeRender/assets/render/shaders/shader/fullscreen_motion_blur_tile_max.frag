#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_post_process_common.h"
#include "render/shaders/common/render_tonemap_common.h"

// sets

#include "render/shaders/common/render_post_process_layout_common.h"

layout(set = 1, binding = 0) uniform sampler2D uVelocity;

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

#define QUALITY_LOW 0
#define QUALITY_MED 1
#define QUALITY_HIGH 2

#define MOTION_BLUR_EPSILON 0.0001

vec4 GetFactor()
{
    return uLocalData.factors[0U];
}

bool IsVelocityZero(const vec2 velocity)
{
    return ((abs(velocity.x) < MOTION_BLUR_EPSILON) || (abs(velocity.y) < MOTION_BLUR_EPSILON));
}

float SqrMagnitude(const vec2 vec)
{
    return vec.x * vec.x + vec.y * vec.y;
}

/*
 * basic motion blur
 */
void main(void)
{
    const vec4 factor = GetFactor();
    const uint sampleCount = 16;

    const vec2 uv = inUv.xy;
    // bilinear sampling 4x4
    const vec2 stepSize = uPc.viewportSizeInvSize.zw / 4.0;
    vec2 velocities[sampleCount];
    uint sampleIdx = 0;
    {
        const vec2 baseUv = uv;
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv, 0).rg;
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv + stepSize.xy * vec2(1.0, 0.0), 0).rg;
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv + stepSize.xy * vec2(2.0, 0.0), 0).rg;
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv + stepSize.xy * vec2(3.0, 0.0), 0).rg;
    }
    {
        const vec2 baseUv = uv + stepSize * vec2(0.0, 1.0);
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv, 0).rg;
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv + stepSize.xy * vec2(1.0, 0.0), 0).rg;
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv + stepSize.xy * vec2(2.0, 0.0), 0).rg;
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv + stepSize.xy * vec2(3.0, 0.0), 0).rg;
    }
    {
        const vec2 baseUv = uv + stepSize * vec2(0.0, 2.0);
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv, 0).rg;
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv + stepSize.xy * vec2(1.0, 0.0), 0).rg;
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv + stepSize.xy * vec2(2.0, 0.0), 0).rg;
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv + stepSize.xy * vec2(3.0, 0.0), 0).rg;
    }
    {
        const vec2 baseUv = uv + stepSize * vec2(0.0, 3.0);
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv, 0).rg;
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv + stepSize.xy * vec2(1.0, 0.0), 0).rg;
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv + stepSize.xy * vec2(2.0, 0.0), 0).rg;
        velocities[sampleIdx++] = textureLod(uVelocity, baseUv + stepSize.xy * vec2(3.0, 0.0), 0).rg;
    }

    vec2 velocity = vec2(0.0);
    float sqrMagnitude = 0.0;
    for (uint idx = 0; idx < sampleCount; ++idx) {
        const vec2 vel = velocities[idx];
        const float sqrMgn = SqrMagnitude(vel);
        if (sqrMgn > sqrMagnitude) {
            velocity = vel;
            sqrMagnitude = sqrMgn;
        }
    }
    outColor = vec4(velocity, 0.0, 0.0);
}
