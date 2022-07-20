/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef SHADERS__COMMON__FXAA_REFERENCE_H
#define SHADERS__COMMON__FXAA_REFERENCE_H

vec4 FxaaPixelShader(
    vec2 pos,
    vec4 fxaaConsolePosPos,
    texture2D tex,
    sampler samp,
    vec2 fxaaQualityRcpFrame,
    vec4 fxaaConsoleRcpFrameOpt,
    vec4 fxaaConsoleRcpFrameOpt2,
    vec4 fxaaConsole360RcpFrameOpt2,
    float fxaaQualitySubpix,
    float fxaaQualityEdgeThreshold,
    float fxaaQualityEdgeThresholdMin,
    float fxaaConsoleEdgeSharpness,
    float fxaaConsoleEdgeThreshold,
    float fxaaConsoleEdgeThresholdMin,
    vec4 fxaaConsole360ConstDir
) {
    return vec4(0.0f, 0.0f, 0.0f, 0.0f);
}
#endif // SHADERS__COMMON__FXAA_REFERENCE_H

