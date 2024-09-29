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

#include <bitset>
#include <limits>
#include <type_traits>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <base/util/uid_util.h>

#include <meta/api/make_callback.h>
#include <meta/ext/object.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_class_registry.h>
#include <meta/interface/intf_object_registry.h>

#include "src/test_runner.h"

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

class IntfClassRegistryTest : public testing::Test {
protected:
    IntfClassRegistryTest() : classRegistry_(META_NS::GetClassRegistry()) {}

    void SetUp() override {}
    void TearDown() override {}
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        ResetTest();
    }

protected:
    IClassRegistry& classRegistry_;
};

MATCHER_P(ContainsClass, uid, "")
{
    for (const auto& info : arg) {
        if (info && info->GetClassInfo().Id() == uid) {
            return true;
        }
    }
    return false;
}

/**
 * @tc.name: IntfClassRegistryTest
 * @tc.desc: test GetAllClasses function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfClassRegistryTest, GetAllClasses, TestSize.Level1)
{
    auto all = classRegistry_.GetAllTypes({});
    EXPECT_THAT(all, Not(IsEmpty()));
}

/**
 * @tc.name: IntfClassRegistryTest
 * @tc.desc: test GetAllTypesUis function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfClassRegistryTest, GetAllTypesUis, TestSize.Level1)
{
    auto animations = classRegistry_.GetAllTypes({ IAnimation::UID });
    EXPECT_THAT(animations, Not(IsEmpty()));

    EXPECT_THAT(animations, ContainsClass(ClassId::KeyframeAnimation));

    auto staggered = classRegistry_.GetAllTypes({ IStaggeredAnimation::UID });

    EXPECT_THAT(staggered, Not(ContainsClass(ClassId::KeyframeAnimation)));
}

META_REGISTER_CLASS(MyObject1, "440d971e-afae-497a-95c3-97c3ba7f5d18", META_NS::ObjectCategoryBits::APPLICATION)
META_REGISTER_CLASS(MyObject2, "f8d418d7-99aa-4b19-92b7-319959cc98f1", META_NS::ObjectCategoryBits::APPLICATION)

class MyObject1 : public META_NS::ObjectFwd<MyObject1, ClassId::MyObject1, META_NS::ClassId::Object> {
public:
};
class MyObject2 : public META_NS::ObjectFwd<MyObject2, ClassId::MyObject2, META_NS::ClassId::Object> {
public:
};

/**
 * @tc.name: IntfClassRegistryTest
 * @tc.desc: test Events function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfClassRegistryTest, Events, TestSize.Level1)
{
    auto& objReg = META_NS::GetObjectRegistry();
    auto& classReg = objReg.GetClassRegistry();

    int registered = 0;
    int unregistered = 0;
    ClassRegistrationInfo lastRegistered;
    ClassRegistrationInfo lastUnregistered;

    auto token1 = classReg.OnClassRegistered()->AddHandler(META_NS::MakeCallback<META_NS::IOnClassRegistrationChanged>(
        [&registered, &lastRegistered](const ClassRegistrationInfo& info) {
            registered++;
            lastRegistered = info;
        }));

    auto token2 =
        classReg.OnClassUnregistered()->AddHandler(META_NS::MakeCallback<META_NS::IOnClassRegistrationChanged>(
            [&unregistered, &lastUnregistered](const ClassRegistrationInfo& info) {
                unregistered++;
                lastUnregistered = info;
            }));

    EXPECT_TRUE(objReg.RegisterObjectType(MyObject1::GetFactory()));
    EXPECT_EQ(registered, 1);
    EXPECT_EQ(unregistered, 0);
    EXPECT_EQ(lastRegistered.classInfo, MyObject1::GetFactory());
    EXPECT_EQ(lastUnregistered.classInfo, nullptr);
    lastRegistered.classInfo.reset();

    EXPECT_FALSE(objReg.RegisterObjectType(MyObject1::GetFactory()));
    EXPECT_EQ(registered, 1);
    EXPECT_EQ(unregistered, 0);
    EXPECT_EQ(lastRegistered.classInfo, nullptr);
    EXPECT_EQ(lastUnregistered.classInfo, nullptr);

    EXPECT_TRUE(objReg.RegisterObjectType(MyObject2::GetFactory()));
    EXPECT_EQ(registered, 2);
    EXPECT_EQ(unregistered, 0);
    EXPECT_EQ(lastRegistered.classInfo, MyObject2::GetFactory());
    EXPECT_EQ(lastUnregistered.classInfo, nullptr);
    lastRegistered.classInfo.reset();

    EXPECT_TRUE(objReg.UnregisterObjectType(MyObject1::GetFactory()));
    EXPECT_EQ(registered, 2);
    EXPECT_EQ(unregistered, 1);
    EXPECT_EQ(lastRegistered.classInfo, nullptr);
    EXPECT_EQ(lastUnregistered.classInfo, MyObject1::GetFactory());
    lastUnregistered.classInfo.reset();

    EXPECT_FALSE(objReg.UnregisterObjectType(MyObject1::GetFactory()));
    EXPECT_EQ(registered, 2);
    EXPECT_EQ(unregistered, 1);
    EXPECT_EQ(lastRegistered.classInfo, nullptr);
    EXPECT_EQ(lastUnregistered.classInfo, nullptr);

    EXPECT_TRUE(objReg.UnregisterObjectType(MyObject2::GetFactory()));
    EXPECT_EQ(registered, 2);
    EXPECT_EQ(unregistered, 2);
    EXPECT_EQ(lastRegistered.classInfo, nullptr);
    EXPECT_EQ(lastUnregistered.classInfo, MyObject2::GetFactory());

    EXPECT_TRUE(classReg.OnClassRegistered()->RemoveHandler(token1));
    EXPECT_TRUE(classReg.OnClassUnregistered()->RemoveHandler(token2));
}

META_END_NAMESPACE()