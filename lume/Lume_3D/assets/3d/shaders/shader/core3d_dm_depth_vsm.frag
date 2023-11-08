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
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets

// in / out

layout (location = 0) out vec4 outColor;

//layout (depth_unchanged) out float gl_FragDepth;

#define CORE_ADJUST_VSM_MOMENTS 1

/*
fragment shader for VSM depth pass.
*/
void main(void)
{
    const float moment1 = gl_FragCoord.z;
    float moment2 = moment1 * moment1;

#if (CORE_ADJUST_VSM_MOMENTS == 1)
    // adjust moments
    float dx = dFdx(moment1);
    float dy = dFdy(moment1);
    moment2 += 0.25*(dx*dx+dy*dy);
#endif

    outColor = vec4(moment1, moment2, 0.0, 0.0);
}
