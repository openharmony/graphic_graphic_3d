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

layout(constant_id = 0) const uint CORE_POST_PROCESS_FLAGS = 0;

#else

#endif

#endif // API_RENDER_SHADERS_COMMON_POST_PROCESS_STRUCTS_COMMON_H
