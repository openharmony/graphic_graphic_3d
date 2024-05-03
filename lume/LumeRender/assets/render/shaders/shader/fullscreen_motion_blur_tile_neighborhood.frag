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

#define MOTION_BLUR_EPSILON 0.0001

#define CORE_SPREAD_TYPE_NEIGHBORHOOD 0
#define CORE_SPREAD_TYPE_HORIZONTAL 1
#define CORE_SPREAD_TYPE_VERTICAL 2

float SqrMagnitude(const vec2 vec)
{
    return vec.x * vec.x + vec.y * vec.y;
}

void GetMaxAround(const vec2 uv, out vec2 velocity)
{
    const uint sampleCount = 9;
    // 0 1 2
    // 3 4 5
    // 6 7 8
    const ivec2 offsets[] = {
        ivec2(-1, -1),
        ivec2(0, -1),
        ivec2(1, -1),
        ivec2(-1, 0),
        ivec2(0, 0), // center
        ivec2(1, 0),
        ivec2(-1, 1),
        ivec2(0, 1),
        ivec2(1, 1),
    };
    vec2 velocities[sampleCount];
    velocities[0] = textureLodOffset(uVelocity, uv, 0, offsets[0]).xy;
    velocities[1] = textureLodOffset(uVelocity, uv, 0, offsets[1]).xy;
    velocities[2] = textureLodOffset(uVelocity, uv, 0, offsets[2]).xy;
    velocities[3] = textureLodOffset(uVelocity, uv, 0, offsets[3]).xy;
    velocities[4] = textureLodOffset(uVelocity, uv, 0, offsets[4]).xy;
    velocities[5] = textureLodOffset(uVelocity, uv, 0, offsets[5]).xy;
    velocities[6] = textureLodOffset(uVelocity, uv, 0, offsets[6]).xy;
    velocities[7] = textureLodOffset(uVelocity, uv, 0, offsets[7]).xy;
    velocities[8] = textureLodOffset(uVelocity, uv, 0, offsets[8]).xy;

    velocity = vec2(0.0);
    float sqrMagnitude = 0.0;
    for (uint idx = 0; idx < sampleCount; ++idx) {
        const vec2 vel = velocities[idx];
        const float sqrMgn = SqrMagnitude(vel);
        if (sqrMgn > sqrMagnitude) {
            velocity = vel;
            sqrMagnitude = sqrMgn;
        }
    }
}

void GetMaxHorizontal(const vec2 uv, out vec2 velocity)
{
    const uint sampleCount = 5;
    // -2 -1 0 1 2
    const ivec2 offsets[] = {
        ivec2(-2, 0),
        ivec2(-1, 0),
        ivec2(0, 0),
        ivec2(1, 0),
        ivec2(2, 0),
    };
    vec2 velocities[sampleCount];
    velocities[0] = textureLodOffset(uVelocity, uv, 0, offsets[0]).xy;
    velocities[1] = textureLodOffset(uVelocity, uv, 0, offsets[1]).xy;
    velocities[2] = textureLodOffset(uVelocity, uv, 0, offsets[2]).xy;
    velocities[3] = textureLodOffset(uVelocity, uv, 0, offsets[3]).xy;
    velocities[4] = textureLodOffset(uVelocity, uv, 0, offsets[4]).xy;

    velocity = vec2(0.0);
    float sqrMagnitude = 0.0;
    for (uint idx = 0; idx < sampleCount; ++idx) {
        const vec2 vel = velocities[idx];
        const float sqrMgn = SqrMagnitude(vel);
        if (sqrMgn > sqrMagnitude) {
            velocity = vel;
            sqrMagnitude = sqrMgn;
        }
    }
}

void GetMaxVertical(const vec2 uv, out vec2 velocity)
{
    const uint sampleCount = 5;
    // -2 -1 0 1 2
    const ivec2 offsets[] = {
        ivec2(0, -2),
        ivec2(0, -1),
        ivec2(0, 0),
        ivec2(0, 1),
        ivec2(0, 2),
    };
    vec2 velocities[sampleCount];
    velocities[0] = textureLodOffset(uVelocity, uv, 0, offsets[0]).xy;
    velocities[1] = textureLodOffset(uVelocity, uv, 0, offsets[1]).xy;
    velocities[2] = textureLodOffset(uVelocity, uv, 0, offsets[2]).xy;
    velocities[3] = textureLodOffset(uVelocity, uv, 0, offsets[3]).xy;
    velocities[4] = textureLodOffset(uVelocity, uv, 0, offsets[4]).xy;

    velocity = vec2(0.0);
    float sqrMagnitude = 0.0;
    for (uint idx = 0; idx < sampleCount; ++idx) {
        const vec2 vel = velocities[idx];
        const float sqrMgn = SqrMagnitude(vel);
        if (sqrMgn > sqrMagnitude) {
            velocity = vel;
            sqrMagnitude = sqrMgn;
        }
    }
}

/*
 * basic motion blur
 */
void main(void)
{
    const vec2 uv = inUv.xy;

    vec2 velocity = vec2(0.0);
    if (CORE_POST_PROCESS_FLAGS == CORE_SPREAD_TYPE_HORIZONTAL) {
        GetMaxHorizontal(uv, velocity);
    } else if (CORE_POST_PROCESS_FLAGS == CORE_SPREAD_TYPE_VERTICAL) {
        GetMaxVertical(uv, velocity);
    } else {
        // default around the center point
        // CORE_SPREAD_TYPE_NEIGHBORHOOD
        GetMaxAround(uv, velocity);
    }

    outColor = vec4(velocity, 0.0, 0.0);
}
