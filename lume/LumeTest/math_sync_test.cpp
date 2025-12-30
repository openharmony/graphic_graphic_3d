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