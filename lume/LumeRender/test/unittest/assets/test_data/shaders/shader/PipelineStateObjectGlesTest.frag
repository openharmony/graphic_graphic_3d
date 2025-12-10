#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets

// in / out

layout(location = 0) in vec3 out0;
layout(location = 1) in vec4 out1;
layout(location = 2) in float out2;
layout(location = 3) flat in ivec4 out3;
layout(location = 4) in vec4 out4;
layout(location = 5) flat in uvec4 out5;
layout(location = 6) flat in uint out6;
layout(location = 7) in vec2 out7;


layout (location = 0) out vec4 outColor;

void main(void)
{
    float sum = out0.x + out1.x + out2.x + out3.x + out4.x + out5.x + out6.x + out7.x;
    outColor = vec4(0.5, sum, 0.5, 1.0);
}
