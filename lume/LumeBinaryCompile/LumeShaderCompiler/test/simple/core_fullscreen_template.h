#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes

// sets
struct Data { float data; };

layout(set = 0, binding = 0) uniform u_data{ Data uData; };

// in / out
layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

//>DECLARATIONS_IMAGE LERP

void main(void)
{
    outColor = vec4(1.0);
    //>FUNCTIONS_IMAGE LERP
}
