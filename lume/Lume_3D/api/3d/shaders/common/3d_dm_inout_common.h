/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

#if (CORE3D_DM_FW_VERT_OUTPUT == 1)
layout(location = 0) out vec3 outPos;       // worldspace
layout(location = 1) out vec3 outNormal;    // worldspace
layout(location = 2) out vec4 outTangentW;  // worldspace
layout(location = 3) out vec3 outVelocityI; // screenspace velocity + instance id
layout(location = 4) out vec4 outUv;        // .xy = 0, .zw = 1
layout(location = 5) out CORE_RELAXEDP vec4 outColor;
#endif

#if (CORE3D_DM_FW_FRAG_INPUT == 1)
layout(location = 0) in vec3 inPos;       // worldspace
layout(location = 1) in vec3 inNormal;    // worldspace
layout(location = 2) in vec4 inTangentW;  // worldspace
layout(location = 3) in vec3 inVelocityI; // screenspace velocity + instance id
layout(location = 4) in vec4 inUv;        // .xy = 0, .zw = 1
layout(location = 5) in CORE_RELAXEDP vec4 inColor;
#endif

#if (CORE3D_DM_DF_VERT_OUTPUT == 1)
layout(location = 0) out vec3 outPos;       // worldspace
layout(location = 1) out vec3 outNormal;    // worldspace
layout(location = 2) out vec4 outTangentW;  // worldspace
layout(location = 3) out vec3 outVelocityI; // screenspace velocity + instance id
layout(location = 4) out vec4 outUv;        // .xy = 0, .zw = 1
layout(location = 5) out CORE_RELAXEDP vec4 outColor;
#endif

#if (CORE3D_DM_DF_FRAG_INPUT == 1)
layout(location = 0) in vec3 inPos;       // worldspace
layout(location = 1) in vec3 inNormal;    // worldspace
layout(location = 2) in vec4 inTangentW;  // worldspace
layout(location = 3) in vec3 inVelocityI; // screenspace velocity + instance id
layout(location = 4) in vec4 inUv;        // .xy = 0, .zw = 1
layout(location = 5) in CORE_RELAXEDP vec4 inColor;
#endif

#if (CORE3D_DM_DEPTH_VERT_INPUT == 1)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in uvec4 inIndex;
layout(location = 2) in CORE_RELAXEDP vec4 inWeight;
#endif

#else

#endif

#endif // SHADERS__COMMON__3D_DEFAULT_MATERIAL_INOUT_COMMON_H
