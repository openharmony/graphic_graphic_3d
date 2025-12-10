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

#include <core/property_tools/core_metadata.inl>
#include <core/property_tools/property_data.h>

#include "test_framework.h"

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(BASE_NS::vector<BASE_NS::Math::Vec2>);
CORE_END_NAMESPACE()

using namespace CORE_NS;
namespace {
struct Test {
    float fValue;
    BASE_NS::Math::Vec4 vec4;
    BASE_NS::Math::Vec2 vec2Array[4];
    BASE_NS::vector<float> fVector;
    BASE_NS::vector<BASE_NS::Math::Vec2> vec2Vector;
};

// clang-format off

PROPERTY_LIST(Test, TestProperties,
    MEMBER_PROPERTY(vec4, "Factor", 0U),
    MEMBER_PROPERTY(vec2Array, "Factor array", 0U),
    MEMBER_PROPERTY(fVector, "Vector of floats", 0U),
    MEMBER_PROPERTY(fValue, "Float Value", 0U),
    MEMBER_PROPERTY(vec2Vector, "Vector of Vec2 Value", 0U))
// clang-format on
} // namespace

template<typename To, typename From>
auto Cast(From* f)
{
    if constexpr (BASE_NS::is_const_v<From>) {
        return static_cast<const To*>(static_cast<const void*>(f));
    } else {
        return static_cast<To*>(static_cast<void*>(f));
    }
}

