#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

layout(constant_id = 0) const uint CORE_POST_PROCESS_FLAGS = 0;

// includes

#include "render/shaders/common/render_color_conversion_common.h"

#include "render/shaders/common/render_post_process_common.h"
#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_tonemap_common.h"

// sets

layout(set = 0, binding = 0) uniform texture2D uTex;
layout(set = 0, binding = 1) uniform sampler uSampler;

layout(push_constant, std430) uniform uPushConstant
{
    PostProcessTonemapStruct uPc;
};

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

//>DECLARATIONS_CORE_POST_PROCESS
#include "render/shaders/common/render_post_process_blocks.h"

/*
fragment shader for post process and tonemapping
*/
void main(void)
{
    outColor = textureLod(sampler2D(uTex, uSampler), inUv, 0);

    if ((uPc.flags.x & POST_PROCESS_SPECIALIZATION_COLOR_FRINGE_BIT) ==
        POST_PROCESS_SPECIALIZATION_COLOR_FRINGE_BIT) {
        const vec2 uvSize = uPc.texSizeInvTexSize.zw;
        // this is cheap chroma
        const vec4 fringeFactor = uPc.colorFringe;
        const vec2 distUv = (inUv - 0.5) * 2.0;
        const CORE_RELAXEDP float chroma = dot(distUv, distUv) * fringeFactor.y * fringeFactor.x;

        const vec2 uvDistToImageCenter = chroma * uvSize;
        const CORE_RELAXEDP float chromaRed =
            textureLod(sampler2D(uTex, uSampler), inUv - vec2(uvDistToImageCenter.x, uvDistToImageCenter.y), 0).x;
        const CORE_RELAXEDP float chromaBlue =
            textureLod(sampler2D(uTex, uSampler), inUv + vec2(uvDistToImageCenter.x, uvDistToImageCenter.y), 0).z;

        outColor.r = chromaRed;
        outColor.b = chromaBlue;
    }

    if ((uPc.flags.x & POST_PROCESS_SPECIALIZATION_TONEMAP_BIT) == POST_PROCESS_SPECIALIZATION_TONEMAP_BIT) {
        const vec4 tonemapFactor = uPc.tonemap;
        const float exposure = tonemapFactor.x;
        const vec3 x = outColor.rgb * exposure;
        const uint tonemapType = uint(tonemapFactor.w);
        if (tonemapType == CORE_POST_PROCESS_TONEMAP_ACES) {
            outColor.rgb = TonemapAces(x);
        } else if (tonemapType == CORE_POST_PROCESS_TONEMAP_ACES_2020) {
            outColor.rgb = TonemapAcesFilmRec2020(x);
        } else if (tonemapType == CORE_POST_PROCESS_TONEMAP_FILMIC) {
            const float exposureEstimate = 6.0f;
            outColor.rgb = TonemapFilmic(x * exposureEstimate);
        }
    }

    if ((uPc.flags.x & POST_PROCESS_SPECIALIZATION_VIGNETTE_BIT) == POST_PROCESS_SPECIALIZATION_VIGNETTE_BIT) {
        const vec2 uvVal = inUv.xy * (vec2(1.0) - inUv.yx);
        const vec4 vignetteFactor = uPc.vignette;
        CORE_RELAXEDP float vignette = uvVal.x * uvVal.y * vignetteFactor.x * 40.0;
        vignette = clamp(pow(vignette, vignetteFactor.y), 0.0, 1.0);
        outColor.rgb *= vignette;
    }

    if ((uPc.flags.x & POST_PROCESS_SPECIALIZATION_COLOR_CONVERSION_BIT) ==
        POST_PROCESS_SPECIALIZATION_COLOR_CONVERSION_BIT) {
        const uint conversionType = CORE_POST_PROCESS_COLOR_CONVERSION_SRGB;
        if (conversionType == CORE_POST_PROCESS_COLOR_CONVERSION_SRGB) {
            outColor.rgb = LinearToSrgb(outColor.rgb);
        }
    }
}
