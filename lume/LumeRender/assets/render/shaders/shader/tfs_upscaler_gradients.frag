#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"

// Tensor-Field Superresolution (TFS) Gradient Pass

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec2 gradientOut; // rg16f

layout(set = 0, binding = 0) uniform texture2D inputTex;
layout(set = 0, binding = 1) uniform sampler uSampler;

struct UpscalePushConstantStruct {
    vec4 inputSize;
    vec4 outputSize;
    float smoothScale;
    float padding[3];
};

layout(push_constant, std430) uniform uPushConstantBlock {
    UpscalePushConstantStruct uPc;
};

float Rgb2Luma(const vec3 rgb) {
    return dot(rgb, vec3(0.299f, 0.587f, 0.114f));
}

void main()
{
    if (any(greaterThanEqual(gl_FragCoord.xy, uPc.inputSize.xy + 1.0f)) || any(lessThan(gl_FragCoord.xy, vec2(-1.0f)))) {
        return;
    }

    const float sigma2 = uPc.smoothScale * uPc.smoothScale * 0.8f * 0.8f;

    vec2 gradient = vec2(0.0);
    float weightSum = 0.0;

    const vec3 offsetSamples[9] = vec3[9](
        textureOffset(sampler2D(inputTex, uSampler), vTexCoord, ivec2(-1, -1)).rgb,
        textureOffset(sampler2D(inputTex, uSampler), vTexCoord, ivec2(0, -1)).rgb,
        textureOffset(sampler2D(inputTex, uSampler), vTexCoord, ivec2(1, -1)).rgb,
        textureOffset(sampler2D(inputTex, uSampler), vTexCoord, ivec2(-1, 0)).rgb,
        textureOffset(sampler2D(inputTex, uSampler), vTexCoord, ivec2(0, 0)).rgb,
        textureOffset(sampler2D(inputTex, uSampler), vTexCoord, ivec2(1, 0)).rgb,
        textureOffset(sampler2D(inputTex, uSampler), vTexCoord, ivec2(-1, 1)).rgb,
        textureOffset(sampler2D(inputTex, uSampler), vTexCoord, ivec2(0, 1)).rgb,
        textureOffset(sampler2D(inputTex, uSampler), vTexCoord, ivec2(1, 1)).rgb
    );

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            const int idx = (dy + 1) * 3 + (dx + 1);
            const vec2 sampleCoord = gl_FragCoord.xy + vec2(dx, dy);

            if (!(all(greaterThanEqual(sampleCoord, vec2(0.0))) && all(lessThan(sampleCoord, uPc.inputSize.xy)))) {
                // out of bounds
                continue;
            }

            const float luma = Rgb2Luma(offsetSamples[idx]);

            const float r2 = float(dx*dx + dy*dy);
            const float gaussianWeight = exp(-r2 / (2.0f * sigma2));

            const vec2 gaussianDev = -vec2(dx, dy) / sigma2 * gaussianWeight;

            gradient += gaussianDev * luma;
            weightSum += gaussianWeight;
        }
    }

    gradientOut = gradient / weightSum;
}