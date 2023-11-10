/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "render/shaders/common/render_color_conversion_common.h"

#include "common/bloom_common.h"

// sets

#include "render/shaders/common/render_post_process_layout_common.h"

layout(set = 1, binding = 0) uniform texture2D uTex;
layout(set = 1, binding = 1) uniform sampler uSampler;

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

///////////////////////////////////////////////////////////////////////////////
// bloom threshold downscale

void main()
{
    const vec2 uv = inUv;
    //vec3 color = bloomDownscale(uv, uPc.viewportSizeInvSize.zw, uTex, uSampler);
    vec3 color = bloomDownscaleWeighted(uv, uPc.viewportSizeInvSize.zw, uTex, uSampler);

    const float luma = CalcLuma(color);
    if (luma < uPc.factor.x)
    {
        color = vec3(0.0);
    } else if (luma < uPc.factor.y)
    {
        const float divisor = uPc.factor.y - uPc.factor.x; // cannot be zero -> if equal, should go to first if
        const float coeff = 1.0 / divisor;
        const float lumaCoeff = (luma - uPc.factor.x) * coeff;
        color *= max(0.0, lumaCoeff);
    }

    color = min(color, CORE_BLOOM_CLAMP_MAX_VALUE);
    outColor = vec4(color, 1.0);
}
