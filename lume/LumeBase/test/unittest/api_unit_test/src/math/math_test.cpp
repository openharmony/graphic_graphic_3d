/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <array>
#include <iostream>
#include <type_traits>

#include "test_framework.h"

#ifndef NDEBUG
#define NDEBUG // matrix_util assert
#endif         // !NDEBUG

#include <base/math/float_packer.h>
#include <base/math/mathf.h>
#include <base/math/matrix.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector.h>
#include <base/math/vector_util.h>

#define M_PI 3.14159265358979323846

using namespace BASE_NS;

// Mathf functions

/**
 * @tc.name: MinFloat
 * @tc.desc: Tests for Min Float. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, MinFloat, testing::ext::TestSize.Level1)
{
    constexpr float min = Math::min(1.0f, 2.0f);

    ASSERT_FLOAT_EQ(min, 1.0f);
}

/**
 * @tc.name: MinInt
 * @tc.desc: Tests for Min Int. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, MinInt, testing::ext::TestSize.Level1)
{
    constexpr int min = Math::min(1, 2);

    ASSERT_EQ(min, 1);
}

/**
 * @tc.name: MaxFloat
 * @tc.desc: Tests for Max Float. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, MaxFloat, testing::ext::TestSize.Level1)
{
    constexpr float max = Math::max(2.0f, 1.0f);

    ASSERT_FLOAT_EQ(max, 2.0f);
}

/**
 * @tc.name: MaxInt
 * @tc.desc: Tests for Max Int. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, MaxInt, testing::ext::TestSize.Level1)
{
    constexpr int max = Math::max(2, 1);

    ASSERT_EQ(max, 2);
}

/**
 * @tc.name: Clamp
 * @tc.desc: Tests for Clamp. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Clamp, testing::ext::TestSize.Level1)
{
    float clampedValue = Math::clamp(0.5f, 1.0f, 2.0f);

    ASSERT_FLOAT_EQ(clampedValue, 1.0f);

    clampedValue = Math::clamp(2.5f, 1.0f, 2.0f);

    ASSERT_FLOAT_EQ(clampedValue, 2.0f);
}

/**
 * @tc.name: Clamp01
 * @tc.desc: Tests for Clamp01. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Clamp01, testing::ext::TestSize.Level1)
{
    {
        constexpr float clamped01 = Math::clamp01(1.2f);
        ASSERT_FLOAT_EQ(clamped01, 1.0f);
    }

    {
        constexpr float clamped01 = Math::clamp01(-0.2f);
        ASSERT_FLOAT_EQ(clamped01, 0.0f);
    }

    {
        constexpr float clamped01 = Math::clamp01(0.2f);
        ASSERT_FLOAT_EQ(clamped01, 0.2f);
    }
}

/**
 * @tc.name: AbsFloat
 * @tc.desc: Tests for Abs Float. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, AbsFloat, testing::ext::TestSize.Level1)
{
    constexpr float nonAbsValue = Math::abs(-1.0f);

    ASSERT_FLOAT_EQ(nonAbsValue, 1.0f);
}

/**
 * @tc.name: AbsInt
 * @tc.desc: Tests for Abs Int. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, AbsInt, testing::ext::TestSize.Level1)
{
    constexpr int nonAbsValue = Math::abs(-1);

    ASSERT_EQ(nonAbsValue, 1);
}

/**
 * @tc.name: FloorToInt
 * @tc.desc: Tests for Floor To Int. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, FloorToInt, testing::ext::TestSize.Level1)
{
    const int floored = Math::floorToInt(1.76f);

    ASSERT_EQ(floored, 1);
}

/**
 * @tc.name: CeilToInt
 * @tc.desc: Tests for Ceil To Int. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, CeilToInt, testing::ext::TestSize.Level1)
{
    const int ceiled = Math::ceilToInt(1.24f);

    ASSERT_EQ(ceiled, 2);
}

/**
 * @tc.name: Lerp
 * @tc.desc: Tests for Lerp. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Lerp, testing::ext::TestSize.Level1)
{
    constexpr float lerped0value = Math::lerp(1.0f, 2.0f, 0.0f);
    ASSERT_FLOAT_EQ(lerped0value, 1.0f);

    constexpr float lerped05value = Math::lerp(1.0f, 2.0f, 0.5f);
    ASSERT_FLOAT_EQ(lerped05value, 1.5f);

    constexpr float lerped1value = Math::lerp(1.0f, 2.0f, 1.0f);
    ASSERT_FLOAT_EQ(lerped1value, 2.0f);
}

/**
 * @tc.name: Round
 * @tc.desc: Tests for Round. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Round, testing::ext::TestSize.Level1)
{
    const float nearestInteger = Math::round(1.87f);

    ASSERT_EQ(nearestInteger, 2.0f);
}

/**
 * @tc.name: Sqrt
 * @tc.desc: Tests for Sqrt. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Sqrt, testing::ext::TestSize.Level1)
{
    const float squared = Math::sqrt(4.0f);

    ASSERT_EQ(squared, 2.0f);
}

/**
 * @tc.name: Sin
 * @tc.desc: Tests for Sin. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Sin, testing::ext::TestSize.Level1)
{
    const float PIdividedBy2 = Math::sin(Math::PI / 2);

    ASSERT_FLOAT_EQ(PIdividedBy2, 1.0f);
}

/**
 * @tc.name: Cos
 * @tc.desc: Tests for Cos. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Cos, testing::ext::TestSize.Level1)
{
    const float PIdividedBy3 = Math::cos(Math::PI / 3);

    ASSERT_FLOAT_EQ(PIdividedBy3, 0.5f);
}

/**
 * @tc.name: Tan
 * @tc.desc: Tests for Tan. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Tan, testing::ext::TestSize.Level1)
{
    const float PIdividedBy4 = Math::tan(Math::PI / 4.0f);

    ASSERT_FLOAT_EQ(PIdividedBy4, 1.0f);
}

/**
 * @tc.name: Pow
 * @tc.desc: Tests for Pow. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Pow, testing::ext::TestSize.Level1)
{
    const float raisedToPow = Math::pow(2.0f, 2.0f);

    ASSERT_FLOAT_EQ(raisedToPow, 4.0f);
}

/**
 * @tc.name: Atan
 * @tc.desc: Tests for Atan. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Atan, testing::ext::TestSize.Level1)
{
    const float PI = 4.0f * Math::atan(1.0f);

    ASSERT_FLOAT_EQ(PI, Math::PI);
}

/**
 * @tc.name: Atan2
 * @tc.desc: Tests for Atan2. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Atan2, testing::ext::TestSize.Level1)
{
    {
        const float polar2 = Math::atan2(1.0f, 1.0f);

        ASSERT_FLOAT_EQ(polar2, 0.785398f);
    }

    {
        const float polar2 = Math::atan2(1.0f, -1.0f);

        ASSERT_FLOAT_EQ(polar2, 2.356194f);
    }

    {
        const float polar2 = Math::atan2(-1.0f, -1.0f);

        ASSERT_FLOAT_EQ(polar2, -2.356194f);
    }

    {
        float const polar2 = Math::atan2(-1.0f, 1.0f);

        ASSERT_FLOAT_EQ(polar2, -0.785398f);
    }
}

/**
 * @tc.name: Asin
 * @tc.desc: Tests for Asin. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Asin, testing::ext::TestSize.Level1)
{
    const float PI = 2 * Math::asin(1.0f);

    ASSERT_FLOAT_EQ(PI, Math::PI);
}

/**
 * @tc.name: Acos
 * @tc.desc: Tests for Acos. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Acos, testing::ext::TestSize.Level1)
{
    const float PI = Math::acos(-1.0f);

    ASSERT_FLOAT_EQ(PI, Math::PI);
}

/**
 * @tc.name: Epsilon
 * @tc.desc: Tests for Epsilon. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Epsilon, testing::ext::TestSize.Level1)
{
    ASSERT_FLOAT_EQ(Math::EPSILON, 1.1920929e-07f); // 32bit epsilon
}

/**
 * @tc.name: Infinity
 * @tc.desc: Tests for Infinity. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Infinity, testing::ext::TestSize.Level1)
{
    ASSERT_EQ(Math::infinity, (float(1e+300) * 1e+300));
}

/**
 * @tc.name: PI
 * @tc.desc: Tests for Pi. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, PI, testing::ext::TestSize.Level1)
{
    const float PI = 3.141592653f;

    ASSERT_FLOAT_EQ(Math::PI, PI);
}

/**
 * @tc.name: Deg2Rad
 * @tc.desc: Tests for Deg2Rad. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Deg2Rad, testing::ext::TestSize.Level1)
{
    ASSERT_FLOAT_EQ(Math::DEG2RAD, 0.017453292f);
}

/**
 * @tc.name: Rad2Deg
 * @tc.desc: Tests for Rad2Deg. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Rad2Deg, testing::ext::TestSize.Level1)
{
    ASSERT_FLOAT_EQ(Math::RAD2DEG, 57.295781219f);
}

/**
 * @tc.name: Sign
 * @tc.desc: Tests for Sign. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_Mathf, Sign, testing::ext::TestSize.Level1)
{
    constexpr float positiveValue = Math::sign(100.0f);
    constexpr float negativeValue = Math::sign(-100.0f);

    ASSERT_FLOAT_EQ(1.0f, positiveValue);
    ASSERT_FLOAT_EQ(-1.0f, negativeValue);
}

// Vector2
template<typename T>
void TestCopyableMovable()
{
    static_assert(std::is_copy_assignable_v<T>);
    static_assert(std::is_trivially_copy_assignable_v<T>);
    static_assert(std::is_nothrow_copy_assignable_v<T>);
    static_assert(std::is_copy_constructible_v<T>);
    static_assert(std::is_trivially_copy_constructible_v<T>);
    static_assert(std::is_nothrow_copy_constructible_v<T>);

    static_assert(std::is_move_assignable_v<T>);
    static_assert(std::is_trivially_move_assignable_v<T>);
    static_assert(std::is_nothrow_move_assignable_v<T>);
    static_assert(std::is_move_constructible_v<T>);
    static_assert(std::is_trivially_move_constructible_v<T>);
    static_assert(std::is_nothrow_move_constructible_v<T>);
}

/**
 * @tc.name: Default
 * @tc.desc: Tests for Default. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2constructors, Default, testing::ext::TestSize.Level1)
{
    TestCopyableMovable<Math::Vec2>();
    TestCopyableMovable<Math::Vec3>();
    TestCopyableMovable<Math::Vec4>();
    TestCopyableMovable<Math::Quat>();
    TestCopyableMovable<Math::Mat3X3>();
    TestCopyableMovable<Math::Mat4X4>();
    TestCopyableMovable<Math::UVec2>();
    TestCopyableMovable<Math::UVec3>();
    TestCopyableMovable<Math::UVec4>();

    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] Math::Vec2 vector = Math::Vec2());
}

/**
 * @tc.name: Initializer
 * @tc.desc: Tests for Initializer. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2constructors, Initializer, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector1 = Math::Vec2(1.000001f, 2.000002f);

    ASSERT_FLOAT_EQ(vector1.x, 1.000001f);
    ASSERT_FLOAT_EQ(vector1.y, 2.000002f);

    constexpr Math::Vec2 vector2 = Math::Vec2(3.000003f);

    ASSERT_FLOAT_EQ(vector2.x, 3.000003f);
    ASSERT_FLOAT_EQ(vector2.y, 3.000003f);
}

/**
 * @tc.name: Copy
 * @tc.desc: Tests for Copy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2constructors, Copy, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector = Math::Vec2(1.000001f, 2.000002f);
    constexpr Math::Vec2 copiedVector = Math::Vec2(vector);

    ASSERT_FLOAT_EQ(copiedVector.x, vector.x);
    ASSERT_FLOAT_EQ(copiedVector.y, vector.y);
}

/**
 * @tc.name: Vector2operatorAddition
 * @tc.desc: Tests for Vector2Operator Addition. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, Vector2operatorAddition, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(4.0f, 6.0f);

    constexpr Math::Vec2 vector3rd = vector1st + vector2nd;

    ASSERT_FLOAT_EQ(vector3rd.x, 6.0f);
    ASSERT_FLOAT_EQ(vector3rd.y, 9.0f);
}
/**
 * @tc.name: Vector2operatorAdditionAssigment
 * @tc.desc: Tests for Vector2Operator Addition Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, Vector2operatorAdditionAssigment, testing::ext::TestSize.Level1)
{
    Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(4.0f, 6.0f);

    vector1st += vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, 6.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 9.0f);
}

/**
 * @tc.name: Vector2operatorSubtraction
 * @tc.desc: Tests for Vector2Operator Subtraction. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, Vector2operatorSubtraction, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(4.0f, 6.0f);

    constexpr Math::Vec2 vector3rd = vector1st - vector2nd;

    ASSERT_FLOAT_EQ(vector3rd.x, -2.0f);
    ASSERT_FLOAT_EQ(vector3rd.y, -3.0f);
}
/**
 * @tc.name: Vector2operatorSubtractionAssigment
 * @tc.desc: Tests for Vector2Operator Subtraction Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, Vector2operatorSubtractionAssigment, testing::ext::TestSize.Level1)
{
    Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(4.0f, 6.0f);

    vector1st -= vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, -2.0f);
    ASSERT_FLOAT_EQ(vector1st.y, -3.0f);
}

/**
 * @tc.name: Vector2operatorMultiplication
 * @tc.desc: Tests for Vector2Operator Multiplication. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, Vector2operatorMultiplication, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(4.0f, 6.0f);

    constexpr Math::Vec2 vector3rd = vector1st * vector2nd;

    ASSERT_FLOAT_EQ(vector3rd.x, 8.0f);
    ASSERT_FLOAT_EQ(vector3rd.y, 18.0f);
}
/**
 * @tc.name: Vector2operatorMultiplicationAssigment
 * @tc.desc: Tests for Vector2Operator Multiplication Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, Vector2operatorMultiplicationAssigment, testing::ext::TestSize.Level1)
{
    Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(4.0f, 6.0f);

    vector1st *= vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, 8.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 18.0f);
}

/**
 * @tc.name: Vector2operatorDivision
 * @tc.desc: Tests for Vector2Operator Division. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, Vector2operatorDivision, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(4.0f, 6.0f);

    constexpr Math::Vec2 vector3rd = vector1st / vector2nd;

    ASSERT_FLOAT_EQ(vector3rd.x, 0.5f);
    ASSERT_FLOAT_EQ(vector3rd.y, 0.5f);
}
/**
 * @tc.name: Vector2operatorDivisionAssigment
 * @tc.desc: Tests for Vector2Operator Division Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, Vector2operatorDivisionAssigment, testing::ext::TestSize.Level1)
{
    Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(4.0f, 6.0f);

    vector1st /= vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, 0.5f);
    ASSERT_FLOAT_EQ(vector1st.y, 0.5f);
}

/**
 * @tc.name: Assigment
 * @tc.desc: Tests for Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, Assigment, testing::ext::TestSize.Level1)
{
    Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(4.0f, 6.0f);

    vector1st = vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, 4.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 6.0f);
}

/**
 * @tc.name: Equality
 * @tc.desc: Tests for Equality. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, Equality, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(2.0f, 3.0f);

    ASSERT_TRUE(vector1st == vector2nd);
}

/**
 * @tc.name: NonEquality
 * @tc.desc: Tests for Non Equality. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, NonEquality, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(3.0f, 4.0f);

    ASSERT_TRUE(vector1st != vector2nd);
}
/**
 * @tc.name: AddWithScalar
 * @tc.desc: Tests for Math::Vec2 Addition With Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, AddWithScalar, testing::ext::TestSize.Level1)
{
    constexpr float addend = 5.0f;
    constexpr Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);

    constexpr Math::Vec2 vector2nd = vector1st + addend;

    ASSERT_FLOAT_EQ(vector2nd.x, 7.0f);
    ASSERT_FLOAT_EQ(vector2nd.y, 8.0f);
}

/**
 * @tc.name: AddWithScalarAssignment
 * @tc.desc: Tests for Math::Vec2 Addition With Scalar Assignment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, AddWithScalarAssignment, testing::ext::TestSize.Level1)
{
    constexpr float addend = 5.0f;
    Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);

    vector1st += addend;

    ASSERT_FLOAT_EQ(vector1st.x, 7.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 8.0f);
}

/**
 * @tc.name: SubtractWithScalar
 * @tc.desc: Tests for Math::Vec2 Subtraction With Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, SubtractWithScalar, testing::ext::TestSize.Level1)
{
    constexpr float subtrahend = 1.0f;
    constexpr Math::Vec2 vector1st = Math::Vec2(5.0f, 6.0f);

    constexpr Math::Vec2 vector2nd = vector1st - subtrahend;

    ASSERT_FLOAT_EQ(vector2nd.x, 4.0f);
    ASSERT_FLOAT_EQ(vector2nd.y, 5.0f);
}

/**
 * @tc.name: SubtractWithScalarAssignment
 * @tc.desc: Tests for Math::Vec2 Subtraction With Scalar Assignment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, SubtractWithScalarAssignment, testing::ext::TestSize.Level1)
{
    constexpr float subtrahend = 1.0f;
    Math::Vec2 vector1st = Math::Vec2(5.0f, 6.0f);

    vector1st -= subtrahend;

    ASSERT_FLOAT_EQ(vector1st.x, 4.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 5.0f);
}

/**
 * @tc.name: MultiplyWithScalar
 * @tc.desc: Tests for Math::Vec2 Multiplication With Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, MultiplyWithScalar, testing::ext::TestSize.Level1)
{
    constexpr float multiplier = 5.0f;
    constexpr Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);

    // Float on RHS
    constexpr Math::Vec2 vector2nd = vector1st * multiplier;

    ASSERT_FLOAT_EQ(vector2nd.x, 10.0f);
    ASSERT_FLOAT_EQ(vector2nd.y, 15.0f);

    // Float on LHS
    constexpr Math::Vec2 vector3rd = multiplier * vector1st;

    ASSERT_FLOAT_EQ(vector3rd.x, 10.0f);
    ASSERT_FLOAT_EQ(vector3rd.y, 15.0f);
}

/**
 * @tc.name: MultiplyWithScalarAssignment
 * @tc.desc: Tests for Math::Vec2 Multiplication With Scalar Assignment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, MultiplyWithScalarAssignment, testing::ext::TestSize.Level1)
{
    constexpr float multiplier = 5.0f;
    Math::Vec2 vector1st = Math::Vec2(2.0f, 3.0f);

    vector1st *= multiplier;

    ASSERT_FLOAT_EQ(vector1st.x, 10.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 15.0f);
}

/**
 * @tc.name: DivideWithScalar
 * @tc.desc: Tests for Math::Vec2 Division With Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, DivideWithScalar, testing::ext::TestSize.Level1)
{
    constexpr float divider = 5.0f;
    constexpr Math::Vec2 vector1st = Math::Vec2(10.0f, 15.0f);

    constexpr Math::Vec2 vector2nd = vector1st / divider;

    ASSERT_FLOAT_EQ(vector2nd.x, 2.0f);
    ASSERT_FLOAT_EQ(vector2nd.y, 3.0f);
}

/**
 * @tc.name: DivideWithScalarAssignment
 * @tc.desc: Tests for Math::Vec2 Division With Scalar Assignment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector2operators, DivideWithScalarAssignment, testing::ext::TestSize.Level1)
{
    constexpr float divider = 5.0f;
    Math::Vec2 vector1st = Math::Vec2(10.0f, 15.0f);

    vector1st /= divider;

    ASSERT_FLOAT_EQ(vector1st.x, 2.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 3.0f);
}

// Vector3

/**
 * @tc.name: Default
 * @tc.desc: Tests for Default. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3constructors, Default, testing::ext::TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] Math::Vec3 vector = Math::Vec3());
}

/**
 * @tc.name: Initializer
 * @tc.desc: Tests for Initializer. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3constructors, Initializer, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector1st = Math::Vec3(1.000001f, 2.000002f, 3.000003f);

    ASSERT_FLOAT_EQ(vector1st.x, 1.000001f);
    ASSERT_FLOAT_EQ(vector1st.y, 2.000002f);
    ASSERT_FLOAT_EQ(vector1st.z, 3.000003f);

    constexpr Math::Vec3 vector2nd = Math::Vec3(Math::Vec2(1.000001f, 2.000002f), 3.000003f);

    ASSERT_FLOAT_EQ(vector2nd.x, 1.000001f);
    ASSERT_FLOAT_EQ(vector2nd.y, 2.000002f);
    ASSERT_FLOAT_EQ(vector2nd.z, 3.000003f);

    constexpr Math::Vec3 vector3rd = Math::Vec3(3.000003f);

    ASSERT_FLOAT_EQ(vector3rd.x, 3.000003f);
    ASSERT_FLOAT_EQ(vector3rd.y, 3.000003f);
    ASSERT_FLOAT_EQ(vector3rd.z, 3.000003f);
}

/**
 * @tc.name: Copy
 * @tc.desc: Tests for Copy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3constructors, Copy, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector = Math::Vec3(1.000001f, 2.000002f, 3.000003f);
    constexpr Math::Vec3 copiedVector = Math::Vec3(vector);

    ASSERT_FLOAT_EQ(copiedVector.x, vector.x);
    ASSERT_FLOAT_EQ(copiedVector.y, vector.y);
    ASSERT_FLOAT_EQ(copiedVector.z, vector.z);
}

/**
 * @tc.name: Vector3operatorAddition
 * @tc.desc: Tests for Vector3Operator Addition. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, Vector3operatorAddition, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector1st = Math::Vec3(2.0f, 3.0f, 4.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(5.0f, 6.0f, 7.0f);

    constexpr Math::Vec3 vector3rd = vector1st + vector2nd;

    ASSERT_FLOAT_EQ(vector3rd.x, 7.0f);
    ASSERT_FLOAT_EQ(vector3rd.y, 9.0f);
    ASSERT_FLOAT_EQ(vector3rd.z, 11.0f);
}

/**
 * @tc.name: Vector3operatorAdditionAssigment
 * @tc.desc: Tests for Vector3Operator Addition Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, Vector3operatorAdditionAssigment, testing::ext::TestSize.Level1)
{
    Math::Vec3 vector1st = Math::Vec3(1.0f, 2.0f, 3.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(4.0f, 5.0f, 6.0f);

    vector1st += vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, 5.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 7.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 9.0f);
}

/**
 * @tc.name: Vector3operatorSubtraction
 * @tc.desc: Tests for Vector3Operator Subtraction. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, Vector3operatorSubtraction, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector1st = Math::Vec3(2.0f, 3.0f, 4.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(5.0f, 6.0f, 7.0f);

    constexpr Math::Vec3 vector3rd = vector1st - vector2nd;

    ASSERT_FLOAT_EQ(vector3rd.x, -3.0f);
    ASSERT_FLOAT_EQ(vector3rd.y, -3.0f);
    ASSERT_FLOAT_EQ(vector3rd.z, -3.0f);
}
/**
 * @tc.name: Vector3operatorSubtractionAssigment
 * @tc.desc: Tests for Vector3Operator Subtraction Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, Vector3operatorSubtractionAssigment, testing::ext::TestSize.Level1)
{
    Math::Vec3 vector1st = Math::Vec3(1.0f, 2.0f, 3.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(4.0f, 5.0f, 6.0f);

    vector1st -= vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, -3.0f);
    ASSERT_FLOAT_EQ(vector1st.y, -3.0f);
    ASSERT_FLOAT_EQ(vector1st.z, -3.0f);
}

/**
 * @tc.name: Vector3operatorMultiplication
 * @tc.desc: Tests for Vector3Operator Multiplication. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, Vector3operatorMultiplication, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector1st = Math::Vec3(2.0f, 3.0f, 4.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(5.0f, 6.0f, 7.0f);

    constexpr Math::Vec3 vector3rd = vector1st * vector2nd;

    ASSERT_FLOAT_EQ(vector3rd.x, 10.0f);
    ASSERT_FLOAT_EQ(vector3rd.y, 18.0f);
    ASSERT_FLOAT_EQ(vector3rd.z, 28.0f);
}
/**
 * @tc.name: Vector3operatorMultiplicationAssigment
 * @tc.desc: Tests for Vector3Operator Multiplication Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, Vector3operatorMultiplicationAssigment, testing::ext::TestSize.Level1)
{
    Math::Vec3 vector1st = Math::Vec3(2.0f, 3.0f, 4.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(5.0f, 6.0f, 7.0f);

    vector1st *= vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, 10.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 18.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 28.0f);
}

/**
 * @tc.name: Vector3operatorDivision
 * @tc.desc: Tests for Vector3Operator Division. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, Vector3operatorDivision, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector1st = Math::Vec3(5.0f, 3.0f, 14.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(5.0f, 6.0f, 7.0f);

    constexpr Math::Vec3 vector3rd = vector1st / vector2nd;

    ASSERT_FLOAT_EQ(vector3rd.x, 1.0f);
    ASSERT_FLOAT_EQ(vector3rd.y, 0.5f);
    ASSERT_FLOAT_EQ(vector3rd.z, 2.0f);
}
/**
 * @tc.name: Vector3operatorDivisionAssigment
 * @tc.desc: Tests for Vector3Operator Division Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, Vector3operatorDivisionAssigment, testing::ext::TestSize.Level1)
{
    Math::Vec3 vector1st = Math::Vec3(5.0f, 3.0f, 14.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(5.0f, 6.0f, 7.0f);

    vector1st /= vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, 1.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 0.5f);
    ASSERT_FLOAT_EQ(vector1st.z, 2.0f);
}

/**
 * @tc.name: Assigment
 * @tc.desc: Tests for Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, Assigment, testing::ext::TestSize.Level1)
{
    Math::Vec3 vector1st = Math::Vec3(1.0f, 2.0f, 3.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(4.0f, 5.0f, 6.0f);

    vector1st = vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, 4.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 5.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 6.0f);
}

/**
 * @tc.name: Equality
 * @tc.desc: Tests for Equality. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, Equality, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector1st = Math::Vec3(1.0f, 2.0f, 3.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(1.0f, 2.0f, 3.0f);

    ASSERT_TRUE(vector1st == vector2nd);
}

/**
 * @tc.name: NonEquality
 * @tc.desc: Tests for Non Equality. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, NonEquality, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector1st = Math::Vec3(1.0f, 2.0f, 3.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(1.0f, 3.0f, 4.0f);

    ASSERT_TRUE(vector1st != vector2nd);
}

/**
 * @tc.name: AddWithScalar
 * @tc.desc: Tests for Addition With Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, AddWithScalar, testing::ext::TestSize.Level1)
{
    constexpr float addend = 5.0f;
    constexpr Math::Vec3 vector1st = Math::Vec3(2.0f, 3.0f, 4.0f);

    constexpr Math::Vec3 vector2nd = vector1st + addend;

    ASSERT_FLOAT_EQ(vector2nd.x, 7.0f);
    ASSERT_FLOAT_EQ(vector2nd.y, 8.0f);
    ASSERT_FLOAT_EQ(vector2nd.z, 9.0f);
}

/**
 * @tc.name: AddWithScalarAssignment
 * @tc.desc: Tests for Addition With Scalar Assignment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, AddWithScalarAssignment, testing::ext::TestSize.Level1)
{
    constexpr float addend = 5.0f;
    Math::Vec3 vector1st = Math::Vec3(2.0f, 3.0f, 4.0f);

    vector1st += addend;

    ASSERT_FLOAT_EQ(vector1st.x, 7.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 8.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 9.0f);
}

/**
 * @tc.name: SubtractWithScalar
 * @tc.desc: Tests for Subtraction With Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, SubtractWithScalar, testing::ext::TestSize.Level1)
{
    constexpr float subtrahend = 1.0f;
    constexpr Math::Vec3 vector1st = Math::Vec3(5.0f, 6.0f, 7.0f);

    constexpr Math::Vec3 vector2nd = vector1st - subtrahend;

    ASSERT_FLOAT_EQ(vector2nd.x, 4.0f);
    ASSERT_FLOAT_EQ(vector2nd.y, 5.0f);
    ASSERT_FLOAT_EQ(vector2nd.z, 6.0f);
}

/**
 * @tc.name: SubtractWithScalarAssignment
 * @tc.desc: Tests for Subtraction With Scalar Assignment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, SubtractWithScalarAssignment, testing::ext::TestSize.Level1)
{
    constexpr float subtrahend = 1.0f;
    Math::Vec3 vector1st = Math::Vec3(5.0f, 6.0f, 7.0f);

    vector1st -= subtrahend;

    ASSERT_FLOAT_EQ(vector1st.x, 4.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 5.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 6.0f);
}

/**
 * @tc.name: MultiplyWithScalar
 * @tc.desc: Tests for Multiply With Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, MultiplyWithScalar, testing::ext::TestSize.Level1)
{
    constexpr float multiplier = 5.0f;
    constexpr Math::Vec3 vector1st = Math::Vec3(2.0f, 3.0f, 4.0f);

    // Float on RHS
    constexpr Math::Vec3 vector2nd = vector1st * multiplier;

    ASSERT_FLOAT_EQ(vector2nd.x, 10.0f);
    ASSERT_FLOAT_EQ(vector2nd.y, 15.0f);
    ASSERT_FLOAT_EQ(vector2nd.z, 20.0f);

    // Float on LHS
    constexpr Math::Vec3 vector3rd = multiplier * vector1st;

    ASSERT_FLOAT_EQ(vector3rd.x, 10.0f);
    ASSERT_FLOAT_EQ(vector3rd.y, 15.0f);
    ASSERT_FLOAT_EQ(vector3rd.z, 20.0f);
}
/**
 * @tc.name: MultiplyWithScalarAssigment
 * @tc.desc: Tests for Multiply With Scalar Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, MultiplyWithScalarAssigment, testing::ext::TestSize.Level1)
{
    constexpr float multiplier = 5.0f;
    Math::Vec3 vector1st = Math::Vec3(2.0f, 3.0f, 4.0f);

    vector1st *= multiplier;

    ASSERT_FLOAT_EQ(vector1st.x, 10.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 15.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 20.0f);
}

/**
 * @tc.name: DivideWithScalar
 * @tc.desc: Tests for Divide With Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, DivideWithScalar, testing::ext::TestSize.Level1)
{
    constexpr float divider = 5.0f;
    constexpr Math::Vec3 vector1st = Math::Vec3(5.0f, 10.0f, 15.0f);

    constexpr Math::Vec3 vector2nd = vector1st / divider;

    ASSERT_FLOAT_EQ(vector2nd.x, 1.0f);
    ASSERT_FLOAT_EQ(vector2nd.y, 2.0f);
    ASSERT_FLOAT_EQ(vector2nd.z, 3.0f);
}
/**
 * @tc.name: DivideWithScalarAssigment
 * @tc.desc: Tests for Divide With Scalar Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector3operators, DivideWithScalarAssigment, testing::ext::TestSize.Level1)
{
    constexpr float divider = 5.0f;
    Math::Vec3 vector1st = Math::Vec3(5.0f, 10.0f, 15.0f);

    vector1st /= divider;

    ASSERT_FLOAT_EQ(vector1st.x, 1.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 2.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 3.0f);
}

// Vector4

/**
 * @tc.name: Default
 * @tc.desc: Tests for Default. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4constructors, Default, testing::ext::TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] Math::Vec4 vector = Math::Vec4());
}

/**
 * @tc.name: Initializer
 * @tc.desc: Tests for Initializer. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4constructors, Initializer, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 vector1st = Math::Vec4(1.000001f, 2.000002f, 3.000003f, 4.000004f);

    ASSERT_FLOAT_EQ(vector1st.x, 1.000001f);
    ASSERT_FLOAT_EQ(vector1st.y, 2.000002f);
    ASSERT_FLOAT_EQ(vector1st.z, 3.000003f);
    ASSERT_FLOAT_EQ(vector1st.w, 4.000004f);

    constexpr Math::Vec4 vector2nd = Math::Vec4(Math::Vec3(1.000001f, 2.000002f, 3.000003f), 4.000004f);

    ASSERT_FLOAT_EQ(vector2nd.x, 1.000001f);
    ASSERT_FLOAT_EQ(vector2nd.y, 2.000002f);
    ASSERT_FLOAT_EQ(vector2nd.z, 3.000003f);
    ASSERT_FLOAT_EQ(vector2nd.w, 4.000004f);

    constexpr Math::Vec4 vector3rd = Math::Vec4(Math::Vec2(1.000001f, 2.000002f), 3.000003f, 4.000004f);

    ASSERT_FLOAT_EQ(vector3rd.x, 1.000001f);
    ASSERT_FLOAT_EQ(vector3rd.y, 2.000002f);
    ASSERT_FLOAT_EQ(vector3rd.z, 3.000003f);
    ASSERT_FLOAT_EQ(vector3rd.w, 4.000004f);

    constexpr Math::Vec4 vector4th = Math::Vec4(5.000005f);

    ASSERT_FLOAT_EQ(vector4th.x, 5.000005f);
    ASSERT_FLOAT_EQ(vector4th.y, 5.000005f);
    ASSERT_FLOAT_EQ(vector4th.z, 5.000005f);
    ASSERT_FLOAT_EQ(vector4th.w, 5.000005f);
}

/**
 * @tc.name: Copy
 * @tc.desc: Tests for Copy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4constructors, Copy, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 vector = Math::Vec4(1.000001f, 2.000002f, 3.000003f, 4.000004f);
    constexpr Math::Vec4 copiedVector = Math::Vec4(vector);

    ASSERT_FLOAT_EQ(copiedVector.x, vector.x);
    ASSERT_FLOAT_EQ(copiedVector.y, vector.y);
    ASSERT_FLOAT_EQ(copiedVector.z, vector.z);
    ASSERT_FLOAT_EQ(copiedVector.w, vector.w);
}

/**
 * @tc.name: Vector4operatorAddition
 * @tc.desc: Tests for Vector4Operator Addition. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, Vector4operatorAddition, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 vector1st = Math::Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    constexpr Math::Vec4 vector2nd = Math::Vec4(5.0f, 6.0f, 7.0f, 8.0f);

    constexpr Math::Vec4 vector3rd = vector1st + vector2nd;

    ASSERT_FLOAT_EQ(vector3rd.x, 6.0f);
    ASSERT_FLOAT_EQ(vector3rd.y, 8.0f);
    ASSERT_FLOAT_EQ(vector3rd.z, 10.0f);
    ASSERT_FLOAT_EQ(vector3rd.w, 12.0f);
}
/**
 * @tc.name: Vector4operatorAdditionAssigment
 * @tc.desc: Tests for Vector4Operator Addition Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, Vector4operatorAdditionAssigment, testing::ext::TestSize.Level1)
{
    Math::Vec4 vector1st = Math::Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    constexpr Math::Vec4 vector2nd = Math::Vec4(4.0f, 5.0f, 6.0f, 7.0f);

    vector1st += vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, 5.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 7.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 9.0f);
    ASSERT_FLOAT_EQ(vector1st.w, 11.0f);
}

/**
 * @tc.name: Vector4operatorSubtraction
 * @tc.desc: Tests for Vector4Operator Subtraction. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, Vector4operatorSubtraction, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 vector1st = Math::Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    constexpr Math::Vec4 vector2nd = Math::Vec4(5.0f, 6.0f, 7.0f, 8.0f);

    constexpr Math::Vec4 vector3rd = vector1st - vector2nd;

    ASSERT_FLOAT_EQ(vector3rd.x, -4.0f);
    ASSERT_FLOAT_EQ(vector3rd.y, -4.0f);
    ASSERT_FLOAT_EQ(vector3rd.z, -4.0f);
    ASSERT_FLOAT_EQ(vector3rd.w, -4.0f);
}
/**
 * @tc.name: Vector4operatorSubtractionAssigment
 * @tc.desc: Tests for Vector4Operator Subtraction Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, Vector4operatorSubtractionAssigment, testing::ext::TestSize.Level1)
{
    Math::Vec4 vector1st = Math::Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    constexpr Math::Vec4 vector2nd = Math::Vec4(4.0f, 5.0f, 6.0f, 7.0f);

    vector1st -= vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, -3.0f);
    ASSERT_FLOAT_EQ(vector1st.y, -3.0f);
    ASSERT_FLOAT_EQ(vector1st.z, -3.0f);
    ASSERT_FLOAT_EQ(vector1st.w, -3.0f);
}

/**
 * @tc.name: Vector4operatorMultiplication
 * @tc.desc: Tests for Vector4Operator Multiplication. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, Vector4operatorMultiplication, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 vector1st = Math::Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    constexpr Math::Vec4 vector2nd = Math::Vec4(5.0f, 6.0f, 7.0f, 8.0f);

    constexpr Math::Vec4 vector3rd = vector1st * vector2nd;

    ASSERT_FLOAT_EQ(vector3rd.x, 5.0f);
    ASSERT_FLOAT_EQ(vector3rd.y, 12.0f);
    ASSERT_FLOAT_EQ(vector3rd.z, 21.0f);
    ASSERT_FLOAT_EQ(vector3rd.w, 32.0f);
}
/**
 * @tc.name: Vector4operatorMultiplicationAssigment
 * @tc.desc: Tests for Vector4Operator Multiplication Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, Vector4operatorMultiplicationAssigment, testing::ext::TestSize.Level1)
{
    Math::Vec4 vector1st = Math::Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    constexpr Math::Vec4 vector2nd = Math::Vec4(5.0f, 6.0f, 7.0f, 8.0f);

    vector1st *= vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, 5.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 12.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 21.0f);
    ASSERT_FLOAT_EQ(vector1st.w, 32.0f);
}

/**
 * @tc.name: Vector4operatorDivision
 * @tc.desc: Tests for Vector4Operator Division. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, Vector4operatorDivision, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 vector1st = Math::Vec4(10.0f, 12.0f, 14.0f, 4.0f);
    constexpr Math::Vec4 vector2nd = Math::Vec4(5.0f, 6.0f, 7.0f, 8.0f);

    constexpr Math::Vec4 vector3rd = vector1st / vector2nd;

    ASSERT_FLOAT_EQ(vector3rd.x, 2.0f);
    ASSERT_FLOAT_EQ(vector3rd.y, 2.0f);
    ASSERT_FLOAT_EQ(vector3rd.z, 2.0f);
    ASSERT_FLOAT_EQ(vector3rd.w, 0.5f);
}
/**
 * @tc.name: Vector4operatorDivisionAssigment
 * @tc.desc: Tests for Vector4Operator Division Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, Vector4operatorDivisionAssigment, testing::ext::TestSize.Level1)
{
    Math::Vec4 vector1st = Math::Vec4(10.0f, 12.0f, 14.0f, 4.0f);
    constexpr Math::Vec4 vector2nd = Math::Vec4(5.0f, 6.0f, 7.0f, 8.0f);

    vector1st /= vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, 2.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 2.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 2.0f);
    ASSERT_FLOAT_EQ(vector1st.w, 0.5f);
}

/**
 * @tc.name: Assigment
 * @tc.desc: Tests for Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, Assigment, testing::ext::TestSize.Level1)
{
    Math::Vec4 vector1st = Math::Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    constexpr Math::Vec4 vector2nd = Math::Vec4(5.0f, 6.0f, 7.0f, 8.0f);

    vector1st = vector2nd;

    ASSERT_FLOAT_EQ(vector1st.x, 5.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 6.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 7.0f);
    ASSERT_FLOAT_EQ(vector1st.w, 8.0f);
}

/**
 * @tc.name: Equality
 * @tc.desc: Tests for Equality. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, Equality, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 vector1st = Math::Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    constexpr Math::Vec4 vector2nd = Math::Vec4(1.0f, 2.0f, 3.0f, 4.0f);

    ASSERT_TRUE(vector1st == vector2nd);
}

/**
 * @tc.name: NonEquality
 * @tc.desc: Tests for Non Equality. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, NonEquality, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 vector1st = Math::Vec4(1.0f, 2.0f, 3.0f, 1.0f);
    constexpr Math::Vec4 vector2nd = Math::Vec4(1.0f, 3.0f, 4.0f, 2.0f);

    ASSERT_TRUE(vector1st != vector2nd);
}

/**
 * @tc.name: AddWithScalar
 * @tc.desc: Tests for Addition With Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, AddWithScalar, testing::ext::TestSize.Level1)
{
    constexpr float addend = 5.0f;
    constexpr Math::Vec4 vector1st = Math::Vec4(2.0f, 3.0f, 4.0f, 1.0f);

    constexpr Math::Vec4 vector2nd = vector1st + addend;

    ASSERT_FLOAT_EQ(vector2nd.x, 7.0f);
    ASSERT_FLOAT_EQ(vector2nd.y, 8.0f);
    ASSERT_FLOAT_EQ(vector2nd.z, 9.0f);
    ASSERT_FLOAT_EQ(vector2nd.w, 6.0f);
}

/**
 * @tc.name: AddWithScalarAssignment
 * @tc.desc: Tests for Addition With Scalar Assignment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, AddWithScalarAssignment, testing::ext::TestSize.Level1)
{
    constexpr float addend = 5.0f;
    Math::Vec4 vector1st = Math::Vec4(2.0f, 3.0f, 4.0f, 1.0f);

    vector1st += addend;

    ASSERT_FLOAT_EQ(vector1st.x, 7.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 8.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 9.0f);
    ASSERT_FLOAT_EQ(vector1st.w, 6.0f);
}

/**
 * @tc.name: SubtractWithScalar
 * @tc.desc: Tests for Subtraction With Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, SubtractWithScalar, testing::ext::TestSize.Level1)
{
    constexpr float subtrahend = 1.0f;
    constexpr Math::Vec4 vector1st = Math::Vec4(5.0f, 6.0f, 7.0f, 4.0f);

    constexpr Math::Vec4 vector2nd = vector1st - subtrahend;

    ASSERT_FLOAT_EQ(vector2nd.x, 4.0f);
    ASSERT_FLOAT_EQ(vector2nd.y, 5.0f);
    ASSERT_FLOAT_EQ(vector2nd.z, 6.0f);
    ASSERT_FLOAT_EQ(vector2nd.w, 3.0f);
}

/**
 * @tc.name: SubtractWithScalarAssignment
 * @tc.desc: Tests for Subtraction With Scalar Assignment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, SubtractWithScalarAssignment, testing::ext::TestSize.Level1)
{
    constexpr float subtrahend = 1.0f;
    Math::Vec4 vector1st = Math::Vec4(5.0f, 6.0f, 7.0f, 4.0f);

    vector1st -= subtrahend;

    ASSERT_FLOAT_EQ(vector1st.x, 4.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 5.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 6.0f);
    ASSERT_FLOAT_EQ(vector1st.w, 3.0f);
}

/**
 * @tc.name: MultiplyWithScalar
 * @tc.desc: Tests for Multiply With Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, MultiplyWithScalar, testing::ext::TestSize.Level1)
{
    constexpr float multiplier = 5.0f;
    constexpr Math::Vec4 vector1st = Math::Vec4(2.0f, 3.0f, 4.0f, 5.0f);

    constexpr Math::Vec4 vector2nd = vector1st * multiplier;

    ASSERT_FLOAT_EQ(vector2nd.x, 10.0f);
    ASSERT_FLOAT_EQ(vector2nd.y, 15.0f);
    ASSERT_FLOAT_EQ(vector2nd.z, 20.0f);
    ASSERT_FLOAT_EQ(vector2nd.w, 25.0f);
}

/**
 * @tc.name: MultiplyWithScalarAssigment
 * @tc.desc: Tests for Multiply With Scalar Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, MultiplyWithScalarAssigment, testing::ext::TestSize.Level1)
{
    constexpr float multiplier = 5.0f;
    Math::Vec4 vector1st = Math::Vec4(2.0f, 3.0f, 4.0f, 5.0f);

    vector1st *= multiplier;

    ASSERT_FLOAT_EQ(vector1st.x, 10.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 15.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 20.0f);
    ASSERT_FLOAT_EQ(vector1st.w, 25.0f);
}

/**
 * @tc.name: DivideWithScalar
 * @tc.desc: Tests for Divide With Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, DivideWithScalar, testing::ext::TestSize.Level1)
{
    constexpr float divider = 5.0f;
    constexpr Math::Vec4 vector1st = Math::Vec4(5.0f, 10.0f, 15.0f, 20.0f);

    constexpr Math::Vec4 vector2nd = vector1st / divider;

    ASSERT_FLOAT_EQ(vector2nd.x, 1.0f);
    ASSERT_FLOAT_EQ(vector2nd.y, 2.0f);
    ASSERT_FLOAT_EQ(vector2nd.z, 3.0f);
    ASSERT_FLOAT_EQ(vector2nd.w, 4.0f);
}

/**
 * @tc.name: DivideWithScalarAssignment
 * @tc.desc: Tests for Divide With Scalar Assignment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVector4operators, DivideWithScalarAssignment, testing::ext::TestSize.Level1)
{
    constexpr float divider = 5.0f;
    Math::Vec4 vector1st = Math::Vec4(5.0f, 10.0f, 15.0f, 20.0f);

    vector1st /= divider;

    ASSERT_FLOAT_EQ(vector1st.x, 1.0f);
    ASSERT_FLOAT_EQ(vector1st.y, 2.0f);
    ASSERT_FLOAT_EQ(vector1st.z, 3.0f);
    ASSERT_FLOAT_EQ(vector1st.w, 4.0f);
}

// VectorUtil

/**
 * @tc.name: Vector2dot
 * @tc.desc: Tests for Vector2Dot. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2dot, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector1st = Math::Vec2(1.0f, 2.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(2.0f, 3.0f);

    constexpr float dotProduct = Math::Dot(vector1st, vector2nd);

    ASSERT_FLOAT_EQ(dotProduct, 8.0f);
}

/**
 * @tc.name: Vector2lerp
 * @tc.desc: Tests for Vector2Lerp. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2lerp, testing::ext::TestSize.Level1)
{
    float value = 0.0f;
    constexpr Math::Vec2 pos1 = Math::Vec2(0.0f, 0.0f);
    constexpr Math::Vec2 pos2 = Math::Vec2(10.0f, 0.0f);

    Math::Vec2 pos3 = Math::Lerp(pos1, pos2, value);
    ASSERT_FLOAT_EQ(pos3.x, 0.0f);

    value = 0.5f;
    pos3 = Math::Lerp(pos1, pos2, value);
    ASSERT_FLOAT_EQ(pos3.x, 5.0f);

    value = 1.0f;
    pos3 = Math::Lerp(pos1, pos2, value);
    ASSERT_FLOAT_EQ(pos3.x, 10.0f);
}

/**
 * @tc.name: Vector2squareMagnitude
 * @tc.desc: Tests for Vector2Square Magnitude. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2squareMagnitude, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector = Math::Vec2(2.0f, 2.0f);
    const float sqrtMagnitude = Math::SqrMagnitude(vector);

    ASSERT_FLOAT_EQ(sqrtMagnitude, 8.0f);
}

/**
 * @tc.name: Vector2distance
 * @tc.desc: Tests for Vector2Distance. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2distance, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector1st = Math::Vec2(1.0f, 1.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(-1.0f, -1.0f);

    const float t = Math::distance(vector1st, vector2nd);

    ASSERT_NEAR(t, 2.f * Math::sqrt(2.f), 0.0001f);
}

/**
 * @tc.name: Vector2distance2
 * @tc.desc: Tests for Vector2Distance2. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2distance2, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector1st = Math::Vec2(1.0f, 1.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(-1.0f, -1.0f);

    const float t = Math::Distance2(vector1st, vector2nd);

    ASSERT_NEAR(t, Math::pow(2.f * Math::sqrt(2.f), 2), 0.0001f);
}

/**
 * @tc.name: Vector2reflect
 * @tc.desc: Tests for Vector2reflect. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2reflect, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 incident = Math::Vec2(-1.0f, -1.0f);
    constexpr Math::Vec2 normal = Math::Vec2(0.0f, 1.0f);

    const Math::Vec2 reflected = Math::Reflect(incident, normal);

    ASSERT_FLOAT_EQ(reflected.x, -1.0f);
    ASSERT_FLOAT_EQ(reflected.y, 1.0f);
}

/**
 * @tc.name: Vector2scale
 * @tc.desc: Tests for Vector2scale. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2scale, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector = Math::Vec2(1.0f, -1.0f);
    const float s = 10.0f;

    const Math::Vec2 scaled = Math::Scale(vector, s);

    ASSERT_FLOAT_EQ(Math::Magnitude(scaled), s);
}

/**
 * @tc.name: Vector2max
 * @tc.desc: Tests for Vector2Max. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2max, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector1st = Math::Vec2(1.0f, -2.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(0.0f, -1.0f);

    const Math::Vec2 t = Math::max(vector1st, vector2nd);

    ASSERT_FLOAT_EQ(t.x, 1.0f);
    ASSERT_FLOAT_EQ(t.y, -1.0f);
}

/**
 * @tc.name: Vector2min
 * @tc.desc: Tests for Vector2Min. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2min, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector1st = Math::Vec2(1.0f, -2.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(0.0f, -1.0f);

    const Math::Vec2 t = Math::min(vector1st, vector2nd);

    ASSERT_FLOAT_EQ(t.x, 0.0f);
    ASSERT_FLOAT_EQ(t.y, -2.0f);
}

/**
 * @tc.name: Vector2Normalize
 * @tc.desc: Tests for Vector2Normalize. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2Normalize, testing::ext::TestSize.Level1)
{
    {
        constexpr Math::Vec2 vector = Math::Vec2(-3.0f, 4.0f);
        Math::Vec2 t = Math::Normalize(vector);

        ASSERT_FLOAT_EQ(t.x, -0.6f);
        ASSERT_FLOAT_EQ(t.y, 0.8f);
    }

    {
        constexpr Math::Vec2 vector = Math::Vec2(0.0f, 0.0f);
        Math::Vec2 t = Math::Normalize(vector);

        ASSERT_FLOAT_EQ(t.x, 0.0f);
        ASSERT_FLOAT_EQ(t.y, 0.0f);
    }
}

/**
 * @tc.name: Vector2PerpendicularCW
 * @tc.desc: Tests for Vector2Perpendicular Cw. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2PerpendicularCW, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector = Math::Vec2(2.0f, 3.0f);
    Math::Vec2 t = Math::PerpendicularCW(vector);
    ASSERT_FLOAT_EQ(t.x, -3.0f);
    ASSERT_FLOAT_EQ(t.y, 2.0f);

    t = Math::PerpendicularCW(t);
    t = Math::PerpendicularCW(t);
    t = Math::PerpendicularCW(t);

    ASSERT_FLOAT_EQ(t.x, 2.0f);
    ASSERT_FLOAT_EQ(t.y, 3.0f);
}

/**
 * @tc.name: Vector2PerpendicularCCW
 * @tc.desc: Tests for Vector2Perpendicular Ccw. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2PerpendicularCCW, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector = Math::Vec2(2.0f, 3.0f);
    Math::Vec2 t = Math::PerpendicularCCW(vector);
    ASSERT_FLOAT_EQ(t.x, 3.0f);
    ASSERT_FLOAT_EQ(t.y, -2.0f);

    t = Math::PerpendicularCW(t);

    ASSERT_FLOAT_EQ(t.x, 2.0f);
    ASSERT_FLOAT_EQ(t.y, 3.0f);
}

/**
 * @tc.name: Vector2rotateCW
 * @tc.desc: Tests for Vector2Rotate Cw. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2rotateCW, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector = Math::Vec2(1.0f, 1.0f);
    {
        constexpr float r = Math::PI / 2;
        Math::Vec2 t = Math::RotateCW(vector, r);

        ASSERT_FLOAT_EQ(t.x, -1.0f);
        ASSERT_FLOAT_EQ(t.y, 1.0f);
    }
    {
        constexpr float r = -Math::PI / 2;
        Math::Vec2 t = Math::RotateCW(vector, r);

        ASSERT_FLOAT_EQ(t.x, 1.0f);
        ASSERT_FLOAT_EQ(t.y, -1.0f);
    }
    {
        constexpr float r = 2 * Math::PI;
        Math::Vec2 t = Math::RotateCW(vector, r);

        ASSERT_FLOAT_EQ(t.x, 1.0f);
        ASSERT_FLOAT_EQ(t.y, 1.0f);
    }
    {
        constexpr float r = Math::PI / 4;
        Math::Vec2 t = Math::RotateCW(vector, r);

        ASSERT_FLOAT_EQ(t.x, 0.0f);
        ASSERT_NEAR(t.y, Math::sqrt(2.0f), 0.00001f);
    }
}

/**
 * @tc.name: Vector2rotateCCW
 * @tc.desc: Tests for Vector2Rotate Ccw. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2rotateCCW, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector = Math::Vec2(1.0f, 1.0f);
    {
        constexpr float r = Math::PI / 2;
        Math::Vec2 t = Math::RotateCCW(vector, r);

        ASSERT_FLOAT_EQ(t.x, 1.0f);
        ASSERT_FLOAT_EQ(t.y, -1.0f);
    }
    {
        constexpr float r = -Math::PI / 2;
        Math::Vec2 t = Math::RotateCCW(vector, r);

        ASSERT_FLOAT_EQ(t.x, -1.0f);
        ASSERT_FLOAT_EQ(t.y, 1.0f);
    }
    {
        constexpr float r = 2 * Math::PI;
        Math::Vec2 t = Math::RotateCCW(vector, r);

        ASSERT_FLOAT_EQ(t.x, 1.0f);
        ASSERT_FLOAT_EQ(t.y, 1.0f);
    }
    {
        constexpr float r = Math::PI / 4;
        Math::Vec2 t = Math::RotateCCW(vector, r);

        ASSERT_NEAR(t.x, Math::sqrt(2.0f), 0.00001f);
        ASSERT_FLOAT_EQ(t.y, 0.0f);
    }
}

/**
 * @tc.name: Vector2angle
 * @tc.desc: Tests for Vector2angle. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2angle, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vector1st = Math::Vec2(1.0f, 0.0f);
    constexpr Math::Vec2 vector2nd = Math::Vec2(0.0f, 1.0f);

    const float angle = Math::Angle(vector1st, vector2nd);

    ASSERT_FLOAT_EQ(angle, 90.0f * Math::DEG2RAD);
}

/**
 * @tc.name: Vector2intersect
 * @tc.desc: Tests for Vector2Intersect. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector2intersect, testing::ext::TestSize.Level1)
{
    bool intersects;
    {
        constexpr Math::Vec2 p0 = Math::Vec2(0.0f, 0.0f);
        constexpr Math::Vec2 v0 = Math::Vec2(1.0f, 1.0f);
        constexpr Math::Vec2 p1 = Math::Vec2(0.0f, 1.0f);
        constexpr Math::Vec2 v1 = Math::Vec2(1.0f, 1.0f);

        Math::Vec2 t = Math::Intersect(p0, v0, p1, v1, false, intersects);
        ASSERT_EQ(intersects, false);

        t = Math::Intersect(p0, v0, p1, v1, true, intersects);
        ASSERT_EQ(intersects, false);
    }

    {
        constexpr Math::Vec2 p0 = Math::Vec2(1.0f, 0.0f);
        constexpr Math::Vec2 v0 = Math::Vec2(0.0f, 2.0f);
        constexpr Math::Vec2 p1 = Math::Vec2(0.0f, 1.0f);
        constexpr Math::Vec2 v1 = Math::Vec2(2.0f, 0.0f);

        Math::Vec2 t = Math::Intersect(p0, v0, p1, v1, false, intersects);
        ASSERT_EQ(intersects, true);
        ASSERT_FLOAT_EQ(t.x, 1.0f);
        ASSERT_FLOAT_EQ(t.y, 1.0f);

        t = Math::Intersect(p0, v0, p1, v1, true, intersects);
        ASSERT_EQ(intersects, true);
        ASSERT_FLOAT_EQ(t.x, 1.0f);
        ASSERT_FLOAT_EQ(t.y, 1.0f);
    }

    {
        constexpr Math::Vec2 p0 = Math::Vec2(1.0f, 3.0f);
        constexpr Math::Vec2 v0 = Math::Vec2(0.0f, 2.0f);
        constexpr Math::Vec2 p1 = Math::Vec2(0.0f, 1.0f);
        constexpr Math::Vec2 v1 = Math::Vec2(2.0f, 0.0f);

        Math::Vec2 t = Math::Intersect(p0, v0, p1, v1, false, intersects);
        ASSERT_EQ(intersects, false);

        t = Math::Intersect(p0, v0, p1, v1, true, intersects);
        ASSERT_EQ(intersects, true);
        ASSERT_FLOAT_EQ(t.x, 1.0f);
        ASSERT_FLOAT_EQ(t.y, 1.0f);
    }
}

/**
 * @tc.name: Vector3max
 * @tc.desc: Tests for Vector3Max. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector3max, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector1st = Math::Vec3(1.0f, 0.0f, -2.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(0.0f, 0.0f, -1.0f);

    const Math::Vec3 t = Math::max(vector1st, vector2nd);

    ASSERT_FLOAT_EQ(t.x, 1.0f);
    ASSERT_FLOAT_EQ(t.y, 0.0f);
    ASSERT_FLOAT_EQ(t.z, -1.0f);
}

/**
 * @tc.name: Vector3min
 * @tc.desc: Tests for Vector3Min. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector3min, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector1st = Math::Vec3(1.0f, 0.0f, -2.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(0.0f, 0.0f, -1.0f);

    const Math::Vec3 t = Math::min(vector1st, vector2nd);

    ASSERT_FLOAT_EQ(t.x, 0.0f);
    ASSERT_FLOAT_EQ(t.y, 0.0f);
    ASSERT_FLOAT_EQ(t.z, -2.0f);
}

/**
 * @tc.name: Vector3distance
 * @tc.desc: Tests for Vector3distance. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector3distance, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector1st = Math::Vec3(1.0f, 1.0f, 1.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(-1.0f, -1.0f, -1.0f);

    const float t = Math::distance(vector1st, vector2nd);

    ASSERT_FLOAT_EQ(t, 2.0f * Math::sqrt(3.0f));
}
/**
 * @tc.name: Vector3distance2
 * @tc.desc: Tests for Vector3Distance2. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector3distance2, testing::ext::TestSize.Level1) // squared
{
    constexpr Math::Vec3 vector1st = Math::Vec3(1.0f, 0.0f, -1.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(0.0f, 1.0f, -2.0f);

    const float t = Math::Distance2(vector1st, vector2nd);

    ASSERT_FLOAT_EQ(t, 3.0f);
}

/**
 * @tc.name: Vector3reflect
 * @tc.desc: Tests for Vector3reflect. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector3reflect, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 incident = Math::Vec3(-1.0f, -1.0f, -1.0f);
    constexpr Math::Vec3 normal = Math::Vec3(0.0f, 1.0f, 0.0f);

    const Math::Vec3 reflected = Math::Reflect(incident, normal);

    ASSERT_FLOAT_EQ(reflected.x, -1.0f);
    ASSERT_FLOAT_EQ(reflected.y, 1.0f);
    ASSERT_FLOAT_EQ(reflected.z, -1.0f);
}

/**
 * @tc.name: Vector3scale
 * @tc.desc: Tests for Vector3scale. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector3scale, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector = Math::Vec3(1.0f, -1.0f, 1.0f);
    const float s = 10.0f;

    const Math::Vec3 scaled = Math::Scale(vector, s);

    ASSERT_FLOAT_EQ(Math::Magnitude(scaled), s);
}

/**
 * @tc.name: Vector3project
 * @tc.desc: Tests for Vector3project. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector3project, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector1st = Math::Vec3(5.0f, 0.0f, 0.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(5.0f, 0.0f, -5.0f);

    const Math::Vec3 projected = Math::Project(vector1st, vector2nd);

    ASSERT_FLOAT_EQ(projected.x, 2.5f);
    ASSERT_FLOAT_EQ(projected.y, 0.0f);
    ASSERT_FLOAT_EQ(projected.z, -2.5f);
}

/**
 * @tc.name: Vector3angle
 * @tc.desc: Tests for Vector3angle. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector3angle, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector1st = Math::Vec3(1.0f, 0.0f, 0.0f);
    const Math::Vec3 vector2nd = Math::Vec3(1.0f / Math::sqrt(2.0f), 0.5f, 0.5f);

    const float angle = Math::Angle(vector1st, vector2nd);

    ASSERT_FLOAT_EQ(angle, 45.0f * Math::DEG2RAD);
}

/**
 * @tc.name: Vector3dot
 * @tc.desc: Tests for Vector3Dot. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector3dot, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector1st = Math::Vec3(1.0f, 2.0f, 3.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(1.0f, 2.0f, 3.0f);

    constexpr float dotProduct = Math::Dot(vector1st, vector2nd);

    ASSERT_FLOAT_EQ(dotProduct, 14.0f);
}

/**
 * @tc.name: Vector3cross
 * @tc.desc: Tests for Vector3Cross. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector3cross, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector1st = Math::Vec3(1.0f, 2.0f, 3.0f);
    constexpr Math::Vec3 vector2nd = Math::Vec3(4.0f, 5.0f, 6.0f);

    const Math::Vec3 crossProduct = Math::Cross(vector1st, vector2nd);

    ASSERT_FLOAT_EQ(crossProduct.x, -3.0f);
    ASSERT_FLOAT_EQ(crossProduct.y, 6.0f);
    ASSERT_FLOAT_EQ(crossProduct.z, -3.0f);
}

/**
 * @tc.name: Vector3lerp
 * @tc.desc: Tests for Vector3Lerp. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector3lerp, testing::ext::TestSize.Level1)
{
    float value = 0.0f;
    constexpr Math::Vec3 pos1 = Math::Vec3(0.0f, 0.0f, 0.0f);
    constexpr Math::Vec3 pos2 = Math::Vec3(10.0f, 0.0f, 20.0f);

    Math::Vec3 pos3 = Math::Lerp(pos1, pos2, value);
    ASSERT_FLOAT_EQ(pos3.x, 0.0f);
    ASSERT_FLOAT_EQ(pos3.y, 0.0f);
    ASSERT_FLOAT_EQ(pos3.z, 0.0f);

    value = 0.5f;
    pos3 = Math::Lerp(pos1, pos2, value);
    ASSERT_FLOAT_EQ(pos3.x, 5.0f);
    ASSERT_FLOAT_EQ(pos3.y, 0.0f);
    ASSERT_FLOAT_EQ(pos3.z, 10.0f);

    value = 1.0f;
    pos3 = Math::Lerp(pos1, pos2, value);
    ASSERT_FLOAT_EQ(pos3.x, 10.0f);
    ASSERT_FLOAT_EQ(pos3.y, 0.0f);
    ASSERT_FLOAT_EQ(pos3.z, 20.0f);
}

/**
 * @tc.name: Vector3squareMagnitude
 * @tc.desc: Tests for Vector3Square Magnitude. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector3squareMagnitude, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector = Math::Vec3(2.0f, 2.0f, 3.0f);
    const float sqrtMagnitude = Math::SqrMagnitude(vector);

    ASSERT_FLOAT_EQ(sqrtMagnitude, 17.0f);
}

/**
 * @tc.name: Vector3magnitude
 * @tc.desc: Tests for Vector3Magnitude. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector3magnitude, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector = Math::Vec3(0.0f, 3.0f, 4.0f); // squareroot from 25 = 5
    const float magnitude = Math::Magnitude(vector);

    ASSERT_FLOAT_EQ(magnitude, 5.0f);
}

/**
 * @tc.name: Vector3normalize
 * @tc.desc: Tests for Vector3Normalize. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector3normalize, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 vector = Math::Vec3(10.0f, 0.0f, 10.0f);

    const Math::Vec3 normalizedVector = Math::Normalize(vector);

    ASSERT_FLOAT_EQ(normalizedVector.x, 0.707107f);
    ASSERT_FLOAT_EQ(normalizedVector.y, 0.0f);
    ASSERT_FLOAT_EQ(normalizedVector.z, 0.707107f);
}

/**
 * @tc.name: Vector4squareMagnitude
 * @tc.desc: Tests for Vector4Square Magnitude. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector4squareMagnitude, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 vector = Math::Vec4(2.0f, 2.0f, 3.0f, 3.0f);
    const float sqrtMagnitude = Math::SqrMagnitude(vector);

    ASSERT_FLOAT_EQ(sqrtMagnitude, 26.0f);
}

/**
 * @tc.name: Vector4magnitude
 * @tc.desc: Tests for Vector4magnitude. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector4magnitude, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 vector = Math::Vec4(0.0f, 3.0f, 4.0f, 5.0f);
    const float magnitude = Math::Magnitude(vector);

    ASSERT_FLOAT_EQ(magnitude, Math::sqrt(50.0f));
}

/**
 * @tc.name: Vector4distance
 * @tc.desc: Tests for Vector4distance. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector4distance, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 vector1st = Math::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    constexpr Math::Vec4 vector2nd = Math::Vec4(-1.0f, -1.0f, -1.0f, -1.0f);

    const float t = Math::distance(vector1st, vector2nd);

    ASSERT_FLOAT_EQ(t, 2.0f * Math::sqrt(4.0f));
}
/**
 * @tc.name: Vector4distance2
 * @tc.desc: Tests for Vector4distance2. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector4distance2, testing::ext::TestSize.Level1) // squared
{
    constexpr Math::Vec4 vector1st = Math::Vec4(1.0f, 0.0f, -1.0f, 1.0f);
    constexpr Math::Vec4 vector2nd = Math::Vec4(0.0f, 1.0f, -2.0f, 0.0f);

    const float t = Math::Distance2(vector1st, vector2nd);

    ASSERT_FLOAT_EQ(t, 4.0f);
}

/**
 * @tc.name: Vector4normalize
 * @tc.desc: Tests for Vector4normalize. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector4normalize, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 vector = Math::Vec4(10.0f, 10.0f, 10.0f, 10.0f);

    const Math::Vec4 normalizedVector = Math::Normalize(vector);

    ASSERT_FLOAT_EQ(normalizedVector.x, 1.0f / Math::sqrt(4.0f));
    ASSERT_FLOAT_EQ(normalizedVector.x, 1.0f / Math::sqrt(4.0f));
    ASSERT_FLOAT_EQ(normalizedVector.z, 1.0f / Math::sqrt(4.0f));
    ASSERT_FLOAT_EQ(normalizedVector.z, 1.0f / Math::sqrt(4.0f));
}

/**
 * @tc.name: Vector4lerp
 * @tc.desc: Tests for Vector4Lerp. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathVectorUtil, Vector4lerp, testing::ext::TestSize.Level1)
{
    float value = 0.0f;
    constexpr Math::Vec4 pos1 = Math::Vec4(0.0f, 0.0f, 0.0f, -10.0f);
    constexpr Math::Vec4 pos2 = Math::Vec4(10.0f, 0.0f, 20.0f, 0.0f);

    Math::Vec4 pos3 = Math::Lerp(pos1, pos2, value);
    ASSERT_FLOAT_EQ(pos3.x, 0.0f);
    ASSERT_FLOAT_EQ(pos3.y, 0.0f);
    ASSERT_FLOAT_EQ(pos3.z, 0.0f);
    ASSERT_FLOAT_EQ(pos3.w, -10.0f);

    value = 0.5f;
    pos3 = Math::Lerp(pos1, pos2, value);
    ASSERT_FLOAT_EQ(pos3.x, 5.0f);
    ASSERT_FLOAT_EQ(pos3.y, 0.0f);
    ASSERT_FLOAT_EQ(pos3.z, 10.0f);
    ASSERT_FLOAT_EQ(pos3.w, -5.0f);

    value = 1.0f;
    pos3 = Math::Lerp(pos1, pos2, value);
    ASSERT_FLOAT_EQ(pos3.x, 10.0f);
    ASSERT_FLOAT_EQ(pos3.y, 0.0f);
    ASSERT_FLOAT_EQ(pos3.z, 20.0f);
    ASSERT_FLOAT_EQ(pos3.w, 0.0f);

    value = 3.0f;
    pos3 = Math::Lerp(pos1, pos2, value);
    ASSERT_FLOAT_EQ(pos3.x, 10.0f);
    ASSERT_FLOAT_EQ(pos3.y, 0.0f);
    ASSERT_FLOAT_EQ(pos3.z, 20.0f);
    ASSERT_FLOAT_EQ(pos3.w, 0.0f);
}

// Quaternion

/**
 * @tc.name: Default
 * @tc.desc: Tests for Default. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionConstructors, Default, testing::ext::TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] Math::Quat quaternion = Math::Quat());
}

/**
 * @tc.name: Initializer
 * @tc.desc: Tests for Initializer. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionConstructors, Initializer, testing::ext::TestSize.Level1)
{
    constexpr Math::Quat quaternion = Math::Quat(1.000001f, 2.000002f, 3.000003f, 4.000004f);

    ASSERT_FLOAT_EQ(quaternion.x, 1.000001f);
    ASSERT_FLOAT_EQ(quaternion.y, 2.000002f);
    ASSERT_FLOAT_EQ(quaternion.z, 3.000003f);
    ASSERT_FLOAT_EQ(quaternion.w, 4.000004f);
}

/**
 * @tc.name: Copy
 * @tc.desc: Tests for Copy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionConstructors, Copy, testing::ext::TestSize.Level1)
{
    constexpr Math::Quat quaternion = Math::Quat(1.000001f, 2.000002f, 3.000003f, 4.000004f);
    constexpr Math::Quat copiedQuaternion = Math::Quat(quaternion);

    ASSERT_FLOAT_EQ(copiedQuaternion.x, quaternion.x);
    ASSERT_FLOAT_EQ(copiedQuaternion.y, quaternion.y);
    ASSERT_FLOAT_EQ(copiedQuaternion.z, quaternion.z);
}

/**
 * @tc.name: QuaternionOperatorMultiplication
 * @tc.desc: Tests for Quaternion Operator Multiplication. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionOperators, QuaternionOperatorMultiplication, testing::ext::TestSize.Level1)
{
    constexpr Math::Quat quaternion1 = Math::Quat(0.5f, 0.5f, 0.5f, 1.0f);
    constexpr Math::Quat quaternion2 = Math::Quat(0.2f, 0.0f, 0.0f, 1.0f);

    constexpr auto quaternion3 = quaternion1 * quaternion2;

    ASSERT_FLOAT_EQ(quaternion3.x, 0.7f); // i
    ASSERT_FLOAT_EQ(quaternion3.y, 0.6f); // j
    ASSERT_FLOAT_EQ(quaternion3.z, 0.4f); // k
    ASSERT_FLOAT_EQ(quaternion3.w, 0.9f); // real
}

/**
 * @tc.name: QuaternionOperatorMultiplicationScalar
 * @tc.desc: Tests for Quaternion Operator Multiplication Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionOperators, QuaternionOperatorMultiplicationScalar, testing::ext::TestSize.Level1)
{
    constexpr Math::Quat quaternion = Math::Quat(0.5f, 0.5f, 0.5f, 1.0f);
    {
        constexpr auto temp = quaternion * 0.001f;

        ASSERT_FLOAT_EQ(temp.x, 0.0005f); // i
        ASSERT_FLOAT_EQ(temp.y, 0.0005f); // j
        ASSERT_FLOAT_EQ(temp.z, 0.0005f); // k
        ASSERT_FLOAT_EQ(temp.w, 0.001f);  // real
    }

    {
        constexpr auto temp = 0.001f * quaternion;

        ASSERT_FLOAT_EQ(temp.x, 0.0005f); // i
        ASSERT_FLOAT_EQ(temp.y, 0.0005f); // j
        ASSERT_FLOAT_EQ(temp.z, 0.0005f); // k
        ASSERT_FLOAT_EQ(temp.w, 0.001f);  // real
    }
}

/**
 * @tc.name: QuaternionOperatorFloatDivision
 * @tc.desc: Tests for Quaternion Operator Float Division. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionOperators, QuaternionOperatorFloatDivision, testing::ext::TestSize.Level1)
{
    Math::Quat quaternion = Math::Quat(0.2f, 0.4f, 0.0f, 1.0f);
    constexpr float divider = 2.0f;

    quaternion = quaternion / divider;

    ASSERT_FLOAT_EQ(quaternion.x, 0.1f); // i
    ASSERT_FLOAT_EQ(quaternion.y, 0.2f); // j
    ASSERT_FLOAT_EQ(quaternion.z, 0.0f); // k
    ASSERT_FLOAT_EQ(quaternion.w, 0.5f); // real
}

/**
 * @tc.name: QuaternionOperatorFloatDivisionAssignment
 * @tc.desc: Tests for Quaternion Operator Float Division Assignment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionOperators, QuaternionOperatorFloatDivisionAssignment, testing::ext::TestSize.Level1)
{
    Math::Quat quaternion = Math::Quat(0.2f, 0.4f, 0.0f, 1.0f);
    constexpr float divider = 2.0f;

    quaternion /= divider;

    ASSERT_FLOAT_EQ(quaternion.x, 0.1f); // i
    ASSERT_FLOAT_EQ(quaternion.y, 0.2f); // j
    ASSERT_FLOAT_EQ(quaternion.z, 0.0f); // k
    ASSERT_FLOAT_EQ(quaternion.w, 0.5f); // real
}

// Quaternion Util
/**
 * @tc.name: FromEulerRad
 * @tc.desc: Tests for From Euler Rad. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionUtil, FromEulerRad, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 eulerVector = Math::Vec3(60.0f * Math::DEG2RAD, 0.0f, 0.0f);

    const Math::Quat quaternion = Math::FromEulerRad(eulerVector);

    ASSERT_FLOAT_EQ(quaternion.x, 0.5f);
    ASSERT_FLOAT_EQ(quaternion.y, 0.0f);
    ASSERT_FLOAT_EQ(quaternion.z, 0.0f);
    ASSERT_FLOAT_EQ(quaternion.w, 0.86602539f);
}

/**
 * @tc.name: LengthSquared
 * @tc.desc: Tests for Length Squared. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionUtil, LengthSquared, testing::ext::TestSize.Level1)
{
    constexpr Math::Quat quaternion = Math::Quat(1.0f, 1.0f, 1.0f, 1.0f);
    const float squaredLength = Math::LengthSquared(quaternion);

    ASSERT_FLOAT_EQ(squaredLength, 4.0f);
}

/**
 * @tc.name: Length
 * @tc.desc: Tests for Length. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionUtil, Length, testing::ext::TestSize.Level1)
{
    constexpr Math::Quat quaternion = Math::Quat(1.0f, 1.0f, 1.0f, 1.0f);
    const float Length = Math::Length(quaternion);

    ASSERT_FLOAT_EQ(Length, 2.0f);
}

/**
 * @tc.name: AngleAxis
 * @tc.desc: Tests for Angle Axis. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionUtil, AngleAxis, testing::ext::TestSize.Level1)
{
    constexpr float amountToRotate = Math::DEG2RAD * 60.0f;

    const Math::Quat quaternion = Math::AngleAxis(amountToRotate, Math::Vec3(0.0f, 1.0f, 0.0f));

    ASSERT_FLOAT_EQ(quaternion.x, 0.0f);
    ASSERT_FLOAT_EQ(quaternion.y, 0.5f);
    ASSERT_FLOAT_EQ(quaternion.z, 0.0f);
    ASSERT_FLOAT_EQ(quaternion.w, 0.86602539f);
}

/**
 * @tc.name: Inverse
 * @tc.desc: Tests for Inverse. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionUtil, Inverse, testing::ext::TestSize.Level1)
{
    Math::Quat quaternion(0.5f, 0.5f, 0.5f, 1.0f);
    quaternion = Math::Inverse(quaternion);

    ASSERT_FLOAT_EQ(quaternion.x, -0.2857143f);
    ASSERT_FLOAT_EQ(quaternion.y, -0.2857143f);
    ASSERT_FLOAT_EQ(quaternion.z, -0.2857143f);
    ASSERT_FLOAT_EQ(quaternion.w, 0.5714286f);
}

/**
 * @tc.name: Euler
 * @tc.desc: Tests for Euler. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionUtil, Euler, testing::ext::TestSize.Level1)
{
    {
        const Math::Quat quaternion = Math::Euler(60.0f, 0.0f, 0.0f);

        ASSERT_FLOAT_EQ(quaternion.x, 0.5f);
        ASSERT_FLOAT_EQ(quaternion.y, 0.0f);
        ASSERT_FLOAT_EQ(quaternion.z, 0.0f);
        ASSERT_FLOAT_EQ(quaternion.w, 0.86602539f);
    }

    {
        constexpr Math::Vec3 vec = Math::Vec3(60.0f, 0.0f, 0.0f);
        const Math::Quat quaternion = Math::Euler(vec);

        ASSERT_FLOAT_EQ(quaternion.x, 0.5f);
        ASSERT_FLOAT_EQ(quaternion.y, 0.0f);
        ASSERT_FLOAT_EQ(quaternion.z, 0.0f);
        ASSERT_FLOAT_EQ(quaternion.w, 0.86602539f);
    }
}

/**
 * @tc.name: Conjugate
 * @tc.desc: Tests for Conjugate. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionUtil, Conjugate, testing::ext::TestSize.Level1)
{
    {
        constexpr Math::Quat quat = Math::Quat(0.0f, 0.0f, -0.707107f, 0.707107f);
        const Math::Quat quaternion = Math::Conjugate(quat);

        ASSERT_FLOAT_EQ(quaternion.x, 0.0f);
        ASSERT_FLOAT_EQ(quaternion.y, 0.0f);
        ASSERT_FLOAT_EQ(quaternion.z, 0.707107f);
        ASSERT_FLOAT_EQ(quaternion.w, 0.707107f);
    }
}

/**
 * @tc.name: VectorMultiplication
 * @tc.desc: Tests for Vector Multiplication. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionUtil, VectorMultiplication, testing::ext::TestSize.Level1)
{
    {
        constexpr Math::Quat quat = Math::Quat(0.0f, 0.0f, -0.707107f, 0.707107f);
        constexpr Math::Vec3 vec = Math::Vec3(1.0f, 2.0f, -3.0f);
        Math::Vec3 temp = quat * vec;

        constexpr Math::Quat qVec = Math::Quat(1.0f, 2.0f, -3.0f, 0.0f);

        Math::Quat qVecMult = quat * qVec * (Math::Conjugate(quat));

        ASSERT_NEAR(temp.x, qVecMult.x, 0.0001f);
        ASSERT_NEAR(temp.y, qVecMult.y, 0.0001f);
        ASSERT_NEAR(temp.z, qVecMult.z, 0.0001f);
    }
    {
        constexpr Math::Quat quat = Math::Quat(0.0f, 0.0f, -0.707107f, 0.707107f);
        constexpr Math::Vec3 vec = Math::Vec3(1.0f, 2.0f, -3.0f);
        Math::Vec3 temp = vec * quat;

        constexpr Math::Quat qVec = Math::Quat(1.0f, 2.0f, -3.0f, 0.0f);

        Math::Quat qVecMult = (Math::Conjugate(quat)) * qVec * quat;

        ASSERT_NEAR(temp.x, qVecMult.x, 0.0001f);
        ASSERT_NEAR(temp.y, qVecMult.y, 0.0001f);
        ASSERT_NEAR(temp.z, qVecMult.z, 0.0001f);
    }
}

/**
 * @tc.name: Normalize
 * @tc.desc: Tests for Normalize. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionUtil, Normalize, testing::ext::TestSize.Level1)
{
    { // Bad input

        constexpr Math::Quat quat = Math::Quat(0.0f, 0.0f, 0.0f, 0.0f);
        const Math::Quat quaternion = Math::Normalize(quat);

        ASSERT_FLOAT_EQ(quaternion.x, 0.0f);
        ASSERT_FLOAT_EQ(quaternion.y, 0.0f);
        ASSERT_FLOAT_EQ(quaternion.z, 0.0f);
        ASSERT_FLOAT_EQ(quaternion.w, 1.0f);
    }

    {
        constexpr Math::Quat quat = Math::Quat(-1.0f, -1.0f, -1.0f, -1.0f);
        const Math::Quat quaternion = Math::Normalize(quat);

        ASSERT_FLOAT_EQ(quaternion.x, -0.5f);
        ASSERT_FLOAT_EQ(quaternion.y, -0.5f);
        ASSERT_FLOAT_EQ(quaternion.z, -0.5f);
        ASSERT_FLOAT_EQ(quaternion.w, -0.5f);
    }
}

/**
 * @tc.name: LookRotation
 * @tc.desc: Tests for Look Rotation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionUtil, LookRotation, testing::ext::TestSize.Level1)
{
    Math::Quat rotation = Math::Quat(0.0f, 0.0f, 0.0f, 1.0f);

    constexpr Math::Vec3 myPosition = Math::Vec3(0.0f, 0.0f, 0.0f);
    constexpr Math::Vec3 targetPosition = Math::Vec3(90.0f, 0.0f, 90.0f);

    constexpr Math::Vec3 relativePos = targetPosition - myPosition;

    rotation = Math::LookRotation(relativePos, Math::Vec3(0.0f, 1.0f, 0.0f));

    ASSERT_FLOAT_EQ(rotation.x, 0.0f);
    ASSERT_FLOAT_EQ(rotation.y, 0.38268346f);
    ASSERT_FLOAT_EQ(rotation.z, 0.0f);
    ASSERT_FLOAT_EQ(rotation.w, 0.92387956f);
}

/**
 * @tc.name: NormalizeAngle
 * @tc.desc: Tests for Normalize Angle. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionUtil, NormalizeAngle, testing::ext::TestSize.Level1)
{
    float angle = -0.1f;
    angle = Math::NormalizeAngle(angle);

    ASSERT_FLOAT_EQ(angle, 359.9f);

    angle = 360.2f;
    angle = Math::NormalizeAngle(angle);

    ASSERT_NEAR(angle, 0.2f, 0.0001f);
}

/**
 * @tc.name: NormalizeAngles
 * @tc.desc: Tests for Normalize Angles. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionUtil, NormalizeAngles, testing::ext::TestSize.Level1)
{
    Math::Vec3 angles = Math::Vec3(-0.1f, 200.0f, -0.1f);
    angles = Math::NormalizeAngles(angles);

    ASSERT_LT(angles.x, 359.91f);
    ASSERT_GT(angles.x, 359.89f);

    ASSERT_FLOAT_EQ(angles.y, 200.0f);

    ASSERT_LT(angles.z, 359.91f);
    ASSERT_GT(angles.z, 359.89f);
}

/**
 * @tc.name: ToEulerRad
 * @tc.desc: Tests for To Euler Rad. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionUtil, ToEulerRad, testing::ext::TestSize.Level1)
{
    constexpr Math::Quat qRot = Math::Quat(0.0f, 0.383f, 0.0f, 0.9235f); // Should be near Y 45 degrees
    const Math::Vec3 rotation = Math::ToEulerRad(qRot);

    ASSERT_FLOAT_EQ(rotation.y, 0.78581429f);
}

/**
 * @tc.name: Slerp
 * @tc.desc: Tests for Slerp. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternionUtil, Slerp, testing::ext::TestSize.Level1)
{
    constexpr Math::Quat qRot1 = Math::Quat(0.0f, 0.0f, -0.707107f, 0.707107f);
    constexpr Math::Quat qRot2 = Math::Quat(0.707107f, 0.0f, 0.707107f, 0.0f);
    {
        const float coeff = 0.0f;
        Math::Quat qT = Math::Slerp(qRot1, qRot2, coeff);

        ASSERT_FLOAT_EQ(qT.x, 0.0f);
        ASSERT_FLOAT_EQ(qT.y, 0.0f);
        ASSERT_FLOAT_EQ(qT.z, -0.707107f);
        ASSERT_FLOAT_EQ(qT.w, 0.707107f);
    }

    {
        const float coeff = 1.0f;
        Math::Quat qT = Math::Slerp(qRot1, qRot2, coeff);

        ASSERT_FLOAT_EQ(qT.x, -0.707107f);
        ASSERT_FLOAT_EQ(qT.y, 0.0f);
        ASSERT_FLOAT_EQ(qT.z, -0.707107f);
        ASSERT_FLOAT_EQ(qT.w, 0.0f);
    }

    {
        const float coeff = 0.5f;
        Math::Quat qT = Math::Slerp(qRot1, qRot2, coeff);

        // trigonometry test

        float cosTeta = Math::Dot(qRot1, qRot2);
        if (cosTeta < 0) {
            cosTeta = -cosTeta;
            float Teta = Math::acos(cosTeta);
            Math::Quat qT1 = sin((1 - coeff) * Teta) / sin(Teta) * qRot1;
            Math::Quat qT2 = -sin(coeff * Teta) / sin(Teta) * qRot2;
            ASSERT_NEAR(qT.x, (qT1.x + qT2.x), 0.0001f);
            ASSERT_NEAR(qT.y, (qT1.y + qT2.y), 0.0001f);
            ASSERT_NEAR(qT.z, (qT1.z + qT2.z), 0.0001f);
            ASSERT_NEAR(qT.w, (qT1.w + qT2.w), 0.0001f);
        }
    }

    {
        constexpr Math::Quat qRotPositive1 = Math::Quat(0.0f, 0.0f, -0.707107f, 0.707107f);
        constexpr Math::Quat qRotPositive2 = Math::Quat(0.707107f, 0.0f, -0.707107f, 0.0f);

        const float coeff = 0.5f;
        Math::Quat qT = Math::Slerp(qRotPositive1, qRotPositive2, coeff);

        // trigonometry test

        float cosTeta = Math::Dot(qRotPositive1, qRotPositive2);
        float Teta = Math::acos(cosTeta);
        Math::Quat qT1 = sin((1 - coeff) * Teta) / sin(Teta) * qRotPositive1;
        Math::Quat qT2 = sin(coeff * Teta) / sin(Teta) * qRotPositive2;
        ASSERT_NEAR(qT.x, (qT1.x + qT2.x), 0.0001f);
        ASSERT_NEAR(qT.y, (qT1.y + qT2.y), 0.0001f);
        ASSERT_NEAR(qT.z, (qT1.z + qT2.z), 0.0001f);
        ASSERT_NEAR(qT.w, (qT1.w + qT2.w), 0.0001f);
    }

    {
        constexpr Math::Quat qRotPositive1 = Math::Quat(0.0f, 0.0f, 0.707107f, 0.707107f);
        constexpr Math::Quat qRotPositive2 = Math::Quat(0.0f, 0.0f, 0.707107f, 0.707107f);

        const float coeff = 0.5f;
        Math::Quat qT = Math::Slerp(qRotPositive1, qRotPositive2, coeff);

        ASSERT_FLOAT_EQ(qT.x, 0.0f);
        ASSERT_FLOAT_EQ(qT.y, 0.0f);
        ASSERT_FLOAT_EQ(qT.z, 0.707107f);
        ASSERT_FLOAT_EQ(qT.w, 0.707107f);
    }
}

// Quaternion remaining base actions

/**
 * @tc.name: DivideZero
 * @tc.desc: Tests for Divide Zero. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternion, DivideZero, testing::ext::TestSize.Level1)
{
    Math::Quat rotation = Math::Quat(0.0f, 0.0f, 0.0f, 1.0f);

    rotation /= 0.0f;

    ASSERT_FLOAT_EQ(rotation.x, HUGE_VALF);
    ASSERT_FLOAT_EQ(rotation.y, HUGE_VALF);
    ASSERT_FLOAT_EQ(rotation.z, HUGE_VALF);
    ASSERT_FLOAT_EQ(rotation.w, HUGE_VALF);
}

/**
 * @tc.name: Compare
 * @tc.desc: Tests for Compare. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathQuaternion, Compare, testing::ext::TestSize.Level1)
{
    constexpr Math::Quat rotation = Math::Quat(0.0f, 0.0f, 0.0f, 1.0f);
    constexpr const Math::Quat rotation2 = Math::Quat(0.0f, 0.0f, 0.0f, 1.0f);
    constexpr const Math::Quat rotation3 = Math::Quat(0.0f, 0.0f, 0.0f, 2.0f);

    // != calls == bouble cover for free
    ASSERT_FALSE(rotation != rotation2);
    ASSERT_FALSE(rotation == rotation3);
    ASSERT_TRUE(rotation == Normalize(rotation3));

    // const index compare
    ASSERT_FLOAT_EQ(rotation2[0], rotation3[0]);
}

// Matrix

/**
 * @tc.name: Default
 * @tc.desc: Tests for Default. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix3x3constructors, Default, testing::ext::TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] Math::Mat3X3 mat = Math::Mat3X3());
}

/**
 * @tc.name: Initializer
 * @tc.desc: Tests for Initializer. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix3x3constructors, Initializer, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1 = Math::Vec3(1.0f, 0.0f, 0.0f);
    constexpr Math::Vec3 col2 = Math::Vec3(0.0f, 1.0f, 0.0f);
    constexpr Math::Vec3 col3 = Math::Vec3(0.0f, 0.0f, 1.0f);
    [[maybe_unused]] constexpr Math::Mat3X3 mat = Math::Mat3X3(col1, col2, col3);

    ASSERT_FLOAT_EQ(col1.x, 1.0f);
    ASSERT_FLOAT_EQ(col1.y, 0.0f);
    ASSERT_FLOAT_EQ(col1.z, 0.0f);

    ASSERT_FLOAT_EQ(col2.x, 0.0f);
    ASSERT_FLOAT_EQ(col2.y, 1.0f);
    ASSERT_FLOAT_EQ(col2.z, 0.0f);

    ASSERT_FLOAT_EQ(col3.x, 0.0f);
    ASSERT_FLOAT_EQ(col3.y, 0.0f);
    ASSERT_FLOAT_EQ(col3.z, 1.0f);
}

/**
 * @tc.name: Copy
 * @tc.desc: Tests for Copy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix3x3constructors, Copy, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1 = Math::Vec3(1.0f, 0.0f, 0.0f);
    constexpr Math::Vec3 col2 = Math::Vec3(0.0f, 1.0f, 0.0f);
    constexpr Math::Vec3 col3 = Math::Vec3(0.0f, 0.0f, 1.0f);
    constexpr Math::Mat3X3 mat = Math::Mat3X3(col1, col2, col3);
    constexpr Math::Mat3X3 copiedMat = Math::Mat3X3(mat);

    ASSERT_FLOAT_EQ(copiedMat.data[0], 1.0f);
    ASSERT_FLOAT_EQ(copiedMat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(copiedMat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(copiedMat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(copiedMat.data[4], 1.0f);
    ASSERT_FLOAT_EQ(copiedMat.data[5], 0.0f);
    ASSERT_FLOAT_EQ(copiedMat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(copiedMat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(copiedMat.data[8], 1.0f);
}

/**
 * @tc.name: Matrix3x3OperatorAddition
 * @tc.desc: Tests for Matrix3X3 Operator Addition. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix3x3operators, Matrix3x3OperatorAddition, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1A = Math::Vec3(1.0f, 2.0f, 3.0f);
    constexpr Math::Vec3 col2A = Math::Vec3(4.0f, 5.0f, 6.0f);
    constexpr Math::Vec3 col3A = Math::Vec3(7.0f, 8.0f, 9.0f);
    constexpr Math::Mat3X3 matA = Math::Mat3X3(col1A, col2A, col3A);

    constexpr Math::Vec3 col1B = Math::Vec3(0.5f, 1.5f, 2.5f);
    constexpr Math::Vec3 col2B = Math::Vec3(3.5f, 4.5f, 5.5f);
    constexpr Math::Vec3 col3B = Math::Vec3(6.5f, 7.5f, 8.5f);
    constexpr Math::Mat3X3 matB = Math::Mat3X3(col1B, col2B, col3B);

    const Math::Mat3X3 result = matA + matB;

    ASSERT_FLOAT_EQ(result.data[0], 1.5f);
    ASSERT_FLOAT_EQ(result.data[1], 3.5f);
    ASSERT_FLOAT_EQ(result.data[2], 5.5f);

    ASSERT_FLOAT_EQ(result.data[3], 7.5f);
    ASSERT_FLOAT_EQ(result.data[4], 9.5f);
    ASSERT_FLOAT_EQ(result.data[5], 11.5f);

    ASSERT_FLOAT_EQ(result.data[6], 13.5f);
    ASSERT_FLOAT_EQ(result.data[7], 15.5f);
    ASSERT_FLOAT_EQ(result.data[8], 17.5f);
}

/**
 * @tc.name: Matrix3x3OperatorAdditionAssignment
 * @tc.desc: Tests for Matrix3X3 Operator Addition Assignment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix3x3operators, Matrix3x3OperatorAdditionAssignment, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1A = Math::Vec3(1.0f, 2.0f, 3.0f);
    constexpr Math::Vec3 col2A = Math::Vec3(4.0f, 5.0f, 6.0f);
    constexpr Math::Vec3 col3A = Math::Vec3(7.0f, 8.0f, 9.0f);
    Math::Mat3X3 matA = Math::Mat3X3(col1A, col2A, col3A);

    constexpr Math::Vec3 col1B = Math::Vec3(0.5f, 1.5f, 2.5f);
    constexpr Math::Vec3 col2B = Math::Vec3(3.5f, 4.5f, 5.5f);
    constexpr Math::Vec3 col3B = Math::Vec3(6.5f, 7.5f, 8.5f);
    constexpr Math::Mat3X3 matB = Math::Mat3X3(col1B, col2B, col3B);

    matA += matB;

    ASSERT_FLOAT_EQ(matA.data[0], 1.5f);
    ASSERT_FLOAT_EQ(matA.data[1], 3.5f);
    ASSERT_FLOAT_EQ(matA.data[2], 5.5f);

    ASSERT_FLOAT_EQ(matA.data[3], 7.5f);
    ASSERT_FLOAT_EQ(matA.data[4], 9.5f);
    ASSERT_FLOAT_EQ(matA.data[5], 11.5f);

    ASSERT_FLOAT_EQ(matA.data[6], 13.5f);
    ASSERT_FLOAT_EQ(matA.data[7], 15.5f);
    ASSERT_FLOAT_EQ(matA.data[8], 17.5f);
}

/**
 * @tc.name: Matrix3x3OperatorSubtraction
 * @tc.desc: Tests for Matrix3X3 Operator Subtraction. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix3x3operators, Matrix3x3OperatorSubtraction, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1A = Math::Vec3(5.0f, -3.0f, 0.0f);
    constexpr Math::Vec3 col2A = Math::Vec3(7.5f, 1.0f, -2.0f);
    constexpr Math::Vec3 col3A = Math::Vec3(3.0f, 4.0f, -5.0f);
    constexpr Math::Mat3X3 matA = Math::Mat3X3(col1A, col2A, col3A);

    constexpr Math::Vec3 col1B = Math::Vec3(2.0f, -1.0f, 0.0f);
    constexpr Math::Vec3 col2B = Math::Vec3(3.5f, 0.5f, -1.0f);
    constexpr Math::Vec3 col3B = Math::Vec3(1.0f, 2.0f, -3.0f);
    constexpr Math::Mat3X3 matB = Math::Mat3X3(col1B, col2B, col3B);

    const Math::Mat3X3 result = matA - matB;

    ASSERT_FLOAT_EQ(result.data[0], 3.0f);
    ASSERT_FLOAT_EQ(result.data[1], -2.0f);
    ASSERT_FLOAT_EQ(result.data[2], 0.0f);

    ASSERT_FLOAT_EQ(result.data[3], 4.0f);
    ASSERT_FLOAT_EQ(result.data[4], 0.5f);
    ASSERT_FLOAT_EQ(result.data[5], -1.0f);

    ASSERT_FLOAT_EQ(result.data[6], 2.0f);
    ASSERT_FLOAT_EQ(result.data[7], 2.0f);
    ASSERT_FLOAT_EQ(result.data[8], -2.0f);
}

/**
 * @tc.name: Matrix3x3OperatorSubtractionAssignment
 * @tc.desc: Tests for Matrix3X3 Operator Subtraction Assignment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix3x3operators, Matrix3x3OperatorSubtractionAssignment, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1A = Math::Vec3(5.0f, -3.0f, 0.0f);
    constexpr Math::Vec3 col2A = Math::Vec3(7.5f, 1.0f, -2.0f);
    constexpr Math::Vec3 col3A = Math::Vec3(3.0f, 4.0f, -5.0f);
    Math::Mat3X3 matA = Math::Mat3X3(col1A, col2A, col3A);

    constexpr Math::Vec3 col1B = Math::Vec3(2.0f, -1.0f, 0.0f);
    constexpr Math::Vec3 col2B = Math::Vec3(3.5f, 0.5f, -1.0f);
    constexpr Math::Vec3 col3B = Math::Vec3(1.0f, 2.0f, -3.0f);
    constexpr Math::Mat3X3 matB = Math::Mat3X3(col1B, col2B, col3B);

    matA -= matB;

    ASSERT_FLOAT_EQ(matA.data[0], 3.0f);
    ASSERT_FLOAT_EQ(matA.data[1], -2.0f);
    ASSERT_FLOAT_EQ(matA.data[2], 0.0f);

    ASSERT_FLOAT_EQ(matA.data[3], 4.0f);
    ASSERT_FLOAT_EQ(matA.data[4], 0.5f);
    ASSERT_FLOAT_EQ(matA.data[5], -1.0f);

    ASSERT_FLOAT_EQ(matA.data[6], 2.0f);
    ASSERT_FLOAT_EQ(matA.data[7], 2.0f);
    ASSERT_FLOAT_EQ(matA.data[8], -2.0f);
}

/**
 * @tc.name: Matrix3x3operatorMultiplication
 * @tc.desc: Tests for Matrix3x3Operator Multiplication. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix3x3operators, Matrix3x3operatorMultiplication, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 m1col1 = Math::Vec3(1.1f, 1.0f, 2.0f);
    constexpr Math::Vec3 m1col2 = Math::Vec3(0.1f, 1.1f, 2.1f);
    constexpr Math::Vec3 m1col3 = Math::Vec3(0.2f, 1.2f, 2.2f);
    constexpr Math::Mat3X3 mat = Math::Mat3X3(m1col1, m1col2, m1col3);

    constexpr Math::Vec3 m2col1 = Math::Vec3(5.0f, 2.0f, 2.2f);
    constexpr Math::Vec3 m2col2 = Math::Vec3(0.1f, 1.1f, 2.1f);
    constexpr Math::Vec3 m2col3 = Math::Vec3(0.2f, 1.2f, 2.2f);
    constexpr Math::Mat3X3 mat2nd = Math::Mat3X3(m2col1, m2col2, m2col3);

    const Math::Mat3X3 mat3rd = mat * mat2nd;

    ASSERT_FLOAT_EQ(mat3rd.data[0], 6.0f);
    ASSERT_FLOAT_EQ(mat3rd.data[1], 5.7f);
    ASSERT_FLOAT_EQ(mat3rd.data[2], 8.92f);

    ASSERT_FLOAT_EQ(mat3rd.data[3], 1.03f);
    ASSERT_FLOAT_EQ(mat3rd.data[4], 3.93f);
    ASSERT_FLOAT_EQ(mat3rd.data[5], 7.15f);

    ASSERT_FLOAT_EQ(mat3rd.data[6], 1.56f);
    ASSERT_FLOAT_EQ(mat3rd.data[7], 4.36f);
    ASSERT_FLOAT_EQ(mat3rd.data[8], 7.8f);
}

/**
 * @tc.name: Default
 * @tc.desc: Tests for Default. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x4constructors, Default, testing::ext::TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] Math::Mat4X4 mat = Math::Mat4X4());
}

/**
 * @tc.name: Initializer
 * @tc.desc: Tests for Initializer. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x4constructors, Initializer, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1 = Math::Vec4(0.0f, 1.0f, 2.0f, 3.0f);
    constexpr Math::Vec4 col2 = Math::Vec4(0.1f, 1.1f, 2.1f, 3.1f);
    constexpr Math::Vec4 col3 = Math::Vec4(0.2f, 1.2f, 2.2f, 3.2f);
    constexpr Math::Vec4 col4 = Math::Vec4(0.3f, 1.3f, 2.3f, 3.3f);
    constexpr Math::Mat4X4 mat = Math::Mat4X4(col1, col2, col3, col4);

    ASSERT_FLOAT_EQ(mat.data[0], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[1], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 2.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 3.0f);

    ASSERT_FLOAT_EQ(mat.data[4], 0.1f);
    ASSERT_FLOAT_EQ(mat.data[5], 1.1f);
    ASSERT_FLOAT_EQ(mat.data[6], 2.1f);
    ASSERT_FLOAT_EQ(mat.data[7], 3.1f);

    ASSERT_FLOAT_EQ(mat.data[8], 0.2f);
    ASSERT_FLOAT_EQ(mat.data[9], 1.2f);
    ASSERT_FLOAT_EQ(mat.data[10], 2.2f);
    ASSERT_FLOAT_EQ(mat.data[11], 3.2f);

    ASSERT_FLOAT_EQ(mat.data[12], 0.3f);
    ASSERT_FLOAT_EQ(mat.data[13], 1.3f);
    ASSERT_FLOAT_EQ(mat.data[14], 2.3f);
    ASSERT_FLOAT_EQ(mat.data[15], 3.3f);
}

/**
 * @tc.name: ConversionFrom3x3
 * @tc.desc: Tests for Conversion From3X3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x4constructors, ConversionFrom3x3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1 = Math::Vec3(0.0f, 1.0f, 2.0f);
    constexpr Math::Vec3 col2 = Math::Vec3(0.1f, 1.1f, 2.1f);
    constexpr Math::Vec3 col3 = Math::Vec3(0.2f, 1.2f, 2.2f);
    constexpr Math::Mat3X3 mat3 = Math::Mat3X3(col1, col2, col3);
    const Math::Mat4X4 mat4 = Math::Mat4X4(mat3);

    ASSERT_FLOAT_EQ(mat4.data[0], 0.0f);
    ASSERT_FLOAT_EQ(mat4.data[1], 1.0f);
    ASSERT_FLOAT_EQ(mat4.data[2], 2.0f);
    ASSERT_FLOAT_EQ(mat4.data[3], 0.0f);

    ASSERT_FLOAT_EQ(mat4.data[4], 0.1f);
    ASSERT_FLOAT_EQ(mat4.data[5], 1.1f);
    ASSERT_FLOAT_EQ(mat4.data[6], 2.1f);
    ASSERT_FLOAT_EQ(mat4.data[7], 0.0f);

    ASSERT_FLOAT_EQ(mat4.data[8], 0.2f);
    ASSERT_FLOAT_EQ(mat4.data[9], 1.2f);
    ASSERT_FLOAT_EQ(mat4.data[10], 2.2f);
    ASSERT_FLOAT_EQ(mat4.data[11], 0.0f);

    ASSERT_FLOAT_EQ(mat4.data[12], 0.0f);
    ASSERT_FLOAT_EQ(mat4.data[13], 0.0f);
    ASSERT_FLOAT_EQ(mat4.data[14], 0.0f);
    ASSERT_FLOAT_EQ(mat4.data[15], 1.0f);
}

/**
 * @tc.name: Matrix4x4OperatorAddition
 * @tc.desc: Tests for Matrix4X4 Operator Addition. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x4operators, Matrix4x4OperatorAddition, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1A = Math::Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    constexpr Math::Vec4 col2A = Math::Vec4(5.0f, 6.0f, 7.0f, 8.0f);
    constexpr Math::Vec4 col3A = Math::Vec4(9.0f, 10.0f, 11.0f, 12.0f);
    constexpr Math::Vec4 col4A = Math::Vec4(13.0f, 14.0f, 15.0f, 16.0f);
    constexpr Math::Mat4X4 matA = Math::Mat4X4(col1A, col2A, col3A, col4A);

    constexpr Math::Vec4 col1B = Math::Vec4(0.5f, 1.5f, 2.5f, 3.5f);
    constexpr Math::Vec4 col2B = Math::Vec4(4.5f, 5.5f, 6.5f, 7.5f);
    constexpr Math::Vec4 col3B = Math::Vec4(8.5f, 9.5f, 10.5f, 11.5f);
    constexpr Math::Vec4 col4B = Math::Vec4(12.5f, 13.5f, 14.5f, 15.5f);
    constexpr Math::Mat4X4 matB = Math::Mat4X4(col1B, col2B, col3B, col4B);

    const Math::Mat4X4 result = matA + matB;

    ASSERT_FLOAT_EQ(result.data[0], 1.5f);
    ASSERT_FLOAT_EQ(result.data[1], 3.5f);
    ASSERT_FLOAT_EQ(result.data[2], 5.5f);
    ASSERT_FLOAT_EQ(result.data[3], 7.5f);

    ASSERT_FLOAT_EQ(result.data[4], 9.5f);
    ASSERT_FLOAT_EQ(result.data[5], 11.5f);
    ASSERT_FLOAT_EQ(result.data[6], 13.5f);
    ASSERT_FLOAT_EQ(result.data[7], 15.5f);

    ASSERT_FLOAT_EQ(result.data[8], 17.5f);
    ASSERT_FLOAT_EQ(result.data[9], 19.5f);
    ASSERT_FLOAT_EQ(result.data[10], 21.5f);
    ASSERT_FLOAT_EQ(result.data[11], 23.5f);

    ASSERT_FLOAT_EQ(result.data[12], 25.5f);
    ASSERT_FLOAT_EQ(result.data[13], 27.5f);
    ASSERT_FLOAT_EQ(result.data[14], 29.5f);
    ASSERT_FLOAT_EQ(result.data[15], 31.5f);
}

/**
 * @tc.name: Matrix4x4OperatorAdditionAssignment
 * @tc.desc: Tests for Matrix4X4 Operator Addition Assignment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x4operators, Matrix4x4OperatorAdditionAssignment, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1A = Math::Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    constexpr Math::Vec4 col2A = Math::Vec4(5.0f, 6.0f, 7.0f, 8.0f);
    constexpr Math::Vec4 col3A = Math::Vec4(9.0f, 10.0f, 11.0f, 12.0f);
    constexpr Math::Vec4 col4A = Math::Vec4(13.0f, 14.0f, 15.0f, 16.0f);
    Math::Mat4X4 matA = Math::Mat4X4(col1A, col2A, col3A, col4A);

    constexpr Math::Vec4 col1B = Math::Vec4(0.5f, 1.5f, 2.5f, 3.5f);
    constexpr Math::Vec4 col2B = Math::Vec4(4.5f, 5.5f, 6.5f, 7.5f);
    constexpr Math::Vec4 col3B = Math::Vec4(8.5f, 9.5f, 10.5f, 11.5f);
    constexpr Math::Vec4 col4B = Math::Vec4(12.5f, 13.5f, 14.5f, 15.5f);
    constexpr Math::Mat4X4 matB = Math::Mat4X4(col1B, col2B, col3B, col4B);

    matA += matB;

    ASSERT_FLOAT_EQ(matA.data[0], 1.5f);
    ASSERT_FLOAT_EQ(matA.data[1], 3.5f);
    ASSERT_FLOAT_EQ(matA.data[2], 5.5f);
    ASSERT_FLOAT_EQ(matA.data[3], 7.5f);

    ASSERT_FLOAT_EQ(matA.data[4], 9.5f);
    ASSERT_FLOAT_EQ(matA.data[5], 11.5f);
    ASSERT_FLOAT_EQ(matA.data[6], 13.5f);
    ASSERT_FLOAT_EQ(matA.data[7], 15.5f);

    ASSERT_FLOAT_EQ(matA.data[8], 17.5f);
    ASSERT_FLOAT_EQ(matA.data[9], 19.5f);
    ASSERT_FLOAT_EQ(matA.data[10], 21.5f);
    ASSERT_FLOAT_EQ(matA.data[11], 23.5f);

    ASSERT_FLOAT_EQ(matA.data[12], 25.5f);
    ASSERT_FLOAT_EQ(matA.data[13], 27.5f);
    ASSERT_FLOAT_EQ(matA.data[14], 29.5f);
    ASSERT_FLOAT_EQ(matA.data[15], 31.5f);
}

/**
 * @tc.name: Matrix4x4OperatorSubtraction
 * @tc.desc: Tests for Matrix4X4 Operator Subtraction. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x4operators, Matrix4x4OperatorSubtraction, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1A = Math::Vec4(10.0f, -5.0f, 0.0f, 2.5f);
    constexpr Math::Vec4 col2A = Math::Vec4(7.0f, 8.5f, -1.0f, 4.0f);
    constexpr Math::Vec4 col3A = Math::Vec4(3.0f, 2.0f, 1.0f, 0.0f);
    constexpr Math::Vec4 col4A = Math::Vec4(-2.0f, 5.0f, 6.0f, 7.0f);
    constexpr Math::Mat4X4 matA = Math::Mat4X4(col1A, col2A, col3A, col4A);

    constexpr Math::Vec4 col1B = Math::Vec4(1.0f, -3.0f, 0.0f, 2.5f);
    constexpr Math::Vec4 col2B = Math::Vec4(4.0f, 2.5f, 1.0f, -1.0f);
    constexpr Math::Vec4 col3B = Math::Vec4(0.5f, 0.5f, 0.5f, 0.5f);
    constexpr Math::Vec4 col4B = Math::Vec4(-1.0f, 3.0f, 4.0f, 5.0f);
    constexpr Math::Mat4X4 matB = Math::Mat4X4(col1B, col2B, col3B, col4B);

    const Math::Mat4X4 result = matA - matB;

    ASSERT_FLOAT_EQ(result.data[0], 9.0f);
    ASSERT_FLOAT_EQ(result.data[1], -2.0f);
    ASSERT_FLOAT_EQ(result.data[2], 0.0f);
    ASSERT_FLOAT_EQ(result.data[3], 0.0f);

    ASSERT_FLOAT_EQ(result.data[4], 3.0f);
    ASSERT_FLOAT_EQ(result.data[5], 6.0f);
    ASSERT_FLOAT_EQ(result.data[6], -2.0f);
    ASSERT_FLOAT_EQ(result.data[7], 5.0f);

    ASSERT_FLOAT_EQ(result.data[8], 2.5f);
    ASSERT_FLOAT_EQ(result.data[9], 1.5f);
    ASSERT_FLOAT_EQ(result.data[10], 0.5f);
    ASSERT_FLOAT_EQ(result.data[11], -0.5f);

    ASSERT_FLOAT_EQ(result.data[12], -1.0f);
    ASSERT_FLOAT_EQ(result.data[13], 2.0f);
    ASSERT_FLOAT_EQ(result.data[14], 2.0f);
    ASSERT_FLOAT_EQ(result.data[15], 2.0f);
}

/**
 * @tc.name: Matrix4x4OperatorSubtractionAssignment
 * @tc.desc: Tests for Matrix4X4 Operator Subtraction Assignment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x4operators, Matrix4x4OperatorSubtractionAssignment, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1A = Math::Vec4(10.0f, -5.0f, 0.0f, 2.5f);
    constexpr Math::Vec4 col2A = Math::Vec4(7.0f, 8.5f, -1.0f, 4.0f);
    constexpr Math::Vec4 col3A = Math::Vec4(3.0f, 2.0f, 1.0f, 0.0f);
    constexpr Math::Vec4 col4A = Math::Vec4(-2.0f, 5.0f, 6.0f, 7.0f);
    Math::Mat4X4 matA = Math::Mat4X4(col1A, col2A, col3A, col4A);

    constexpr Math::Vec4 col1B = Math::Vec4(1.0f, -3.0f, 0.0f, 2.5f);
    constexpr Math::Vec4 col2B = Math::Vec4(4.0f, 2.5f, 1.0f, -1.0f);
    constexpr Math::Vec4 col3B = Math::Vec4(0.5f, 0.5f, 0.5f, 0.5f);
    constexpr Math::Vec4 col4B = Math::Vec4(-1.0f, 3.0f, 4.0f, 5.0f);
    constexpr Math::Mat4X4 matB = Math::Mat4X4(col1B, col2B, col3B, col4B);

    matA -= matB;

    ASSERT_FLOAT_EQ(matA.data[0], 9.0f);
    ASSERT_FLOAT_EQ(matA.data[1], -2.0f);
    ASSERT_FLOAT_EQ(matA.data[2], 0.0f);
    ASSERT_FLOAT_EQ(matA.data[3], 0.0f);

    ASSERT_FLOAT_EQ(matA.data[4], 3.0f);
    ASSERT_FLOAT_EQ(matA.data[5], 6.0f);
    ASSERT_FLOAT_EQ(matA.data[6], -2.0f);
    ASSERT_FLOAT_EQ(matA.data[7], 5.0f);

    ASSERT_FLOAT_EQ(matA.data[8], 2.5f);
    ASSERT_FLOAT_EQ(matA.data[9], 1.5f);
    ASSERT_FLOAT_EQ(matA.data[10], 0.5f);
    ASSERT_FLOAT_EQ(matA.data[11], -0.5f);

    ASSERT_FLOAT_EQ(matA.data[12], -1.0f);
    ASSERT_FLOAT_EQ(matA.data[13], 2.0f);
    ASSERT_FLOAT_EQ(matA.data[14], 2.0f);
    ASSERT_FLOAT_EQ(matA.data[15], 2.0f);
}

/**
 * @tc.name: Matrix4x4operatorMultiplication
 * @tc.desc: Tests for Matrix4X4Operator Multiplication. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x4operators, Matrix4x4operatorMultiplication, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 m1col1 = Math::Vec4(1.1f, 1.0f, 2.0f, 3.0f);
    constexpr Math::Vec4 m1col2 = Math::Vec4(0.1f, 1.1f, 2.1f, 3.1f);
    constexpr Math::Vec4 m1col3 = Math::Vec4(0.2f, 1.2f, 2.2f, 3.2f);
    constexpr Math::Vec4 m1col4 = Math::Vec4(0.3f, 1.3f, 2.3f, 3.3f);
    constexpr Math::Mat4X4 mat = Math::Mat4X4(m1col1, m1col2, m1col3, m1col4);

    constexpr Math::Vec4 m2col1 = Math::Vec4(5.0f, 2.0f, 2.2f, 3.1f);
    constexpr Math::Vec4 m2col2 = Math::Vec4(0.1f, 1.1f, 2.1f, 3.1f);
    constexpr Math::Vec4 m2col3 = Math::Vec4(0.2f, 1.2f, 2.2f, 3.2f);
    constexpr Math::Vec4 m2col4 = Math::Vec4(0.3f, 1.3f, 2.3f, 3.3f);
    constexpr Math::Mat4X4 mat2nd = Math::Mat4X4(m2col1, m2col2, m2col3, m2col4);

    const Math::Mat4X4 mat3rd = mat * mat2nd;

    ASSERT_FLOAT_EQ(mat3rd.data[0], 7.07f);
    ASSERT_FLOAT_EQ(mat3rd.data[1], 13.87f);
    ASSERT_FLOAT_EQ(mat3rd.data[2], 26.17f);
    ASSERT_FLOAT_EQ(mat3rd.data[3], 38.47f);

    ASSERT_FLOAT_EQ(mat3rd.data[4], 1.57f);
    ASSERT_FLOAT_EQ(mat3rd.data[5], 7.86f);
    ASSERT_FLOAT_EQ(mat3rd.data[6], 14.26f);
    ASSERT_FLOAT_EQ(mat3rd.data[7], 20.66f);

    ASSERT_FLOAT_EQ(mat3rd.data[8], 1.74f);
    ASSERT_FLOAT_EQ(mat3rd.data[9], 8.32f);
    ASSERT_FLOAT_EQ(mat3rd.data[10], 15.12f);
    ASSERT_FLOAT_EQ(mat3rd.data[11], 21.92f);

    ASSERT_FLOAT_EQ(mat3rd.data[12], 1.91f);
    ASSERT_FLOAT_EQ(mat3rd.data[13], 8.78f);
    ASSERT_FLOAT_EQ(mat3rd.data[14], 15.98f);
    ASSERT_FLOAT_EQ(mat3rd.data[15], 23.18f);
}

/**
 * @tc.name: Matrix4x4operatorMultiplicationWithScalar
 * @tc.desc: Tests for Matrix4X4Operator Multiplication With Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x4operators, Matrix4x4operatorMultiplicationWithScalar, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1 = Math::Vec4(0.0f, 1.0f, 2.0f, 3.0f);
    constexpr Math::Vec4 col2 = Math::Vec4(0.1f, 1.1f, 2.1f, 3.1f);
    constexpr Math::Vec4 col3 = Math::Vec4(0.2f, 1.2f, 2.2f, 3.2f);
    constexpr Math::Vec4 col4 = Math::Vec4(0.3f, 1.3f, 2.3f, 3.3f);
    constexpr Math::Mat4X4 mat = Math::Mat4X4(col1, col2, col3, col4);

    constexpr float multiplier = 2.0f;
    constexpr Math::Mat4X4 result = mat * multiplier;

    ASSERT_FLOAT_EQ(result.data[0], multiplier * 0.0f);
    ASSERT_FLOAT_EQ(result.data[1], multiplier * 1.0f);
    ASSERT_FLOAT_EQ(result.data[2], multiplier * 2.0f);
    ASSERT_FLOAT_EQ(result.data[3], multiplier * 3.0f);

    ASSERT_FLOAT_EQ(result.data[4], multiplier * 0.1f);
    ASSERT_FLOAT_EQ(result.data[5], multiplier * 1.1f);
    ASSERT_FLOAT_EQ(result.data[6], multiplier * 2.1f);
    ASSERT_FLOAT_EQ(result.data[7], multiplier * 3.1f);

    ASSERT_FLOAT_EQ(result.data[8], multiplier * 0.2f);
    ASSERT_FLOAT_EQ(result.data[9], multiplier * 1.2f);
    ASSERT_FLOAT_EQ(result.data[10], multiplier * 2.2f);
    ASSERT_FLOAT_EQ(result.data[11], multiplier * 3.2f);

    ASSERT_FLOAT_EQ(result.data[12], multiplier * 0.3f);
    ASSERT_FLOAT_EQ(result.data[13], multiplier * 1.3f);
    ASSERT_FLOAT_EQ(result.data[14], multiplier * 2.3f);
    ASSERT_FLOAT_EQ(result.data[15], multiplier * 3.3f);
}

#ifdef DISABLED_TESTS_ON
/**
 * @tc.name: VectorMultipliedByMatrix4x4
 * @tc.desc: Tests for Vector Multiplied By Matrix4X4. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x4operators, DISABLED_VectorMultipliedByMatrix4x4, testing::ext::TestSize.Level1)
{
    // This test was here to test against GLM matrix implementation that
    // has been removed as it was an unneeded extra dependency.
}
#endif

/**
 * @tc.name: Matrix4x4operatorAssigment
 * @tc.desc: Tests for Matrix4X4Operator Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x4operators, Matrix4x4operatorAssigment, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1 = Math::Vec4(0.0f, 1.0f, 2.0f, 3.0f);
    constexpr Math::Vec4 col2 = Math::Vec4(0.1f, 1.1f, 2.1f, 3.1f);
    constexpr Math::Vec4 col3 = Math::Vec4(0.2f, 1.2f, 2.2f, 3.2f);
    constexpr Math::Vec4 col4 = Math::Vec4(0.3f, 1.3f, 2.3f, 3.3f);
    constexpr Math::Mat4X4 mat = Math::Mat4X4(col1, col2, col3, col4);

    constexpr Math::Vec4 C1 = { 1.0f, 0.0f, 0.0f, 0.0f };
    constexpr Math::Vec4 C2 = { 0.0f, 1.0f, 0.0f, 0.0f };
    constexpr Math::Vec4 C3 = { 0.0f, 0.0f, 1.0f, 0.0f };
    constexpr Math::Vec4 C4 = { 0.0f, 0.0f, 0.0f, 1.0f };
    constexpr Math::Mat4X4 identity(C1, C2, C3, C4);

    Math::Mat4X4 mat2nd(identity);

    mat2nd = mat;

    ASSERT_FLOAT_EQ(mat2nd.data[0], 0.0f);
    ASSERT_FLOAT_EQ(mat2nd.data[1], 1.0f);
    ASSERT_FLOAT_EQ(mat2nd.data[2], 2.0f);
    ASSERT_FLOAT_EQ(mat2nd.data[3], 3.0f);

    ASSERT_FLOAT_EQ(mat2nd.data[4], 0.1f);
    ASSERT_FLOAT_EQ(mat2nd.data[5], 1.1f);
    ASSERT_FLOAT_EQ(mat2nd.data[6], 2.1f);
    ASSERT_FLOAT_EQ(mat2nd.data[7], 3.1f);

    ASSERT_FLOAT_EQ(mat2nd.data[8], 0.2f);
    ASSERT_FLOAT_EQ(mat2nd.data[9], 1.2f);
    ASSERT_FLOAT_EQ(mat2nd.data[10], 2.2f);
    ASSERT_FLOAT_EQ(mat2nd.data[11], 3.2f);

    ASSERT_FLOAT_EQ(mat2nd.data[12], 0.3f);
    ASSERT_FLOAT_EQ(mat2nd.data[13], 1.3f);
    ASSERT_FLOAT_EQ(mat2nd.data[14], 2.3f);
    ASSERT_FLOAT_EQ(mat2nd.data[15], 3.3f);
}

/**
 * @tc.name: Identity
 * @tc.desc: Tests for Identity. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x4, Identity, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 C1 = { 1.0f, 0.0f, 0.0f, 0.0f };
    constexpr Math::Vec4 C2 = { 0.0f, 1.0f, 0.0f, 0.0f };
    constexpr Math::Vec4 C3 = { 0.0f, 0.0f, 1.0f, 0.0f };
    constexpr Math::Vec4 C4 = { 0.0f, 0.0f, 0.0f, 1.0f };
    constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
    constexpr Math::Mat4X4 mat = identity;

    ASSERT_FLOAT_EQ(mat.data[0], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);

    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[5], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);

    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[10], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[11], 0.0f);

    ASSERT_FLOAT_EQ(mat.data[12], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[13], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[14], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[15], 1.0f);
}

/**
 * @tc.name: Default
 * @tc.desc: Tests for Default. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x3constructors, Default, testing::ext::TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE([[maybe_unused]] Math::Mat4X3 mat = Math::Mat4X3());
}

/**
 * @tc.name: Initializer
 * @tc.desc: Tests for Initializer. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x3constructors, Initializer, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1 = Math::Vec3(0.0f, 1.0f, 2.0f);
    constexpr Math::Vec3 col2 = Math::Vec3(0.1f, 1.1f, 2.1f);
    constexpr Math::Vec3 col3 = Math::Vec3(0.2f, 1.2f, 2.2f);
    constexpr Math::Vec3 col4 = Math::Vec3(0.3f, 1.3f, 2.3f);
    constexpr Math::Mat4X3 mat = Math::Mat4X3(col1, col2, col3, col4);

    ASSERT_FLOAT_EQ(mat.data[0], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[1], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 2.0f);

    ASSERT_FLOAT_EQ(mat.data[3], 0.1f);
    ASSERT_FLOAT_EQ(mat.data[4], 1.1f);
    ASSERT_FLOAT_EQ(mat.data[5], 2.1f);

    ASSERT_FLOAT_EQ(mat.data[6], 0.2f);
    ASSERT_FLOAT_EQ(mat.data[7], 1.2f);
    ASSERT_FLOAT_EQ(mat.data[8], 2.2f);

    ASSERT_FLOAT_EQ(mat.data[9], 0.3f);
    ASSERT_FLOAT_EQ(mat.data[10], 1.3f);
    ASSERT_FLOAT_EQ(mat.data[11], 2.3f);
}

/**
 * @tc.name: Matrix4x3operatorMultiplicationWithScalar
 * @tc.desc: Tests for Matrix4X3Operator Multiplication With Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x3operators, Matrix4x3operatorMultiplicationWithScalar, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1 = Math::Vec3(0.0f, 1.0f, 2.0f);
    constexpr Math::Vec3 col2 = Math::Vec3(0.1f, 1.1f, 2.1f);
    constexpr Math::Vec3 col3 = Math::Vec3(0.2f, 1.2f, 2.2f);
    constexpr Math::Vec3 col4 = Math::Vec3(0.3f, 1.3f, 2.3f);
    constexpr Math::Mat4X3 mat = Math::Mat4X3(col1, col2, col3, col4);

    constexpr float multiplier = 2.0f;
    constexpr Math::Mat4X3 result = mat * multiplier;

    ASSERT_FLOAT_EQ(result.data[0], multiplier * 0.0f);
    ASSERT_FLOAT_EQ(result.data[1], multiplier * 1.0f);
    ASSERT_FLOAT_EQ(result.data[2], multiplier * 2.0f);

    ASSERT_FLOAT_EQ(result.data[3], multiplier * 0.1f);
    ASSERT_FLOAT_EQ(result.data[4], multiplier * 1.1f);
    ASSERT_FLOAT_EQ(result.data[5], multiplier * 2.1f);

    ASSERT_FLOAT_EQ(result.data[6], multiplier * 0.2f);
    ASSERT_FLOAT_EQ(result.data[7], multiplier * 1.2f);
    ASSERT_FLOAT_EQ(result.data[8], multiplier * 2.2f);

    ASSERT_FLOAT_EQ(result.data[9], multiplier * 0.3f);
    ASSERT_FLOAT_EQ(result.data[10], multiplier * 1.3f);
    ASSERT_FLOAT_EQ(result.data[11], multiplier * 2.3f);
}

/**
 * @tc.name: Matrix4x3operatorAssigment
 * @tc.desc: Tests for Matrix4X3Operator Assigment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x3operators, Matrix4x3operatorAssigment, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1 = Math::Vec3(0.0f, 1.0f, 2.0f);
    constexpr Math::Vec3 col2 = Math::Vec3(0.1f, 1.1f, 2.1f);
    constexpr Math::Vec3 col3 = Math::Vec3(0.2f, 1.2f, 2.2f);
    constexpr Math::Vec3 col4 = Math::Vec3(0.3f, 1.3f, 2.3f);
    constexpr Math::Mat4X3 mat = Math::Mat4X3(col1, col2, col3, col4);

    constexpr Math::Vec3 C1 = { 1.0f, 0.0f, 0.0f };
    constexpr Math::Vec3 C2 = { 0.0f, 1.0f, 0.0f };
    constexpr Math::Vec3 C3 = { 0.0f, 0.0f, 1.0f };
    constexpr Math::Vec3 C4 = { 0.0f, 0.0f, 0.0f };
    constexpr Math::Mat4X3 identity(C1, C2, C3, C4);

    Math::Mat4X3 mat2nd(identity);

    mat2nd = mat;

    ASSERT_FLOAT_EQ(mat2nd.data[0], 0.0f);
    ASSERT_FLOAT_EQ(mat2nd.data[1], 1.0f);
    ASSERT_FLOAT_EQ(mat2nd.data[2], 2.0f);

    ASSERT_FLOAT_EQ(mat2nd.data[3], 0.1f);
    ASSERT_FLOAT_EQ(mat2nd.data[4], 1.1f);
    ASSERT_FLOAT_EQ(mat2nd.data[5], 2.1f);

    ASSERT_FLOAT_EQ(mat2nd.data[6], 0.2f);
    ASSERT_FLOAT_EQ(mat2nd.data[7], 1.2f);
    ASSERT_FLOAT_EQ(mat2nd.data[8], 2.2f);

    ASSERT_FLOAT_EQ(mat2nd.data[9], 0.3f);
    ASSERT_FLOAT_EQ(mat2nd.data[10], 1.3f);
    ASSERT_FLOAT_EQ(mat2nd.data[11], 2.3f);
}

/**
 * @tc.name: Identity
 * @tc.desc: Tests for Identity. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrix4x3, Identity, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 C1 = { 1.0f, 0.0f, 0.0f };
    constexpr Math::Vec3 C2 = { 0.0f, 1.0f, 0.0f };
    constexpr Math::Vec3 C3 = { 0.0f, 0.0f, 1.0f };
    constexpr Math::Vec3 C4 = { 0.0f, 0.0f, 0.0f };
    constexpr Math::Mat4X3 identity(C1, C2, C3, C4);
    constexpr Math::Mat4X3 mat = identity;

    ASSERT_FLOAT_EQ(mat.data[0], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);

    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[5], 0.0f);

    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 1.0f);

    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[10], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[11], 0.0f);
}

// MatrixUtil

/**
 * @tc.name: Transpose
 * @tc.desc: Tests for Transpose. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, Transpose, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1(1.0f, 2.0f, 3.0f, 4.0f);
    constexpr Math::Vec4 col2(5.0f, 6.0f, 7.0f, 8.0f);
    constexpr Math::Vec4 col3(9.0f, 10.0f, 11.0f, 12.0f);
    constexpr Math::Vec4 col4(13.0f, 14.0f, 15.0f, 16.0f);

    Math::Mat4X4 mat;
    mat.base[0] = col1;
    mat.base[1] = col2;
    mat.base[2] = col3;
    mat.base[3] = col4;

    const Math::Mat4X4 transposeMat = Math::Transpose(mat);

    ASSERT_FLOAT_EQ(transposeMat.data[0], 1.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[1], 5.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[2], 9.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[3], 13.0f);

    ASSERT_FLOAT_EQ(transposeMat.data[4], 2.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[5], 6.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[6], 10.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[7], 14.0f);

    ASSERT_FLOAT_EQ(transposeMat.data[8], 3.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[9], 7.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[10], 11.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[11], 15.0f);

    ASSERT_FLOAT_EQ(transposeMat.data[12], 4.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[13], 8.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[14], 12.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[15], 16.0f);
}

/**
 * @tc.name: TransposeM3
 * @tc.desc: Tests for Transpose M3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, TransposeM3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1(1.0f, 2.0f, 3.0f);
    constexpr Math::Vec3 col2(5.0f, 6.0f, 7.0f);
    constexpr Math::Vec3 col3(9.0f, 10.0f, 11.0f);

    Math::Mat3X3 mat;
    mat.base[0] = col1;
    mat.base[1] = col2;
    mat.base[2] = col3;

    const Math::Mat3X3 transposeMat = Math::Transpose(mat);

    ASSERT_FLOAT_EQ(transposeMat.data[0], 1.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[1], 5.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[2], 9.0f);

    ASSERT_FLOAT_EQ(transposeMat.data[3], 2.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[4], 6.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[5], 10.0f);

    ASSERT_FLOAT_EQ(transposeMat.data[6], 3.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[7], 7.0f);
    ASSERT_FLOAT_EQ(transposeMat.data[8], 11.0f);
}

/**
 * @tc.name: Scale
 * @tc.desc: Tests for Scale. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, Scale, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1 = Math::Vec4(0.0f, 1.0f, 2.0f, 3.0f);
    constexpr Math::Vec4 col2 = Math::Vec4(0.1f, 1.1f, 2.1f, 3.1f);
    constexpr Math::Vec4 col3 = Math::Vec4(0.2f, 1.2f, 2.2f, 3.2f);
    constexpr Math::Vec4 col4 = Math::Vec4(0.3f, 1.3f, 2.3f, 3.3f);
    Math::Mat4X4 mat = Math::Mat4X4(col1, col2, col3, col4);

    constexpr Math::Vec3 scaler(1.5f, 2.5f, 3.5f);

    mat = Math::Scale(mat, scaler);

    ASSERT_FLOAT_EQ(mat.data[0], 1.5f * 0.0f);
    ASSERT_FLOAT_EQ(mat.data[1], 1.5f * 1.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 1.5f * 2.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 1.5f * 3.0f);

    ASSERT_FLOAT_EQ(mat.data[4], 2.5f * 0.1f);
    ASSERT_FLOAT_EQ(mat.data[5], 2.5f * 1.1f);
    ASSERT_FLOAT_EQ(mat.data[6], 2.5f * 2.1f);
    ASSERT_FLOAT_EQ(mat.data[7], 2.5f * 3.1f);

    ASSERT_FLOAT_EQ(mat.data[8], 3.5f * 0.2f);
    ASSERT_FLOAT_EQ(mat.data[9], 3.5f * 1.2f);
    ASSERT_FLOAT_EQ(mat.data[10], 3.5f * 2.2f);
    ASSERT_FLOAT_EQ(mat.data[11], 3.5f * 3.2f);

    ASSERT_FLOAT_EQ(mat.data[12], 0.3f);
    ASSERT_FLOAT_EQ(mat.data[13], 1.3f);
    ASSERT_FLOAT_EQ(mat.data[14], 2.3f);
    ASSERT_FLOAT_EQ(mat.data[15], 3.3f);
}

/**
 * @tc.name: PostScale
 * @tc.desc: Tests for Post Scale. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, PostScale, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1 = Math::Vec4(0.0f, 1.0f, 2.0f, 3.0f);
    constexpr Math::Vec4 col2 = Math::Vec4(0.1f, 1.1f, 2.1f, 3.1f);
    constexpr Math::Vec4 col3 = Math::Vec4(0.2f, 1.2f, 2.2f, 3.2f);
    constexpr Math::Vec4 col4 = Math::Vec4(0.3f, 1.3f, 2.3f, 3.3f);
    Math::Mat4X4 mat = Math::Mat4X4(col1, col2, col3, col4);

    constexpr Math::Vec3 scaler(1.5f, 2.5f, 3.5f);

    mat = Math::PostScale(mat, scaler);

    ASSERT_FLOAT_EQ(mat.data[0], 1.5f * 0.0f);
    ASSERT_FLOAT_EQ(mat.data[1], 2.5f * 1.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 3.5f * 2.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 3.0f);

    ASSERT_FLOAT_EQ(mat.data[4], 1.5f * 0.1f);
    ASSERT_FLOAT_EQ(mat.data[5], 2.5f * 1.1f);
    ASSERT_FLOAT_EQ(mat.data[6], 3.5f * 2.1f);
    ASSERT_FLOAT_EQ(mat.data[7], 3.1f);

    ASSERT_FLOAT_EQ(mat.data[8], 1.5f * 0.2f);
    ASSERT_FLOAT_EQ(mat.data[9], 2.5f * 1.2f);
    ASSERT_FLOAT_EQ(mat.data[10], 3.5f * 2.2f);
    ASSERT_FLOAT_EQ(mat.data[11], 3.2f);

    ASSERT_FLOAT_EQ(mat.data[12], 1.5f * 0.3f);
    ASSERT_FLOAT_EQ(mat.data[13], 2.5f * 1.3f);
    ASSERT_FLOAT_EQ(mat.data[14], 3.5f * 2.3f);
    ASSERT_FLOAT_EQ(mat.data[15], 3.3f);
}

/**
 * @tc.name: ScaleM3
 * @tc.desc: Tests for Scale M3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, ScaleM3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1 = Math::Vec3(0.0f, 1.0f, 2.0f);
    constexpr Math::Vec3 col2 = Math::Vec3(0.1f, 1.1f, 2.1f);
    constexpr Math::Vec3 col3 = Math::Vec3(0.2f, 1.2f, 2.2f);
    Math::Mat3X3 mat = Math::Mat3X3(col1, col2, col3);

    constexpr Math::Vec2 scaler(1.5f, 2.5f);

    mat = Math::Scale(mat, scaler);

    ASSERT_FLOAT_EQ(mat.data[0], 1.5f * 0.0f);
    ASSERT_FLOAT_EQ(mat.data[1], 1.5f * 1.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 1.5f * 2.0f);

    ASSERT_FLOAT_EQ(mat.data[3], 2.5f * 0.1f);
    ASSERT_FLOAT_EQ(mat.data[4], 2.5f * 1.1f);
    ASSERT_FLOAT_EQ(mat.data[5], 2.5f * 2.1f);

    ASSERT_FLOAT_EQ(mat.data[6], 0.2f);
    ASSERT_FLOAT_EQ(mat.data[7], 1.2f);
    ASSERT_FLOAT_EQ(mat.data[8], 2.2f);
}

/**
 * @tc.name: PostScaleM3
 * @tc.desc: Tests for Post Scale M3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, PostScaleM3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1 = Math::Vec3(0.0f, 1.0f, 2.0f);
    constexpr Math::Vec3 col2 = Math::Vec3(0.1f, 1.1f, 2.1f);
    constexpr Math::Vec3 col3 = Math::Vec3(0.2f, 1.2f, 2.2f);
    Math::Mat3X3 mat = Math::Mat3X3(col1, col2, col3);

    constexpr Math::Vec2 scaler(1.5f, 2.5f);

    mat = Math::PostScale(mat, scaler);

    ASSERT_FLOAT_EQ(mat.data[0], 1.5f * 0.0f);
    ASSERT_FLOAT_EQ(mat.data[1], 2.5f * 1.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 2.0f);

    ASSERT_FLOAT_EQ(mat.data[3], 1.5f * 0.1f);
    ASSERT_FLOAT_EQ(mat.data[4], 2.5f * 1.1f);
    ASSERT_FLOAT_EQ(mat.data[5], 2.1f);

    ASSERT_FLOAT_EQ(mat.data[6], 1.5f * 0.2f);
    ASSERT_FLOAT_EQ(mat.data[7], 2.5f * 1.2f);
    ASSERT_FLOAT_EQ(mat.data[8], 2.2f);
}

/**
 * @tc.name: GetColumn
 * @tc.desc: Tests for Get Column. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, GetColumn, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1 = Math::Vec4(0.0f, 1.0f, 2.0f, 3.0f);
    constexpr Math::Vec4 col2 = Math::Vec4(0.1f, 1.1f, 2.1f, 3.1f);
    constexpr Math::Vec4 col3 = Math::Vec4(0.2f, 1.2f, 2.2f, 3.2f);
    constexpr Math::Vec4 col4 = Math::Vec4(0.3f, 1.3f, 2.3f, 3.3f);
    constexpr Math::Mat4X4 mat = Math::Mat4X4(col1, col2, col3, col4);

    Math::Vec4 temp = Math::GetColumn(mat, 0);
    ASSERT_FLOAT_EQ(temp.data[0], 0.0f);
    ASSERT_FLOAT_EQ(temp.data[1], 1.0f);
    ASSERT_FLOAT_EQ(temp.data[2], 2.0f);
    ASSERT_FLOAT_EQ(temp.data[3], 3.0f);

    temp = Math::GetColumn(mat, 1);
    ASSERT_FLOAT_EQ(temp.data[0], 0.1f);
    ASSERT_FLOAT_EQ(temp.data[1], 1.1f);
    ASSERT_FLOAT_EQ(temp.data[2], 2.1f);
    ASSERT_FLOAT_EQ(temp.data[3], 3.1f);

    temp = Math::GetColumn(mat, 2);
    ASSERT_FLOAT_EQ(temp.data[0], 0.2f);
    ASSERT_FLOAT_EQ(temp.data[1], 1.2f);
    ASSERT_FLOAT_EQ(temp.data[2], 2.2f);
    ASSERT_FLOAT_EQ(temp.data[3], 3.2f);

    temp = Math::GetColumn(mat, 3);
    ASSERT_FLOAT_EQ(temp.data[0], 0.3f);
    ASSERT_FLOAT_EQ(temp.data[1], 1.3f);
    ASSERT_FLOAT_EQ(temp.data[2], 2.3f);
    ASSERT_FLOAT_EQ(temp.data[3], 3.3f);

    temp = Math::GetColumn(mat, 10);
    ASSERT_FLOAT_EQ(temp.data[0], 0.0f);
    ASSERT_FLOAT_EQ(temp.data[1], 0.0f);
    ASSERT_FLOAT_EQ(temp.data[2], 0.0f);
    ASSERT_FLOAT_EQ(temp.data[3], 0.0f);
}

/**
 * @tc.name: GetColumnM3
 * @tc.desc: Tests for Get Column M3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, GetColumnM3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1 = Math::Vec3(0.0f, 1.0f, 2.0f);
    constexpr Math::Vec3 col2 = Math::Vec3(0.1f, 1.1f, 2.1f);
    constexpr Math::Vec3 col3 = Math::Vec3(0.2f, 1.2f, 2.2f);
    constexpr Math::Mat3X3 mat = Math::Mat3X3(col1, col2, col3);

    Math::Vec3 temp = Math::GetColumn(mat, 0);
    ASSERT_FLOAT_EQ(temp.data[0], 0.0f);
    ASSERT_FLOAT_EQ(temp.data[1], 1.0f);
    ASSERT_FLOAT_EQ(temp.data[2], 2.0f);

    temp = Math::GetColumn(mat, 1);
    ASSERT_FLOAT_EQ(temp.data[0], 0.1f);
    ASSERT_FLOAT_EQ(temp.data[1], 1.1f);
    ASSERT_FLOAT_EQ(temp.data[2], 2.1f);

    temp = Math::GetColumn(mat, 2);
    ASSERT_FLOAT_EQ(temp.data[0], 0.2f);
    ASSERT_FLOAT_EQ(temp.data[1], 1.2f);
    ASSERT_FLOAT_EQ(temp.data[2], 2.2f);

    temp = Math::GetColumn(mat, 10);

    ASSERT_FLOAT_EQ(temp.data[0], 0.0f);
    ASSERT_FLOAT_EQ(temp.data[1], 0.0f);
    ASSERT_FLOAT_EQ(temp.data[2], 0.0f);
}

/**
 * @tc.name: GetRow
 * @tc.desc: Tests for Get Row. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, GetRow, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1 = Math::Vec4(0.0f, 1.0f, 2.0f, 3.0f);
    constexpr Math::Vec4 col2 = Math::Vec4(0.1f, 1.1f, 2.1f, 3.1f);
    constexpr Math::Vec4 col3 = Math::Vec4(0.2f, 1.2f, 2.2f, 3.2f);
    constexpr Math::Vec4 col4 = Math::Vec4(0.3f, 1.3f, 2.3f, 3.3f);
    constexpr Math::Mat4X4 mat = Math::Mat4X4(col1, col2, col3, col4);

    Math::Vec4 temp = Math::GetRow(mat, 0);
    ASSERT_FLOAT_EQ(temp.data[0], 0.0f);
    ASSERT_FLOAT_EQ(temp.data[1], 0.1f);
    ASSERT_FLOAT_EQ(temp.data[2], 0.2f);
    ASSERT_FLOAT_EQ(temp.data[3], 0.3f);

    temp = Math::GetRow(mat, 1);
    ASSERT_FLOAT_EQ(temp.data[0], 1.0f);
    ASSERT_FLOAT_EQ(temp.data[1], 1.1f);
    ASSERT_FLOAT_EQ(temp.data[2], 1.2f);
    ASSERT_FLOAT_EQ(temp.data[3], 1.3f);

    temp = Math::GetRow(mat, 2);
    ASSERT_FLOAT_EQ(temp.data[0], 2.0f);
    ASSERT_FLOAT_EQ(temp.data[1], 2.1f);
    ASSERT_FLOAT_EQ(temp.data[2], 2.2f);
    ASSERT_FLOAT_EQ(temp.data[3], 2.3f);

    temp = Math::GetRow(mat, 3);
    ASSERT_FLOAT_EQ(temp.data[0], 3.0f);
    ASSERT_FLOAT_EQ(temp.data[1], 3.1f);
    ASSERT_FLOAT_EQ(temp.data[2], 3.2f);
    ASSERT_FLOAT_EQ(temp.data[3], 3.3f);

    temp = Math::GetRow(mat, 10);

    ASSERT_FLOAT_EQ(temp.data[0], 0.0f);
    ASSERT_FLOAT_EQ(temp.data[1], 0.0f);
    ASSERT_FLOAT_EQ(temp.data[2], 0.0f);
    ASSERT_FLOAT_EQ(temp.data[3], 0.0f);
}

/**
 * @tc.name: GetRowM3
 * @tc.desc: Tests for Get Row M3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, GetRowM3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1 = Math::Vec3(0.0f, 1.0f, 2.0f);
    constexpr Math::Vec3 col2 = Math::Vec3(0.1f, 1.1f, 2.1f);
    constexpr Math::Vec3 col3 = Math::Vec3(0.2f, 1.2f, 2.2f);
    constexpr Math::Mat3X3 mat = Math::Mat3X3(col1, col2, col3);

    Math::Vec3 temp = Math::GetRow(mat, 0);
    ASSERT_FLOAT_EQ(temp.data[0], 0.0f);
    ASSERT_FLOAT_EQ(temp.data[1], 0.1f);
    ASSERT_FLOAT_EQ(temp.data[2], 0.2f);

    temp = Math::GetRow(mat, 1);
    ASSERT_FLOAT_EQ(temp.data[0], 1.0f);
    ASSERT_FLOAT_EQ(temp.data[1], 1.1f);
    ASSERT_FLOAT_EQ(temp.data[2], 1.2f);

    temp = Math::GetRow(mat, 2);
    ASSERT_FLOAT_EQ(temp.data[0], 2.0f);
    ASSERT_FLOAT_EQ(temp.data[1], 2.1f);
    ASSERT_FLOAT_EQ(temp.data[2], 2.2f);

    temp = Math::GetRow(mat, 10);

    ASSERT_FLOAT_EQ(temp.data[0], 0.0f);
    ASSERT_FLOAT_EQ(temp.data[1], 0.0f);
    ASSERT_FLOAT_EQ(temp.data[2], 0.0f);
}

/**
 * @tc.name: DimensionShift
 * @tc.desc: Tests for Dimension Shift. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, DimensionShift, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1 = Math::Vec3(0.0f, 1.0f, 2.0f);
    constexpr Math::Vec3 col2 = Math::Vec3(0.1f, 1.1f, 2.1f);
    constexpr Math::Vec3 col3 = Math::Vec3(0.2f, 1.2f, 2.2f);
    constexpr Math::Mat3X3 mat = Math::Mat3X3(col1, col2, col3);
    Math::Mat4X4 temp = Math::DimensionalShift(mat);
    // Shifted by fist col line
    constexpr Math::Vec4 tCol1 = Math::Vec4(1.0f, 0.0f, 0.0f, 0.0f);
    constexpr Math::Vec4 tCol2 = Math::Vec4(0.0f, 0.0f, 1.0f, 2.0f);
    constexpr Math::Vec4 tCol3 = Math::Vec4(0.0f, 0.1f, 1.1f, 2.1f);
    constexpr Math::Vec4 tCol4 = Math::Vec4(0.0f, 0.2f, 1.2f, 2.2f);
    constexpr Math::Mat4X4 tMat = Math::Mat4X4(tCol1, tCol2, tCol3, tCol4);

    ASSERT_FLOAT_EQ(Math::Determinant(temp), Math::Determinant(tMat));
}

/**
 * @tc.name: mat3_cast
 * @tc.desc: Tests for Mat3Cast. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, mat3_cast, testing::ext::TestSize.Level1)
{
    Math::Quat quaternion(0.0f, 0.3f, 0.0f, 1.0f);
    const float l = Math::Length(quaternion);
    quaternion /= l;
    const Math::Mat3X3 fromQuatCast = Math::Mat3Cast(quaternion);

    // Checked against glm and is ok
    ASSERT_FLOAT_EQ(fromQuatCast.data[0], 0.83486235f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[1], 0.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[2], -0.55045867f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[3], 0.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[4], 1.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[5], 0.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[6], 0.55045867f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[7], 0.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[8], 0.83486235f);
}

/**
 * @tc.name: Mat4Cast
 * @tc.desc: Tests for Mat4Cast. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, Mat4Cast, testing::ext::TestSize.Level1)
{
    Math::Quat quaternion(0.0f, 0.3f, 0.0f, 1.0f);
    const float l = Math::Length(quaternion);
    quaternion /= l;
    const Math::Mat4X4 fromQuatCast = Math::Mat4Cast(quaternion);

    ASSERT_FLOAT_EQ(fromQuatCast.data[0], 0.83486235f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[1], 0.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[2], -0.55045867f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[3], 0.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[4], 0.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[5], 1.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[6], 0.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[7], 0.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[8], 0.55045867f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[9], 0.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[10], 0.83486235f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[11], 0.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[12], 0.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[13], 0.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[14], 0.0f);
    ASSERT_FLOAT_EQ(fromQuatCast.data[15], 1.0f);
}

/**
 * @tc.name: Translate
 * @tc.desc: Tests for Translate. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, Translate, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 C1 = { 1.0f, 0.0f, 0.0f, 2.0f };
    constexpr Math::Vec4 C2 = { 0.0f, 1.0f, 0.0f, 5.0f };
    constexpr Math::Vec4 C3 = { 0.0f, 0.0f, 1.0f, 6.0f };
    constexpr Math::Vec4 C4 = { 0.0f, 0.0f, 0.0f, 1.0f };
    constexpr Math::Mat4X4 identity(C1, C2, C3, C4);

    Math::Mat4X4 tMat(identity);

    constexpr Math::Vec3 translation(2.0f, 5.0f, 6.0f);

    Math::Mat4X4 mat = Math::Translate(Math::IDENTITY_4X4, translation);

    ASSERT_FLOAT_EQ(mat.data[0], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[5], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[10], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[11], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[12], 2.0f); // x
    ASSERT_FLOAT_EQ(mat.data[13], 5.0f); // y
    ASSERT_FLOAT_EQ(mat.data[14], 6.0f); // z
    ASSERT_FLOAT_EQ(mat.data[15], 1.0f);

    ASSERT_FLOAT_EQ(Math::Determinant(mat), Math::Determinant(tMat));
}

/**
 * @tc.name: Translate2x4
 * @tc.desc: Tests for Translate2X4. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, Translate2x4, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 C1 = { 1.0f, 0.0f, 0.0f, 0.0f };
    constexpr Math::Vec4 C2 = { 0.0f, 1.0f, 0.0f, 0.0f };
    constexpr Math::Vec4 C3 = { 0.0f, 0.0f, 1.0f, 0.0f };
    constexpr Math::Vec4 C4 = { 2.0f, 5.0f, 0.0f, 1.0f };
    constexpr Math::Mat4X4 tMat = Math::Mat4X4(C1, C2, C3, C4);

    constexpr Math::Vec2 translation(2.0f, 5.0f);

    Math::Mat4X4 mat = Math::Translate(Math::IDENTITY_4X4, translation);

    ASSERT_FLOAT_EQ(mat.data[0], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[5], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[10], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[11], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[12], 2.0f); // x
    ASSERT_FLOAT_EQ(mat.data[13], 5.0f); // y
    ASSERT_FLOAT_EQ(mat.data[14], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[15], 1.0f);

    ASSERT_FLOAT_EQ(Math::Determinant(mat), Math::Determinant(tMat));
}

/**
 * @tc.name: SkewXY2x4
 * @tc.desc: Tests for Skew Xy2X4. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, SkewXY2x4, testing::ext::TestSize.Level1)
{
    constexpr float skewX = 0.5f; // skew angle on x in radian
    constexpr float skewY = 0.8f; // skew angle on y in radian
    constexpr Math::Vec4 col1 = Math::Vec4(1.1f, 1.2f, 1.3f, 1.4f);
    constexpr Math::Vec4 col2 = Math::Vec4(2.1f, 2.2f, 2.3f, 2.4f);
    constexpr Math::Vec4 col3 = Math::Vec4(3.1f, 3.2f, 3.3f, 3.4f);
    constexpr Math::Vec4 col4 = Math::Vec4(4.1f, 4.2f, 4.3f, 4.4f);
    constexpr Math::Mat4X4 mat(col1, col2, col3, col4);

    const Math::Mat4X4 result = Math::SkewXY(mat, { skewX, skewY });

    const float tanX = tanf(skewX);
    const float tanY = tanf(skewY);

    ASSERT_FLOAT_EQ(result.data[0], 1.1f + 2.1f * tanY);
    ASSERT_FLOAT_EQ(result.data[1], 1.2f + 2.2f * tanY);
    ASSERT_FLOAT_EQ(result.data[2], 1.3f + 2.3f * tanY);
    ASSERT_FLOAT_EQ(result.data[3], 1.4f + 2.4f * tanY);

    ASSERT_FLOAT_EQ(result.data[4], 2.1f + 1.1f * tanX);
    ASSERT_FLOAT_EQ(result.data[5], 2.2f + 1.2f * tanX);
    ASSERT_FLOAT_EQ(result.data[6], 2.3f + 1.3f * tanX);
    ASSERT_FLOAT_EQ(result.data[7], 2.4f + 1.4f * tanX);
}

/**
 * @tc.name: RotateZCWRadians
 * @tc.desc: Tests for Rotate Zcwradians. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, RotateZCWRadians, testing::ext::TestSize.Level1)
{
    constexpr float alpha = 0.5f; // rotation angle in radian
    constexpr Math::Vec4 col1 = Math::Vec4(1.1f, 1.2f, 1.3f, 1.4f);
    constexpr Math::Vec4 col2 = Math::Vec4(2.1f, 2.2f, 2.3f, 2.4f);
    constexpr Math::Vec4 col3 = Math::Vec4(3.1f, 3.2f, 3.3f, 3.4f);
    constexpr Math::Vec4 col4 = Math::Vec4(4.1f, 4.2f, 4.3f, 4.4f);
    constexpr Math::Mat4X4 mat(col1, col2, col3, col4);

    const Math::Mat4X4 result = Math::RotateZCWRadians(mat, alpha);

    const float co = cosf(alpha);
    const float si = sinf(alpha);

    ASSERT_FLOAT_EQ(result.data[0], 1.1f * co + 2.1f * si);
    ASSERT_FLOAT_EQ(result.data[1], 1.2f * co + 2.2f * si);
    ASSERT_FLOAT_EQ(result.data[2], 1.3f * co + 2.3f * si);
    ASSERT_FLOAT_EQ(result.data[3], 1.4f * co + 2.4f * si);

    ASSERT_FLOAT_EQ(result.data[4], 2.1f * co - 1.1f * si);
    ASSERT_FLOAT_EQ(result.data[5], 2.2f * co - 1.2f * si);
    ASSERT_FLOAT_EQ(result.data[6], 2.3f * co - 1.3f * si);
    ASSERT_FLOAT_EQ(result.data[7], 2.4f * co - 1.4f * si);
}

/**
 * @tc.name: TranslateM3
 * @tc.desc: Tests for Translate M3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, TranslateM3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 C1 = { 1.0f, 0.0f, 2.0f };
    constexpr Math::Vec3 C2 = { 0.0f, 1.0f, 5.0f };
    constexpr Math::Vec3 C3 = { 0.0f, 0.0f, 1.0f };
    constexpr Math::Mat3X3 tMat = Math::Mat3X3(C1, C2, C3);

    constexpr Math::Vec2 translation(2.0f, 5.0f);

    Math::Mat3X3 mat = Math::Translate(Math::IDENTITY_3X3, translation);

    ASSERT_FLOAT_EQ(mat.data[0], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[5], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[6], 2.0f); // x
    ASSERT_FLOAT_EQ(mat.data[7], 5.0f); // y
    ASSERT_FLOAT_EQ(mat.data[8], 1.0f);

    ASSERT_FLOAT_EQ(
        Math::Determinant(Math::DimensionalShift(mat)), Math::Determinant(Math::DimensionalShift(tMat))); // to 4x4
}
/**
 * @tc.name: EigenDecompositionM3
 * @tc.desc: Tests eigenvalues for matrices with 1, 2, and 3 distinct real roots
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, EigenDecompositionM3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 M1C1 = { 1.0f, 1.0f, 1.0f };
    constexpr Math::Vec3 M1C2 = { 0.0f, 1.0f, 1.0f };
    constexpr Math::Vec3 M1C3 = { 0.0f, 0.0f, 1.0f };

    constexpr Math::Vec3 M2C1 = { 2.0f, 0.0f, 0.0f };
    constexpr Math::Vec3 M2C2 = { 0.0f, 3.0f, 1.0f };
    constexpr Math::Vec3 M2C3 = { 0.0f, 0.0f, 3.0f };

    constexpr Math::Vec3 M3C1 = { 3.0f, 0.0f, 0.0f };
    constexpr Math::Vec3 M3C2 = { 0.0f, 2.0f, 0.0f };
    constexpr Math::Vec3 M3C3 = { 0.0f, 0.0f, 1.0f };

    constexpr Math::Mat3X3 matrices[] = { Math::Mat3X3(M1C1, M1C2, M1C3), Math::Mat3X3(M2C1, M2C2, M2C3),
        Math::Mat3X3(M3C1, M3C2, M3C3) };

    constexpr Math::Vec3 expectedEigenvalues[] = { Math::Vec3 { 1.0f, NAN, NAN }, Math::Vec3 { 2.0f, 3.0f, NAN },
        Math::Vec3 { 1.0f, 2.0f, 3.0f } };

    for (size_t i = 0; i < std::size(matrices); ++i) {
        Math::Vec3 eigenvalues = Math::EigenvalueDecompositionAnalytical(matrices[i]);

        std::array<float, 3> sortedEigen = { eigenvalues.x, eigenvalues.y, eigenvalues.z };
        std::array<float, 3> sortedExpected = { expectedEigenvalues[i].x, expectedEigenvalues[i].y,
            expectedEigenvalues[i].z };
        std::sort(sortedEigen.begin(), sortedEigen.end());
        std::sort(sortedExpected.begin(), sortedExpected.end());

        for (size_t j = 0; j < 3; ++j) {
            // if both nan, pass
            if (std::isnan(sortedEigen[j]) && std::isnan(sortedExpected[j])) {
                continue;
            }
            ASSERT_NEAR(sortedEigen[j], sortedExpected[j], 1e-5f);
        }
    }
}

/**
 * @tc.name: ExtractEigenVectorM3
 * @tc.desc: Tests extraction of eigen vectors from eigen values.
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, ExtractEigenVectorM3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 C1 = { 3.0f, 0.0f, 0.0f };
    constexpr Math::Vec3 C2 = { 0.0f, 2.0f, 0.0f };
    constexpr Math::Vec3 C3 = { 0.0f, 0.0f, 1.0f };

    constexpr Math::Mat3X3 mat = Math::Mat3X3(C1, C2, C3);

    Math::Vec3 eigenValues = Math::EigenvalueDecompositionAnalytical(mat);

    Math::Mat3X3 eigenVectors = Math::ExtractEigenvectors(mat, eigenValues);

    // Returns the vectors in this order, could do sorting but is complicated for vectors
    // and this is only one test case anyways
    constexpr Math::Vec3 EX1 = { 1.0f, 0.0f, 0.0f };
    constexpr Math::Vec3 EX2 = { 0.0f, 0.0f, 1.0f };
    constexpr Math::Vec3 EX3 = { 0.0f, 1.0f, 0.0f };

    Math::Mat3X3 expectedEigenVectors = Math::Mat3X3(EX1, EX2, EX3);

    for (size_t i = 0; i < 3; i++) {
        const Math::Vec3 actual = eigenVectors[i];
        const Math::Vec3 expected = expectedEigenVectors[i];

        ASSERT_NEAR(actual.x, expected.x, 1e-5f);
        ASSERT_NEAR(actual.y, expected.y, 1e-5f);
        ASSERT_NEAR(actual.z, expected.z, 1e-5f);
    }
}

/**
 * @tc.name: TRS
 * @tc.desc: Tests for Trs. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, TRS, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 C1 = { 1.0f, 0.0f, 0.0f, 0.0f };
    constexpr Math::Vec4 C2 = { 0.0f, 1.0f, 0.0f, 0.0f };
    constexpr Math::Vec4 C3 = { 0.0f, 0.0f, 1.0f, 0.0f };
    constexpr Math::Vec4 C4 = { 0.0f, 0.0f, 0.0f, 1.0f };
    [[maybe_unused]] constexpr Math::Mat4X4 identity(C1, C2, C3, C4);

    constexpr Math::Vec3 translation(12.0f, 13.0f, 14.0f);
    constexpr Math::Vec3 scale(Math::Vec3(1.0f, 1.0f, 1.0f));

    constexpr float amountToRotate = Math::DEG2RAD * 20.0f;
    const Math::Quat rotation = Math::AngleAxis(amountToRotate, Math::Vec3(0.0f, 1.0f, 0.0f));

    const Math::Mat4X4 mat = Math::Trs(translation, rotation, scale);

    ASSERT_FLOAT_EQ(mat.data[0], 0.93969262f);
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], -0.342020f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[5], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.34202012f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[10], 0.93969262f);
    ASSERT_FLOAT_EQ(mat.data[11], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[12], 12.0f);
    ASSERT_FLOAT_EQ(mat.data[13], 13.0f);
    ASSERT_FLOAT_EQ(mat.data[14], 14.0f);
    ASSERT_FLOAT_EQ(mat.data[15], 1.0f);
}

/**
 * @tc.name: OperatorStar
 * @tc.desc: Tests for Operator Star. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, OperatorStar, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1 = Math::Vec4(0.0f, 1.0f, 2.0f, 3.0f);
    constexpr Math::Vec4 col2 = Math::Vec4(0.1f, 1.1f, 2.1f, 3.1f);
    constexpr Math::Vec4 col3 = Math::Vec4(0.2f, 1.2f, 2.2f, 3.2f);
    constexpr Math::Vec4 col4 = Math::Vec4(0.3f, 1.3f, 2.3f, 3.3f);

    constexpr Math::Mat4X4 identity(col1, col2, col3, col4);

    Math::Mat4X4 mat(identity);

    constexpr Math::Vec4 vec(1.1f, 2.2f, 3.3f, 4.4f);

    Math::Vec4 temp = mat * vec;

    ASSERT_FLOAT_EQ(temp.data[0], 0.0f * 1.1f + 0.1f * 2.2f + 0.2f * 3.3f + 0.3f * 4.4f);
    ASSERT_FLOAT_EQ(temp.data[1], 1.0f * 1.1f + 1.1f * 2.2f + 1.2f * 3.3f + 1.3f * 4.4f);
    ASSERT_FLOAT_EQ(temp.data[2], 2.0f * 1.1f + 2.1f * 2.2f + 2.2f * 3.3f + 2.3f * 4.4f);
    ASSERT_FLOAT_EQ(temp.data[3], 3.0f * 1.1f + 3.1f * 2.2f + 3.2f * 3.3f + 3.3f * 4.4f);
}

/**
 * @tc.name: OperatorStarV3
 * @tc.desc: Tests for Operator Star V3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, OperatorStarV3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1 = Math::Vec3(0.0f, 1.0f, 2.0f);
    constexpr Math::Vec3 col2 = Math::Vec3(0.1f, 1.1f, 2.1f);
    constexpr Math::Vec3 col3 = Math::Vec3(0.2f, 1.2f, 2.2f);

    constexpr Math::Mat3X3 identity(col1, col2, col3);

    Math::Mat3X3 mat(identity);

    constexpr Math::Vec3 vec(1.1f, 2.2f, 3.3f);

    Math::Vec3 temp = mat * vec;

    ASSERT_FLOAT_EQ(temp.data[0], 0.0f * 1.1f + 0.1f * 2.2f + 0.2f * 3.3f);
    ASSERT_FLOAT_EQ(temp.data[1], 1.0f * 1.1f + 1.1f * 2.2f + 1.2f * 3.3f);
    ASSERT_FLOAT_EQ(temp.data[2], 2.0f * 1.1f + 2.1f * 2.2f + 2.2f * 3.3f);
}

/**
 * @tc.name: MultiplyVector
 * @tc.desc: Tests for Multiply Vector. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, MultiplyVector, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1 = Math::Vec4(0.0f, 1.0f, 2.0f, 3.0f);
    constexpr Math::Vec4 col2 = Math::Vec4(0.1f, 1.1f, 2.1f, 3.1f);
    constexpr Math::Vec4 col3 = Math::Vec4(0.2f, 1.2f, 2.2f, 3.2f);
    constexpr Math::Vec4 col4 = Math::Vec4(0.3f, 1.3f, 2.3f, 3.3f);
    constexpr Math::Mat4X4 identity(col1, col2, col3, col4);
    constexpr Math::Mat4X4 mat(identity);

    constexpr Math::Vec3 direction(1.1f, 2.2f, 3.3f);

    const Math::Vec3 result = Math::MultiplyVector(mat, direction);

    ASSERT_FLOAT_EQ(result.data[0], 0.0f * 1.1f + 0.1f * 2.2f + 0.2f * 3.3f);
    ASSERT_FLOAT_EQ(result.data[1], 1.0f * 1.1f + 1.1f * 2.2f + 1.2f * 3.3f);
    ASSERT_FLOAT_EQ(result.data[2], 2.0f * 1.1f + 2.1f * 2.2f + 2.2f * 3.3f);
}

/**
 * @tc.name: MultiplyPoint2X3
 * @tc.desc: Tests for Multiply Point2X3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, MultiplyPoint2X3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 C1 = { 0.1f, 0.2f, 0.0f };
    constexpr Math::Vec3 C2 = { 0.4f, 0.5f, 0.0f };
    // w' = P.x*M02+P.y*M12+M22, but this function does not do Perspective division => w'=1 => Mrow[2]=[0,0,1]
    constexpr Math::Vec3 C3 = { 0.7f, 0.8f, 1.0f };
    constexpr Math::Mat3X3 identity(C1, C2, C3);
    constexpr Math::Mat3X3 mat(identity);

    constexpr Math::Vec2 position(1.1f, 2.2f);

    const Math::Vec2 result = Math::MultiplyPoint2X3(mat, position);

    // NOTE: vertice array which we multiply
    ASSERT_FLOAT_EQ(result.data[0], 1.1f * 0.1f + 2.2f * 0.4f + 0.7f);
    ASSERT_FLOAT_EQ(result.data[1], 1.1f * 0.2f + 2.2f * 0.5f + 0.8f);
}

/**
 * @tc.name: MultiplyPoint2X4
 * @tc.desc: Tests for Multiply Point2X4. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, MultiplyPoint2X4, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 C1 = { 0.1f, 0.2f, 0.3f, 0.0f };
    constexpr Math::Vec4 C2 = { 0.5f, 0.6f, 0.7f, 0.0f };
    // Unimportant column+row at index 3, z=0 => does not affect result
    constexpr Math::Vec4 C3 = { 0.8f, 0.9f, 1.0f, 1.1f };
    // w' = P.x*M03+P.y*M13+M33, but this function does not do Perspective division => w'=1 => Mrow[3]=[0,0,any,1]
    constexpr Math::Vec4 C4 = { 1.2f, 1.3f, 1.4f, 1.0f };
    constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
    constexpr Math::Mat4X4 mat(identity);

    constexpr Math::Vec2 position(1.1f, 2.2f);

    const Math::Vec2 result = Math::MultiplyPoint2X4(mat, position);

    // NOTE: vertice array which we multiply
    ASSERT_FLOAT_EQ(result.data[0], 1.1f * 0.1f + 2.2f * 0.5f + 1.2f);
    ASSERT_FLOAT_EQ(result.data[1], 1.1f * 0.2f + 2.2f * 0.6f + 1.3f);
}

/**
 * @tc.name: MultiplyVector2X4
 * @tc.desc: Tests for Multiply Vector2X4. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, MultiplyVector2X4, testing::ext::TestSize.Level1)
{
    // same as MultiplyPoint2X4 but without translation
    constexpr Math::Vec4 C1 = { 0.1f, 0.2f, 0.3f, 0.0f };
    constexpr Math::Vec4 C2 = { 0.5f, 0.6f, 0.7f, 0.0f };
    constexpr Math::Vec4 C3 = { 0.8f, 0.9f, 1.0f, 1.1f };
    constexpr Math::Vec4 C4 = { 1.2f, 1.3f, 1.4f, 1.0f };
    constexpr Math::Mat4X4 mat(C1, C2, C3, C4);

    constexpr Math::Vec2 direction(1.1f, 2.2f);

    const Math::Vec2 result = Math::MultiplyVector2X4(mat, direction);

    ASSERT_FLOAT_EQ(result.data[0], 1.1f * 0.1f + 2.2f * 0.5f);
    ASSERT_FLOAT_EQ(result.data[1], 1.1f * 0.2f + 2.2f * 0.6f);
}

/**
 * @tc.name: MultiplyPoint3X4
 * @tc.desc: Tests for Multiply Point3X4. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, MultiplyPoint3X4, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 C1 = { 0.1f, 0.2f, 0.3f, 0.0f };
    constexpr Math::Vec4 C2 = { 0.5f, 0.6f, 0.7f, 0.0f };
    constexpr Math::Vec4 C3 = { 0.8f, 0.9f, 1.0f, 0.0f };
    // w' = P.x*M03+P.y*M13+M33, but this function does not do Perspective division => w'=1 => Mrow[3]=[0,0,0,1]
    constexpr Math::Vec4 C4 = { 1.2f, 1.3f, 1.4f, 1.0f };
    constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
    constexpr Math::Mat4X4 mat(identity);

    constexpr Math::Vec3 position(1.1f, 2.2f, 3.3f);

    const Math::Vec3 result = Math::MultiplyPoint3X4(mat, position);

    // NOTE: vertice array which we multiply
    ASSERT_FLOAT_EQ(result.data[0], 1.1f * 0.1f + 2.2f * 0.5f + 3.3f * 0.8f + 1.2f);
    ASSERT_FLOAT_EQ(result.data[1], 1.1f * 0.2f + 2.2f * 0.6f + 3.3f * 0.9f + 1.3f);
    ASSERT_FLOAT_EQ(result.data[2], 1.1f * 0.3f + 2.2f * 0.7f + 3.3f * 1.0f + 1.4f);
}

/**
 * @tc.name: Determinant
 * @tc.desc: Tests for Determinant. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, Determinant, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1 = Math::Vec4(1.0f, 0.0f, 5.0f, 9.0f);
    constexpr Math::Vec4 col2 = Math::Vec4(8.0f, 3.0f, 1.0f, 2.0f);
    constexpr Math::Vec4 col3 = Math::Vec4(4.0f, 3.0f, 9.0f, 7.0f);
    constexpr Math::Vec4 col4 = Math::Vec4(5.0f, 1.0f, 0.0f, 9.0f);
    Math::Mat4X4 mat = Math::Mat4X4(col1, col2, col3, col4);

    float det = Math::Determinant(mat);

    ASSERT_FLOAT_EQ(det, 412.f);
}

/**
 * @tc.name: Inverse
 * @tc.desc: Tests for Inverse. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, Inverse, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 col1 = Math::Vec4(1.0f, 3.0f, 5.0f, 9.0f);
    constexpr Math::Vec4 col2 = Math::Vec4(1.0f, 3.0f, 1.0f, 7.0f);
    constexpr Math::Vec4 col3 = Math::Vec4(4.0f, 3.0f, 9.0f, 7.0f);
    constexpr Math::Vec4 col4 = Math::Vec4(5.0f, 2.0f, 0.0f, 9.0f);
    Math::Mat4X4 mat = Math::Mat4X4(col1, col2, col3, col4);

    mat = Math::Inverse(mat);

    // These may vary, depends on pc, limits could be used
    ASSERT_FLOAT_EQ(mat.data[0], -0.27659574f);
    ASSERT_FLOAT_EQ(mat.data[1], 0.04255319f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.14893617f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.12765957f);

    ASSERT_FLOAT_EQ(mat.data[4], -0.625f);
    ASSERT_FLOAT_EQ(mat.data[5], 0.875f);
    ASSERT_FLOAT_EQ(mat.data[6], 0.24999999f);
    ASSERT_FLOAT_EQ(mat.data[7], -0.24999999f);

    ASSERT_FLOAT_EQ(mat.data[8], 0.1037234f);
    ASSERT_FLOAT_EQ(mat.data[9], -0.14095744f);
    ASSERT_FLOAT_EQ(mat.data[10], 0.069148935f);
    ASSERT_FLOAT_EQ(mat.data[11], -0.047872338f);

    ASSERT_FLOAT_EQ(mat.data[12], 0.29255319f);
    ASSERT_FLOAT_EQ(mat.data[13], -0.2180851f);
    ASSERT_FLOAT_EQ(mat.data[14], -0.13829787f);
    ASSERT_FLOAT_EQ(mat.data[15], 0.095744677f);
}

/**
 * @tc.name: InverseM3
 * @tc.desc: Tests for Inverse M3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, InverseM3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 col1 = Math::Vec3(1.0f, 2.0f, 3.0f);
    constexpr Math::Vec3 col2 = Math::Vec3(4.0f, 5.0f, 6.0f);
    constexpr Math::Vec3 col3 = Math::Vec3(7.0f, 9.0f, 8.0f);
    Math::Mat3X3 mat = Math::Mat3X3(col1, col2, col3);

    mat = Math::Inverse(mat);

    // These may vary, depends on pc, limits could be used
    ASSERT_NEAR((9.0f * mat.data[0]), -14.0f, 0.00000001f);
    ASSERT_NEAR((9.0f * mat.data[1]), 11.0f, 0.00000001f);
    ASSERT_NEAR((9.0f * mat.data[2]), -3.0f, 0.00000001f);

    ASSERT_NEAR((9.0f * mat.data[3]), 10.0f, 0.00000001f);
    ASSERT_NEAR((9.0f * mat.data[4]), -13.0f, 0.00000001f);
    ASSERT_NEAR((9.0f * mat.data[5]), 6.0f, 0.00000001f);

    ASSERT_NEAR((9.0f * mat.data[6]), 1.0f, 0.00000001f);
    ASSERT_NEAR((9.0f * mat.data[7]), 5.0f, 0.00000001f);
    ASSERT_NEAR((9.0f * mat.data[8]), -3.0f, 0.00000001f);
}

/**
 * @tc.name: PackAndUnpackUnorm
 * @tc.desc: Tests for Pack And Unpack Unorm. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathFloatPacker, PackAndUnpackUnorm, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vec1(0.2f, 0.f);
    constexpr Math::Vec2 vec2(88.765f, 0.f);

    const uint32_t packedUNorm = Math::PackUnorm2X16(vec1);
    const uint32_t packedUNorm2 = Math::PackUnorm2X16(vec2);

    union {
        uint16_t pv[2];
        uint32_t v;
    } v1;
    v1.v = packedUNorm;

    union {
        uint16_t pv[2];
        uint32_t v;
    } v2;
    v2.v = packedUNorm2;

    const uint16_t firstTestHalf = v1.pv[0];
    const uint16_t secondTestHalf = v2.pv[0];

    union {
        uint16_t pv[2];
        uint32_t v;
    } v3;

    v3.pv[0] = firstTestHalf;
    v3.pv[1] = secondTestHalf;

    // Final packed value in uint32_t
    const uint32_t packed = v3.v;

    Math::Vec2 vec3;

    vec3 = Math::UnpackUnorm2X16(packed);
    ASSERT_FLOAT_EQ(vec3.x, 0.2f);
    ASSERT_FLOAT_EQ(vec3.y, 1.0f);
}

/**
 * @tc.name: PackAndUnpackF16F32
 * @tc.desc: Tests for Pack And Unpack F16F32. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathFloatPacker, PackAndUnpackF16F32, testing::ext::TestSize.Level1)
{
    {
        const uint16_t packed = Math::F32ToF16(12345.f);

        // Notice packing loss and return change in values
        ASSERT_FLOAT_EQ(Math::F16ToF32(packed), 12344);
    }
    {
        const uint16_t packed = Math::F32ToF16(6.11543655396e-05f);
        ASSERT_FLOAT_EQ(Math::F16ToF32(packed), 6.11543655396e-05f);
    }
    {
        // smallest positive normal number
        const uint16_t packed = Math::F32ToF16(6.103515625e-05f);
        ASSERT_FLOAT_EQ(Math::F16ToF32(packed), 6.103515625e-05f);
    }
    {
        const uint16_t packed = Math::F32ToF16(65504.f);
        ASSERT_FLOAT_EQ(Math::F16ToF32(packed), 65504);
    }
    {
        // largest number less than one
        const uint16_t packed = Math::F32ToF16(0.99951172f);
        ASSERT_FLOAT_EQ(Math::F16ToF32(packed), 0.99951172f);
    }
    {
        const uint16_t packed = Math::F32ToF16(1.f);
        ASSERT_FLOAT_EQ(Math::F16ToF32(packed), 1.f);
    }
    {
        // smallest number larger than one
        const uint16_t packed = Math::F32ToF16(1.00097656f);
        ASSERT_FLOAT_EQ(Math::F16ToF32(packed), 1.00097656f);
    }
    {
        const uint16_t packed = Math::F32ToF16(0.f);
        ASSERT_FLOAT_EQ(Math::F16ToF32(packed), 0.f);
    }
    {
        const uint16_t packed = Math::F32ToF16(-0.f);
        ASSERT_FLOAT_EQ(Math::F16ToF32(packed), -0.f);
    }
    {
        const uint16_t packed = Math::F32ToF16(INFINITY);
        ASSERT_FLOAT_EQ(Math::F16ToF32(packed), INFINITY);
    }
    {
        const uint16_t packed = Math::F32ToF16(-INFINITY);
        ASSERT_FLOAT_EQ(Math::F16ToF32(packed), -INFINITY);
    }
}

/**
 * @tc.name: PackAndUnpackSnorm
 * @tc.desc: Tests for Pack And Unpack Snorm. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathFloatPacker, PackAndUnpackSnorm, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vec1(-14234.f, 0.f);
    constexpr Math::Vec2 vec2(1423.f, 0.f);

    const uint32_t packedSNorm = Math::PackSnorm2X16(vec1);
    const uint32_t packedSNorm2 = Math::PackSnorm2X16(vec2);

    union {
        int16_t pv[2];
        uint32_t v;
    } v1;
    v1.v = packedSNorm;

    union {
        int16_t pv[2];
        uint32_t v;
    } v2;
    v2.v = packedSNorm2;

    const int16_t firstTestHalf = v1.pv[0];
    const int16_t secondTestHalf = v2.pv[0];

    union {
        int16_t pv[2];
        uint32_t v;
    } v3;

    v3.pv[0] = firstTestHalf;
    v3.pv[1] = secondTestHalf;

    // Final packed value in uint32_t
    const uint32_t packed = v3.v;

    const Math::Vec2 vec3 = Math::UnpackSnorm2X16(packed);

    ASSERT_FLOAT_EQ(vec3.x, -1.0f);
    ASSERT_FLOAT_EQ(vec3.y, 1.0f);
}

/**
 * @tc.name: PackAndUnpackHalf
 * @tc.desc: Tests for Pack And Unpack Half. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathFloatPacker, PackAndUnpackHalf, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec2 vec1(234.234f, 0.f);
    constexpr Math::Vec2 vec2(888.765f, 0.f);

    const uint32_t packedHalf = Math::PackHalf2X16(vec1);
    const uint32_t packedHalf2 = Math::PackHalf2X16(vec2);

    union {
        uint16_t pv[2];
        uint32_t v;
    } v1;
    v1.v = packedHalf;

    union {
        uint16_t pv[2];
        uint32_t v;
    } v2;
    v2.v = packedHalf2;

    const uint16_t firstTestHalf = v1.pv[0];
    const uint16_t secondTestHalf = v2.pv[0];

    union {
        uint16_t pv[2];
        uint32_t v;
    } v3;

    v3.pv[0] = firstTestHalf;
    v3.pv[1] = secondTestHalf;

    // Final packed value in uint32_t
    const uint32_t packed = v3.v;

    const Math::Vec2 vec3 = Math::UnpackHalf2X16(packed);

    // Below values where compression loss is visible
    ASSERT_FLOAT_EQ(vec3.x, 234.125f);
    ASSERT_FLOAT_EQ(vec3.y, 888.5f);
}

/**
 * @tc.name: HasRotationIdentity4X4
 * @tc.desc: Tests for Has Rotation Identity4X4. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, HasRotationIdentity4X4, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 C1 = { 1.0f, 0.0f, 0.0f, 0.0f };
    constexpr Math::Vec4 C2 = { 0.0f, 1.0f, 0.0f, 0.0f };
    constexpr Math::Vec4 C3 = { 0.0f, 0.0f, 1.0f, 0.0f };
    constexpr Math::Vec4 C4 = { 0.0f, 0.0f, 0.0f, 1.0f };
    constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
    constexpr Math::Mat4X4 mat(identity);
    bool hasRotation = Math::HasRotation(mat);

    ASSERT_EQ(hasRotation, false);
}

/**
 * @tc.name: HasRotationTranslated4X4
 * @tc.desc: Tests for Has Rotation Translated4X4. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, HasRotationTranslated4X4, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 C1 = { 1.0f, 0.0f, 0.0f, 1.0f };
    constexpr Math::Vec4 C2 = { 0.0f, 1.0f, 0.0f, 2.0f };
    constexpr Math::Vec4 C3 = { 0.0f, 0.0f, 1.0f, 3.0f };
    constexpr Math::Vec4 C4 = { 0.0f, 0.0f, 0.0f, 1.0f };
    constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
    constexpr Math::Mat4X4 mat(identity);
    bool hasRotation = Math::HasRotation(mat);

    ASSERT_EQ(hasRotation, false);
}

/**
 * @tc.name: HasRotationRotated4X4
 * @tc.desc: Tests for Has Rotation Rotated4X4. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, HasRotationRotated4X4, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 C1 = { 1.0f, 0.0f, 0.0f, 0.0f };
    constexpr Math::Vec4 C2 = { 0.0f, 0.5f, -0.5f, 0.0f };
    constexpr Math::Vec4 C3 = { 0.0f, 0.5f, 0.5f, 0.0f };
    constexpr Math::Vec4 C4 = { 0.0f, 0.0f, 0.0f, 1.0f };
    constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
    constexpr Math::Mat4X4 mat(identity);
    bool hasRotation = Math::HasRotation(mat);

    ASSERT_EQ(hasRotation, true);
}

/**
 * @tc.name: HasRotationIdentity3X3
 * @tc.desc: Tests for Has Rotation Identity3X3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, HasRotationIdentity3X3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 C1 = { 1.0f, 0.0f, 0.0f };
    constexpr Math::Vec3 C2 = { 0.0f, 1.0f, 0.0f };
    constexpr Math::Vec3 C3 = { 0.0f, 0.0f, 1.0f };
    constexpr Math::Mat3X3 identity(C1, C2, C3);
    constexpr Math::Mat3X3 mat(identity);
    bool hasRotation = Math::HasRotation(mat);

    ASSERT_EQ(hasRotation, false);
}

/**
 * @tc.name: HasRotationTranslated3X3
 * @tc.desc: Tests for Has Rotation Translated3X3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, HasRotationTranslated3X3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 C1 = { 1.0f, 0.0f, 2.0f };
    constexpr Math::Vec3 C2 = { 0.0f, 1.0f, 3.0f };
    constexpr Math::Vec3 C3 = { 0.0f, 0.0f, 1.0f };
    constexpr Math::Mat3X3 identity(C1, C2, C3);
    constexpr Math::Mat3X3 mat(identity);
    bool hasRotation = Math::HasRotation(mat);

    ASSERT_EQ(hasRotation, false);
}

/**
 * @tc.name: HasRotationRotated3X3
 * @tc.desc: Tests for Has Rotation Rotated3X3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, HasRotationRotated3X3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 C1 = { 0.5f, -0.5f, 0.0f };
    constexpr Math::Vec3 C2 = { 0.5f, 0.5f, 0.0f };
    constexpr Math::Vec3 C3 = { 0.0f, 0.0f, 1.0f };
    constexpr Math::Mat3X3 identity(C1, C2, C3);
    constexpr Math::Mat3X3 mat(identity);
    bool hasRotation = Math::HasRotation(mat);

    ASSERT_EQ(hasRotation, true);
}

static void symmetricScreenCoordinates(
    const float fovy, const float aspect, const float n, const float f, float& l, float& r, float& b, float& t)
{
    const float scale = tan(fovy * 0.5f) * n;
    r = aspect * scale;
    l = -r;
    t = scale;
    b = -t;
}

/**
 * @tc.name: OuterProduct4X4
 * @tc.desc: Tests for OuterProduct4X4. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, OuterProduct4X4, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec4 u = Math::Vec4(1.0f, 2.0f, 3.0f, 4.0f);
    constexpr Math::Vec4 v = Math::Vec4(5.0f, 6.0f, 7.0f, 8.0f);

    const Math::Mat4X4 result = Math::OuterProduct(u, v);

    ASSERT_FLOAT_EQ(result.data[0], u.x * v.x);
    ASSERT_FLOAT_EQ(result.data[1], u.y * v.x);
    ASSERT_FLOAT_EQ(result.data[2], u.z * v.x);
    ASSERT_FLOAT_EQ(result.data[3], u.w * v.x);

    ASSERT_FLOAT_EQ(result.data[4], u.x * v.y);
    ASSERT_FLOAT_EQ(result.data[5], u.y * v.y);
    ASSERT_FLOAT_EQ(result.data[6], u.z * v.y);
    ASSERT_FLOAT_EQ(result.data[7], u.w * v.y);

    ASSERT_FLOAT_EQ(result.data[8], u.x * v.z);
    ASSERT_FLOAT_EQ(result.data[9], u.y * v.z);
    ASSERT_FLOAT_EQ(result.data[10], u.z * v.z);
    ASSERT_FLOAT_EQ(result.data[11], u.w * v.z);

    ASSERT_FLOAT_EQ(result.data[12], u.x * v.w);
    ASSERT_FLOAT_EQ(result.data[13], u.y * v.w);
    ASSERT_FLOAT_EQ(result.data[14], u.z * v.w);
    ASSERT_FLOAT_EQ(result.data[15], u.w * v.w);
}

/**
 * @tc.name: OuterProduct3X3
 * @tc.desc: Tests for OuterProduct3X3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, OuterProduct3X3, testing::ext::TestSize.Level1)
{
    constexpr Math::Vec3 u = Math::Vec3(1.0f, 2.0f, 3.0f);
    constexpr Math::Vec3 v = Math::Vec3(4.0f, 5.0f, 6.0f);

    const Math::Mat3X3 result = Math::OuterProduct(u, v);

    ASSERT_FLOAT_EQ(result.data[0], u.x * v.x);
    ASSERT_FLOAT_EQ(result.data[1], u.y * v.x);
    ASSERT_FLOAT_EQ(result.data[2], u.z * v.x);

    ASSERT_FLOAT_EQ(result.data[3], u.x * v.y);
    ASSERT_FLOAT_EQ(result.data[4], u.y * v.y);
    ASSERT_FLOAT_EQ(result.data[5], u.z * v.y);

    ASSERT_FLOAT_EQ(result.data[6], u.x * v.z);
    ASSERT_FLOAT_EQ(result.data[7], u.y * v.z);
    ASSERT_FLOAT_EQ(result.data[8], u.z * v.z);
}

/**
 * @tc.name: PerspectiveRhNo
 * @tc.desc: Tests for Perspective Rh No. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, PerspectiveRhNo, testing::ext::TestSize.Level1)
{
    // basicaly creates the following matrix
    // c 0 0 0
    // 0 d 0 0
    // 0 0 a b
    // 0 0 -1 0
    //
    // aspect = dx/dy
    // fovy = view angle of camera => tg(1/2*fovy) = y/2zNear
    // c=2*zNear/(r-l)=2*zNear/x=2*zNear/(y*aspect)=>c=1/tg(1/2fovy)/aspect
    // d=2*zNear/(t-b)=2*zNear/y=>d=1/tg(1/2fovy)
    // zNear, zFar = distance to start and end of frustum
    // a = -(near+far)/(far-near)
    // b = -(2*near*far)/(near-far)

    Math::Mat4X4 mat = Math::PerspectiveRhNo(1.0f, 0.5f, 1.0f, 10.0f);

    ASSERT_NEAR(mat.data[0], 1.0f / (0.5f * tan(0.5f)), Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_NEAR(mat.data[5], 1.0f / (tan(0.5f)), Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_NEAR(mat.data[10], -11.0f / 9.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[11], -1.0f);
    ASSERT_FLOAT_EQ(mat.data[12], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[13], 0.0f);
    ASSERT_NEAR(mat.data[14], -2 * 1.0f * 10.0f / 9.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[15], 0.0f);

    // verify that the version supporting non-symmetric perspective creates the same matrix with symmetric screen
    // coordinates.
    float left = 0.f;
    float right = 0.f;
    float bottom = 0.f;
    float top = 0.f;
    symmetricScreenCoordinates(1.0f, 0.5f, 1.0f, 10.0f, left, right, bottom, top);
    Math::Mat4X4 mat2 = Math::PerspectiveRhNo(left, right, bottom, top, 1.0f, 10.f);
    for (unsigned i = 0U; i < BASE_NS::countof(mat.data); ++i) {
        EXPECT_FLOAT_EQ(mat.data[i], mat2.data[i]);
    }

    mat = Math::PerspectiveRhNo(2.f * Math::PI, 0.0f, 1.0f, 1.0f);

    ASSERT_FLOAT_EQ(mat.data[0], HUGE_VALF);
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[5], HUGE_VALF);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[10], -HUGE_VALF); // error + filp
    ASSERT_FLOAT_EQ(mat.data[11], -1.0f);      // error + flip
    ASSERT_FLOAT_EQ(mat.data[12], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[13], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[14], HUGE_VALF); // error
    ASSERT_FLOAT_EQ(mat.data[15], 0.0f);
}

/**
 * @tc.name: PerspectiveLhNo
 * @tc.desc: Tests for Perspective Lh No. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, PerspectiveLhNo, testing::ext::TestSize.Level1)
{
    // All values swapped for Z axis

    Math::Mat4X4 mat = Math::PerspectiveLhNo(1.0f, 0.5f, 1.0f, 10.0f);

    ASSERT_NEAR(mat.data[0], 1.0f / (0.5f * tan(0.5f)), Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_NEAR(mat.data[5], 1.0f / (tan(0.5f)), Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_NEAR(mat.data[10], 11.0f / 9.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[11], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[12], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[13], 0.0f);
    ASSERT_NEAR(mat.data[14], -2 * 1.0f * 10.0f / 9.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[15], 0.0f);

    // verify that the version supporting non-symmetric perspective creates the same matrix with symmetric screen
    // coordinates.
    float left = 0.f;
    float right = 0.f;
    float bottom = 0.f;
    float top = 0.f;
    symmetricScreenCoordinates(1.0f, 0.5f, 1.0f, 10.0f, left, right, bottom, top);
    Math::Mat4X4 mat2 = Math::PerspectiveLhNo(left, right, bottom, top, 1.0f, 10.f);
    for (unsigned i = 0U; i < BASE_NS::countof(mat.data); ++i) {
        EXPECT_FLOAT_EQ(mat.data[i], mat2.data[i]);
    }

    mat = Math::PerspectiveLhNo(2.f * Math::PI, 0.0f, 1.0f, 1.0f);

    ASSERT_FLOAT_EQ(mat.data[0], HUGE_VALF);
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[5], HUGE_VALF);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[10], HUGE_VALF); // error
    ASSERT_FLOAT_EQ(mat.data[11], 1.0f);      // error
    ASSERT_FLOAT_EQ(mat.data[12], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[13], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[14], HUGE_VALF); // error
    ASSERT_FLOAT_EQ(mat.data[15], 0.0f);
}

/**
 * @tc.name: PerspectiveLhZo
 * @tc.desc: Tests for Perspective Lh Zo. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, PerspectiveLhZo, testing::ext::TestSize.Level1)
{
    // a = (near+far)/(far-near)
    // b = (2*near*far)/(near-far)
    // z' = (a+b/z) => z=near z'= (a+b/near) = near/div+far/div-2far/div=(near-far)=-1
    // z' = far => z' = (a+b/far)=near/div+far/div-2near/div= (far-near)/div=1
    // now
    // a = far/(far-near)
    // b = near*far/(near-far)
    // z' = (a+b/z); z' = near = far-far=0
    // z' = (a+b/z); z' = far = (far-near)/div = 1

    Math::Mat4X4 mat = Math::PerspectiveLhZo(1.0f, 0.5f, 1.0f, 10.0f);

    ASSERT_NEAR(mat.data[0], 1.0f / (0.5f * tan(0.5f)), Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_NEAR(mat.data[5], 1.0f / (tan(0.5f)), Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_NEAR(mat.data[10], 10.0f / 9.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[11], 1.0f);
    ASSERT_FLOAT_EQ(mat.data[12], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[13], 0.0f);
    ASSERT_NEAR(mat.data[14], -1.0f * 10.0f / 9.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[15], 0.0f);

    // verify that the version supporting non-symmetric perspective creates the same matrix with symmetric screen
    // coordinates.
    float left = 0.f;
    float right = 0.f;
    float bottom = 0.f;
    float top = 0.f;
    symmetricScreenCoordinates(1.0f, 0.5f, 1.0f, 10.0f, left, right, bottom, top);
    Math::Mat4X4 mat2 = Math::PerspectiveLhZo(left, right, bottom, top, 1.0f, 10.f);
    for (unsigned i = 0U; i < BASE_NS::countof(mat.data); ++i) {
        EXPECT_FLOAT_EQ(mat.data[i], mat2.data[i]);
    }

    mat = Math::PerspectiveLhZo(2.f * Math::PI, 0.0f, 1.0f, 1.0f);

    ASSERT_FLOAT_EQ(mat.data[0], HUGE_VALF);
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[5], HUGE_VALF);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[10], HUGE_VALF); // error
    ASSERT_FLOAT_EQ(mat.data[11], 1.0f);      // error
    ASSERT_FLOAT_EQ(mat.data[12], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[13], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[14], HUGE_VALF); // error
    ASSERT_FLOAT_EQ(mat.data[15], 0.0f);
}

/**
 * @tc.name: PerspectiveRhZo
 * @tc.desc: Tests for Perspective Rh Zo. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, PerspectiveRhZo, testing::ext::TestSize.Level1)
{
    Math::Mat4X4 mat = Math::PerspectiveRhZo(1.0f, 0.5f, 1.0f, 10.0f);

    ASSERT_NEAR(mat.data[0], 1.0f / (0.5f * tan(0.5f)), Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_NEAR(mat.data[5], 1.0f / (tan(0.5f)), Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_NEAR(mat.data[10], -10.0f / 9.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[11], -1.0f);
    ASSERT_FLOAT_EQ(mat.data[12], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[13], 0.0f);
    ASSERT_NEAR(mat.data[14], -1.0f * 10.0f / 9.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[15], 0.0f);

    // verify that the version supporting non-symmetric perspective creates the same matrix with symmetric screen
    // coordinates.
    float left = 0.f;
    float right = 0.f;
    float bottom = 0.f;
    float top = 0.f;
    symmetricScreenCoordinates(1.0f, 0.5f, 1.0f, 10.0f, left, right, bottom, top);
    Math::Mat4X4 mat2 = Math::PerspectiveRhZo(left, right, bottom, top, 1.0f, 10.f);
    for (unsigned i = 0U; i < BASE_NS::countof(mat.data); ++i) {
        EXPECT_FLOAT_EQ(mat.data[i], mat2.data[i]);
    }

    mat = Math::PerspectiveRhZo(2.f * Math::PI, 0.0f, 1.0f, 1.0f);

    ASSERT_FLOAT_EQ(mat.data[0], HUGE_VALF);
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[5], HUGE_VALF);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[10], -HUGE_VALF); // error + flip
    ASSERT_FLOAT_EQ(mat.data[11], -1.0f);      // error + flip
    ASSERT_FLOAT_EQ(mat.data[12], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[13], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[14], HUGE_VALF); // error
    ASSERT_FLOAT_EQ(mat.data[15], 0.0f);
}

/**
 * @tc.name: OrthoLhNo
 * @tc.desc: Tests for Ortho Lh No. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, OrthoLhNo, testing::ext::TestSize.Level1)
{
    // basicaly creates the following matrix
    // c 0 0 e
    // 0 d 0 f
    // 0 0 a b
    // 0 0 0 1

    // dist = 2 (to make a square with sides -1 1)
    // c = 2/(r-l) (scale of x)
    // d = 2/(t-b) (scale of y)
    // e = (r+l)/(r-l) (transformation at x)
    // f = (t+b)/(t-b) (transformation at y)
    // zNear, zFar = distance to start and end of frustum
    // distZ = 2 (map to -1 1)
    // a = 2/(far-near)
    // b = -(far+near)/(far-near) (transformation at z)
    // z'= ((z*a+b)) = (2near-near-far)/div= (near-far)/(far-near)=-1
    // z'= ((z*a+b)) = (2far-near-far)/div = (far-nerar)/(far-near)=1

    Math::Mat4X4 mat = Math::OrthoLhNo(1.0f, 3.0f, -2.0f, 8.0f, 2.0f, 5.0f);

    ASSERT_NEAR(mat.data[0], 1.0f, Math::EPSILON); // 2/(3-1)
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_NEAR(mat.data[5], 0.2f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_NEAR(mat.data[10], 2.0f / 3.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[11], 0.0f);
    ASSERT_NEAR(mat.data[12], -2.0f, Math::EPSILON);
    ASSERT_NEAR(mat.data[13], -0.6f, Math::EPSILON);
    ASSERT_NEAR(mat.data[14], -7.0f / 3.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[15], 1.0f);
}

/**
 * @tc.name: OrthoRhNo
 * @tc.desc: Tests for Ortho Rh No. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, OrthoRhNo, testing::ext::TestSize.Level1)
{
    // swap column Z
    Math::Mat4X4 mat = Math::OrthoRhNo(1.0f, 3.0f, -2.0f, 8.0f, 2.0f, 5.0f);

    ASSERT_NEAR(mat.data[0], 1.0f, Math::EPSILON); // 2/(3-1)
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_NEAR(mat.data[5], 0.2f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_NEAR(mat.data[10], -2.0f / 3.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[11], 0.0f);
    ASSERT_NEAR(mat.data[12], -2.0f, Math::EPSILON);
    ASSERT_NEAR(mat.data[13], -0.6f, Math::EPSILON);
    ASSERT_NEAR(mat.data[14], -7.0f / 3.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[15], 1.0f);
}

/**
 * @tc.name: OrthoLhZo
 * @tc.desc: Tests for Ortho Lh Zo. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, OrthoLhZo, testing::ext::TestSize.Level1)
{
    // distZ = 1 (map to 0 1)
    // a = 1/(far-near)
    // b = -(near)/(far-near) (transformation at z)
    // z'= ((z*a+b)) = (near-near)/div= 0
    // z'= ((z*a+b)/a) = (far-near)/div = (far-nerar)/(far-near)=1

    Math::Mat4X4 mat = Math::OrthoLhZo(1.0f, 3.0f, -2.0f, 8.0f, 2.0f, 5.0f);

    ASSERT_NEAR(mat.data[0], 1.0f, Math::EPSILON); // 2/(3-1)
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_NEAR(mat.data[5], 0.2f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_NEAR(mat.data[10], 1.0f / 3.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[11], 0.0f);
    ASSERT_NEAR(mat.data[12], -2.0f, Math::EPSILON);
    ASSERT_NEAR(mat.data[13], -0.6f, Math::EPSILON);
    ASSERT_NEAR(mat.data[14], -2.0f / 3.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[15], 1.0f);
}

/**
 * @tc.name: OrthoRhZo
 * @tc.desc: Tests for Ortho Rh Zo. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, OrthoRhZo, testing::ext::TestSize.Level1)
{
    Math::Mat4X4 mat = Math::OrthoRhZo(1.0f, 3.0f, -2.0f, 8.0f, 2.0f, 5.0f);

    ASSERT_NEAR(mat.data[0], 1.0f, Math::EPSILON); // 2/(3-1)
    ASSERT_FLOAT_EQ(mat.data[1], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[2], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[4], 0.0f);
    ASSERT_NEAR(mat.data[5], 0.2f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[6], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[8], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[9], 0.0f);
    ASSERT_NEAR(mat.data[10], -1.0f / 3.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[11], 0.0f);
    ASSERT_NEAR(mat.data[12], -2.0f, Math::EPSILON);
    ASSERT_NEAR(mat.data[13], -0.6f, Math::EPSILON);
    ASSERT_NEAR(mat.data[14], -2.0f / 3.0f, Math::EPSILON);
    ASSERT_FLOAT_EQ(mat.data[15], 1.0f);
}

/**
 * @tc.name: LookAtRh
 * @tc.desc: Tests for Look At Rh. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, LookAtRh, testing::ext::TestSize.Level1)
{
    // Returns affine matrix
    //   Xx Xy Xz (-X dot eye)
    //	Yx Yy Yz (-Y dot eye)
    //	Zx Zy Zz (-Z dot eye)
    //	0  0  0  1
    //
    //  Z = eye - at (new scale and share of Z depending on distance (want to keep everything as if d to center is 1))
    //  Z = normalize(Z)
    //  Y=up
    //  X=YxZ (new basis)
    //  Y=ZxX (orthoganal to X)
    //  X=normalize(X)
    //  Y=normalize(Y)
    //  -X dot eye = transformation of potionion to object

    Math::Vec3 eye = Math::Vec3(3.0f, -5.0f, 10.0f); // sum 8
    Math::Vec3 at = Math::Vec3(-3.0f, 2.0f, -1.0f);  // sum 2
    Math::Vec3 up = Math::Vec3(1.0f, 2.0f, -3.0f);   // 1,1,1 will look at the other side from object => inf
    Math::Mat4X4 mat = Math::LookAtRh(eye, at, up);

    float Zlen = Math::sqrt(static_cast<float>(6 * 6 + 7 * 7 + 11 * 11));
    Math::Vec3 Z = Math::Vec3(6.0f / Zlen, -7.0f / Zlen, 11.0f / Zlen);
    Math::Vec3 X = Math::Vec3(2.0f * Z.z + 3.0f * Z.y, 1.0f * Z.z + 3.0f * Z.x, 1.0f * Z.y - 2.0f * Z.x);
    float Xlen = Math::sqrt(X.x * X.x + X.y * X.y + X.z * X.z);
    X.x = X.x / Xlen;  //-1 * sign of column
    X.y = -X.y / Xlen; //-1 * sign of column
    X.z = X.z / Xlen;  //-1 * sign of column
    Math::Vec3 Y = Math::Vec3(Z.y * X.z - Z.z * X.y, Z.x * X.z - Z.z * X.x, Z.x * X.y - Z.y * X.x);
    float Ylen = Math::sqrt(Y.x * Y.x + Y.y * Y.y + Y.z * Y.z);
    Y.x = Y.x / Ylen;  //-1 * sign of column
    Y.y = -Y.y / Ylen; //-1 * sign of column
    Y.z = Y.z / Ylen;  //-1 * sign of column

    ASSERT_NEAR(mat.data[0], X.x, 0.000001f);
    ASSERT_NEAR(mat.data[1], Y.x, 0.000001f);
    ASSERT_NEAR(mat.data[2], Z.x, 0.000001f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_NEAR(mat.data[4], X.y, 0.000001f);
    ASSERT_NEAR(mat.data[5], Y.y, 0.000001f);
    ASSERT_NEAR(mat.data[6], Z.y, 0.000001f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_NEAR(mat.data[8], X.z, 0.000001f);
    ASSERT_NEAR(mat.data[9], Y.z, 0.000001f);
    ASSERT_NEAR(mat.data[10], Z.z, 0.000001f);
    ASSERT_FLOAT_EQ(mat.data[11], 0.0f);
    ASSERT_NEAR(mat.data[12], -(X.x * 3.0f - X.y * 5.0f + X.z * 10.0f), 0.000001f);
    ASSERT_NEAR(mat.data[13], -(Y.x * 3.0f - Y.y * 5.0f + Y.z * 10.0f), 0.000001f);
    ASSERT_NEAR(mat.data[14], -(Z.x * 3.0f - Z.y * 5.0f + Z.z * 10.0f), 0.000001f);
    ASSERT_FLOAT_EQ(mat.data[15], 1.0f);

    eye = Math::Vec3(0.0f, 0.0f, 1.0f);
    at = Math::Vec3(0.0f, 0.0f, 0.0f);
    up = Math::Vec3(0.0f, 1.0f, 0.0f);
    mat = Math::LookAtRh(eye, at, up);
    Math::Mat4X4 tMat = Math::Translate(Math::IDENTITY_4X4, Math::Vec3(0.0f, 0.0f, -1.0f));
    for (int i = 0; i < 15; i++)
        ASSERT_FLOAT_EQ(mat.data[i], tMat.data[i]);
}

/**
 * @tc.name: LookAtLh
 * @tc.desc: Tests for Look At Lh. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, LookAtLh, testing::ext::TestSize.Level1)
{
    Math::Vec3 eye = Math::Vec3(3.0f, -5.0f, 10.0f); // sum 8
    Math::Vec3 at = Math::Vec3(-3.0f, 2.0f, -1.0f);  // sum 2
    Math::Vec3 up = Math::Vec3(1.0f, 2.0f, -3.0f);   // 1,1,1 will look at the other side from object => inf
    Math::Mat4X4 mat = Math::LookAtLh(eye, at, up);

    float Zlen = Math::sqrt(static_cast<float>(6 * 6 + 7 * 7 + 11 * 11));
    Math::Vec3 Z = Math::Vec3(-6.0f / Zlen, 7.0f / Zlen, -11.0f / Zlen); // Swaped values
    Math::Vec3 X = Math::Vec3(2.0f * Z.z + 3.0f * Z.y, 1.0f * Z.z + 3.0f * Z.x, 1.0f * Z.y - 2.0f * Z.x);
    float Xlen = Math::sqrt(X.x * X.x + X.y * X.y + X.z * X.z);
    X.x = X.x / Xlen;  //-1 * sign of column
    X.y = -X.y / Xlen; //-1 * sign of column
    X.z = X.z / Xlen;  //-1 * sign of column
    Math::Vec3 Y = Math::Vec3(Z.y * X.z - Z.z * X.y, Z.x * X.z - Z.z * X.x, Z.x * X.y - Z.y * X.x);
    float Ylen = Math::sqrt(Y.x * Y.x + Y.y * Y.y + Y.z * Y.z);
    Y.x = Y.x / Ylen;  //-1 * sign of column
    Y.y = -Y.y / Ylen; //-1 * sign of column
    Y.z = Y.z / Ylen;  //-1 * sign of column

    ASSERT_NEAR(mat.data[0], X.x, 0.000001f);
    ASSERT_NEAR(mat.data[1], Y.x, 0.000001f);
    ASSERT_NEAR(mat.data[2], Z.x, 0.000001f);
    ASSERT_FLOAT_EQ(mat.data[3], 0.0f);
    ASSERT_NEAR(mat.data[4], X.y, 0.000001f);
    ASSERT_NEAR(mat.data[5], Y.y, 0.000001f);
    ASSERT_NEAR(mat.data[6], Z.y, 0.000001f);
    ASSERT_FLOAT_EQ(mat.data[7], 0.0f);
    ASSERT_NEAR(mat.data[8], X.z, 0.000001f);
    ASSERT_NEAR(mat.data[9], Y.z, 0.000001f);
    ASSERT_NEAR(mat.data[10], Z.z, 0.000001f);
    ASSERT_FLOAT_EQ(mat.data[11], 0.0f);
    ASSERT_NEAR(mat.data[12], -(X.x * 3.0f - X.y * 5.0f + X.z * 10.0f), 0.000001f);
    ASSERT_NEAR(mat.data[13], -(Y.x * 3.0f - Y.y * 5.0f + Y.z * 10.0f), 0.000001f);
    ASSERT_NEAR(mat.data[14], -(Z.x * 3.0f - Z.y * 5.0f + Z.z * 10.0f), 0.000001f);
    ASSERT_FLOAT_EQ(mat.data[15], 1.0f);

    eye = Math::Vec3(0.0f, 0.0f, 1.0f);
    at = Math::Vec3(0.0f, 0.0f, 1.0f);
    up = Math::Vec3(0.0f, 0.0f, 0.0f);
    mat = Math::LookAtLh(eye, at, up);
    for (int i = 0; i < 14; i++)
        ASSERT_FLOAT_EQ(mat.data[i], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[15], 1.0f);
    eye = Math::Vec3(0.0f, 0.0f, 0.0f);
    at = Math::Vec3(0.0f, 0.0f, 0.0f);
    up = Math::Vec3(1.0f, 0.0f, 0.0f);
    mat = Math::LookAtLh(eye, at, up);
    for (int i = 0; i < 14; i++)
        ASSERT_FLOAT_EQ(mat.data[i], 0.0f);
    ASSERT_FLOAT_EQ(mat.data[15], 1.0f);
}

/**
 * @tc.name: Decompose
 * @tc.desc: Tests for Decompose. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, Decompose, testing::ext::TestSize.Level1) // RotationScaleTranslation z>x>y
{
    // Mat4X4 const& modelMatrix, Vec3& scale, Quat& orientation, Vec3& translation, Vec3& skew, Vec4& perspective
    Math::Vec3 scale;
    Math::Quat orientation;
    Math::Vec3 translation;
    Math::Vec3 skew;
    Math::Vec4 perspective;

    { // Identity decompose

        constexpr Math::Mat4X4 mat = Math::IDENTITY_4X4;
        bool dec = Math::Decompose(mat, scale, orientation, translation, skew, perspective);

        EXPECT_TRUE(dec);
        ASSERT_FLOAT_EQ(scale.data[0], 1.0f);
        ASSERT_FLOAT_EQ(scale.data[1], 1.0f);
        ASSERT_FLOAT_EQ(scale.data[2], 1.0f);

        ASSERT_FLOAT_EQ(translation.data[0], 0.0f);
        ASSERT_FLOAT_EQ(translation.data[1], 0.0f);
        ASSERT_FLOAT_EQ(translation.data[2], 0.0f);

        ASSERT_FLOAT_EQ(skew.data[0], 0.0f);
        ASSERT_FLOAT_EQ(skew.data[1], 0.0f);
        ASSERT_FLOAT_EQ(skew.data[2], 0.0f);

        ASSERT_FLOAT_EQ(orientation.data[0], 0.0f);
        ASSERT_FLOAT_EQ(orientation.data[1], 0.0f);
        ASSERT_FLOAT_EQ(orientation.data[2], 0.0f);
        ASSERT_FLOAT_EQ(orientation.data[3], 1.0f);

        ASSERT_FLOAT_EQ(perspective.data[0], 0.0f);
        ASSERT_FLOAT_EQ(perspective.data[1], 0.0f);
        ASSERT_FLOAT_EQ(perspective.data[2], 0.0f);
        ASSERT_FLOAT_EQ(perspective.data[3], 1.0f);
    }

    { // W, Scale, Rotation, Translation, Skew,

        constexpr Math::Vec4 C1 = { 2.0f, 0.0f, 0.0f, 0.0f };
        constexpr Math::Vec4 C2 = { 3.0f, 3.0f, 0.0f, 0.0f };
        constexpr Math::Vec4 C3 = { -4.0f, 5.0f, 4.0f, 0.0f };
        constexpr Math::Vec4 C4 = { 1.0f, 2.0f, 3.0f, -10.0f };
        constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
        constexpr Math::Mat4X4 mat(identity);

        bool dec = Math::Decompose(mat, scale, orientation, translation, skew, perspective);

        EXPECT_TRUE(dec);
        ASSERT_FLOAT_EQ(scale.data[0], -0.2f);
        ASSERT_FLOAT_EQ(scale.data[1], -0.3f);
        ASSERT_FLOAT_EQ(scale.data[2], -0.4f);

        ASSERT_FLOAT_EQ(translation.data[0], -0.1f);
        ASSERT_FLOAT_EQ(translation.data[1], -0.2f);
        ASSERT_FLOAT_EQ(translation.data[2], -0.3f);

        ASSERT_FLOAT_EQ(skew.data[0], 5.0f / 4.0f);  // y,z
        ASSERT_FLOAT_EQ(skew.data[1], -4.0f / 4.0f); // x,z
        ASSERT_FLOAT_EQ(skew.data[2], 3.0f / 3.0f);  // x,y

        ASSERT_FLOAT_EQ(orientation.data[0], 0.0f);
        ASSERT_FLOAT_EQ(orientation.data[1], 0.0f);
        ASSERT_FLOAT_EQ(orientation.data[2], 0.0f);
        ASSERT_FLOAT_EQ(orientation.data[3], 1.0f);

        ASSERT_FLOAT_EQ(perspective.data[0], 0.0f);
        ASSERT_FLOAT_EQ(perspective.data[1], 0.0f);
        ASSERT_FLOAT_EQ(perspective.data[2], 0.0f);
        ASSERT_FLOAT_EQ(perspective.data[3], 1.0f);
    }

    { // no W component

        constexpr Math::Vec4 C1 = { 2.0f, 0.0f, 0.0f, 1.0f };
        constexpr Math::Vec4 C2 = { 0.0f, 3.0f, 0.0f, 2.0f };
        constexpr Math::Vec4 C3 = { 0.0f, 0.0f, 4.0f, 3.0f };
        constexpr Math::Vec4 C4 = { 1.0f, 2.0f, 3.0f, 0.0f };
        constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
        constexpr Math::Mat4X4 mat(identity);

        bool dec = Math::Decompose(mat, scale, orientation, translation, skew, perspective);

        EXPECT_FALSE(dec);
    }

    { // Perspectives

        constexpr Math::Vec4 C1 = { 1.0f, 0.0f, 0.0f, -2.0f };
        constexpr Math::Vec4 C2 = { 0.0f, 0.2f, 0.0f, 0.6f };
        constexpr Math::Vec4 C3 = { 0.0f, 0.0f, 2.0f, -7.0f };
        constexpr Math::Vec4 C4 = { 0.0f, 0.0f, 0.0f, 2.0f };
        constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
        constexpr Math::Mat4X4 mat(identity);

        bool dec = Math::Decompose(mat, scale, orientation, translation, skew, perspective);

        EXPECT_TRUE(dec);
        ASSERT_FLOAT_EQ(perspective.data[0], -2.0f);
        ASSERT_FLOAT_EQ(perspective.data[1], 3.0f);
        ASSERT_FLOAT_EQ(perspective.data[2], -3.5f);
        ASSERT_FLOAT_EQ(perspective.data[3], 1.0f);
    }

    { // Determinant 0

        constexpr Math::Vec4 C1 = { 1.0f, 1.0f, 1.0f, 1.0f };
        constexpr Math::Vec4 C2 = { 1.0f, 1.0f, 1.0f, 1.0f };
        constexpr Math::Vec4 C3 = { 1.0f, 1.0f, 1.0f, 1.0f };
        constexpr Math::Vec4 C4 = { 0.0f, 0.0f, 0.0f, 1.0f };
        constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
        constexpr Math::Mat4X4 mat(identity);

        bool dec = Math::Decompose(mat, scale, orientation, translation, skew, perspective);

        EXPECT_FALSE(dec);
    }
    { // 90 deg xy

        constexpr Math::Vec4 C1 = { 0.0f, 1.0f, 0.0f, 0.0f };
        constexpr Math::Vec4 C2 = { -1.0f, 0.0f, 0.0f, 0.0f };
        constexpr Math::Vec4 C3 = { 0.0f, 0.0f, 1.0f, 0.0f };
        constexpr Math::Vec4 C4 = { 0.0f, 0.0f, 0.0f, 1.0f };
        constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
        constexpr Math::Mat4X4 mat(identity);

        bool dec = Math::Decompose(mat, scale, orientation, translation, skew, perspective);

        EXPECT_TRUE(dec);

        ASSERT_NEAR(orientation.data[0], 0.0f, 0.001f);
        ASSERT_NEAR(orientation.data[1], 0.0f, 0.001f);
        ASSERT_NEAR(orientation.data[2], 1.f / Math::sqrt(2.f), 0.001f);
        ASSERT_NEAR(orientation.data[3], 1.f / Math::sqrt(2.f), 0.001f);
    }

    { // RotationScaleTranslation z>x>y

        // constexpr Math::Vec3 translationIn(1.0f, 2.0f, 3.0f);
        // constexpr Math::Vec3 scaleIn(Math::Vec3(5.0f, 4.0f, 3.0f));
        // const Math::Quat rotationIn = Math::Quat(0.627f, -0.353f, 0.659f, -0.219f);
        // Math::Mat4X4 mat = Math::Trs(translationIn, rotationIn, scaleIn);

        constexpr Math::Vec4 C1 = { -0.588899f, -3.656520f, 3.358860f, 0.000000f };
        constexpr Math::Vec4 C2 = { -0.616080f, -2.619279f, -2.959520f, 0.000000f };
        constexpr Math::Vec4 C3 = { 2.943000f, -0.571884f, -0.106428f, 0.000000f };
        constexpr Math::Vec4 C4 = { 1.000000f, 2.000000f, 3.000000f, 1.000000f };
        constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
        constexpr Math::Mat4X4 mat(identity);

        bool dec = Math::Decompose(mat, scale, orientation, translation, skew, perspective);

        EXPECT_TRUE(dec);

        ASSERT_NEAR(orientation.data[0], 0.627f, 0.001f);
        ASSERT_NEAR(orientation.data[1], -0.353f, 0.001f);
        ASSERT_NEAR(orientation.data[2], 0.659f, 0.001f);
        ASSERT_NEAR(orientation.data[3], -0.219f, 0.001f);
        ASSERT_NEAR(scale.data[0], 5.0f, 0.001f);
        ASSERT_NEAR(scale.data[1], 4.0f, 0.001f);
        ASSERT_NEAR(scale.data[2], 3.0f, 0.001f);
        ASSERT_NEAR(translation.data[0], 1.0f, 0.001f);
        ASSERT_NEAR(translation.data[1], 2.0f, 0.001f);
        ASSERT_NEAR(translation.data[2], 3.0f, 0.001f);
    }

    { // RotationScaleTranslation z>y>x

        // constexpr Math::Vec3 translationIn(1.0f, 2.0f, 3.0f);
        // constexpr Math::Vec3 scaleIn(Math::Vec3(3.0f, 4.0f, 5.0f));
        // const Math::Quat rotationIn = Math::Quat(-0.353f, 0.627f, 0.659f, -0.219f);
        // Math::Mat4X4 mat = Math::Trs(translationIn, rotationIn, scaleIn);

        constexpr Math::Vec4 C1 = { -1.964460f, -2.193912f, -0.571884f, 0.000000f };
        constexpr Math::Vec4 C2 = { -0.616080f, -0.471119f, 3.924000f, 0.000000f };
        constexpr Math::Vec4 C3 = { -3.699400f, 3.358860f, -0.177379f, 0.000000f };
        constexpr Math::Vec4 C4 = { 1.000000f, 2.000000f, 3.000000f, 1.000000f };
        constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
        constexpr Math::Mat4X4 mat(identity);

        bool dec = Math::Decompose(mat, scale, orientation, translation, skew, perspective);

        EXPECT_TRUE(dec);

        ASSERT_NEAR(orientation.data[0], -0.353f, 0.001f);
        ASSERT_NEAR(orientation.data[1], 0.627f, 0.001f);
        ASSERT_NEAR(orientation.data[2], 0.659f, 0.001f);
        ASSERT_NEAR(orientation.data[3], -0.219f, 0.001f);
        ASSERT_NEAR(scale.data[0], 3.0f, 0.001f);
        ASSERT_NEAR(scale.data[1], 4.0f, 0.001f);
        ASSERT_NEAR(scale.data[2], 5.0f, 0.001f);
        ASSERT_NEAR(translation.data[0], 1.0f, 0.001f);
        ASSERT_NEAR(translation.data[1], 2.0f, 0.001f);
        ASSERT_NEAR(translation.data[2], 3.0f, 0.001f);
    }
}

/**
 * @tc.name: Decompose2
 * @tc.desc: Tests for Decompose2. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MathMatrixUtil, Decompose2, testing::ext::TestSize.Level1) // RotationScaleTranslation z>x>y
{
    // Mat4X4 const& modelMatrix, Vec3& scale, Quat& orientation, Vec3& translation
    Math::Vec3 scale;
    Math::Quat orientation;
    Math::Vec3 translation;

    { // Identity decompose

        constexpr Math::Mat4X4 mat = Math::IDENTITY_4X4;
        bool dec = Math::Decompose(mat, scale, orientation, translation);

        EXPECT_TRUE(dec);
        ASSERT_FLOAT_EQ(scale.data[0], 1.0f);
        ASSERT_FLOAT_EQ(scale.data[1], 1.0f);
        ASSERT_FLOAT_EQ(scale.data[2], 1.0f);

        ASSERT_FLOAT_EQ(translation.data[0], 0.0f);
        ASSERT_FLOAT_EQ(translation.data[1], 0.0f);
        ASSERT_FLOAT_EQ(translation.data[2], 0.0f);

        ASSERT_FLOAT_EQ(orientation.data[0], 0.0f);
        ASSERT_FLOAT_EQ(orientation.data[1], 0.0f);
        ASSERT_FLOAT_EQ(orientation.data[2], 0.0f);
        ASSERT_FLOAT_EQ(orientation.data[3], 1.0f);
    }

    { // Scale, Rotation, Translation
        constexpr auto inT = Math::Vec3(-0.1f, -0.2f, -0.3f);
        const auto inR = Math::AngleAxis(45.f * Math::DEG2RAD, Math::Vec3(0.f, 1.f, 0.f));
        constexpr auto inS = Math::Vec3(0.2f, 0.3f, 0.4f);
        Math::Mat4X4 mat(Math::Trs(inT, inR, inS));

        bool dec = Math::Decompose(mat, scale, orientation, translation);

        EXPECT_TRUE(dec);
        ASSERT_FLOAT_EQ(scale.data[0], inS.data[0]);
        ASSERT_FLOAT_EQ(scale.data[1], inS.data[1]);
        ASSERT_FLOAT_EQ(scale.data[2], inS.data[2]);

        ASSERT_FLOAT_EQ(translation.data[0], inT.data[0]);
        ASSERT_FLOAT_EQ(translation.data[1], inT.data[1]);
        ASSERT_FLOAT_EQ(translation.data[2], inT.data[2]);

        ASSERT_FLOAT_EQ(orientation.data[0], inR.data[0]);
        ASSERT_FLOAT_EQ(orientation.data[1], inR.data[1]);
        ASSERT_FLOAT_EQ(orientation.data[2], inR.data[2]);
        ASSERT_FLOAT_EQ(orientation.data[3], inR.data[3]);
    }

    { // Perspectives

        constexpr Math::Vec4 C1 = { 1.0f, 0.0f, 0.0f, -2.0f };
        constexpr Math::Vec4 C2 = { 0.0f, 0.2f, 0.0f, 0.6f };
        constexpr Math::Vec4 C3 = { 0.0f, 0.0f, 2.0f, -7.0f };
        constexpr Math::Vec4 C4 = { 0.0f, 0.0f, 0.0f, 2.0f };
        constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
        constexpr Math::Mat4X4 mat(identity);

        bool dec = Math::Decompose(mat, scale, orientation, translation);

        EXPECT_TRUE(dec);
    }

    { // 90 deg xy

        constexpr Math::Vec4 C1 = { 0.0f, 1.0f, 0.0f, 0.0f };
        constexpr Math::Vec4 C2 = { -1.0f, 0.0f, 0.0f, 0.0f };
        constexpr Math::Vec4 C3 = { 0.0f, 0.0f, 1.0f, 0.0f };
        constexpr Math::Vec4 C4 = { 0.0f, 0.0f, 0.0f, 1.0f };
        constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
        constexpr Math::Mat4X4 mat(identity);

        bool dec = Math::Decompose(mat, scale, orientation, translation);

        EXPECT_TRUE(dec);

        ASSERT_NEAR(orientation.data[0], 0.0f, 0.001f);
        ASSERT_NEAR(orientation.data[1], 0.0f, 0.001f);
        ASSERT_NEAR(orientation.data[2], 1.f / Math::sqrt(2.f), 0.001f);
        ASSERT_NEAR(orientation.data[3], 1.f / Math::sqrt(2.f), 0.001f);
    }

    { // RotationScaleTranslation z>x>y

        // constexpr Math::Vec3 translationIn(1.0f, 2.0f, 3.0f);
        // constexpr Math::Vec3 scaleIn(Math::Vec3(5.0f, 4.0f, 3.0f));
        // const Math::Quat rotationIn = Math::Quat(0.627f, -0.353f, 0.659f, -0.219f);
        // Math::Mat4X4 mat = Math::Trs(translationIn, rotationIn, scaleIn);

        constexpr Math::Vec4 C1 = { -0.588899f, -3.656520f, 3.358860f, 0.000000f };
        constexpr Math::Vec4 C2 = { -0.616080f, -2.619279f, -2.959520f, 0.000000f };
        constexpr Math::Vec4 C3 = { 2.943000f, -0.571884f, -0.106428f, 0.000000f };
        constexpr Math::Vec4 C4 = { 1.000000f, 2.000000f, 3.000000f, 1.000000f };
        constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
        constexpr Math::Mat4X4 mat(identity);

        bool dec = Math::Decompose(mat, scale, orientation, translation);

        EXPECT_TRUE(dec);

        ASSERT_NEAR(orientation.data[0], 0.627f, 0.001f);
        ASSERT_NEAR(orientation.data[1], -0.353f, 0.001f);
        ASSERT_NEAR(orientation.data[2], 0.659f, 0.001f);
        ASSERT_NEAR(orientation.data[3], -0.219f, 0.001f);
        ASSERT_NEAR(scale.data[0], 5.0f, 0.001f);
        ASSERT_NEAR(scale.data[1], 4.0f, 0.001f);
        ASSERT_NEAR(scale.data[2], 3.0f, 0.001f);
        ASSERT_NEAR(translation.data[0], 1.0f, 0.001f);
        ASSERT_NEAR(translation.data[1], 2.0f, 0.001f);
        ASSERT_NEAR(translation.data[2], 3.0f, 0.001f);
    }

    { // RotationScaleTranslation z>y>x

        // constexpr Math::Vec3 translationIn(1.0f, 2.0f, 3.0f);
        // constexpr Math::Vec3 scaleIn(Math::Vec3(3.0f, 4.0f, 5.0f));
        // const Math::Quat rotationIn = Math::Quat(-0.353f, 0.627f, 0.659f, -0.219f);
        // Math::Mat4X4 mat = Math::Trs(translationIn, rotationIn, scaleIn);

        constexpr Math::Vec4 C1 = { -1.964460f, -2.193912f, -0.571884f, 0.000000f };
        constexpr Math::Vec4 C2 = { -0.616080f, -0.471119f, 3.924000f, 0.000000f };
        constexpr Math::Vec4 C3 = { -3.699400f, 3.358860f, -0.177379f, 0.000000f };
        constexpr Math::Vec4 C4 = { 1.000000f, 2.000000f, 3.000000f, 1.000000f };
        constexpr Math::Mat4X4 identity(C1, C2, C3, C4);
        constexpr Math::Mat4X4 mat(identity);

        bool dec = Math::Decompose(mat, scale, orientation, translation);

        EXPECT_TRUE(dec);

        ASSERT_NEAR(orientation.data[0], -0.353f, 0.001f);
        ASSERT_NEAR(orientation.data[1], 0.627f, 0.001f);
        ASSERT_NEAR(orientation.data[2], 0.659f, 0.001f);
        ASSERT_NEAR(orientation.data[3], -0.219f, 0.001f);
        ASSERT_NEAR(scale.data[0], 3.0f, 0.001f);
        ASSERT_NEAR(scale.data[1], 4.0f, 0.001f);
        ASSERT_NEAR(scale.data[2], 5.0f, 0.001f);
        ASSERT_NEAR(translation.data[0], 1.0f, 0.001f);
        ASSERT_NEAR(translation.data[1], 2.0f, 0.001f);
        ASSERT_NEAR(translation.data[2], 3.0f, 0.001f);
    }
}
