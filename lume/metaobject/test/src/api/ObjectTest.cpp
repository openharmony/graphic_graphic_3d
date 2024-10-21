/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/object.h>

#include "src/test_runner.h"
#include "src/testing_objects.h"

using namespace testing::ext;

META_BEGIN_NAMESPACE()

class ObjectTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        ResetTest();
    }
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: ObjectTest
 * @tc.desc: test ObjectBaseAPI function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ObjectTest, ObjectBaseAPI, TestSize.Level1)
{
    META_NS::Object object;

    EXPECT_NE(object.GetIObject(), nullptr);
    EXPECT_NE(object.GetInterfacePtr<IObject>(), nullptr);
    EXPECT_EQ(object.GetInterfacePtr<IObject>(), object.GetIObject());
    EXPECT_EQ(interface_pointer_cast<IObject>(object), object);
    EXPECT_EQ(interface_cast<IObject>(object), object.GetIObject().get());

    META_NS::Object object2;
    ASSERT_TRUE(object2.Initialize(object));
    EXPECT_EQ(object, object2);
    EXPECT_TRUE(object == object2);
    EXPECT_FALSE(object != object2);

    META_NS::Object object3;
    EXPECT_TRUE(object != object3);
    EXPECT_FALSE(object == object3);
    EXPECT_FALSE(object3);
    EXPECT_NE(object3.GetInterfacePtr<IObject>(), nullptr);
    EXPECT_TRUE(object3);
}

/**
 * @tc.name: ObjectTest
 * @tc.desc: test TypeConversion function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ObjectTest, TypeConversion, TestSize.Level1)
{
    META_NS::Object o1(interface_pointer_cast<IObject>(CreateTestType()));
    META_NS::Object o2(interface_pointer_cast<IObject>(CreateTestContainer()));

    EXPECT_TRUE(o1);
    EXPECT_TRUE(o2);
    EXPECT_NE(o1.GetIObject()->GetClassId(), o2.GetIObject()->GetClassId());

    o1 = o2;
    EXPECT_TRUE(o1);
    EXPECT_EQ(o1.GetIObject(), o2.GetIObject());
    EXPECT_EQ(o1.GetIObject()->GetClassId(), o2.GetIObject()->GetClassId());
}

/**
 * @tc.name: ObjectTest
 * @tc.desc: test LazyInitBase function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ObjectTest, LazyInitBase, TestSize.Level1)
{
    static constexpr auto propName = "value";
    auto o1 = Object();
    Object o2;

    EXPECT_FALSE(o1);
    EXPECT_FALSE(o2);
    o2 = o1;
    EXPECT_TRUE(o1);
    EXPECT_TRUE(o2);
    Object o3;
    EXPECT_FALSE(o3);
    o1.MetaProperty(propName, 5);
    o3 = o1;
    EXPECT_TRUE(o1);
    EXPECT_TRUE(o2);
    EXPECT_TRUE(o3);
    EXPECT_EQ(o1, o2);
    EXPECT_EQ(o1, o3);

    auto p1 = o1.Metadata().GetPropertyByName(propName);
    auto p2 = o2.Metadata().GetPropertyByName(propName);
    auto p3 = o3.Metadata().GetPropertyByName(propName);

    EXPECT_EQ(p1, p3);
    EXPECT_EQ(p2, p1);
}

/**
 * @tc.name: ObjectTest
 * @tc.desc: test ResetIObject function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ObjectTest, ResetIObject, TestSize.Level1)
{
    META_NS::Object object1;
    META_NS::Object object2;
    auto iobject1 = object1.GetIObject();
    object2 = object1;
    auto iobject2 = object2.GetIObject();
    EXPECT_EQ(iobject1, iobject2);

    object1.ResetIObject();
    EXPECT_FALSE(object1);
    EXPECT_NE(object1.GetIObject(), iobject2);
}
META_END_NAMESPACE()