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

#include <meta/api/call_context.h>
#include <meta/api/function.h>

#include "src/test_runner.h"
#include "src/testing_objects.h"
#include "src/util.h"

using namespace testing::ext;

namespace {
    const int TEN = 10; // test value for test functions
}

META_BEGIN_NAMESPACE()

using namespace testing;

class IntfCallContextTest : public testing::Test {
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
 * @tc.name: IntfCallContextTest
 * @tc.desc: test IntroduceInterfaces function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfCallContextTest, Empty, TestSize.Level1)
{
    auto context = GetObjectRegistry().ConstructDefaultCallContext();
    EXPECT_FALSE(Get<int>(context, "some"));
    EXPECT_TRUE(context->GetParameters().empty());
    EXPECT_FALSE(context->Succeeded());
    EXPECT_FALSE(GetResult<int>(context));

    EXPECT_TRUE(DefineResult<void>(context));

    EXPECT_FALSE(context->Succeeded());
    EXPECT_TRUE(SetResult(context));
    EXPECT_TRUE(context->Succeeded());
}

/**
 * @tc.name: IntfCallContextTest
 * @tc.desc: test ReturnType function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfCallContextTest, ReturnType, TestSize.Level1)
{
    auto context = GetObjectRegistry().ConstructDefaultCallContext();
    EXPECT_TRUE(DefineResult<int>(context));

    EXPECT_FALSE(context->Succeeded());
    EXPECT_TRUE(SetResult<int>(context, 2));
    EXPECT_TRUE(context->Succeeded());
    auto res = GetResult<int>(context);
    ASSERT_TRUE(res);
    EXPECT_EQ(*res, 2);
}

/**
 * @tc.name: IntfCallContextTest
 * @tc.desc: test Parameters function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfCallContextTest, Parameters, TestSize.Level1)
{
    auto context = GetObjectRegistry().ConstructDefaultCallContext();
    DefineParameter<int>(context, "test");
    DefineParameter<BASE_NS::string>(context, "some", "hips");
    EXPECT_FALSE(context->Succeeded());
    auto p1 = Get<int>(context, "test");
    ASSERT_TRUE(p1);
    EXPECT_EQ(*p1, 0);
    auto p2 = Get<BASE_NS::string>(context, "some");
    ASSERT_TRUE(p2);
    EXPECT_EQ(*p2, "hips");
    EXPECT_FALSE(Get<float>(context, "test"));
    EXPECT_FALSE(Set<float>(context, "test", 3.0));
    EXPECT_TRUE(Set<int>(context, "test", 2));
    auto ap1 = Get<int>(context, "test");
    ASSERT_TRUE(ap1);
    EXPECT_EQ(*ap1, 2);
    SetResult(context);
    EXPECT_TRUE(context->Succeeded());
}

namespace {
struct TestClass : IObjectInstance {
    int TestFunc(int a, int b, BASE_NS::string c)
    {
        return a + b + c.size();
    }
    void TestFuncMeta(const ICallContext::Ptr& c)
    {
        CallFunction(c, this, &TestClass::TestFunc);
    }
    int TestFunc2(int a, int b)
    {
        return a + TEN * b;
    }
    void TestFunc3(int a, int b, float c) {}
    void ConstFunc() const {}
    void ConstFuncMeta(const ICallContext::Ptr& c) const
    {
        CallFunction(c, this, &TestClass::ConstFunc);
    }
    void RefFunc(const int& in, int& out)
    {
        out = in + 1;
    }
    void RefFuncMeta(const ICallContext::Ptr& c)
    {
        CallFunction(c, this, &TestClass::RefFunc);
    }

    IObject::Ptr GetSelf() const override
    {
        return self_;
    }

    ObjectId GetClassId() const override
    {
        return {};
    }
    BASE_NS::string_view GetClassName() const override
    {
        return {};
    }
    InstanceId GetInstanceId() const override
    {
        return {};
    }
    BASE_NS::string GetName() const override
    {
        return {};
    }
    IObject::Ptr Resolve(const RefUri& uri) const override
    {
        return {};
    }
    BASE_NS::vector<BASE_NS::Uid> GetInterfaces() const override
    {
        return {};
    }
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override
    {
        return this;
    }
    IInterface* GetInterface(const BASE_NS::Uid& uid) override
    {
        return this;
    }

    void Ref() override {}
    void Unref() override {}

    IObject::Ptr self_ { this, [](void*) {} };
};
} // namespace

/**
 * @tc.name: IntfCallContextTest
 * @tc.desc: test CreateCallContext function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfCallContextTest, CreateCallContext, TestSize.Level1)
{
    BASE_NS::string_view arr[] = { "1", "2", "3" };
    EXPECT_FALSE(CreateCallContext(&TestClass::TestFunc2, arr));
    auto c = CreateCallContext(&TestClass::TestFunc, arr);
    ASSERT_TRUE(c);
    auto params = c->GetParameters();
    ASSERT_EQ(params.size(), 3);
    for (int i = 0; i != 3; ++i) {
        EXPECT_EQ(params[i].name, arr[i]);
    }
    EXPECT_TRUE(IsCompatibleWith<int>(*params[0].value));
    EXPECT_TRUE(IsCompatibleWith<int>(*params[1].value));
    EXPECT_TRUE(IsCompatibleWith<BASE_NS::string>(*params[2].value));

    TestClass t;
    auto f = CreateFunction("test", &t, &TestClass::TestFuncMeta, [] {
        BASE_NS::string_view arr[] = { "1", "2", "3" };
        return CreateCallContext(&TestClass::TestFunc, arr);
    });
    ASSERT_TRUE(f);
    auto res = CallMetaFunction<int, int, int, BASE_NS::string>(f, 1, 2, "12");
    ASSERT_TRUE(res);
    EXPECT_EQ(res.value, 5);
}

/**
 * @tc.name: IntfCallContextTest
 * @tc.desc: test BadCallFunction function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfCallContextTest, BadCallFunction, TestSize.Level1)
{
    BASE_NS::string_view arr[] = { "1", "2", "3" };
    auto c = CreateCallContext(&TestClass::TestFunc, arr);
    TestClass t;
    EXPECT_FALSE(CallFunction(c, &t, &TestClass::TestFunc2));
    EXPECT_FALSE(c->Succeeded());
    EXPECT_FALSE(CallFunction(c, &t, &TestClass::TestFunc3));
    EXPECT_FALSE(c->Succeeded());
}

/**
 * @tc.name: IntfCallContextTest
 * @tc.desc: test ConstFunctions function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfCallContextTest, ConstFunctions, TestSize.Level1)
{
    auto context = [] { return CreateCallContext(&TestClass::ConstFunc, {}); };
    auto c = context();
    ASSERT_TRUE(c);
    auto params = c->GetParameters();
    ASSERT_EQ(params.size(), 0);
    {
        TestClass t;
        auto f = CreateFunction("test", &t, &TestClass::ConstFuncMeta, context);
        ASSERT_TRUE(f);
        auto res = CallMetaFunction<void>(f);
        ASSERT_TRUE(res);
    }
}

/**
 * @tc.name: IntfCallContextTest
 * @tc.desc: test ReferenceArgs function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfCallContextTest, ReferenceArgs, TestSize.Level1)
{
    auto context = [] {
        BASE_NS::string_view arr[] = { "in", "out" };
        return CreateCallContext(&TestClass::RefFunc, arr);
    };
    auto c = context();
    TestClass t;
    auto f = CreateFunction("test", &t, &TestClass::RefFuncMeta, context);
    ASSERT_TRUE(f);
    auto res = CallMetaFunction<void>(f, 1, 0);
    ASSERT_TRUE(res);
    ASSERT_TRUE(res.context);

    auto p1 = Get<int>(res.context, "in");
    ASSERT_TRUE(p1);
    EXPECT_EQ(*p1, 1);
    auto p2 = Get<int>(res.context, "out");
    ASSERT_TRUE(p2);
    EXPECT_EQ(*p2, 2);
}

/**
 * @tc.name: IntfCallContextTest
 * @tc.desc: test CallArgumentOrder function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfCallContextTest, CallArgumentOrder, TestSize.Level1)
{
    BASE_NS::string_view arr[] = { "1", "2" };

    {
        auto c = CreateCallContext(&TestClass::TestFunc2, arr);
        Set<int>(c, "1", 1);
        Set<int>(c, "2", 2);
        TestClass t;
        EXPECT_TRUE(CallFunction(c, &t, &TestClass::TestFunc2));
        auto res = GetResult<int>(c);
        ASSERT_TRUE(res);
        EXPECT_EQ(*res, 21);
    }
    {
        auto c = CreateCallContext(&TestClass::TestFunc2, arr);
        Set<int>(c, "1", 1);
        Set<int>(c, "2", 2);
        TestClass t;
        BASE_NS::string_view order[] = { "2", "1" };
        EXPECT_TRUE(CallFunction(c, &t, &TestClass::TestFunc2, order));
        auto res = GetResult<int>(c);
        ASSERT_TRUE(res);
        EXPECT_EQ(*res, 12);
    }
}

META_END_NAMESPACE()