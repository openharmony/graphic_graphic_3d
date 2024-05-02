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

#ifndef SHADERS__COMMON__3D_DEFAULT_MATERIAL_INOUT_COMMON_H
#define SHADERS__COMMON__3D_DEFAULT_MATERIAL_INOUT_COMMON_H

#include "render/shaders/common/render_compatibility_common.h"

#ifdef VULKAN

#if (CORE3D_DM_FW_VERT_INPUT == 1)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUv0;
layout(location = 3) in vec2 inUv1;
layout(location = 4) in vec4 inTangent;
layout(location = 5) in uvec4 inIndex;
layout(location = 6) in CORE_RELAXEDP vec4 inWeight;
layout(location = 7) in CORE_RELAXEDP vec4 inColor;
#endif

#if (CORE3D_DM_FW_VERT_OUTPUT == 1) || (CORE3D_DM_DF_VERT_OUTPUT == 1)
layout(location = 0) out vec3 outPos;      // worldspace
layout(location = 1) out vec3 outNormal;   // worldspace
layout(location = 2) out vec4 outTangentW; // worldspace
layout(location = 3) out vec4 outPrevPosI; // prev frame world pos + (instance id DEPRECATED use outIndices)
layout(location = 4) out vec4 outUv;       // .xy = 0, .zw = 1
layout(location = 5) out CORE_RELAXEDP vec4 outColor;
layout(location = 6) out flat uint outIndices; // packed camera and instance index
#endif

#if (CORE3D_DM_FW_FRAG_INPUT == 1) || (CORE3D_DM_DF_FRAG_INPUT == 1)
layout(location = 0) in vec3 inPos;      // worldspace
layout(location = 1) in vec3 inNormal;   // worldspace
layout(location = 2) in vec4 inTangentW; // worldspace
layout(location = 3) in vec4 inPrevPosI; // prev frame world pos + (instance id DEPRECATED use outIndices)
layout(location = 4) in vec4 inUv;       // .xy = 0, .zw = 1
layout(location = 5) in CORE_RELAXEDP vec4 inColor;
layout(location = 6) in flat uint inIndices; // packed camera and instance index
#endif

#if (CORE3D_DM_DEPTH_VERT_INPUT == 1)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in uvec4 inIndex;
layout(location = 2) in CORE_RELAXEDP vec4 inWeight;
#endif

#else

#endif

#endif // SHADERS__COMMON__3D_DEFAULT_MATERIAL_INOUT_COMMON_H
