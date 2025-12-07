#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform sampler2D uCmaaSampler;
layout(set = 0, binding = 1) uniform sampler2D uColorSampler;

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outBlit;

void main() 
{
    const vec4 cmaaValue = textureLod(uCmaaSampler, inUv, 0);
    const vec4 colorSample = textureLod(uColorSampler, inUv, 0);

    outBlit = cmaaValue.xyz == vec3(0) ? colorSample : vec4(cmaaValue.rgb, colorSample.a);
}