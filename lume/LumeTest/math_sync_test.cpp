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