#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// From Core:
#include "render/shaders/common/render_color_conversion_common.h"

#define CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT 16
#define CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_COUNT 6

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

#define HIGH_QUALITY

#ifdef HIGH_QUALITY
    #define SSAO_KERNEL_SIZE 64
#else
    #define SSAO_KERNEL_SIZE 16
#endif
#define SSAO_FALLOFF_START 0.2f
#define SSAO_FALLOFF_END 2.0f

layout(set = 0, binding = 0, std140) uniform uShaderData
{
    vec4 inParams[2]; // [resX, resY, time, hasValidNormalTex], [radius, bias, intensity, contrast]
};

layout(set = 0, binding = 1) uniform sampler2D uColorSampler;
layout(set = 0, binding = 2) uniform sampler2D uDepthSampler;

layout(set = 0, binding = 3, std140) uniform uCameraMatrices
{
    DefaultCameraMatrixStruct uCameras[CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT];
};
layout(set = 0, binding = 4) uniform sampler2D uVelocityNormal;

// in / out
layout(location = 0) in vec2 inUv;
layout(location = 0) out float outOcclusion;

#ifdef HIGH_QUALITY
    // hemisphere samples
    const vec3 ssaoKernel[SSAO_KERNEL_SIZE] = vec3[](
        vec3(0.0426f, -0.0646f, 0.0634f),
        vec3(0.0868f, 0.0489f, 0.0104f),
        vec3(0.0724f, -0.0272f, 0.0648f),
        vec3(-0.0844f, -0.0424f, 0.0385f),
        vec3(-0.0801f, 0.0396f, 0.0523f),
        vec3(-0.0635f, -0.0110f, 0.0835f),
        vec3(0.0723f, -0.0688f, 0.0410f),
        vec3(0.0604f, 0.0644f, 0.0669f),
        vec3(-0.0414f, 0.0549f, 0.0910f),
        vec3(0.0805f, 0.0838f, 0.0195f),
        vec3(-0.0684f, -0.0776f, 0.0647f),
        vec3(0.0870f, 0.0665f, 0.0635f),
        vec3(0.0966f, -0.0552f, 0.0703f),
        vec3(-0.0443f, -0.0711f, 0.1085f),
        vec3(-0.0967f, -0.0837f, 0.0640f),
        vec3(0.0391f, -0.1291f, 0.0644f),
        vec3(0.0428f, 0.0853f, 0.1237f),
        vec3(-0.0179f, -0.0366f, 0.1583f),
        vec3(-0.0365f, 0.1667f, 0.0132f),
        vec3(0.1576f, -0.0763f, 0.0386f),
        vec3(-0.1799f, -0.0536f, 0.0075f),
        vec3(0.0843f, 0.0866f, 0.1554f),
        vec3(0.1472f, -0.0450f, 0.1375f),
        vec3(-0.1111f, 0.1798f, 0.0459f),
        vec3(0.0291f, 0.1092f, 0.1964f),
        vec3(-0.1309f, 0.0055f, 0.1979f),
        vec3(0.1676f, -0.1470f, 0.1100f),
        vec3(0.0124f, -0.1375f, 0.2205f),
        vec3(-0.2409f, -0.1221f, 0.0344f),
        vec3(0.1379f, 0.1298f, 0.2127f),
        vec3(0.0953f, 0.2533f, 0.1242f),
        vec3(-0.0113f, 0.0020f, 0.3109f),
        vec3(-0.0869f, 0.2853f, 0.1291f),
        vec3(0.2641f, 0.1072f, 0.1840f),
        vec3(0.1868f, -0.2712f, 0.1299f),
        vec3(0.3408f, 0.0985f, 0.1022f),
        vec3(0.3037f, -0.1824f, 0.1502f),
        vec3(-0.1252f, 0.3052f, 0.2277f),
        vec3(0.0175f, -0.3854f, 0.1589f),
        vec3(0.1390f, 0.4101f, 0.0327f),
        vec3(0.1867f, -0.3990f, 0.0992f),
        vec3(0.2845f, 0.2117f, 0.3075f),
        vec3(-0.4656f, 0.1025f, 0.1022f),
        vec3(0.1446f, -0.4002f, 0.2744f),
        vec3(-0.4691f, -0.1856f, 0.1468f),
        vec3(-0.3824f, -0.0103f, 0.3881f),
        vec3(-0.5490f, 0.1176f, 0.0632f),
        vec3(-0.0123f, 0.3881f, 0.4381f),
        vec3(-0.4351f, -0.2107f, 0.3659f),
        vec3(0.4285f, 0.2545f, 0.3814f),
        vec3(0.5052f, -0.3965f, 0.0961f),
        vec3(-0.3149f, -0.2081f, 0.5554f),
        vec3(-0.6084f, 0.2016f, 0.2666f),
        vec3(-0.4934f, 0.2256f, 0.4691f),
        vec3(0.6003f, 0.1191f, 0.4173f),
        vec3(-0.5400f, -0.0278f, 0.5407f),
        vec3(-0.4980f, -0.4564f, 0.4078f),
        vec3(0.5286f, 0.1759f, 0.5933f),
        vec3(-0.1347f, 0.7743f, 0.2941f),
        vec3(-0.6247f, 0.5929f, 0.0792f),
        vec3(-0.3589f, 0.1328f, 0.8047f),
        vec3(0.2537f, -0.2234f, 0.8531f),
        vec3(-0.5378f, -0.0164f, 0.7764f),
        vec3(-0.2474f, -0.8224f, 0.4554f)
    );
