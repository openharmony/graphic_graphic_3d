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

#ifndef API_RENDER_SHADERS_COMMON_TONEMAP_COMMON_H
#define API_RENDER_SHADERS_COMMON_TONEMAP_COMMON_H

#include "render_post_process_common.h"

/*
Aces tonemapping.
https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
*/
vec3 TonemapAces(vec3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

/*
Aces film rec 2020 tonemapping.
https://knarkowicz.wordpress.com/2016/08/31/hdr-display-first-steps/
*/
vec3 TonemapAcesFilmRec2020(vec3 x)
{
    float a = 15.8f;
    float b = 2.12f;
    float c = 1.2f;
    float d = 5.92f;
    float e = 1.9f;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

/*
Filmic tonemapping.
// http://filmicgames.com/archives/75
*/
float TonemapFilmic(float x)
{
    const float a = 0.15f;
    const float b = 0.50f;
    const float c = 0.10f;
    const float d = 0.20f;
    const float e = 0.02f;
    const float f = 0.30f;
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}
vec3 TonemapFilmic(vec3 x)
{
    const float a = 0.15f;
    const float b = 0.50f;
    const float c = 0.10f;
    const float d = 0.20f;
    const float e = 0.02f;
    const float f = 0.30f;
    const float w = 11.2f;
    const vec3 curr = ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
    const float whiteScale = 1.0 / TonemapFilmic(w);
    return curr * vec3(whiteScale);
}

#endif // API_RENDER_SHADERS_COMMON_TONEMAP_COMMON_H
