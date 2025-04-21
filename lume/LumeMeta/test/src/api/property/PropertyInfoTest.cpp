/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <TestRunner.h>
#include <src/testing_objects.h>

#include <gtest/gtest.h>

#include <meta/api/property/property_info.h>
#include <meta/interface/detail/enum.h>
#include <meta/interface/enum_macros.h>
#include <meta/interface/property/construct_property.h>

using namespace testing;
using namespace testing::ext;

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

class PropertyInfoTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: Info
 * @tc.desc: test Info
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(PropertyInfoTest, Info, TestSize.Level1)
{
    auto p = ConstructProperty<MyPropEnum>("test");
    PropertyInfo info(p);
    ASSERT_TRUE(info);
    EXPECT_EQ(info->GetName(), "MyPropEnum");
    EXPECT_EQ(info->GetDescription(), "Nice Enum");
}

/**
 * @tc.name: ConstEnum
 * @tc.desc: test ConstEnum
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(PropertyInfoTest, ConstEnum, TestSize.Level1)
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
 * @tc.desc: test Enum
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(PropertyInfoTest, Enum, TestSize.Level1)
{
    auto p = ConstructProperty<MyPropEnum>("test");
    PropertyEnumInfo info(p);
    ASSERT_TRUE(info);
    EXPECT_TRUE(info->SetValueIndex(1));
    EXPECT_EQ(info->GetValueIndex(), 1);
    EXPECT_EQ(p->GetValue(), MyPropEnum::B);
}

META_END_NAMESPACE()
