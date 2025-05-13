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
#include "common/cloud_scattering.h"
#include "common/cloud_structs.h"
#include "render/shaders/common/render_compatibility_common.h"

// sets

layout(set = 0, binding = 0) uniform sampler uSampler;
layout(set = 0, binding = 1, std140) uniform u
{
    UniformStruct uParams;
};

layout(set = 1, binding = 0) uniform texture2D uTex;
layout(set = 1, binding = 1) uniform texture2D uTexPrev;
layout(set = 1, binding = 2, std140) uniform uCameraMatrices
{
    DefaultCameraMatrixStruct uCameras[CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT];
};

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

float frameIndex()
{
    return uPc.params[0].y;
}

float lodStep()
{
    return 1.0;
}

#include "common/cloud_lighting.h"
#include "common/cloud_postprocess.h"

void main(void)
{
    int downsampledPixels = 4;

    const uint cameraIdx = 0;
    ivec2 fragment = ivec2(gl_FragCoord.xy);
    int frameIndex = int(frameIndex()) % (downsampledPixels * downsampledPixels);
    int sampleIndex = ((fragment.y % downsampledPixels) * downsampledPixels) + (fragment.x % downsampledPixels);

    // get the pixel location, assuming the texture is downscaled by n pixel
    ivec2 offset = (fragment & (downsampledPixels - 1));
    ivec2 block = fragment - offset;
    ivec2 size = textureSize(sampler2D(uTex, uSampler), 0) * downsampledPixels;
    vec2 texelLocation = (block + vec2(downsampledPixels * 0.5)) / vec2(size);

    // get pixel's information
    vec4 next = textureLod(sampler2D(uTex, uSampler), texelLocation, 0);

    Ray ray = CreateRay(0);

    // reconstruct the world position based on the depth of current rendered pixel
    vec4 worldspace = vec4(ray.o + ray.dir + next.b, 1.0);
    mat4x4 b = mat4x4(uCameras[cameraIdx].viewProjPrevFrame);

    // convert world space to reprojected clip space
    vec4 repojectClipspace = b * worldspace;
    repojectClipspace.xyz /= repojectClipspace.w;
    repojectClipspace.w = 1.0;

    // get reprojected uv coordinates
    vec2 reprojectUv = vec2(repojectClipspace.xy * 0.5 + 0.5);

    vec4 previous = textureLod(sampler2D(uTexPrev, uSampler), reprojectUv, 0);
    bool reusable =
        all(lessThan(repojectClipspace.xyz, vec3(1))) && abs(previous.b - next.b) < 100.0f && previous.b != 0;
    outColor = mix(previous, next, frameIndex == sampleIndex || reusable == false ? 1.0 : (1.0 / 16.0));
}
