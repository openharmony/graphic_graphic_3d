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
#include "render/shaders/common/render_post_process_common.h"

// sets

#include "render/shaders/common/render_post_process_layout_common.h"

layout(set = 1, binding = 0) uniform sampler uSampler;
layout(set = 1, binding = 1) uniform texture2D uTex;

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out float outColor;

void main(void)
{
    // expects to have downscale to half resolution
    const float d0 = texture(sampler2D(uTex, uSampler), inUv).x;
    const float d1 = textureOffset(sampler2D(uTex, uSampler), inUv, ivec2(1, 0)).x;
    const float d2 = textureOffset(sampler2D(uTex, uSampler), inUv, ivec2(0, 1)).x;
    const float d3 = textureOffset(sampler2D(uTex, uSampler), inUv, ivec2(1, 1)).x;

    const float depth = max(max(d0, d1), max(d2, d3));

    outColor = depth;
}
