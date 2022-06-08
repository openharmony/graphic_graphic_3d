/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef SHADERS__COMMON__3D_DEFAULT_MATERIAL_ENV_FRAGMENT_LAYOUT_COMMON_H
#define SHADERS__COMMON__3D_DEFAULT_MATERIAL_ENV_FRAGMENT_LAYOUT_COMMON_H

#include "3d/shaders/common/3d_dm_structures_common.h"
#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"

#ifdef VULKAN

// sets

// set 0 for general, camera, and lighting data
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
layout(set = 0, binding = 4, std140) uniform uPostProcessStructData
{
    GlobalPostProcessStruct uPostProcessData;
};

layout(set = 1, binding = 0) uniform sampler2D uImgSampler;
layout(set = 1, binding = 1) uniform samplerCube uImgCubeSampler;

layout(constant_id = 0) const uint CORE_DEFAULT_ENV_TYPE = 0;
layout(constant_id = 1) const uint CORE_POST_PROCESS_FLAGS = 0;

#else

#endif

#endif // SHADERS__COMMON__3D_DEFAULT_MATERIAL_ENV_FRAGMENT_LAYOUT_COMMON_H
