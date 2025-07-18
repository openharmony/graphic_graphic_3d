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
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/array_util.h>
#include <meta/api/function.h>
#include <meta/api/object.h>
#include <meta/api/property/array_element_bind.h>
#include <meta/interface/property/array_property.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/property/property.h>

#include "TestRunner.h"
#include "helpers/testing_objects.h"
#include "helpers/util.h"

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

class ArrayPropertyTest : public testing::Test {
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
 * @tc.name: Values
 * @tc.desc: test Values
 * @tc.type: FUNC
 */
HWTEST_F(ArrayPropertyTest, Values, TestSize.Level1)
{
    auto p = ConstructArrayProperty<int>("test");

    EXPECT_EQ(p->GetSize(), 0);
    EXPECT_TRUE(p->SetDefaultValue({ 1, 2, 3 }));
    EXPECT_EQ(p->GetSize(), 3);
    EXPECT_EQ(p->GetDefaultValue(), (BASE_NS::vector<int> { 1, 2, 3 }));
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<int> { 1, 2, 3 }));
    EXPECT_TRUE(p->SetValue({ 1, 2 }));
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<int> { 1, 2 }));
    EXPECT_EQ(p->GetValueAt(0), 1);
    EXPECT_EQ(p->GetValueAt(1), 2);
    EXPECT_TRUE(p->AddValue(5));
    EXPECT_EQ(p->GetSize(), 3);
    EXPECT_EQ(p->GetValueAt(2), 5);
    EXPECT_TRUE(p->InsertValueAt(1, 7));
    EXPECT_EQ(p->GetSize(), 4);
    EXPECT_EQ(p->GetValueAt(1), 7);
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<int> { 1, 7, 2, 5 }));
    EXPECT_TRUE(p->RemoveAt(0));
    EXPECT_EQ(p->GetSize(), 3);
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<int> { 7, 2, 5 }));
    EXPECT_TRUE(p->SetValueAt(1, 11));
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<int> { 7, 11, 5 }));
}

/**
 * @tc.name: ValuesTypeless
 * @tc.desc: test ValuesTypeless
 * @tc.type: FUNC
 */
HWTEST_F(ArrayPropertyTest, ValuesTypeless, TestSize.Level1)
{
    auto p = ConstructArrayProperty<int>("test");

    Any<int> value1 { 1 };
    Any<int> value2 { 2 };
    Any<int> value3 { 3 };
    p->AddAny(value1);
    EXPECT_EQ(p->GetSize(), 1);
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<int> { 1 }));
    p->AddAny(value3);
    p->InsertAnyAt(1, value2);
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<int> { 1, 2, 3 }));
    Any<int> v;
    EXPECT_TRUE(p->GetAnyAt(0, v));
    EXPECT_EQ(GetValue<int>(v), 1);
}

/**
 * @tc.name: ArrayElementBind
 * @tc.desc: test ArrayElementBind
 * @tc.type: FUNC
 */
HWTEST_F(ArrayPropertyTest, ArrayElementBind, TestSize.Level1)
{
    auto arr = ConstructArrayProperty<int>("test", BASE_NS::vector<int> { 1, 2, 3 });
    auto p = ConstructProperty<int>("p");
    AddArrayElementBind(p, arr, 1);

    int arrCount = 0;
    arr->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] { ++arrCount; }));

    int pCount = 0;
    p->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] { ++pCount; }));

    EXPECT_EQ(arr->GetValue(), (BASE_NS::vector<int> { 1, 2, 3 }));
    EXPECT_TRUE(p->SetValue(4));
    EXPECT_EQ(pCount, 1);
    EXPECT_EQ(arrCount, 1);
    EXPECT_EQ(arr->GetValue(), (BASE_NS::vector<int> { 1, 4, 3 }));
    EXPECT_TRUE(arr->SetValue({ 1, 1, 1 }));
    EXPECT_EQ(pCount, 2);
    EXPECT_EQ(arrCount, 2);
    EXPECT_EQ(p->GetValue(), 1);
    EXPECT_TRUE(arr->SetValue({ 1, 1, 4 }));
    EXPECT_EQ(p->GetValue(), 1);
}

/**
 * @tc.name: ValidArrayPropertyLock
 * @tc.desc: test ValidArrayPropertyLock
 * @tc.type: FUNC
 */
