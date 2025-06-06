#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// in / out

layout(location = 0) in vec4 inColor;
layout(location = 0) out vec4 outColor;

/*
fragment shader for dot field effect.
*/
void main(void)
{
    const vec2 t = (gl_PointCoord.xy - 0.5f) * 2.f;
    float r = clamp(1.0f - length(t), 0.f, 1.f);
    outColor = inColor * r;
}
