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

#ifndef CORE_UTIL_COMPONENT_UTIL_FUNCTIONS_H
#define CORE_UTIL_COMPONENT_UTIL_FUNCTIONS_H

#include <base/math/mathf.h>
#include <core/namespace.h>

CORE3D_BEGIN_NAMESPACE()
namespace ComponentUtilFunctions {
constexpr float LIGHT_COMPONENT_USER_SET_MIN_LIGHT_RANGE = 0.0001f;
/**
 * Calculates (tries to evaluate) a valid light range if one is not given by the user
 * @param lightRange If smaller than LIGHT_COMPONENT_USER_SET_MIN_LIGHT_RANGE (i.e. 0) new range is calculated.
 * @param lightIntensity Used to calculate range if light range is not set.
 */
inline float CalculateSafeLightRange(const float lightRange, const float lightIntensity)
{
    float range = lightRange;
    if (range <= LIGHT_COMPONENT_USER_SET_MIN_LIGHT_RANGE) { // calculate range
        constexpr float minLightIntensityAdd = 20.0f;
        constexpr float powValue = 1.5f;
        range = BASE_NS::Math::sqrt(BASE_NS::Math::pow(lightIntensity + minLightIntensityAdd, powValue));
    }
    return range;
}
} // namespace ComponentUtilFunctions
CORE3D_END_NAMESPACE()

#endif // CORE_UTIL_COMPONENT_UTIL_FUNCTIONS_H