HWTEST_F(ArrayPropertyTest, ValidArrayPropertyLock, TestSize.Level1)
{
    {
        auto p = ConstructArrayProperty<int>("p").GetProperty();
        ArrayPropertyLock lock { p };
        EXPECT_TRUE(lock);
        EXPECT_TRUE(lock.IsValid());
        EXPECT_TRUE(lock.GetProperty());

        TypedArrayPropertyLock<int> typedLock { p };
        EXPECT_TRUE(typedLock);
        EXPECT_TRUE(typedLock.IsValid());
        EXPECT_TRUE(typedLock.GetProperty());

        TypedArrayPropertyLock<float> bad { p };
        EXPECT_FALSE(bad);
        EXPECT_FALSE(bad.IsValid());
        EXPECT_FALSE(bad.GetProperty());
    }
    {
        auto p = ConstructProperty<int>("p").GetProperty();
        ArrayPropertyLock lock { p };
        EXPECT_FALSE(lock);
        EXPECT_FALSE(lock.IsValid());
        EXPECT_FALSE(lock.GetProperty());

        TypedArrayPropertyLock<int> typedLock { p };
        EXPECT_FALSE(typedLock);
        EXPECT_FALSE(typedLock.IsValid());
        EXPECT_FALSE(typedLock.GetProperty());

        TypedArrayPropertyLock<float> bad { p };
        EXPECT_FALSE(bad);
        EXPECT_FALSE(bad.IsValid());
        EXPECT_FALSE(bad.GetProperty());
    }
}

/**
 * @tc.name: TypelessAccess
 * @tc.desc: test TypelessAccess
 * @tc.type: FUNC
 */
HWTEST_F(ArrayPropertyTest, TypelessAccess, TestSize.Level1)
{
    auto p = ConstructArrayProperty<int>("p", { 1, 2, 3, 4, 5 });
    {
        auto any = p->GetAnyAt(0);
        ASSERT_TRUE(any);
        EXPECT_EQ(GetValue<int>(*any), 1);
        EXPECT_FALSE(p->GetAnyAt(5));
    }
    {
        EXPECT_FALSE(p->SetAnyAt(0, Any<float>(0)));
        EXPECT_FALSE(p->InsertAnyAt(0, Any<float>(0)));
        EXPECT_FALSE(p->AddAny(Any<float>(0)));
        EXPECT_FALSE(p->RemoveAt(5));
    }
}

/**
 * @tc.name: TypelessAccessCompatibility
 * @tc.desc: test TypelessAccessCompatibility
 * @tc.type: FUNC
 */
HWTEST_F(ArrayPropertyTest, TypelessAccessCompatibility, TestSize.Level1)
{
    Object a(CreateInstance(ClassId::Object)), b(CreateInstance(ClassId::Object)), c(CreateInstance(ClassId::Object)),
        d(CreateInstance(ClassId::Object));
    auto p = ConstructArrayProperty<SharedPtrIInterface>("p", { a.GetPtr(), b.GetPtr(), c.GetPtr() });
    {
        ArrayPropertyLock l { p.GetProperty() };

        EXPECT_TRUE(l->SetAnyAt(0, Any<IObject::Ptr> { d.GetPtr() }));

        auto any = l->GetAnyAt(0);
        ASSERT_TRUE(any);
        EXPECT_EQ(GetValue<SharedPtrIInterface>(*any), interface_pointer_cast<CORE_NS::IInterface>(d));
        EXPECT_FALSE(p->GetAnyAt(5));
    }
}

/**
 * @tc.name: PropertyHelpers
 * @tc.desc: test PropertyHelpers
 * @tc.type: FUNC
 */
HWTEST_F(ArrayPropertyTest, PropertyHelpers, TestSize.Level1)
{
    Object a(CreateInstance(ClassId::Object)), b(CreateInstance(ClassId::Object)), c(CreateInstance(ClassId::Object));
    auto p = ConstructArrayProperty<SharedPtrIInterface>("p", { a.GetPtr(), b.GetPtr(), c.GetPtr() });

    EXPECT_FALSE(IsGetPointer(p));
    EXPECT_FALSE(IsSetPointer(p));
    EXPECT_TRUE(IsGetPointerArray(p));
    EXPECT_TRUE(IsSetPointerArray(p));
    EXPECT_EQ(GetPointerAt(0, p), interface_pointer_cast<CORE_NS::IInterface>(a));
    EXPECT_EQ(GetPointerAt<IObject>(0, p), a.GetPtr());

    EXPECT_TRUE(IsArrayProperty(p));
    EXPECT_FALSE(IsArrayProperty(ConstructProperty<int>("p").GetProperty()));

    auto other = CreateTestType<IObject>();

    EXPECT_TRUE(SetPointerAt(0, p, other));
    EXPECT_EQ(GetPointerAt(0, p), interface_pointer_cast<CORE_NS::IInterface>(other));

    auto cp = ConstructArrayProperty<IObject::ConstPtr>("p", { a.GetPtr(), b.GetPtr(), c.GetPtr() });
    EXPECT_TRUE(IsGetPointerArray(cp));
    EXPECT_TRUE(IsSetPointerArray(cp));
    EXPECT_TRUE(SetPointerAt(0, cp, other));
    EXPECT_EQ(GetConstPointerAt(0, cp), interface_pointer_cast<CORE_NS::IInterface>(other));

    IObject::ConstPtr some = CreateTestType<IObject>();
    EXPECT_TRUE(SetPointerAt(0, cp, some));
    EXPECT_EQ(GetConstPointerAt(0, cp), interface_pointer_cast<CORE_NS::IInterface>(some));
}

META_END_NAMESPACE()
