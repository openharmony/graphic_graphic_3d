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

#include <gtest/gtest.h>

#include <meta/api/function.h>
#include <meta/interface/intf_metadata.h>

#include "src/test_runner.h"s
#include "src/testing_objects.h"
#include "src/util.h"

using namespace testing;
using namespace testing::ext;

namespace {
    const int ZERO = 0; // test value for Set/Get function
    const int ONE = 1; // test value for Set/Get function
    const int TWO = 2; // test value for Set/Get function
    const int THREE = 3; // test value for Set/Get function
}

META_BEGIN_NAMESPACE()

class IntfMetadataTest : public testing::Test {
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
 * @tc.name: IntfMetadataTest
 * @tc.desc: test Functions function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfMetadataTest, Functions, TestSize.Level1)
{
    auto p = CreateTestType();
    ITestType::ConstPtr cp = p;

    auto m = interface_cast<IMetadata>(p);
    ASSERT_TRUE(m);

    {
        auto f = m->GetFunctionByName("MyTestFunc");
        ASSERT_TRUE(f);
        EXPECT_FALSE(CallMetaFunction<float>(f, 1));
        EXPECT_FALSE(CallMetaFunction<int>(f, 0.0));
        EXPECT_FALSE(CallMetaFunction<int>(f, 1, 1));
        auto res = CallMetaFunction<int>(f, 1);
        ASSERT_TRUE(res);
        EXPECT_EQ(res.value, 1);
        EXPECT_EQ(f->GetDestination(), interface_pointer_cast<IObject>(p));
    }
    {
        auto f = m->GetFunctionByName("MyConstTestFunc");
        ASSERT_TRUE(f);
        auto res = CallMetaFunction<int>(f, 2);
        ASSERT_TRUE(res);
        EXPECT_EQ(res.value, 2);
    }
    {
        auto f = m->GetFunctionByName("NormalMember");
        ASSERT_TRUE(f);
        auto res = CallMetaFunction<void>(f);
        ASSERT_TRUE(res);
    }
    {
        auto f = m->GetFunctionByName("OtherNormalMember");
        ASSERT_TRUE(f);

        auto c = f->CreateCallContext();
        ASSERT_TRUE(c);
        ASSERT_TRUE(Get<int>(c, "value"));
        ASSERT_TRUE(Get<int>(c, "some"));

        auto res = CallMetaFunction<int>(f, 2, 3);
        ASSERT_TRUE(res);
        EXPECT_EQ(res.value, 5);
    }
}

/**
 * @tc.name: IntfMetadataTest
 * @tc.desc: test Events function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfMetadataTest, Events, TestSize.Level1)
{
    auto p = CreateTestType();
    auto m = interface_cast<IMetadata>(p);
    ASSERT_TRUE(m);

    auto e = m->GetEventByName("OnTest");
    ASSERT_TRUE(e);
}

template<typename Set, typename Get>
void TestGetSetValue(Set sp, Get gp)
{
    ASSERT_TRUE(SetValue(sp, "First", ONE));
    ASSERT_EQ(GetValue(gp, "First", TWO), ONE);

    ASSERT_TRUE(SetValue<int>(sp, "First", TWO));
    ASSERT_EQ(GetValue<int>(gp, "First"), TWO);
    ASSERT_EQ(GetValue<int>(gp, "First", ONE), TWO);

    ASSERT_FALSE(SetValue<float>(sp, "First", TWO));
    ASSERT_EQ(GetValue<float>(gp, "First"), ZERO);
    ASSERT_EQ(GetValue<float>(gp, "First", ONE), ONE);

    ASSERT_FALSE(SetValue(Set {}, "First", THREE));
    Get nil {};
    ASSERT_EQ(GetValue(nil, "First", THREE), THREE);
    ASSERT_EQ(GetValue<int>(nil, "First", THREE), THREE);
}

template<typename Type>
void TestGetSetValue(BASE_NS::shared_ptr<Type> p)
{
    BASE_NS::shared_ptr<const Type> c_p = p;
    BASE_NS::weak_ptr<Type> w_p = p;
    BASE_NS::weak_ptr<const Type> cw_p = p;

    TestGetSetValue(p, p);
    TestGetSetValue(p, c_p);
    TestGetSetValue(p, w_p);
    TestGetSetValue(p, cw_p);
    TestGetSetValue(w_p, p);
}

/**
 * @tc.name: IntfMetadataTest
 * @tc.desc: test GetSetValue function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfMetadataTest, GetSetValue, TestSize.Level1)
{
    auto p = CreateTestType();

    TestGetSetValue<ITestType>(p);
    TestGetSetValue<IMetadata>(interface_pointer_cast<IMetadata>(p));
    TestGetSetValue<CORE_NS::IInterface>(interface_pointer_cast<CORE_NS::IInterface>(p));
}

template<typename Type>
static auto* GetMeta(const BASE_NS::vector<Type>& vec, const BASE_NS::string& name)
{
    for (size_t i = 0; i != vec.size(); ++i) {
        if (vec[i].name == name) {
            return &vec[i];
        }
    }
    return decltype(&vec[0])(nullptr);
}

/**
 * @tc.name: IntfMetadataTest
 * @tc.desc: test StaticMetadata function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfMetadataTest, StaticMetadata, TestSize.Level1)
{
    EXPECT_EQ(META_NS::InterfaceId::ITestType.ReadableName(), "This is test type");
    EXPECT_EQ(META_NS::InterfaceId::ITestContainer.ReadableName(), "ITestContainer");

    auto fac = GetObjectRegistry().GetObjectFactory(ClassId::TestType);
    ASSERT_TRUE(fac);
    auto sm = fac->GetClassStaticMetadata();

    auto p = GetMeta(sm.properties, "First");
    ASSERT_TRUE(p);
    ASSERT_TRUE(p->interfaceInfo.IsValid());
    EXPECT_EQ(p->interfaceInfo.ReadableName(), "This is test type");

    auto e = GetMeta(sm.events, "OnTest");
    ASSERT_TRUE(e);
    ASSERT_TRUE(e->interfaceInfo.IsValid());
    EXPECT_EQ(e->interfaceInfo.ReadableName(), "This is test type");

    auto f = GetMeta(sm.functions, "NormalMember");
    ASSERT_TRUE(f);
    ASSERT_TRUE(f->interfaceInfo.IsValid());
    EXPECT_EQ(f->interfaceInfo.ReadableName(), "This is test type");
}
META_END_NAMESPACE()