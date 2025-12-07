#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets

// in / out

layout(location = 0) in vec3 inPos;

void main(void)
{
    CORE_VERTEX_OUT(vec4(inPos.x, inPos.y, inPos.z, 1));
}
