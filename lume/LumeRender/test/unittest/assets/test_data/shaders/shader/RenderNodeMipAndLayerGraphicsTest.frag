#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets

layout(set = 0, binding = 0) uniform sampler2D uImgSampler;

struct PushDataStruct {
    vec4 texSizeInvSize;
    vec4 color;
};

layout(push_constant) uniform PushConstant
{
    PushDataStruct pc;
};

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

void main(void)
{
    outColor = pc.color;
    outColor += texelFetch(uImgSampler, ivec2(0, 0), 0);
}
