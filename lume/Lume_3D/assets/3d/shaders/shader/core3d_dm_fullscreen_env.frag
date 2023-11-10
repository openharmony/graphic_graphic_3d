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

#include "3d/shaders/common/3d_dm_structures_common.h"
#include "3d/shaders/common/3d_dm_indirect_lighting_common.h"
#include "3d/shaders/common/3d_dm_target_packing_common.h"

// sets

#include "3d/shaders/common/3d_dm_env_frag_layout_common.h"
#include "3d/shaders/common/3d_dm_inplace_post_process.h"

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outVelocityNormal;

//>DECLARATIONS_CORE3D_ENV
#include "common/core3d_env_blocks.h"

/*
fragment shader for environment sampling
*/
void main(void)
{
    outColor = vec4(0.0, 0.0, 0.0, 1.0);

    EnvironmentMapSampleBlock(CORE_DEFAULT_ENV_TYPE, inUv, uImgCubeSampler, uImgSampler, outColor);
    //>FUNCTIONS_CORE3D_ENV

    // specialization for post process
    if (CORE_POST_PROCESS_FLAGS > 0) {
        vec2 fragUv;
        CORE_GET_FRAGCOORD_UV(fragUv, gl_FragCoord.xy, uGeneralData.viewportSizeInvViewportSize.zw);

        InplacePostProcess(fragUv, outColor);
    }

    // TODO: calculate camera velocity
    outVelocityNormal = GetPackVelocityAndNormal(vec2(0.0), vec3(0.0));
}
