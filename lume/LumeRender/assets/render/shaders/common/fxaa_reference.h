/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SHADERS_COMMON_FXAA_REFERENCE_H
#define SHADERS_COMMON_FXAA_REFERENCE_H

#define FXAA_USE_PATCHES 1

// prefer high quality
#define FXAA_QUALITY 29

/**
 * Based on the NVIDIA FXAA 3.11 reference implementation  by TIMOTHY LOTTES
 * See: https://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf
 */

// integration settings
#ifndef FXAA_USE_PATCHES
#define FXAA_USE_PATCHES 0
#endif

#ifndef FXAA_LUMA_GREEN
#define FXAA_LUMA_GREEN 0
#endif

#define FxaaTexTop(t, s, p) textureLod(sampler2D(t, s), p, 0.0)
#define FxaaTexOff(t, s, p, o, r) textureLodOffset(sampler2D(t, s), p, 0.0, o)

#if (FXAA_LUMA_GREEN == 0)
float FxaaCompLuma(vec4 rgba)
{
    return rgba.w;
}
#else
float FxaaCompLuma(vec4 rgba)
{
    return rgba.y;
}
#endif

vec4 FxaaPixelShader(vec2 fragCoord, vec4 neighborCoords, texture2D texture, sampler samp, vec2 invFrameSize,
    vec4 invFrameOpt, vec4 invFrameOpt2, vec4 console360InvFrameOpt2, float subpixelQuality, float edgeThresholdQuality,
    float edgeThresholdMinQuality, float edgeSharpness, float edgeThreshold, float edgeThresholdMin,
    vec4 console360ConstDir)
{
    const float lumaNw = FxaaCompLuma(FxaaTexTop(texture, samp, neighborCoords.xy));
    const float lumaSw = FxaaCompLuma(FxaaTexTop(texture, samp, neighborCoords.xw));
    float lumaNe = FxaaCompLuma(FxaaTexTop(texture, samp, neighborCoords.zy));
    const float lumaSe = FxaaCompLuma(FxaaTexTop(texture, samp, neighborCoords.zw));

    // current pixel's color and luminance
    const vec4 colorM = FxaaTexTop(texture, samp, fragCoord.xy);

#if (FXAA_LUMA_GREEN == 0)
    const float lumaM = colorM.w;
#else
    const float lumaM = colorM.y;
#endif

    // min-max luminance around the pixel
    const float lumaMaxNwSw = max(lumaNw, lumaSw);
#if FXAA_USE_PATCHES == 0
    lumaNe += 1.0 / 384.0;
#endif
    const float lumaMinNwSw = min(lumaNw, lumaSw);
    const float lumaMaxNeSe = max(lumaNe, lumaSe);
    const float lumaMinNeSe = min(lumaNe, lumaSe);
    const float lumaMax = max(lumaMaxNeSe, lumaMaxNwSw);
    const float lumaMin = min(lumaMinNeSe, lumaMinNwSw);

    const float lumaMaxScaled = lumaMax * edgeThreshold;

    const float lumaMinM = min(lumaMin, lumaM);
    const float lumaMaxScaledClamped = max(edgeThresholdMin, lumaMaxScaled);

    const float lumaMaxM = max(lumaMax, lumaM);

    // edge direction
    float dirSwMinusNe = lumaSw - lumaNe;
#if FXAA_USE_PATCHES
    dirSwMinusNe += 1.0 / 512.0;
#endif

    const float lumaRange = lumaMaxM - lumaMinM;
    const float dirSeMinusNw = lumaSe - lumaNw;

    if (lumaRange < lumaMaxScaledClamped) {
        return colorM;
    }

    vec2 edgeDir;
    edgeDir.x = dirSwMinusNe + dirSeMinusNw;
    edgeDir.y = dirSwMinusNe - dirSeMinusNw;
    const vec2 normEdgeDir = normalize(edgeDir.xy);

    // sample colors along the edge
    const vec4 colorN1 = FxaaTexTop(texture, samp, fragCoord.xy - normEdgeDir * invFrameOpt.zw);
    const vec4 colorP1 = FxaaTexTop(texture, samp, fragCoord.xy + normEdgeDir * invFrameOpt.zw);

#if FXAA_USE_PATCHES
    const float dirAbsMinTimesC = max(abs(normEdgeDir.x), abs(normEdgeDir.y)) * edgeSharpness * 0.015;
    const vec2 scaledEdgeDir = normEdgeDir.xy * min(lumaRange / dirAbsMinTimesC, 3);
#else
    const float dirAbsMinTimesC = min(abs(normEdgeDir.x), abs(normEdgeDir.y)) * edgeSharpness;
    const vec2 scaledEdgeDir = clamp(normEdgeDir.xy / dirAbsMinTimesC, -2, 2);
#endif

    const vec4 colorN2 = FxaaTexTop(texture, samp, fragCoord.xy - scaledEdgeDir * invFrameOpt2.zw);
    const vec4 colorP2 = FxaaTexTop(texture, samp, fragCoord.xy + scaledEdgeDir * invFrameOpt2.zw);

    const vec4 blendedA = colorN1 + colorP1;
    vec4 blendedB = ((colorN2 + colorP2) * 0.25) + (blendedA * 0.25);

#if (FXAA_LUMA_GREEN == 0)
    const bool useTwoTap = (blendedB.w < lumaMin) || (blendedB.w > lumaMax);
#else
    const bool useTwoTap = (blendedB.y < lumaMin) || (blendedB.y > lumaMax);
#endif
    if (useTwoTap) {
        blendedB.xyz = blendedA.xyz * 0.5;
    }

#if FXAA_USE_PATCHES
    blendedB = mix(blendedB, colorM, 0.25);
#endif
    return blendedB;
}

#endif // SHADERS_COMMON_FXAA_REFERENCE_H
