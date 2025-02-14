#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets
#include "3d/shaders/common/3d_dm_env_frag_layout_common.h"

struct CustomEnvProperties {
    mat4 transMat;
};
layout(set = 1, binding = 0) uniform sampler2D uCustomSampler;
layout(set = 1, binding = 1) uniform uCustomBuffer
{
    CustomEnvProperties customProperties;
};
layout(set = 1, binding = 2) uniform uCustomBuffer2
{
    CustomEnvProperties customProperties2;
};
layout(set = 1, binding = 3) uniform uCustomBuffer3
{
    CustomEnvProperties customProperties3;
};

// in / out

layout(location = 0) out vec2 outUv;
layout(location = 1) out flat uint outIndices; // packed camera

/*
 * Fullscreen env triangle.
 */
void main(void)
{
    float x = -1.0 + float((gl_VertexIndex & 1) << 2);
    float y = 1.0 - float((gl_VertexIndex & 2) << 1);
    CORE_VERTEX_OUT(vec4(x, y, 1.0, 1.0));
    outUv = vec2(x, y);

    const uint cameraIdx = GetUnpackCameraIndex(uGeneralData);
    outIndices = GetPackFlatIndices(cameraIdx, 0);

    vec4 normCoord = vec4((outUv + 1.0) * 0.5, 0.0, 1.0);
    normCoord = transpose(customProperties.transMat) * normCoord;
    outUv = normCoord.xy;
}
