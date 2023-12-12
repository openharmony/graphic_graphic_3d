/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets

layout(set = 0, binding = 0) uniform sampler uSampler;
layout(set = 0, binding = 1) uniform texture2D uTex;

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

struct PushConstantStruct
{
    vec4 texSizeInvTexSize;
};

layout(push_constant, std430) uniform uPushConstant
{
    PushConstantStruct uPc;
};

void main(void)
{
    const vec2 ths = uPc.texSizeInvTexSize.zw * 0.5;
    const vec2 uv = inUv;

    // center
    vec4 color = textureLod(sampler2D(uTex, uSampler), uv, 0) * 0.5;
    // corners
    // 1.0 / 8.0 = 0.125
    color += textureLod(sampler2D(uTex, uSampler), uv - ths, 0) * 0.125;
    color += textureLod(sampler2D(uTex, uSampler), vec2(uv.x + ths.x, uv.y - ths.y), 0) * 0.125;
    color += textureLod(sampler2D(uTex, uSampler), vec2(uv.x - ths.x, uv.y + ths.y), 0) * 0.125;
    color += textureLod(sampler2D(uTex, uSampler), uv + ths, 0) * 0.125;

    outColor = color.rgba;
}
