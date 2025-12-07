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

#include <meta/api/number.h>
#include <meta/interface/detail/any.h>

#include "helpers/testing_objects.h"
#include "test_framework.h"

META_BEGIN_NAMESPACE()

namespace UTest {

enum MyEnum : int16_t { ONE = 1, TWO, THREE };

/**
 * @tc.name: Values
 * @tc.desc: Tests for Values. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_NumberTest, Values, testing::ext::TestSize.Level1)
{
    Number n = 1;
    EXPECT_EQ(n.Get<uint32_t>(), 1);
    EXPECT_EQ(n.Get<int32_t>(), 1);
    EXPECT_EQ(n.Get<float>(), 1);
    EXPECT_EQ(n.Get<MyEnum>(), MyEnum::ONE);

    n = Any<uint64_t>(10);
    EXPECT_EQ(n.Get<uint32_t>(), 10);

    n = 1.2f;
    EXPECT_EQ(n.Get<uint32_t>(), 1);
    EXPECT_EQ(n.Get<int32_t>(), 1);
    EXPECT_NEAR(n.Get<float>(), 1.2, 0.0001);

    Number en = MyEnum::TWO;
    EXPECT_EQ(en.Get<uint32_t>(), 2);
    EXPECT_EQ(en.Get<MyEnum>(), MyEnum::TWO);
}

template<class T>
void GetSetNumber(const IAny::Ptr& any, T value)
{
    auto typeId = META_NS::UidFromType<T>();
    auto size = sizeof(T);
    EXPECT_TRUE(any->SetData(typeId, &value, size));
    T current {};
    EXPECT_TRUE(any->GetData(typeId, &current, size));
    EXPECT_EQ(value, current);

    Any<float> f(4.f);
    Any<double> d(2.f);
    Any<bool> b(false);
    Any<uint8_t> u8(4);
    Any<uint16_t> u16(5);
    Any<uint32_t> u32(6);
    Any<uint64_t> u64(7);
    Any<int8_t> i8(8);
    Any<int16_t> i16(9);
    Any<int32_t> i32(10);
    Any<int64_t> i64(11);
    EXPECT_TRUE(any->CopyFrom(f));
    EXPECT_TRUE(any->CopyFrom(d));
    EXPECT_TRUE(any->CopyFrom(b));
    EXPECT_TRUE(any->CopyFrom(u8));
    EXPECT_TRUE(any->CopyFrom(u16));
    EXPECT_TRUE(any->CopyFrom(u32));
    EXPECT_TRUE(any->CopyFrom(u64));
    EXPECT_TRUE(any->CopyFrom(i8));
    EXPECT_TRUE(any->CopyFrom(i16));
    EXPECT_TRUE(any->CopyFrom(i32));
    EXPECT_TRUE(any->CopyFrom(i64));
}

/**
 * @tc.name: Types
 * @tc.desc: Test all supported types for Number.
 * @tc.type: FUNC
 */
UNIT_TEST(API_NumberTest, Types, testing::ext::TestSize.Level1)
{
    auto number = META_NS::GetObjectRegistry().Create<IAny>(META_NS::ClassId::Number);
    ASSERT_TRUE(number);
    GetSetNumber<float>(number, 42.f);
    GetSetNumber<double>(number, 21.f);
    GetSetNumber<bool>(number, true);
    GetSetNumber<uint8_t>(number, 42);
    GetSetNumber<uint16_t>(number, 43);
    GetSetNumber<uint32_t>(number, 44);
    GetSetNumber<uint64_t>(number, 45);
    GetSetNumber<int8_t>(number, 46);
    GetSetNumber<int16_t>(number, 47);
    GetSetNumber<int32_t>(number, 48);
    GetSetNumber<int64_t>(number, 49);

    EXPECT_FALSE(number->GetCompatibleTypes(META_NS::CompatibilityDirection::GET).empty());
    EXPECT_FALSE(number->GetCompatibleTypes(META_NS::CompatibilityDirection::SET).empty());
    EXPECT_FALSE(number->GetCompatibleTypes(META_NS::CompatibilityDirection::BOTH).empty());
}

/**
 * @tc.name: Functions
 * @tc.desc: Test IAny interface for Number.
 * @tc.type: FUNC
 */
UNIT_TEST(API_NumberTest, Functions, testing::ext::TestSize.Level1)
{
    auto number = META_NS::GetObjectRegistry().Create<IAny>(META_NS::ClassId::Number);
    ASSERT_TRUE(number);

    auto any = META_NS::Any<IObject::Ptr>(); // Invalid type for Number

    EXPECT_FALSE(number->SetData({}, nullptr, 0));
    EXPECT_FALSE(number->GetData({}, nullptr, 0));
    EXPECT_TRUE(number->ResetValue());
    EXPECT_FALSE(number->CopyFrom(any));
    META_NS::AnyCloneOptions opt;
    opt.value = META_NS::CloneValueType::COPY_VALUE;
    opt.role = META_NS::TypeIdRole::ITEM;
    EXPECT_TRUE(number->Clone(opt));
    opt.value = META_NS::CloneValueType::DEFAULT_VALUE;
    EXPECT_TRUE(number->Clone(opt));
    opt.role = META_NS::TypeIdRole::ARRAY;
    EXPECT_FALSE(number->Clone(opt));
    EXPECT_NE(number->GetTypeId(META_NS::TypeIdRole::ITEM), META_NS::TypeId {});
    EXPECT_NE(number->GetTypeId(META_NS::TypeIdRole::ARRAY), META_NS::TypeId {});
    EXPECT_FALSE(number->GetTypeIdString().empty());
}

} // namespace UTest

META_END_NAMESPACE()
