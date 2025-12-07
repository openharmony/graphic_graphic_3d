#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"

// Tensor-Field Superresolution (TFS) Tensor Field Upscaling Pass

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform texture2D inputTex;
layout(set = 0, binding = 1) uniform texture2D tensorDataTex;    // principalAngle, coherence, edgeStrength, isotropy
layout(set = 0, binding = 2) uniform sampler uSamplerNearest;

#define BASE_SIGMA 0.5f

struct UpscalePushConstantStruct {
    vec4 inputSize;
    vec4 outputSize;
    float smoothScale;

    float structureSensitivity;  // controls anisotropic vs isotropic behavior, default = 1.0f
    float edgeSharpness;         // controls edge preservation vs smoothing, default = 1.0f

    float padding;
};

layout(push_constant, std430) uniform uPushConstantBlock {
    UpscalePushConstantStruct uPc;
};

struct StructureTensor {
    float edgeStrength;
    float coherence;
    float isotropy;
    vec2 principalDir;
    vec2 orthogonalDir;
};

StructureTensor LoadStructureTensor(const vec2 uv)
{
    StructureTensor st;

    const vec4 tensorData = texture(sampler2D(tensorDataTex, uSamplerNearest), uv);

    const float principalAngle = tensorData.x;
    st.coherence = tensorData.y;
    st.edgeStrength = tensorData.z;
    st.isotropy = tensorData.w;

    st.principalDir = vec2(cos(principalAngle), sin(principalAngle));
    st.orthogonalDir = vec2(-sin(principalAngle), cos(principalAngle));

    return st;
}

