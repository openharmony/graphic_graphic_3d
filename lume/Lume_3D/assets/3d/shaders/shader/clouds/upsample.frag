#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_control_flow_attributes : enable

// specialization

// includes
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "common/cloud_structs.h"
#include "render/shaders/common/render_compatibility_common.h"

// sets

layout(set = 0, binding = 0) uniform sampler uSampler;
layout(set = 0, binding = 1, std140) uniform u
{
    UniformStruct uParams;
};

layout(set = 1, binding = 0) uniform texture2D uTex;
layout(set = 1, binding = 1, std140) uniform uCameraMatrices
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
#include "common/cloud_constants.h"
#include "common/cloud_lighting.h"
#include "common/cloud_math.h"
#include "common/cloud_postprocess.h"
#include "common/cloud_scattering.h"

float lodStep()
{
    return 1.0;
}

void main(void)
{
    const int pxd = 800;
    const int pyd = 480;
    const bool debug = int(gl_FragCoord.x) > (pxd - 3) && int(gl_FragCoord.x) < (pxd + 3) &&
                       int(gl_FragCoord.y) > (pyd - 3) && int(gl_FragCoord.y) < (pyd + 3);
    const bool trace = int(gl_FragCoord.x) == pxd && int(gl_FragCoord.y) == pyd;

    // float nativeDepth = texture(sampler2D(uDepth, uSampler), inUv.xy).r;
    //  if( nativeDepth != 1.0 ) discard;

    Ray ray = CreateRay(0);
    vec2 size = 1.0 / textureSize(sampler2D(uTex, uSampler), 0);
    vec2 ts = size * 2.0;
    const vec2 uv = inUv;

    vec4 color;

    vec4 results;

    /*
    // center
    results = textureLod(sampler2D(uTex, uSampler), uv, 0) * 0.5;
    // corners
    // 1.0 / 8.0 = 0.125
    results += textureLod(sampler2D(uTex, uSampler), uv - ts, 0) * 0.125;
    results += textureLod(sampler2D(uTex, uSampler), uv + vec2(ts.x, -ts.y), 0) * 0.125;
    results += textureLod(sampler2D(uTex, uSampler), uv + vec2(-ts.x, ts.y), 0) * 0.125;
    results += textureLod(sampler2D(uTex, uSampler), uv + ts, 0) * 0.125;
    */

    results = textureLod(sampler2D(uTex, uSampler), uv, 0);

    VolumeSample o;
    o.intensity = results.r;
    o.ambient = results.g;
    o.depth = results.b;
    o.transmission = 1 - results.a;

    color = Postprocess(ray, o, 0);
    // outColor = mix(color, vec4(1.0, 0.0, 0.0, 1.0), debug ? 1.0 : 0.0);

    outColor = color;
}
