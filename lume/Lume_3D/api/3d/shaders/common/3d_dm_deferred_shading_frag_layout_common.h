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

#ifndef SHADERS__COMMON__3D_DEFAULT_MATERIAL_DEFERRED_SHADING_FRAGMENT_LAYOUT_COMMON_H
#define SHADERS__COMMON__3D_DEFAULT_MATERIAL_DEFERRED_SHADING_FRAGMENT_LAYOUT_COMMON_H

#include "3d/shaders/common/3d_dm_structures_common.h"
#include "3d/shaders/common/3d_post_process_layout_common.h"
#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"

#ifdef VULKAN

// sets

// NOTE: maps to 3d_post_process_layout_common.h
// set 0 is coming from post process layout

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

// not used
layout(constant_id = 1) const uint CORE_MATERIAL_FLAGS = 0;
// used
layout(constant_id = 2) const uint CORE_LIGHTING_FLAGS = 0;
layout(constant_id = 4) const uint CORE_CAMERA_FLAGS = 0;

#else

#endif

#endif // SHADERS__COMMON__3D_DEFAULT_MATERIAL_DEFERRED_SHADING_FRAGMENT_LAYOUT_COMMON_H
