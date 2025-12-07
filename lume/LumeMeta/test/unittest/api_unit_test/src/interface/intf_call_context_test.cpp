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

#include <meta/api/call_context.h>
#include <meta/api/function.h>

#include "helpers/testing_objects.h"
#include "helpers/util.h"
#include "test_framework.h"

META_BEGIN_NAMESPACE()
namespace UTest {

/**
 * @tc.name: Empty
 * @tc.desc: Tests for Empty. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_CallContextTest, Empty, testing::ext::TestSize.Level1)
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
 * @tc.name: ReturnType
 * @tc.desc: Tests for Return Type. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_CallContextTest, ReturnType, testing::ext::TestSize.Level1)
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
 * @tc.name: Parameters
 * @tc.desc: Tests for Parameters. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_CallContextTest, Parameters, testing::ext::TestSize.Level1)
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

/**
 * @tc.name: Clone
 * @tc.desc: Tests for Clone. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_CallContextTest, Clone, testing::ext::TestSize.Level1)
{
    auto& r = GetObjectRegistry();
    auto context = r.ConstructDefaultCallContext();
    auto cloneable = interface_cast<ICloneable>(context);
    ASSERT_TRUE(cloneable);
    ASSERT_TRUE(cloneable->GetClone());

    auto result = ConstructAny<bool>();
    EXPECT_TRUE(context->DefineResult(ConstructAny<bool>()));
    EXPECT_TRUE(context->SetResult(*result));
    auto param = ConstructAny<uint32_t>();
    EXPECT_TRUE(context->DefineParameter("param", ConstructAny<uint32_t>()));
    EXPECT_TRUE(context->Set("param", *param));
    ASSERT_TRUE(cloneable->GetClone());
}

namespace {
struct TestClass : IntroduceInterfaces<IObjectInstance> {
    int TestFunc(int a, int b, BASE_NS::string c)
    {
        return a + b + c.size();
    }
    int TestFunc2(int a, int b)
    {
        return a + 10 * b;
    }
    void TestFunc3(int a, int b, float c) {}
    void ConstFunc() const {}
    void RefFunc(const int& in, int& out)
    {
        out = in + 1;
    }

    IObject::Ptr GetSelf() const override
    {
        return self_.lock();
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

    IObject::WeakPtr self_ {};
};
} // namespace

/**
 * @tc.name: CreateCallContext
 * @tc.desc: Tests for Create Call Context. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_CallContextTest, CreateCallContext, testing::ext::TestSize.Level1)
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

    BASE_NS::shared_ptr<TestClass> t(new TestClass);
    t->self_ = t;
    auto f = CreateFunction("test", t, &TestClass::TestFunc, [] {
        BASE_NS::string_view arr[] = { "1", "2", "3" };
        return META_NS::ConstructAny<META_NS::ICallContext::Ptr>(CreateCallContext(&TestClass::TestFunc, arr));
    });
    ASSERT_TRUE(f);
    auto res = CallMetaFunction<int, int, int, BASE_NS::string>(f, 1, 2, "12");
    ASSERT_TRUE(res);
    EXPECT_EQ(res.value, 5);
}

/**
 * @tc.name: BadCallFunction
 * @tc.desc: Tests for Bad Call Function. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_CallContextTest, BadCallFunction, testing::ext::TestSize.Level1)
{
    BASE_NS::string_view arr[] = { "1", "2", "3" };
    auto c = CreateCallContext(&TestClass::TestFunc, arr);
    BASE_NS::shared_ptr<TestClass> t(new TestClass);
    t->self_ = t;
    EXPECT_FALSE(CallFunction(c, t.get(), &TestClass::TestFunc2));
    EXPECT_FALSE(c->Succeeded());
    EXPECT_FALSE(CallFunction(c, t.get(), &TestClass::TestFunc3));
    EXPECT_FALSE(c->Succeeded());
}

/**
 * @tc.name: ConstFunctions
 * @tc.desc: Tests for Const Functions. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_CallContextTest, ConstFunctions, testing::ext::TestSize.Level1)
{
    auto context = [] {
        return META_NS::ConstructAny<META_NS::ICallContext::Ptr>(CreateCallContext(&TestClass::ConstFunc, {}));
    };
    auto c = GetValue<ICallContext::Ptr>(*context());
    ASSERT_TRUE(c);
    auto params = c->GetParameters();
    ASSERT_EQ(params.size(), 0);

    {
        BASE_NS::shared_ptr<TestClass> t(new TestClass);
        t->self_ = t;
        auto f = CreateFunction("test", t, &TestClass::ConstFunc, context);
        ASSERT_TRUE(f);
        auto res = CallMetaFunction<void>(f);
        ASSERT_TRUE(res);
    }
}

/**
 * @tc.name: ReferenceArgs
 * @tc.desc: Tests for Reference Args. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_CallContextTest, ReferenceArgs, testing::ext::TestSize.Level1)
{
    auto context = [] {
        BASE_NS::string_view arr[] = { "in", "out" };
        return META_NS::ConstructAny<META_NS::ICallContext::Ptr>(CreateCallContext(&TestClass::RefFunc, arr));
    };
    auto c = context();
    BASE_NS::shared_ptr<TestClass> t(new TestClass);
    t->self_ = t;
    auto f = CreateFunction("test", t, &TestClass::RefFunc, context);
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
 * @tc.name: CallArgumentOrder
 * @tc.desc: Tests for Call Argument Order. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_CallContextTest, CallArgumentOrder, testing::ext::TestSize.Level1)
{
    BASE_NS::string_view arr[] = { "1", "2" };

    {
        auto c = CreateCallContext(&TestClass::TestFunc2, arr);
        Set<int>(c, "1", 1);
        Set<int>(c, "2", 2);
        BASE_NS::shared_ptr<TestClass> t(new TestClass);
        t->self_ = t;
        EXPECT_TRUE(CallFunction(c, t.get(), &TestClass::TestFunc2));
        auto res = GetResult<int>(c);
        ASSERT_TRUE(res);
        EXPECT_EQ(*res, 21);
    }
    {
        auto c = CreateCallContext(&TestClass::TestFunc2, arr);
        Set<int>(c, "1", 1);
        Set<int>(c, "2", 2);
        BASE_NS::shared_ptr<TestClass> t(new TestClass);
        t->self_ = t;
        BASE_NS::string_view order[] = { "2", "1" };
        EXPECT_TRUE(CallFunction(c, t.get(), &TestClass::TestFunc2, order));
        auto res = GetResult<int>(c);
        ASSERT_TRUE(res);
        EXPECT_EQ(*res, 12);
    }
}

} // namespace UTest
META_END_NAMESPACE()
