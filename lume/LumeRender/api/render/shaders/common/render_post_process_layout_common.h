/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_RENDER_SHADERS_COMMON_POST_PROCESS_LAYOUT_COMMON_H
#define API_RENDER_SHADERS_COMMON_POST_PROCESS_LAYOUT_COMMON_H

#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"

// defines
#ifdef VULKAN

layout(set = 0, binding = 0, std140) uniform uGlobalStructData
{
    GlobalPostProcessStruct uGlobalData;
};
layout(set = 0, binding = 1, std140) uniform uLocalStructData
{
    LocalPostProcessStruct uLocalData;
};

layout(push_constant, std430) uniform uPostProcessPushConstant
{
    LocalPostProcessPushConstantStruct uPc;
};

#else

#endif

#endif // API_RENDER_SHADERS_COMMON_POST_PROCESS_STRUCTS_COMMON_H