/**
 * @tc.name: FindProperty
 * @tc.desc: Tests for Find Property. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilPropertyToolsTest, FindProperty, testing::ext::TestSize.Level1)
{
    ::Test t { 42.f, { 1.f, 2.f, 3.f, 4.f }, { { 5.f, 6.f }, { 7.f, 8.f }, { 9.f, 10.f }, { 11.f, 12.f } } };
    t.fVector.push_back(13.f);
    const auto* tStart = Cast<const uint8_t>(&t);
    // direct member
    {
        const auto fValueProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "fValue", 0);
        EXPECT_TRUE((fValueProperty));
        EXPECT_EQ(fValueProperty.offset, offsetof(::Test, fValue));
        EXPECT_EQ(*Cast<float>(tStart + fValueProperty.offset), t.fValue);
    }
    {
        const auto vec4Property = CORE_NS::PropertyData::FindProperty(TestProperties, "vec4", 0);
        EXPECT_TRUE((vec4Property));
        EXPECT_EQ(vec4Property.offset, offsetof(::Test, vec4));
        EXPECT_EQ(*Cast<BASE_NS::Math::Vec4>(tStart + vec4Property.offset), t.vec4);
    }
    {
        // try giving the address of the variable. returned offset should then point directly to the member and e.g. can
        // be used with the properties containerMethods.
        const auto fVectorProperty =
            CORE_NS::PropertyData::FindProperty(TestProperties, "fVector", reinterpret_cast<uintptr_t>(tStart));
        ASSERT_TRUE((fVectorProperty));
        EXPECT_EQ(fVectorProperty.offset, reinterpret_cast<uintptr_t>(tStart) + offsetof(::Test, fVector));
        const auto* containerMethods = fVectorProperty.property->metaData.containerMethods;
        ASSERT_TRUE(containerMethods);
        EXPECT_EQ(containerMethods->size(fVectorProperty.offset), t.fVector.size());
        EXPECT_EQ(*reinterpret_cast<const float*>(containerMethods->get(fVectorProperty.offset, 0)), t.fVector[0]);
    }

    // missing properties
    {
        const auto vecProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec", 0);
        EXPECT_FALSE((vecProperty));
    }
}

/**
 * @tc.name: FindPropertyMember
 * @tc.desc: Tests for Find Property Member. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilPropertyToolsTest, FindPropertyMember, testing::ext::TestSize.Level1)
{
    ::Test t { 42.f, { 1.f, 2.f, 3.f, 4.f }, { { 5.f, 6.f }, { 7.f, 8.f }, { 9.f, 10.f }, { 11.f, 12.f } } };
    t.fVector.push_back(13.f);
    const auto* tStart = Cast<const uint8_t>(&t);

    // member's member
    {
        const auto vec4YProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec4.y", 0);
        EXPECT_TRUE((vec4YProperty));
        EXPECT_EQ(vec4YProperty.offset, offsetof(::Test, vec4) + offsetof(BASE_NS::Math::Vec4, y));
        EXPECT_EQ(*Cast<float>(tStart + vec4YProperty.offset), t.vec4.y);
    }

    // missing properties
    {
        const auto vec4BProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec4.b", 0);
        EXPECT_FALSE((vec4BProperty));
    }
    {
        const auto vec4Property = CORE_NS::PropertyData::FindProperty(TestProperties, "vec4.", 0);
        EXPECT_FALSE((vec4Property));
    }

    // invalid syntax
    {
        const auto property = CORE_NS::PropertyData::FindProperty(TestProperties, ".", 0);
        EXPECT_FALSE((property));
    }
}

/**
 * @tc.name: FindPropertyArray
 * @tc.desc: Tests for Find Property Array. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilPropertyToolsTest, FindPropertyArray, testing::ext::TestSize.Level1)
{
    ::Test t { 42.f, { 1.f, 2.f, 3.f, 4.f }, { { 5.f, 6.f }, { 7.f, 8.f }, { 9.f, 10.f }, { 11.f, 12.f } } };
    t.fVector.push_back(13.f);
    t.vec2Vector.push_back({ 14.f, 15.f });
    t.vec2Vector.push_back({ 16.f, 17.f });

    const auto* tStart = Cast<const uint8_t>(&t);

    // array
    {
        const auto vec2ArrayProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Array[1]", 0);
        EXPECT_TRUE((vec2ArrayProperty));
        EXPECT_EQ(vec2ArrayProperty.offset, offsetof(::Test, vec2Array) + sizeof(BASE_NS::Math::Vec2));
        EXPECT_EQ(*Cast<BASE_NS::Math::Vec2>(tStart + vec2ArrayProperty.offset), t.vec2Array[1]);
    }

    // member from array
    {
        const auto vec2ArrayProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Array[2].x", 0);
        EXPECT_TRUE((vec2ArrayProperty));
        EXPECT_EQ(vec2ArrayProperty.offset,
            offsetof(::Test, vec2Array) + (2 * sizeof(BASE_NS::Math::Vec2)) + offsetof(BASE_NS::Math::Vec2, x));
        EXPECT_EQ(*Cast<float>(tStart + vec2ArrayProperty.offset), t.vec2Array[2].x);
    }

    // member from vector
    {
        const auto vec2VectorProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Vector", 0);
        EXPECT_TRUE((vec2VectorProperty));
        EXPECT_EQ(vec2VectorProperty.offset, offsetof(::Test, vec2Vector));
        const auto vec2Property =
            CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Vector[1]", reinterpret_cast<uintptr_t>(tStart));
        EXPECT_TRUE((vec2Property));
        EXPECT_EQ(vec2Property.offset, reinterpret_cast<uintptr_t>(&t.vec2Vector[1]));
        const auto floatProperty =
            CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Vector[1].y", reinterpret_cast<uintptr_t>(tStart));
        EXPECT_TRUE((floatProperty));
        EXPECT_EQ(*reinterpret_cast<const float*>(floatProperty.offset), (t.vec2Vector[1].y));
    }

    // invalid syntax
    {
        const auto vec2ArrayProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Array[]", 0);
        EXPECT_FALSE((vec2ArrayProperty));
    }
    {
        const auto vec2ArrayProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Array[1", 0);
        EXPECT_FALSE((vec2ArrayProperty));
    }
    {
        const auto vec2ArrayProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Array[", 0);
        EXPECT_FALSE((vec2ArrayProperty));
    }

    // over indexing
    {
        const auto vec2ArrayProperty = CORE_NS::PropertyData::FindProperty(TestProperties, "vec2Array[6]", 0);
        EXPECT_FALSE((vec2ArrayProperty));
    }
}
