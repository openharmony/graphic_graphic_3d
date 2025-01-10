#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_multiview : enable

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets

// in / out

layout(location = 0) out vec2 outUv;
layout(location = 1) out flat uint outIndices;

//
// fullscreen triangle with multi-view
void main(void)
{
    float x = -1.0 + float((gl_VertexIndex & 1) << 2);
    float y = 1.0 - float((gl_VertexIndex & 2) << 1);
    outUv.x = (x + 1.0) * 0.5;
    outUv.y = (y + 1.0) * 0.5;
    CORE_VERTEX_OUT(vec4(x, y, 0, 1));

    outIndices = uint(gl_ViewIndex);
}
