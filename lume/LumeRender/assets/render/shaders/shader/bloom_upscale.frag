#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "render/shaders/common/render_post_process_structs_common.h"

#include "common/bloom_common.h"

// sets

layout(set = 0, binding = 0) uniform texture2D uTex;
layout(set = 0, binding = 1) uniform texture2D uInputColor;
layout(set = 0, binding = 2) uniform sampler uSampler;

layout(push_constant, std430) uniform uPostProcessPushConstant
{
    LocalPostProcessPushConstantStruct uPc;
};

layout(constant_id = 0) const uint CORE_POST_PROCESS_FLAGS = 0;

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

///////////////////////////////////////////////////////////////////////////////
// bloom upscale

void main()
{
    const vec2 uv = inUv;
    vec3 color = vec3(0.0f);
    if (uPc.factor.x == CORE_BLOOM_TYPE_NORMAL) {
        color = BloomUpscale(uv, uPc.viewportSizeInvSize.zw, uTex, uSampler);
    } else if (uPc.factor.x == CORE_BLOOM_TYPE_HORIZONTAL) {
        color = BloomUpscaleHorizontal(uv, uPc.viewportSizeInvSize.zw, uTex, uSampler);
    } else if (uPc.factor.x == CORE_BLOOM_TYPE_VERTICAL) {
        color = BloomDownScaleVertical(uv, uPc.viewportSizeInvSize.zw, uTex, uSampler);
    }
    const vec3 baseColor = textureLod(sampler2D(uInputColor, uSampler), uv, 0).xyz;
    const float scatter = uPc.factor.w;
    color = min((baseColor + color * scatter), CORE_BLOOM_CLAMP_MAX_VALUE);

    outColor = vec4(color, 1.0);
}
