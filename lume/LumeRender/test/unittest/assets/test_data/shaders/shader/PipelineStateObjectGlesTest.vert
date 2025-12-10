#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets

// in / out

layout(location = 0) in vec3 in0;
layout(location = 1) in vec4 in1;
layout(location = 2) in float in2;
layout(location = 3) in ivec4 in3;
layout(location = 4) in vec4 in4;
layout(location = 5) in uvec4 in5;
layout(location = 6) in uint in6;
layout(location = 7) in vec2 in7;

layout(location = 0) out vec3 out0;
layout(location = 1) out vec4 out1;
layout(location = 2) out float out2;
layout(location = 3) flat out ivec4 out3;
layout(location = 4) out vec4 out4;
layout(location = 5) flat out uvec4 out5;
layout(location = 6) flat out uint out6;
layout(location = 7) out vec2 out7;


void main(void)
{
    out0 = in0;
    out1 = in1;
    out2 = in2;
    out3 = in3;
    out4 = in4;
    out5 = in5;
    out6 = in6;
    out7 = in7;
    CORE_VERTEX_OUT(vec4(in0.x, in0.y, in0.z, 1));
}
