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

#ifndef SHADERS__COMMON__MORPHTARGET_STRUCTURES_COMMON_H
#define SHADERS__COMMON__MORPHTARGET_STRUCTURES_COMMON_H

#include "render/shaders/common/render_compatibility_common.h"

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
