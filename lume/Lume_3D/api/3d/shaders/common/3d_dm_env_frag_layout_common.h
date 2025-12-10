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

#ifndef SHADERS_COMMON_3D_DEFAULT_MATERIAL_ENV_FRAGMENT_LAYOUT_COMMON_H
#define SHADERS_COMMON_3D_DEFAULT_MATERIAL_ENV_FRAGMENT_LAYOUT_COMMON_H

// disable set 1 and 2
#define CORE3D_ENVIRONMENT_FRAG_LAYOUT
#include "3d/shaders/common/3d_dm_frag_layout_common.h"
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"

#ifdef VULKAN

// sets

// initial sets from default material

layout(set = 0, binding = 2, std140) uniform uEnvironmentStructDataArray
{
    DefaultMaterialEnvironmentStruct uEnvironmentDataArray[CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT];
};

layout(set = 3, binding = 0) uniform sampler2D uImgSampler;
layout(set = 3, binding = 1) uniform samplerCube uImgCubeSampler;
layout(set = 3, binding = 2) uniform samplerCube uImgCubeSamplerBlender;
layout(set = 3, binding = 3) uniform sampler2D uImgTLutSampler;

layout(constant_id = CORE_DM_CONSTANT_ID_ENV_TYPE) const uint CORE_DEFAULT_ENV_TYPE = 0;

#else

#endif

#endif // SHADERS_COMMON_3D_DEFAULT_MATERIAL_ENV_FRAGMENT_LAYOUT_COMMON_H
