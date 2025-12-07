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

#include <test_framework.h>

#include <meta/api/animation.h>
#include <meta/api/curves.h>
#include <meta/base/namespace.h>

#include "helpers/test_data_helpers.h"

using namespace META_NS::TimeSpanLiterals;

META_BEGIN_NAMESPACE()

namespace UTest {

// clang-format off

const TestTypes InterpolationTestData(
    TType<bool> { true, false },
    TType<int8_t> { 2, -100 },
    TType<int16_t> { 4012, -10 },
    TType<int32_t> { 111111, -45114 },
    TType<int64_t> { 1234567890LL, -5555555LL },
    TType<uint8_t> { 2, 200 },
    TType<uint16_t> { 4012, 56000 },
    TType<uint32_t> { 115111, 455647114 },
    TType<uint64_t> { 5, 99999999999ULL },
    TType<float> { 1.0f, 2.33333f },
    TType<double> { 3.0, -4363.12455 },
    TType<BASE_NS::Math::Vec2> { BASE_NS::Math::Vec2 { 1.3f, 5.0f },  BASE_NS::Math::Vec2 { -3.1f, 13.0f } },
    TType<BASE_NS::Math::Vec3> { BASE_NS::Math::Vec3 { 1.f, 2.f, 3.f }, BASE_NS::Math::Vec3 { 4.f, 1.2f, 0.88f } },
    TType<BASE_NS::Math::Vec4> { BASE_NS::Math::Vec4 { 4.f, 5.f, 6.f, 7.f }, BASE_NS::Math::Vec4 { -0.005f, 0.13f, 13.f, 100.f } },
    TType<BASE_NS::Math::UVec2> { BASE_NS::Math::UVec2 { 1, 2 }, BASE_NS::Math::UVec2 { 6, 7 } },
    TType<BASE_NS::Math::UVec3> { BASE_NS::Math::UVec3 { 1, 2, 3 }, BASE_NS::Math::UVec3 { 6, 7, 8 } },
    TType<BASE_NS::Math::UVec4> { BASE_NS::Math::UVec4 { 1, 2, 3, 4 }, BASE_NS::Math::UVec4 { 6, 7, 8, 9 } },
    TType<BASE_NS::Math::IVec2> { BASE_NS::Math::IVec2 { -1, 1 }, BASE_NS::Math::IVec2 { 60, -50 } },
    TType<BASE_NS::Math::IVec3> { BASE_NS::Math::IVec3 { -1, 1, 0 }, BASE_NS::Math::IVec3 { 60, -50, 1 } },
    TType<BASE_NS::Math::IVec4> { BASE_NS::Math::IVec4 { -1, 1, 9, -50 }, BASE_NS::Math::IVec4 { 60, -50, 2, 3 } },
    TType<BASE_NS::Math::Quat> { BASE_NS::Math::Quat { 4.f, 5.f, 6.f, 7.f }, BASE_NS::Math::Quat { -0.005f, 0.13f, 13.f, 100.f } },
    TType<BASE_NS::Math::Mat3X3> { BASE_NS::Math::Mat3X3 { BASE_NS::Math::Vec3 { 1.f, 2.f, 3.f }, BASE_NS::Math::Vec3 { 4.f, 5.f, 6.f }, BASE_NS::Math::Vec3 { 7.f, 8.f, 9.f } }, BASE_NS::Math::Mat3X3 { 3.3f } },
    TType<BASE_NS::Math::Mat4X4> { BASE_NS::Math::Mat4X4 { 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f }, BASE_NS::Math::Mat4X4 { 2.2f } });
// clang-format on

template<typename Type>
class API_InterpolatorTest : public ::testing::Test {
public:
    API_InterpolatorTest()
        : value1_(InterpolationTestData.GetValue<Type>(0)), value2_(InterpolationTestData.GetValue<Type>(1))
    {}

protected:
    void SetUp() override
    {
        typeId_ = UidFromType<Type>();
        interpolator_ = GetObjectRegistry().CreateInterpolator(typeId_);
        ASSERT_TRUE(interpolator_);
    }

    BASE_NS::Uid GetTypeId() const
    {
        return typeId_;
    }

    IInterpolator::Ptr GetInterpolator() const
    {
        return interpolator_;
    }

    Type GetDefValue() const
    {
        return {};
    }
    Type GetValue1() const
    {
        return value1_;
    }
    Type GetValue2() const
    {
        return value2_;
    }

private:
    Type value1_;
    Type value2_;
    BASE_NS::Uid typeId_;
    IInterpolator::Ptr interpolator_;
};

TYPED_TEST_SUITE(API_InterpolatorTest, decltype(InterpolationTestData)::Types);

TYPED_TEST(API_InterpolatorTest, Compatibility)
{
    EXPECT_TRUE(this->GetInterpolator()->IsCompatibleWith(this->GetTypeId()));
}

TYPED_TEST(API_InterpolatorTest, CompatibilityInvalid)
{
    auto interpolator = this->GetInterpolator();
    // Default interpolator is compatible with anything
    bool isDefault = interface_cast<IObject>(interpolator)->GetClassId() == ClassId::DefaultInterpolator;
    EXPECT_EQ(interpolator->IsCompatibleWith(UidFromType<BASE_NS::string>()), isDefault);
}

TYPED_TEST(API_InterpolatorTest, Interpolate)
{
    Any<TypeParam> v1(this->GetValue1());
    Any<TypeParam> v2(this->GetValue2());
    Any<TypeParam> output;
    auto interpolator = this->GetInterpolator();
    EXPECT_TRUE(interpolator->Interpolate(output, v1, v2, 0.5f));
    EXPECT_TRUE(interpolator->Interpolate(output, v1, v2, 0.f));
    EXPECT_TRUE(interpolator->Interpolate(output, v1, v2, 1.f));
    EXPECT_TRUE(interpolator->Interpolate(output, v1, v2, -1.f));
    EXPECT_TRUE(interpolator->Interpolate(output, v1, v2, 2.f));
}

TYPED_TEST(API_InterpolatorTest, InterpolateInvalid)
{
    Any<TypeParam> v1(this->GetValue1());
    Any<TypeParam> v2(this->GetValue2());
    Any<TypeParam> output;
    Any<BASE_NS::string> invalid;
    auto interpolator = this->GetInterpolator();
    // Default interpolator is compatible with anything
    bool isDefault = interface_cast<IObject>(interpolator)->GetClassId() == ClassId::DefaultInterpolator;
    // Always fail set to invalid
    EXPECT_EQ(interpolator->Interpolate(invalid, v1, v2, 0.5f), false);
    // Should copy from first param as t<0.5f
    EXPECT_EQ(interpolator->Interpolate(output, invalid, v2, 0.f), false);
    // Should copy from second param as t>0.5f
    EXPECT_EQ(interpolator->Interpolate(output, v1, invalid, 1.f), false);
    // Should copy from second param as t>0.5f
    EXPECT_EQ(interpolator->Interpolate(output, invalid, v2, 1.f), isDefault);
    // Should copy from first param as t<0.5f
    EXPECT_EQ(interpolator->Interpolate(output, v1, invalid, 0.f), isDefault);
}

} // namespace UTest

META_END_NAMESPACE()
