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

#include <gtest/gtest.h>

#include <meta/api/connector.h>
#include <meta/api/function.h>
#include <meta/ext/any_builder.h>
#include <meta/ext/attachment/attachment.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/intf_connector.h>
#include <meta/interface/property/property.h>

#include "src/serialisation_utils.h"
#include "src/testing_objects.h"
#include "src/util.h"

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

namespace {

class IFuncTestType : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IFuncTestType, "5c79a9ab-f673-49b5-b18a-f16d0db50c96")
public:
    virtual void MyFunc() = 0;
    virtual int MyFunc2(int) = 0;
    virtual void OnStart() = 0;

    META_EVENT(IOnChanged, OnTest)
    META_PROPERTY(int, TestProperty)
};

META_REGISTER_CLASS(TestType, "abcd6e8b-36d4-44e4-802d-b2d80e50c612", META_NS::ObjectCategoryBits::APPLICATION)

class TestType final : public IntroduceInterfaces<META_NS::ObjectFwd, IFuncTestType> {
    META_OBJECT(TestType, Meta::ClassId::TestType, IntroduceInterfaces, META_NS::ClassId::Object)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_FUNCTION_DATA(IFuncTestType, MyFunc)
    META_STATIC_FUNCTION_DATA(IFuncTestType, MyFunc2, "value")
    META_STATIC_FUNCTION_DATA(IFuncTestType, OnStart, "state")
    META_STATIC_PROPERTY_DATA(IFuncTestType, int, TestProperty)
    META_STATIC_EVENT_DATA(IFuncTestType, IOnChanged, OnTest)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(int, TestProperty)
    META_IMPLEMENT_EVENT(IOnChanged, OnTest)

    void MyFunc() override
    {
        myFuncCalled_ = true;
    }
    int MyFunc2(int v) override
    {
        myFunc2Called_ = v;
        return v;
    }

    void OnStart() override
    {
        onStartCalled_ = true;
    }

public:
    bool myFuncCalled_ {};
    int myFunc2Called_ {};
    bool onStartCalled_ {};
};
} // namespace

class FunctionTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
        UnregisterObjectType<TestType>();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: EventConnect
 * @tc.desc: test EventConnect
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(FunctionTest, EventConnect, TestSize.Level1)
{
    RegisterObjectType<TestType>();
    auto& registry = META_NS::GetObjectRegistry();
    auto p = ConstructProperty<int>(registry, "prop", 0);

    auto object = registry.Create(Meta::ClassId::TestType);
    ASSERT_TRUE(object);

    auto f = interface_cast<IMetadata>(object)->GetFunction("MyFunc");
    ASSERT_TRUE(f);

    auto f2 = interface_cast<IMetadata>(object)->GetFunction("MyFunc2");
    ASSERT_TRUE(f2);

    EXPECT_TRUE(p->OnChanged()->IsCompatibleWith(f));
    EXPECT_TRUE(p->OnChanged()->IsCompatibleWith(f2));

    p->OnChanged()->AddHandler(f);
    p->OnChanged()->AddHandler(f2);
    p->SetValue(2);

    auto* t = reinterpret_cast<TestType*>(object.get());
    EXPECT_TRUE(t->myFuncCalled_);
    // uses the default call context argument
    EXPECT_TRUE(t->myFunc2Called_ == 0);

    UnregisterObjectType<TestType>();
}

