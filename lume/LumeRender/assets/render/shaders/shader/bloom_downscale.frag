#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "render/shaders/common/render_post_process_structs_common.h"

#include "common/bloom_common.h"

// sets

layout(set = 0, binding = 0) uniform texture2D uTex;
layout(set = 0, binding = 1) uniform sampler uSampler;

layout(push_constant, std430) uniform uPostProcessPushConstant
{
    LocalPostProcessPushConstantStruct uPc;
};

layout(constant_id = 0) const uint CORE_POST_PROCESS_FLAGS = 0;

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

///////////////////////////////////////////////////////////////////////////////
// bloom downscale

void main()
{
    // texSizeInvTexSize needs to be the output resolution
    const vec2 uv = inUv;

    vec3 color = vec3(0.0);
    if (uPc.factor.x == CORE_BLOOM_TYPE_NORMAL) {
        if ((CORE_BLOOM_QUALITY_NORMAL & CORE_POST_PROCESS_FLAGS) == CORE_BLOOM_QUALITY_NORMAL) {
            color = BloomDownscale9(uv, uPc.viewportSizeInvSize.zw, uTex, uSampler);
        } else {
            color = BloomDownscale(uv, uPc.viewportSizeInvSize.zw, uTex, uSampler);
        }
    } else if (uPc.factor.x == CORE_BLOOM_TYPE_HORIZONTAL) {
        color = BloomDownScaleHorizontal(uv, uPc.viewportSizeInvSize.zw, uTex, uSampler);
    } else if (uPc.factor.x == CORE_BLOOM_TYPE_VERTICAL) {
        color = BloomDownScaleVertical(uv, uPc.viewportSizeInvSize.zw, uTex, uSampler);
    }

    outColor = vec4(min(color, CORE_BLOOM_CLAMP_MAX_VALUE), 1.0);
}
