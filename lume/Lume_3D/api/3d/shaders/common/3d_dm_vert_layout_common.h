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

#ifndef SHADERS_COMMON_3D_DEFAULT_MATERIAL_VERT_LAYOUT_COMMON_H
#define SHADERS_COMMON_3D_DEFAULT_MATERIAL_VERT_LAYOUT_COMMON_H

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

layout(constant_id = CORE_DM_CONSTANT_ID_SUBMESH_FLAGS) const uint CORE_SUBMESH_FLAGS = 0;
layout(constant_id = CORE_DM_CONSTANT_ID_MATERIAL_FLAGS) const uint CORE_MATERIAL_FLAGS = 0;
#else

#endif

#endif // SHADERS_COMMON_3D_DEFAULT_MATERIAL_VERT_LAYOUT_COMMON_H
