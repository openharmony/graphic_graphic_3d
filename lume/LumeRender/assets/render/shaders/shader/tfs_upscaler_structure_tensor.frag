#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"

// Tensor-Field Superresolution (TFS) Structure Tensor Pass

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 tensorData;     // principalAngle, coherence, edgeStrength, isotropy

layout(set = 0, binding = 0) uniform texture2D gradientTex;
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

#define PI2 1.570796327f // pi/2

void main()
{
    if (any(greaterThanEqual(gl_FragCoord.xy, uPc.inputSize.xy + 1.0f)) || any(lessThan(gl_FragCoord.xy, vec2(-1.0f)))) {
        return;
    }

    mat2 accumulatedTensor = mat2(0.0f);
    float totalWeight = 0.0f;

    const vec2 offsetSamples[9] = vec2[9](
        textureOffset(sampler2D(gradientTex, uSampler), vTexCoord, ivec2(-1, -1)).rg,
        textureOffset(sampler2D(gradientTex, uSampler), vTexCoord, ivec2(0, -1)).rg,
        textureOffset(sampler2D(gradientTex, uSampler), vTexCoord, ivec2(1, -1)).rg,
        textureOffset(sampler2D(gradientTex, uSampler), vTexCoord, ivec2(-1, 0)).rg,
        textureOffset(sampler2D(gradientTex, uSampler), vTexCoord, ivec2(0, 0)).rg,
        textureOffset(sampler2D(gradientTex, uSampler), vTexCoord, ivec2(1, 0)).rg,
        textureOffset(sampler2D(gradientTex, uSampler), vTexCoord, ivec2(-1, 1)).rg,
        textureOffset(sampler2D(gradientTex, uSampler), vTexCoord, ivec2(0, 1)).rg,
        textureOffset(sampler2D(gradientTex, uSampler), vTexCoord, ivec2(1, 1)).rg
    );

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            const int idx = (dy + 1) * 3 + (dx + 1);
            const vec2 sampleCoord = gl_FragCoord.xy + vec2(dx, dy);

            if (!(all(greaterThanEqual(sampleCoord, vec2(0.0))) && all(lessThan(sampleCoord, uPc.inputSize.xy)))) {
                // out of bounds
                continue;
            }

            const vec2 gradient = offsetSamples[idx];

            mat2 localTensor;
            localTensor[0][0] = gradient.x * gradient.x;
            localTensor[0][1] = gradient.x * gradient.y;
            localTensor[1][0] = gradient.x * gradient.y;
            localTensor[1][1] = gradient.y * gradient.y;

            const float dist = length(vec2(dx, dy));
            const float weight = exp(-dist * dist);

            accumulatedTensor += localTensor * weight;
            totalWeight += weight;
        }
    }

    // normalize the tensor
    mat2 tensor = totalWeight > 0.0f ? accumulatedTensor / totalWeight : mat2(0.0f);

    // regularization
    tensor += 1e-6f;

    // eigenvalues
    const float trace = tensor[0][0] + tensor[1][1];
    const float det = determinant(tensor);
    const float discriminant = max(trace * trace - 4.0f * det, 0.0f);
    const float sqrtDisc = sqrt(discriminant);
    const float lambda1 = (trace + sqrtDisc) * 0.5f;
    const float lambda2 = (trace - sqrtDisc) * 0.5f;
    const float edgeStrength = lambda1 - lambda2;
    const float coherence = (lambda1 - lambda2) / (lambda1 + lambda2 + 1e-6f);

    float principalAngle = 0.0f;
    const float offDiagMag = abs(tensor[0][1]);
    const float diagDiff = tensor[0][0] - tensor[1][1];

    if (offDiagMag > max(1e-6f, abs(diagDiff) * 0.1f)) {
        principalAngle = 0.5f * atan(2.0f * tensor[0][1], diagDiff);
    } else {
        if (abs(diagDiff) > 1e-6f) {
            principalAngle = (tensor[0][0] > tensor[1][1]) ? 0.0f : PI2;
        } else {
            principalAngle = 0.0f;
        }
    }

    const float isotropy = clamp(lambda2 / (lambda1 + 1e-6f), 0.0f, 1.0f);

    /**
     * principalAngle: angle of the principal eigenvector. Perpendicular to an edge. [0, pi]
     * coherence: directional consistency. coherence = 0: noise or uniform color. coherence = 1: gradients aligned, e.g. edges.
     * edgeStrength: absolute anisotropy.
     * isotropy: local area metric. isotropy = 0: pure edge. isotropy = 1: uniform/corner-like.
     */

    tensorData = vec4(principalAngle, coherence, edgeStrength, isotropy);
}