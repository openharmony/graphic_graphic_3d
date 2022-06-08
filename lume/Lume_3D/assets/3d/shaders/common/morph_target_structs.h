/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef SHADERS__COMMON__MORPHTARGET_STRUCTURES_COMMON_H
#define SHADERS__COMMON__MORPHTARGET_STRUCTURES_COMMON_H

#include "render/shaders/common/render_compatibility_common.h"

#define CORE_MAX_MORPH_TARGET_COUNT (256)
#define CORE_MORPH_USE_PACKED_NOR_TAN
#define CORE_MORPH_USE_16BIT_NOR_TAN

struct MorphTargetInfoStruct {
    uvec4 target; // contains 4 targets.
    vec4 weight;  // contains 4 weights.
};

struct MorphInputData {
    vec4 pos;
#if defined(CORE_MORPH_USE_PACKED_NOR_TAN)
    uvec4 nortan;
#else
    vec4 nor;
    vec4 tan;
#endif
};

struct MorphObjectPushConstantStruct {
    uint morphSet;         // Index to morphDataStruct.data.   start of id / weight information for submesh.
    uint vertexCount;      // Number of vertices in submesh
    uint morphTargetCount; // Total count of targets in targetBuffer.
    uint activeTargets;    // Active targets (count of data in morphDataStruct.data)
};
#endif
