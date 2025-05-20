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
#include "render/shaders/common/render_compatibility_common.h"

#ifdef VULKAN

#define MAX_NUM_RIPPLE 8
#define WATER_RIPPLE_TGS 8
// damping per unit of time
#define DAMPING_COEFF 5.0

struct DefaultWaterRippleDataStruct {
    uvec4 inArgs;
    uvec2 newRipplePositions[MAX_NUM_RIPPLE];

    // new ripple depth in meters
    float newRippleDepth;
    // radius of the new ripple in pixels
    int rippleRadius;
    float waterDepth;
    float rippleFormationSpeed;

    // .x oscillations, .y smoothness
    vec2 turbulenceFactors;

    // Fade inner and outer radiuses
    vec2 innerOutterRadius;
};

#else

constexpr uint32_t MAX_NUM_RIPPLE { 8u };
constexpr uint32_t WATER_RIPPLE_TGS { 8u };

struct DefaultWaterRippleDataStruct {
    uvec4 inArgs;
    uvec2 newRipplePositions[MAX_NUM_RIPPLE];

    // new ripple depth in meters
    float newRippleDepth { 0.019f };
    // radius of the new ripple in pixels
    int rippleRadius { 6 };
    float waterDepth { 0.02f };
    float rippleFormationSpeed { 20.0f };

    // .x oscillations, .y smoothness
    vec2 turbulenceFactors { 5.0f, 0.1f };

    // Fade inner and outer border/radiuses (in range [0, 1])
    vec2 innerOutterRadius { 0.97f, 1.0f };
};

#endif
