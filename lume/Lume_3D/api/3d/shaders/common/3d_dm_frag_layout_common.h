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

#ifndef SHADERS_COMMON_3D_DEFAULT_MATERIAL_FRAGMENT_LAYOUT_COMMON_H
#define SHADERS_COMMON_3D_DEFAULT_MATERIAL_FRAGMENT_LAYOUT_COMMON_H

#include "3d/shaders/common/3d_dm_structures_common.h"
#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"

#ifdef VULKAN

// sets

// Set 0 for general, camera, and lighting data
// UBOs visible in both VS and FS
layout(set = 0, binding = 0, std140) uniform uCameraMatrices
{
    DefaultCameraMatrixStruct uCameras[CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT];
};
layout(set = 0, binding = 1, std140) uniform uGeneralStructData
{
    DefaultMaterialGeneralDataStruct uGeneralData;
};
layout(set = 0, binding = 2, std140) uniform uEnvironmentStructData
{
    DefaultMaterialEnvironmentStruct uEnvironmentData;
};
layout(set = 0, binding = 3, std140) uniform uFogStructData
{
    DefaultMaterialFogStruct uFogData;
};
layout(set = 0, binding = 4, std140) uniform uLightStructData
{
    DefaultMaterialLightStruct uLightData;
};
layout(set = 0, binding = 5, std140) uniform uPostProcessStructData
{
    GlobalPostProcessStruct uPostProcessData;
};
layout(set = 0, binding = 6, std430) buffer uLightClusterIndexData
{
    DefaultMaterialLightClusterData uLightClusterData[CORE_DEFAULT_MATERIAL_MAX_CLUSTERS_COUNT];
};
layout(set = 0, binding = 7) uniform CORE_RELAXEDP sampler2D uSampColorPrePass;
layout(set = 0, binding = 8) uniform sampler2D uSampColorShadow;       // VSM or other
layout(set = 0, binding = 9) uniform sampler2DShadow uSampDepthShadow; // PCF
layout(set = 0, binding = 10) uniform samplerCube uSampRadiance;

// disable set 1 and 2 for environment
#ifndef CORE3D_ENVIRONMENT_FRAG_LAYOUT

// set 1 for dynamic uniform buffers
// UBOs visible in both VS and FS
layout(set = 1, binding = 0, std140) uniform uMeshStructData
{
    DefaultMaterialMeshStruct uMeshMatrix;
};
layout(set = 1, binding = 1, std140) uniform uObjectSkinStructData
{
    DefaultMaterialSkinStruct uSkinData;
};
layout(set = 1, binding = 2, std140) uniform uMaterialStructData
{
    DefaultMaterialMaterialStruct uMaterialData;
};
layout(set = 1, binding = 3, std140) uniform uMaterialTransformStructData
{
    DefaultMaterialTransformMaterialStruct uMaterialTransformData;
};
layout(set = 1, binding = 4, std140) uniform uMaterialUserStructData
{
    DefaultMaterialUserMaterialStruct uMaterialUserData;
};

// set 2 is for material data (base color separated for e.g. automatic hardware buffer static sampler support)
layout(set = 2, binding = 0) uniform CORE_RELAXEDP sampler2D uSampTextureBase;
layout(set = 2, binding = 1) uniform CORE_RELAXEDP sampler2D uSampTextures[CORE_MATERIAL_SAMPTEX_COUNT];

#endif // CORE3D_ENVIRONMENT_FRAG_LAYOUT

layout(constant_id = CORE_DM_CONSTANT_ID_MATERIAL_TYPE) const uint CORE_MATERIAL_TYPE = 0;
layout(constant_id = CORE_DM_CONSTANT_ID_MATERIAL_FLAGS) const uint CORE_MATERIAL_FLAGS = 0;
layout(constant_id = CORE_DM_CONSTANT_ID_LIGHTING_FLAGS) const uint CORE_LIGHTING_FLAGS = 0;
layout(constant_id = CORE_DM_CONSTANT_ID_POST_PROCESS_FLAGS) const uint CORE_POST_PROCESS_FLAGS = 0;
layout(constant_id = CORE_DM_CONSTANT_ID_CAMERA_FLAGS) const uint CORE_CAMERA_FLAGS = 0;

#else

#endif

#endif // SHADERS_COMMON_3D_DEFAULT_MATERIAL_STRUCTURES_COMMON_H
