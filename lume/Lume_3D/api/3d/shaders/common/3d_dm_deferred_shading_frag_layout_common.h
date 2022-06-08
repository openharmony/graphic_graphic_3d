/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef SHADERS__COMMON__3D_DEFAULT_MATERIAL_DEFERRED_SHADING_FRAGMENT_LAYOUT_COMMON_H
#define SHADERS__COMMON__3D_DEFAULT_MATERIAL_DEFERRED_SHADING_FRAGMENT_LAYOUT_COMMON_H

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
    uint uLightClusterData;
};
layout(set = 0, binding = 7) uniform CORE_RELAXEDP sampler2D uSampColorPrePass;
layout(set = 0, binding = 8) uniform sampler2D uSampColorShadow;       // VSM or other
layout(set = 0, binding = 9) uniform sampler2DShadow uSampDepthShadow; // PCF
layout(set = 0, binding = 10) uniform samplerCube uSampRadiance;

// set 1 GBuffer
#if (ENABLE_INPUT_ATTACHMENTS == 1)
layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput uGBufferDepthBuffer;
layout(input_attachment_index = 0, set = 1, binding = 1) uniform subpassInput uGBufferVelocityNormal;
layout(input_attachment_index = 0, set = 1, binding = 2) uniform subpassInput uGBufferBaseColor;
layout(input_attachment_index = 0, set = 1, binding = 3) uniform subpassInput uGBufferMaterial;
#else
layout(set = 1, binding = 0) uniform texture2D uGBufferDepthBuffer;
layout(set = 1, binding = 1) uniform texture2D uGBufferVelocityNormal;
layout(set = 1, binding = 2) uniform texture2D uGBufferBaseColor;
layout(set = 1, binding = 3) uniform texture2D uGBufferMaterial;
#endif

layout(constant_id = 0) const uint CORE_MATERIAL_TYPE = 0;
layout(constant_id = 1) const uint CORE_MATERIAL_FLAGS = 0;
layout(constant_id = 2) const uint CORE_LIGHTING_FLAGS = 0;
layout(constant_id = 3) const uint CORE_POST_PROCESS_FLAGS = 0;

#else

#endif

#endif // SHADERS__COMMON__3D_DEFAULT_MATERIAL_DEFERRED_SHADING_FRAGMENT_LAYOUT_COMMON_H