/**
 * @tc.name: Connector
 * @tc.desc: test Connector
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(FunctionTest, Connector, TestSize.Level1)
{
    RegisterObjectType<TestType>();

    auto& registry = META_NS::GetObjectRegistry();

    auto object = registry.Create(Meta::ClassId::TestType);
    ASSERT_TRUE(object);

    ASSERT_TRUE(Connect(object, "OnTest", object, "MyFunc"));

    auto* t = reinterpret_cast<TestType*>(object.get());

    EXPECT_FALSE(t->myFuncCalled_);
    Invoke<IOnChanged>(t->OnTest());
    EXPECT_TRUE(t->myFuncCalled_);

    t->myFuncCalled_ = false;
    ASSERT_TRUE(Disconnect(object, "OnTest", object, "MyFunc"));
    Invoke<IOnChanged>(t->OnTest());
    EXPECT_FALSE(t->myFuncCalled_);

    UnregisterObjectType<TestType>();
}

/**
 * @tc.name: Forwarding
 * @tc.desc: test Forwarding
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(FunctionTest, Forwarding, TestSize.Level1)
{
    RegisterObjectType<TestType>();
    auto& registry = META_NS::GetObjectRegistry();
    auto object = registry.Create(Meta::ClassId::TestType);
    ASSERT_TRUE(object);
    auto func = CreateFunction(object, "MyFunc");
    ASSERT_TRUE(func);

    ASSERT_TRUE(CallMetaFunction<void>(func));
    auto* t = reinterpret_cast<TestType*>(object.get());
    EXPECT_TRUE(t->myFuncCalled_);

    auto func2 = CreateFunction(object, "MyFunc2");
    ASSERT_TRUE(func2);

    EXPECT_EQ(CallMetaFunction<int>(func2, 2).value, 2);
    EXPECT_EQ(t->myFunc2Called_, 2);

    UnregisterObjectType<TestType>();
}

/**
 * @tc.name: SerialiseConnector
 * @tc.desc: test SerialiseConnector
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(FunctionTest, SerialiseConnector, TestSize.Level1)
{
    RegisterObjectType<TestType>();
    auto& registry = META_NS::GetObjectRegistry();
    TestSerialiser ser;

    {
        auto object = registry.Create(Meta::ClassId::TestType);
        ASSERT_TRUE(object);

        ASSERT_TRUE(Connect(object, "OnTest", object, "MyFunc"));
        ASSERT_TRUE(ser.Export(object));
    }

    ser.Dump("file://./connector.json");

    auto object = ser.Import();
    ASSERT_TRUE(object);

    auto* t = reinterpret_cast<TestType*>(object.get());

    EXPECT_FALSE(t->myFuncCalled_);
    Invoke<IOnChanged>(t->OnTest());
    EXPECT_TRUE(t->myFuncCalled_);

    UnregisterObjectType<TestType>();
}

/**
 * @tc.name: Serialization
 * @tc.desc: test Serialization
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(FunctionTest, DISABLED_Serialization, TestSize.Level1)
{
    RegisterObjectType<TestType>();
    auto& registry = META_NS::GetObjectRegistry();

    TestSerialiser ser;
    {
        auto object = registry.Create(Meta::ClassId::TestType);
        ASSERT_TRUE(object);

        auto func = CreateFunction(object, "MyFunc");
        ASSERT_TRUE(func);
        auto func2 = CreateFunction(object, "MyFunc2");
        ASSERT_TRUE(func2);

        auto p = ConstructArrayProperty<IFunction::Ptr>("functions");
        p->AddValue(interface_pointer_cast<IFunction>(func));
        p->AddValue(interface_pointer_cast<IFunction>(func2));

        interface_cast<IMetadata>(object)->AddProperty(p);
        ASSERT_TRUE(ser.Export(object));
    }

    ser.Dump("file://./func_ser.json");

    auto obj = ser.Import();
    ASSERT_TRUE(obj);

    auto metad = interface_pointer_cast<IMetadata>(obj);
    ASSERT_TRUE(metad);

    auto p = ArrayProperty<IFunction::Ptr>(metad->GetProperty("functions"));
    ASSERT_TRUE(p);
    ASSERT_EQ(p->GetSize(), 2);

    ASSERT_TRUE(CallMetaFunction<void>(p->GetValueAt(0)));
    EXPECT_EQ(CallMetaFunction<int>(p->GetValueAt(1), 2).value, 2);

    auto* t = reinterpret_cast<TestType*>(obj.get());
    EXPECT_TRUE(t->myFuncCalled_);
    EXPECT_EQ(t->myFunc2Called_, 2);

    UnregisterObjectType<TestType>();
}

META_END_NAMESPACE()
