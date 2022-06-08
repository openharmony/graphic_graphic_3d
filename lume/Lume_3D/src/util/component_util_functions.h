/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
