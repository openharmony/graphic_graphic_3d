#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_multiview : enable

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets

// in / out

layout(location = 0) out vec3 outCubeUv;

CORE_RELAXEDP vec3 GetCubemapCoord(CORE_RELAXEDP vec2 uv)
{
    const CORE_RELAXEDP vec3 coords[] = {
        vec3(1.0, -uv.y, -uv.x),  // +X
        vec3(-1.0, -uv.y, uv.x),  // -X
        vec3(uv.x, 1.0, uv.y),    // +Y
        vec3(uv.x, -1.0, -uv.y),  // -Y
        vec3(uv.x, -uv.y, 1.0),   // +Z
        vec3(-uv.x, -uv.y, -1.0), // -Z
    };
    const uint index = min(gl_ViewIndex, 5U);
    return coords[index];
}

//
// fullscreen triangle with multi-view
void main(void)
{
    CORE_RELAXEDP float x = -1.0 + float((gl_VertexIndex & 1) << 2);
    CORE_RELAXEDP float y = 1.0 - float((gl_VertexIndex & 2) << 1);
    CORE_VERTEX_OUT(vec4(x, y, 0, 1));

    outCubeUv = GetCubemapCoord(vec2(x, y));
}
