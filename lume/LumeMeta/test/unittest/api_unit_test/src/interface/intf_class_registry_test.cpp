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

#include <bitset>
#include <limits>
#include <test_framework.h>
#include <type_traits>

#include <base/util/uid_util.h>

#include <meta/api/make_callback.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_class_registry.h>
#include <meta/interface/intf_object_registry.h>

META_BEGIN_NAMESPACE()
namespace UTest {

class API_ClassRegistryTest : public ::testing::Test {
protected:
    API_ClassRegistryTest() : classRegistry_(META_NS::GetClassRegistry()) {}

    void SetUp() override {}
    void TearDown() override {}

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
 * @tc.name: GetAllClasses
 * @tc.desc: Tests for Get All Classes. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ClassRegistryTest, GetAllClasses, testing::ext::TestSize.Level1)
{
    auto all = classRegistry_.GetAllTypes({});
    EXPECT_THAT(all, testing::Not(testing::IsEmpty()));
}

/**
 * @tc.name: GetAllTypesUis
 * @tc.desc: Tests for Get All Types Uis. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ClassRegistryTest, GetAllTypesUis, testing::ext::TestSize.Level1)
{
    auto animations = classRegistry_.GetAllTypes({ IAnimation::UID });
    EXPECT_THAT(animations, testing::Not(testing::IsEmpty()));
    EXPECT_THAT(animations, ContainsClass(META_NS::ClassId::KeyframeAnimation));

    auto staggered = classRegistry_.GetAllTypes({ IStaggeredAnimation::UID });
    EXPECT_THAT(staggered, testing::Not(ContainsClass(META_NS::ClassId::KeyframeAnimation)));
}

META_REGISTER_CLASS(MyObject1, "440d971e-afae-497a-95c3-97c3ba7f5d18", META_NS::ObjectCategoryBits::APPLICATION)
META_REGISTER_CLASS(MyObject2, "f8d418d7-99aa-4b19-92b7-319959cc98f1", META_NS::ObjectCategoryBits::APPLICATION)

class MyObject1 : public META_NS::ObjectFwd {
    META_OBJECT(MyObject1, ClassId::MyObject1, ObjectFwd, META_NS::ClassId::Object)
};
class MyObject2 : public META_NS::ObjectFwd {
    META_OBJECT(MyObject2, ClassId::MyObject2, ObjectFwd, META_NS::ClassId::Object)
};

/**
 * @tc.name: Invalid
 * @tc.desc: Tests for Invalid. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ClassRegistryTest, Invalid, testing::ext::TestSize.Level1)
{
    auto& reg = META_NS::GetObjectRegistry();
    EXPECT_FALSE(reg.RegisterObjectType(nullptr));
    EXPECT_FALSE(reg.UnregisterObjectType(nullptr));
}

/**
 * @tc.name: Events
 * @tc.desc: Tests for Events. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ClassRegistryTest, Events, testing::ext::TestSize.Level1)
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

} // namespace UTest
META_END_NAMESPACE()
