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

#include "3d/shaders/common/3d_dm_frag_layout_common.h"

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

float GetFrameIndex()
{
    return float(floatBitsToUint(uGeneralData.sceneTimingData.w) + 0.5);
}

ivec2 GetTextureSize()
{
    return textureSize(uSampTextureBase, 0);
}

vec4 GetTextureSample(vec2 uv)
{
    return textureLod(uSampTextureBase, uv, 0);
}

vec4 GetPrevTextureSample(vec2 uv)
{
    return textureLod(uSampTextures[CORE_MATERIAL_TEX_NORMAL_IDX], uv, 0);
}

Ray CreateRay(uint cameraIdx)
{
    // avoid precision loss, by decomposing the matrix
    vec3 position = vec3(uCameras[cameraIdx].view[3].xzy);

    mat4x4 a = mat4(vec4(uCameras[cameraIdx].view[0].xyzw), vec4(uCameras[cameraIdx].view[1].xyzw),
        vec4(uCameras[cameraIdx].view[2].xyzw), vec4(0.0, 0.0, 0.0, 1.0));

    // assume affine matrix, so just take the transpose
    mat4x4 v = transpose(a) * uCameras[cameraIdx].projInv;

    Ray ray;
    vec2 coord = vec2(inUv.x, inUv.y);
    vec2 clip = vec2(coord - 0.5) * vec2(2.0, 2.0);
    vec4 far = v * vec4(clip, 1.0, 1.0);
    vec4 near = v[2];
    near.xyz /= near.w;
    far.xyz /= far.w;
    ray.o = near.xyz - position;
    ray.dir = normalize(far.xyz - near.xyz);

    return ray;
}

void main(void)
{
    int downsampledPixels = 4;

    // TODO: get camera index
    const uint cameraIdx = 0;
    ivec2 fragment = ivec2(gl_FragCoord.xy);
    int frameIndex = int(GetFrameIndex()) % (downsampledPixels * downsampledPixels);
    int sampleIndex = ((fragment.y % downsampledPixels) * downsampledPixels) + (fragment.x % downsampledPixels);

    // get the pixel location, assuming the texture is downscaled by n pixel
    ivec2 offset = (fragment & (downsampledPixels - 1));
    ivec2 block = fragment - offset;
    ivec2 size = GetTextureSize() * downsampledPixels;
    vec2 texelLocation = (block + vec2(downsampledPixels * 0.5)) / vec2(size);

    // get pixel's information
    vec4 next = GetTextureSample(texelLocation);

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

    vec4 previous = GetPrevTextureSample(reprojectUv);
    bool reusable =
        all(lessThan(repojectClipspace.xyz, vec3(1))) && abs(previous.b - next.b) < 100.0f && previous.b != 0;
    outColor = mix(previous, next, frameIndex == sampleIndex || reusable == false ? 1.0 : (1.0 / 16.0));
}
