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

// clang-format off
#include "helpers/testing_objects.h"
#include "helpers/test_utils.h"
// clang-format on

#include <meta/api/property/property_info.h>
#include <meta/interface/detail/enum.h>
#include <meta/interface/enum_macros.h>
#include <meta/interface/property/construct_property.h>

#include "test_framework.h"

META_BEGIN_NAMESPACE()
enum class MyPropEnum {
    A = 1,
    B = 2,
};

META_BEGIN_ENUM(MyPropEnum, "Nice Enum", MyPropEnum::A)
META_ENUM_VALUE(MyPropEnum::A, "MyA", "My A enumerator")
META_ENUM_VALUE(MyPropEnum::B, "MyB", "My B enumerator")
META_END_ENUM()

META_ENUM_TYPE(MyPropEnum)

namespace UTest {

/**
 * @tc.name: Info
 * @tc.desc: Tests for Info. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_PropertyInfoTest, Info, testing::ext::TestSize.Level1)
{
    auto p = ConstructProperty<MyPropEnum>("test");
    PropertyInfo info(p);
    ASSERT_TRUE(info);
    EXPECT_EQ(info->GetName(), "MyPropEnum");
    EXPECT_EQ(info->GetDescription(), "Nice Enum");
}

/**
 * @tc.name: ConstEnum
 * @tc.desc: Tests for Const Enum. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_PropertyInfoTest, ConstEnum, testing::ext::TestSize.Level1)
{
    auto p = ConstructProperty<MyPropEnum>("test");
    ConstPropertyEnumInfo info(p);
    ASSERT_TRUE(info);
    EXPECT_EQ(info->GetName(), "MyPropEnum");
    EXPECT_EQ(info->GetDescription(), "Nice Enum");
    EXPECT_EQ(info->GetEnumType(), EnumType::SINGLE_VALUE);
    EXPECT_EQ(info->GetValueIndex(), 0);
    EXPECT_EQ(info->GetValues().size(), 2);
}

/**
 * @tc.name: Enum
 * @tc.desc: Tests for Enum. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_PropertyInfoTest, Enum, testing::ext::TestSize.Level1)
{
    auto p = ConstructProperty<MyPropEnum>("test");
    PropertyEnumInfo info(p);
    ASSERT_TRUE(info);
    EXPECT_TRUE(info->SetValueIndex(1));
    EXPECT_EQ(info->GetValueIndex(), 1);
    EXPECT_EQ(p->GetValue(), MyPropEnum::B);
}

} // namespace UTest

META_END_NAMESPACE()
