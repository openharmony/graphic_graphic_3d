/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_BASE_MATH_SPLINE_H
#define API_BASE_MATH_SPLINE_H

#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
namespace Math {
/** @ingroup group_math_spline */
/** For hermite interpolation */
template<typename T>
inline constexpr T Hermite(T const& v1, T const& t1, T const& v2, T const& t2, float s)
{
    const float s2 = s * s;
    const float s3 = s2 * s;
    const float s23 = 2.f * s3;
    const float s32 = 3.f * s2;

    const float f1 = s23 - s32 + 1.f;
    const float f2 = -s23 + s32;
    const float f3 = s3 - 2.f * s2 + s;
    const float f4 = s3 - s2;

    return v1 * f1 + v2 * f2 + t1 * f3 + t2 * f4;
}
} // namespace Math
BASE_END_NAMESPACE()

#endif // API_BASE_MATH_SPLINE_H
