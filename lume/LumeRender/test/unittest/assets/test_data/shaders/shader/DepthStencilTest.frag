#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets

// in / out

layout (location = 0) out vec4 outColor;

void main(void)
{
    outColor = vec4(0.5, 0.5, 0.5, 1.0);
}
