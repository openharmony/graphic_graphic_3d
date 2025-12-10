#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "render/shaders/common/render_color_conversion_common.h"

#define CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT 16u
#define CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_COUNT 6u

struct DefaultCameraMatrixStruct {
    mat4 view;
    mat4 proj;
    mat4 viewProj;

    mat4 viewInv;
    mat4 projInv;
    mat4 viewProjInv;

    mat4 viewPrevFrame;
    mat4 projPrevFrame;
    mat4 viewProjPrevFrame;

    mat4 shadowViewProj;
    mat4 shadowViewProjInv;

    vec4 jitter;
    vec4 jitterPrevFrame;

    uvec4 indices;
    uvec4 multiViewIndices;

    vec4 frustumPlanes[CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_COUNT];

    uvec4 counts;
    uvec4 pad0;
    mat4 envProjInv;
    mat4 matPad1;
};

#define NUM_SAMPLES 5
#define DEPTH_SIGMA 0.5f
#define SAMPLE_DISTANCE 1.5f

layout(set = 0, binding = 0) uniform sampler2D uSSAOSampler;
layout(set = 0, binding = 1) uniform sampler2D uColorSampler;
layout(set = 0, binding = 2) uniform sampler2D uDepthSampler;

layout(set = 0, binding = 3, std140) uniform uCameraMatrices
{
    DefaultCameraMatrixStruct uCameras[CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT];
};

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

float linearizeDepth(const float depth, const float near, const float far) {
    return (2.0f * near) / (far + near - depth * (far - near));
}

vec2 ExtractNearFar(const mat4 proj)
{
    const float m22 = proj[2u][2u];
    const float m32 = proj[3u][2u];

    return vec2(m32 / (m22 - 1.0f), m32 / (m22 + 1.0f));
}

void main() {
    const vec4 originalColor = texture(uColorSampler, inUv);

    const float centerDepth = texture(uDepthSampler, inUv).r;

    const vec2 nearFar = ExtractNearFar(uCameras[0].proj);
    const float centerLinearDepth = linearizeDepth(centerDepth, nearFar.x, nearFar.y);

    const vec2 ssaoTexelSize = 1.0f / vec2(textureSize(uSSAOSampler, 0));

    const float depthSigma2 = DEPTH_SIGMA * DEPTH_SIGMA;

    const vec2 offsets[NUM_SAMPLES] = vec2[NUM_SAMPLES](
        vec2(0.0f, 0.0f),
        vec2(-SAMPLE_DISTANCE, 0.0f),
        vec2(SAMPLE_DISTANCE, 0.0f),
        vec2(0.0f, -SAMPLE_DISTANCE),
        vec2(0.0f, SAMPLE_DISTANCE)
    );

    const float spatialWeights[NUM_SAMPLES] = float[NUM_SAMPLES](1.0f, 0.6065f, 0.6065f, 0.6065f, 0.6065f);

    float totalWeight = 0.0f;
    float weightedSSAO = 0.0f;

    // depth-guided bilateral filtering
    for (int ii = 0; ii < NUM_SAMPLES; ii++) {
        const vec2 sampleUV = inUv + offsets[ii] * ssaoTexelSize;

        const float ssaoSample = texture(uSSAOSampler, sampleUV).r;

        const float sampleDepth = texture(uDepthSampler, sampleUV).r;
        const float sampleLinearDepth = linearizeDepth(sampleDepth, nearFar.x, nearFar.y);

        const float depthDiff = abs(centerLinearDepth - sampleLinearDepth);
        const float depthWeight = exp(-(depthDiff * depthDiff) / (2.0f * depthSigma2));

        const float weight = spatialWeights[ii] * depthWeight;

        weightedSSAO += ssaoSample * weight;
        totalWeight += weight;
    }

    const float finalSSAO = weightedSSAO / max(totalWeight, 1e-4f);
    const vec3 finalColor = originalColor.rgb * finalSSAO;

    outColor = vec4(finalColor, originalColor.a);
}