#else
    // hemisphere samples
    const vec3 ssaoKernel[SSAO_KERNEL_SIZE] = vec3[](
        vec3(-0.0507f, -0.0684f, 0.0525f),
        vec3(0.0958f, 0.0311f, 0.0052f),
        vec3(0.0654f, 0.0170f, 0.0784f),
        vec3(-0.0183f, -0.0711f, 0.0791f),
        vec3(0.0064f, 0.0237f, 0.1114f),
        vec3(0.0729f, 0.0466f, 0.0860f),
        vec3(-0.0342f, -0.1145f, 0.0553f),
        vec3(0.0963f, 0.0928f, 0.0508f),
        vec3(0.0067f, 0.1060f, 0.1146f),
        vec3(0.0753f, -0.0968f, 0.1195f),
        vec3(0.0977f, 0.1283f, 0.0964f),
        vec3(-0.0529f, -0.0939f, 0.1760f),
        vec3(0.1240f, -0.1816f, 0.0547f),
        vec3(-0.2207f, 0.0605f, 0.0969f),
        vec3(0.1770f, -0.1449f, 0.1477f),
        vec3(-0.1633f, 0.1239f, 0.2160f)
    );
#endif

vec3 GetViewPos(const vec2 uv, const float depth, const mat4 projInv) {
    const vec4 ndc = vec4(uv * 2.0f - 1.0f, depth, 1.0f);    
    const vec4 viewPos = projInv * ndc;

    return viewPos.xyz / viewPos.w;
}

vec3 ReconstructNormal(const vec2 uv, const vec3 viewPos, const vec2 texelSize, const mat4 projInv) {
    const float depthRight = textureLod(uDepthSampler, uv + vec2(texelSize.x, 0.0f), 0).r;
    const float depthUp = textureLod(uDepthSampler, uv + vec2(0.0f, texelSize.y), 0).r;

    const vec3 posRight = GetViewPos(uv + vec2(texelSize.x, 0.0f), depthRight, projInv);
    const vec3 posUp = GetViewPos(uv + vec2(0.0f, texelSize.y), depthUp, projInv);

    const vec3 dx = posRight - viewPos;
    const vec3 dy = posUp - viewPos;

    const vec3 normal = normalize(cross(dx, dy));
    return normal.z < 0.0f ? -normal : normal;
}