vec3 TensorUpscale(const vec2 subPixelPos, const ivec2 inputPixelInt, const StructureTensor st)
{
    const vec2 centeredPos = subPixelPos - vec2(0.5f);

    const float projPrincipal = dot(centeredPos, st.principalDir);
    const float projOrthogonal = dot(centeredPos, st.orthogonalDir);

    vec3 result = vec3(0.0f);
    float totalWeight = 0.0f;

    const ivec2 centerCoord = clamp(inputPixelInt, ivec2(0), ivec2(uPc.inputSize.xy) - 1);
    const vec2 centerUV = (vec2(centerCoord) + 0.5f) * uPc.inputSize.zw;

    const float strengthFactor = clamp(st.edgeStrength * 2.0f, 0.1f, 1.0f);

    const float coherenceContribution = mix(0.8f, 0.3f, st.coherence);
    const float strengthModulation = mix(1.0f, 0.7f, strengthFactor);

    const float isotropicSigma = BASE_SIGMA * coherenceContribution * strengthModulation;
    const float anisotropicPrincipalSigma = BASE_SIGMA * coherenceContribution * strengthModulation * 0.6f;
    const float anisotropicOrthogonalSigma = BASE_SIGMA * mix(0.8f, 1.8f, st.coherence * strengthFactor);

    const float adjustedIsotropy = clamp(st.isotropy * (2.0f - clamp(uPc.structureSensitivity, 0.0f, 2.0f)), 0.0f, 1.0f);

    const float principalSigma = pow(mix(anisotropicPrincipalSigma, isotropicSigma, adjustedIsotropy), uPc.edgeSharpness);
    const float orthogonalSigma = pow(mix(anisotropicOrthogonalSigma, isotropicSigma, adjustedIsotropy), uPc.edgeSharpness);

    const float baseColorSigma = mix(0.5f, 0.05f, st.coherence * strengthFactor);
    const float colorSigma = mix(baseColorSigma, baseColorSigma * 1.5f, adjustedIsotropy);

    const ivec2 sampleOffsets[5] = ivec2[5](
        ivec2(0, 0),

        ivec2(-1, 0),
        ivec2(1, 0),
        ivec2(0, -1),
        ivec2(0, 1)
    );

    const vec3 offsetSamples[5] = vec3[5](
        textureOffset(sampler2D(inputTex, uSamplerNearest), centerUV, sampleOffsets[0]).rgb,
        textureOffset(sampler2D(inputTex, uSamplerNearest), centerUV, sampleOffsets[1]).rgb,
        textureOffset(sampler2D(inputTex, uSamplerNearest), centerUV, sampleOffsets[2]).rgb,
        textureOffset(sampler2D(inputTex, uSamplerNearest), centerUV, sampleOffsets[3]).rgb,
        textureOffset(sampler2D(inputTex, uSamplerNearest), centerUV, sampleOffsets[4]).rgb
    );

    for (int ii = 0; ii < 5; ii++) {
        const vec2 sampleOffset = vec2(sampleOffsets[ii]);
        const ivec2 sampleCoord = inputPixelInt + ivec2(sampleOffset);

        const bool isValidSample = all(greaterThanEqual(sampleCoord, ivec2(0))) && all(lessThan(sampleCoord, ivec2(uPc.inputSize.xy)));
        const vec3 colorSample = isValidSample ? offsetSamples[ii] : offsetSamples[0];

        const float samplePrincipal = dot(sampleOffset, st.principalDir);
        const float sampleOrthogonal = dot(sampleOffset, st.orthogonalDir);

        const float distPrincipal = samplePrincipal - projPrincipal;
        const float distOrthogonal = sampleOrthogonal - projOrthogonal;

        const float weightPrincipal = exp(-0.5f * (distPrincipal * distPrincipal) / principalSigma);
        const float weightOrthogonal = exp(-0.5f * (distOrthogonal * distOrthogonal) / orthogonalSigma);

        const float directionalSpatialWeight = weightPrincipal * weightOrthogonal;
        const float isotropicSpatialWeight = exp(-0.5f * (dot(vec2(distPrincipal, distOrthogonal), vec2(distPrincipal, distOrthogonal))) / (isotropicSigma * isotropicSigma));
        const float spatialWeight = mix(directionalSpatialWeight, isotropicSpatialWeight, adjustedIsotropy * 0.3f);

        const vec3 colorDiff = colorSample - offsetSamples[0];
        const float colorDistSq = dot(colorDiff, colorDiff);
        const float colorWeight = exp(-colorDistSq / (colorSigma * colorSigma));

        const float baseColorInfluence = mix(0.2f, 0.6f, st.coherence * strengthFactor);
        const float colorInfluence = mix(baseColorInfluence, baseColorInfluence * 0.7f, adjustedIsotropy);

        // reduce weight for out-of-bounds samples
        const float boundsWeight = isValidSample ? 1.0f : 0.5f;
        const float finalWeight = spatialWeight * mix(1.0f, colorWeight, clamp(colorInfluence, 0.0f, 1.0f)) * boundsWeight;

        result += colorSample * finalWeight;
        totalWeight += finalWeight;
    }

    return totalWeight > 1e-6f ? result / totalWeight : offsetSamples[0];
}

void main()
{
    const ivec2 outputCoord = ivec2(gl_FragCoord.xy);
    const ivec2 iOutputSize = ivec2(uPc.outputSize.xy);

    if (any(greaterThanEqual(outputCoord, iOutputSize)) || any(lessThan(outputCoord, ivec2(0)))) {
        return;
    }

    const vec2 inputTexelUV = (vec2(outputCoord) + 0.5f) * uPc.outputSize.zw;
    const vec2 inputPixel = inputTexelUV * uPc.inputSize.xy - 0.5f;

    const ivec2 inputPixelInt = ivec2(floor(inputPixel));
    const vec2 subPixelOffset = fract(inputPixel);

    const vec2 centerUV = (vec2(inputPixelInt) + 0.5f) * uPc.inputSize.zw;

    const StructureTensor st = LoadStructureTensor(centerUV);
    const vec3 reconstructedColor = TensorUpscale(subPixelOffset, inputPixelInt, st);

    FragColor = vec4(reconstructedColor, 1.0f);
}