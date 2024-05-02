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

#ifndef API_RENDER_SHADERS_COMMON_POST_PROCESS_COMMON_H
#define API_RENDER_SHADERS_COMMON_POST_PROCESS_COMMON_H

#include "render_post_process_structs_common.h"

#ifdef VULKAN

/**
 * returns vignette coefficient with values in the range 0-1
 */
float getVignetteCoeff(const vec2 uv, CORE_RELAXEDP const float coeff, CORE_RELAXEDP const float power)
{
    vec2 uvVal = uv.xy * (vec2(1.0) - uv.yx);
    float vignette = uvVal.x * uvVal.y * coeff;
    vignette = pow(vignette, power);
    return clamp(vignette, 0.0, 1.0);
}

/**
 * returns chroma coefficient with values in the range 0-1
 */
float getChromaCoeff(const vec2 uv, CORE_RELAXEDP const float chromaCoefficient)
{
    // this is cheap chroma
    vec2 distUv = (uv - 0.5) * 2.0;
    return dot(distUv, distUv) * chromaCoefficient;
}

/**
 * Returns 0-1 and input should be 0-1
 */
float RandomDither(vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

#endif

#endif // API_RENDER_SHADERS_COMMON_POST_PROCESS_COMMON_H