vec3 NormalOctDecode(vec2 f)
{
    f = f * 2.0 - 1.0;
    vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    const float t = clamp(-n.z, 0.0, 1.0);
    n.xy += vec2((n.x >= 0.0) ? -t : t, (n.y >= 0.0) ? -t : t);

    return normalize(n);
}

vec3 GetNormalSample(const vec2 uv, const mat4 viewMat) {
    const vec2 packed = texture(uVelocityNormal, uv).zw;
    const vec3 worldSpaceNormal = NormalOctDecode(packed);

    // world normal to view normal
    return normalize((viewMat * vec4(worldSpaceNormal, 0.0)).xyz);
}

mat3 CalculateTbn(const vec3 normal, const vec3 randomVec) {
    const vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    const vec3 bitangent = cross(normal, tangent);

    return mat3(tangent, bitangent, normal);
}

float InterleavedGradientNoise(const vec2 position) {
    const vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
    return fract(magic.z * fract(dot(position, magic.xy)));
}

vec3 GetRandomVector(const vec2 screenPos) {
    const float noise = InterleavedGradientNoise(screenPos);
    const float angle = noise * 2.0f * 3.14159265f;
    return vec3(cos(angle), sin(angle), 0.0f);
}

void main() {
    const bool hasValidNormalTex = inParams[0].w == 1.0f;
    const float ssaoRadius = inParams[1].x;
    const float ssaoBias = inParams[1].y;
    const float ssaoIntensity = inParams[1].z;

    const uint cameraIndex = 0;

    const float depth = textureLod(uDepthSampler, inUv, 0).r;
    if (depth >= 0.999f) {
        outOcclusion = 1.0f;
        return;
    }

    const vec2 screenSize = vec2(textureSize(uDepthSampler, 0));
    const vec2 texelSize = 1.0f / screenSize;
    const mat4 projInv = uCameras[cameraIndex].projInv;
    const mat4 viewInv = uCameras[cameraIndex].viewInv;

    const vec3 viewPos = GetViewPos(inUv, depth, projInv);
    const vec3 normal = hasValidNormalTex ? GetNormalSample(inUv, uCameras[cameraIndex].view) : ReconstructNormal(inUv, viewPos, texelSize, projInv);

    const vec2 screenPos = inUv * screenSize;
    const vec3 randomVec = GetRandomVector(screenPos);

    const mat3 tbn = CalculateTbn(normal, randomVec);
    const float projScale = uCameras[cameraIndex].proj[1][1];
    const float screenSpaceRadius = ssaoRadius * projScale / abs(viewPos.z);

    // occlusion accumulation
    float occlusion = 0.0f;

    for (int ii = 0; ii < SSAO_KERNEL_SIZE; ii++) {
        const vec3 sampleOffset = tbn * ssaoKernel[ii];
        const vec3 samplePos = viewPos + sampleOffset * screenSpaceRadius;

        // project to screen space
        const vec4 offset = uCameras[cameraIndex].proj * vec4(samplePos, 1.0f);
        const vec2 sampleUV = clamp((offset.xy / offset.w) * 0.5f + 0.5f, texelSize, 1.0f - texelSize);

        const float sampleDepth = textureLod(uDepthSampler, sampleUV, 0).r;

        // far plane
        if (sampleDepth >= 0.999f) {
            continue;
        }

        const vec3 sampleViewPos = GetViewPos(sampleUV, sampleDepth, projInv);

        const float depthDiff = sampleViewPos.z - viewPos.z;
        const float rangeCheck = smoothstep(SSAO_FALLOFF_END, SSAO_FALLOFF_START, abs(depthDiff) / screenSpaceRadius);

        const float occlusionTest = smoothstep(0.0f, ssaoBias, depthDiff);

        occlusion += occlusionTest * rangeCheck;
    }

    occlusion = occlusion * 1.0f / float(SSAO_KERNEL_SIZE);
    occlusion = 1.0f - occlusion;
    occlusion = pow(occlusion, ssaoIntensity);

    outOcclusion = occlusion;
}