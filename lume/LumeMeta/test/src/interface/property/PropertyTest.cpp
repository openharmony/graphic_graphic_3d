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
#include <meta/api/util.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/minimal_object.h>
#include <meta/ext/object_fwd.h>
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

class PropertyTest : public testing::Test {
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

namespace {
struct TestType {
    int i {};
};
} // namespace

META_TYPE(TestType);

/**
 * @tc.name: Interfaces
 * @tc.desc: test Interfaces
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, Interfaces, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    EXPECT_TRUE(interface_cast<IObject>(p.GetProperty()));
    EXPECT_TRUE(interface_cast<INotifyOnChange>(p.GetProperty()));
    EXPECT_TRUE(interface_cast<IPropertyInternal>(p.GetProperty()));
    EXPECT_TRUE(interface_cast<IPropertyInternalAny>(p.GetProperty()));
    EXPECT_TRUE(interface_cast<IStackProperty>(p.GetProperty()));
}

/**
 * @tc.name: Values
 * @tc.desc: test Values
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, Values, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");

    EXPECT_EQ(p->GetValue(), 0);
    p->SetDefaultValue(2);
    EXPECT_EQ(p->GetDefaultValue(), 2);
    EXPECT_EQ(p->GetValue(), 2);
    p->SetValue(3);
    EXPECT_EQ(p->GetValue(), 3);
    p->ResetValue();
    EXPECT_EQ(p->GetValue(), 2);
}

/**
 * @tc.name: Misc
 * @tc.desc: test Misc
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, Misc, TestSize.Level1)
{
    Object owner(CreateNew);
    auto p = ConstructProperty<int>("test");
    if (auto pp = interface_pointer_cast<IPropertyInternal>(p.GetProperty())) {
        pp->SetOwner(interface_pointer_cast<IOwner>(owner));
    }
    EXPECT_EQ(p->GetTypeId(), UidFromType<int>());
    EXPECT_TRUE(p->IsCompatible(UidFromType<int>()));
    EXPECT_EQ(p->GetName(), "test");
    EXPECT_EQ(p->GetOwner().lock(), interface_pointer_cast<IOwner>(owner));
}

/**
 * @tc.name: OnChangedEvent
 * @tc.desc: test OnChangedEvent
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, OnChangedEvent, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");

    // does nothing as there are no handlers added
    p.GetProperty()->NotifyChange();

    int count = 0;
    p->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] { ++count; }));

    p->SetDefaultValue(2);
    EXPECT_EQ(count, 1);

    p->SetValue(2);
    EXPECT_EQ(count, 2);

    // settings default does not notify after we have value
    p->SetDefaultValue(2);
    EXPECT_EQ(count, 2);
    {
        auto acc = p.GetLockedAccess();
        acc.SetValue(3);
        EXPECT_EQ(count, 2);
    }
    EXPECT_EQ(count, 3);
    {
        auto acc = p.GetUnlockedAccess();
        acc.SetValue(4);
        EXPECT_EQ(count, 4);
    }
    // setting same value does not notify
    p->SetValue(4);
    EXPECT_EQ(count, 4);

    p->ResetValue();
    EXPECT_EQ(count, 5);

    p.GetProperty()->NotifyChange();
    EXPECT_EQ(count, 6);
}

/**
 * @tc.name: OnChangedEventWithBind
 * @tc.desc: test OnChangedEventWithBind
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, OnChangedEventWithBind, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto source = ConstructProperty<int>("source");
    source->SetValue(2);

    int count = 0;
    p->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] { ++count; }));

    EXPECT_TRUE(p->SetBind(source));
    EXPECT_EQ(count, 1);
    EXPECT_EQ(p->GetValue(), 2);

    source->SetValue(3);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(p->GetValue(), 3);
}

/**
 * @tc.name: OnChangedEventWithValues
 * @tc.desc: test OnChangedEventWithValues
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, OnChangedEventWithValues, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto a = ConstructProperty<int>("a");
    auto b = ConstructProperty<int>("b");

    int count = 0;
    p->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] { ++count; }));

    p->PushValue(a.GetProperty());
    EXPECT_EQ(count, 1);
    p->PushValue(b.GetProperty());
    EXPECT_EQ(count, 2);
    interface_cast<IStackProperty>(p.GetProperty())->RemoveValue(interface_pointer_cast<IValue>(a));
    EXPECT_EQ(count, 2);
    p->PopValue();
    EXPECT_EQ(count, 3);
}

/**
 * @tc.name: PropertyBind
 * @tc.desc: test PropertyBind
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, PropertyBind, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto source = ConstructProperty<int>("source");
    source->SetValue(2);

    EXPECT_TRUE(p->SetBind(source));
    EXPECT_EQ(p->GetValue(), 2);

    source->SetValue(3);
    EXPECT_EQ(p->GetValue(), 3);

    source->ResetValue();
    EXPECT_EQ(p->GetValue(), 0);
}

/**
 * @tc.name: FunctionBind
 * @tc.desc: test FunctionBind
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, FunctionBind, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto source = CreateTestType("source");
    auto mdata = interface_cast<IMetadata>(source);

    auto f = mdata->GetFunction("NormalMember");
    ASSERT_TRUE(f);
    EXPECT_TRUE(p->SetBind(f));

    source->First()->SetValue(3);
    EXPECT_EQ(p->GetValue(), 3);

    source->First()->SetValue(4);
    EXPECT_EQ(p->GetValue(), 4);
}

/**
 * @tc.name: LambdaBindMultiDep
 * @tc.desc: test LambdaBindMultiDep
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, LambdaBindMultiDep, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto s1 = ConstructProperty<int>("s1");
    auto s2 = ConstructProperty<int>("s2");
    auto s3 = ConstructProperty<int>("s3");

    ASSERT_TRUE(s2->SetBind(CreateBindFunction([&] { return s3->GetValue(); })));
    ASSERT_TRUE(p->SetBind(CreateBindFunction([&] { return s1->GetValue() + s2->GetValue(); })));

    s3->SetValue(3);
    EXPECT_EQ(p->GetValue(), 3);

    s1->SetValue(2);
    EXPECT_EQ(p->GetValue(), 5);

    s2->SetValue(5);
    EXPECT_EQ(p->GetValue(), 7);
}

/**
 * @tc.name: LambdaBindInvalid
 * @tc.desc: test LambdaBindInvalid
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, LambdaBindInvalid, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto s1 = ConstructProperty<int>("s1");

    ASSERT_TRUE(s1->SetBind(CreateBindFunction([&] { return p->GetValue(); })));
    p->SetValue(2);
    EXPECT_EQ(s1->GetValue(), 2);
    ASSERT_FALSE(p->SetBind(CreateBindFunction([&] { return s1->GetValue(); })));

    s1->SetValue(3);
    EXPECT_EQ(p->GetValue(), 2);

    p->SetValue(4);
    EXPECT_EQ(p->GetValue(), 4);
}

/**
 * @tc.name: LambdaBind
 * @tc.desc: test LambdaBind
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, LambdaBind, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    EXPECT_TRUE(p->SetBind(CreateBindFunction([] { return 5; })));
    EXPECT_EQ(p->GetValue(), 5);

    EXPECT_TRUE(p->SetBind(CreateBindFunction([](auto...) { return 5; })));
    EXPECT_TRUE(p->SetBind(CreateBindFunction([](auto...) mutable { return 5; })));
}

/**
 * @tc.name: LambdaBindWithDependency
 * @tc.desc: test LambdaBindWithDependency
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, LambdaBindWithDependency, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto source = ConstructProperty<int>("source");
    source->SetValue(2);
    EXPECT_TRUE(p->SetBind(CreateBindFunction([&] { return source->GetValue(); }),
        (BASE_NS::vector<INotifyOnChange::ConstPtr> { source.GetProperty() })));
    EXPECT_EQ(p->GetValue(), 2);
    source->SetValue(4);
    EXPECT_EQ(p->GetValue(), 4);
}

/**
 * @tc.name: MultipleBinds
 * @tc.desc: test MultipleBinds
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, MultipleBinds, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto source1 = ConstructProperty<int>("source1", 1);
    auto source2 = ConstructProperty<int>("source2", 2);

    EXPECT_EQ(p->GetStackProperty()->GetValues({}, false).size(), 0);

    EXPECT_TRUE(p->SetBind(source1));
    EXPECT_EQ(p->GetValue(), 1);

    EXPECT_EQ(p->GetStackProperty()->GetValues({}, false).size(), 1);

    EXPECT_TRUE(p->SetBind(source2));
    EXPECT_EQ(p->GetValue(), 2);

    EXPECT_EQ(p->GetStackProperty()->GetValues({}, false).size(), 1);
}

/**
 * @tc.name: BindingBind
 * @tc.desc: test BindingBind
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, BindingBind, TestSize.Level1)
{
    auto p1 = ConstructProperty<int>("p1", 0);
    auto p2 = ConstructProperty<int>("p2", 0);
    auto p3 = ConstructProperty<int>("p3", 0);

    p3->SetValue(2);

    EXPECT_TRUE(p1->SetBind(CreateBindFunction([&]() {
        p2->SetBind(CreateBindFunction([&p3]() { return p3->GetValue(); }));
        return p2->GetValue();
    })));

    EXPECT_EQ(p1->GetValue(), 2);
}

/**
 * @tc.name: PropertyInProperty
 * @tc.desc: test PropertyInProperty
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, PropertyInProperty, TestSize.Level1)
{
    auto p1 = ConstructProperty<int>("p1");
    auto p2 = ConstructProperty<int>("p2");
    auto sp1 = interface_cast<IStackProperty>(p1.GetProperty());
    auto vp2 = interface_pointer_cast<IValue>(p2.GetProperty());
    ASSERT_TRUE(sp1);
    ASSERT_TRUE(vp2);

    EXPECT_TRUE(sp1->PushValue(vp2));
    EXPECT_EQ(p1->GetValue(), 0);
    p2->SetValue(2);
    EXPECT_EQ(p1->GetValue(), 2);
    p1->SetValue(5);
    EXPECT_EQ(p1->GetValue(), 5);
    EXPECT_EQ(p2->GetValue(), 5);
}

/**
 * @tc.name: ResetInternalAny
 * @tc.desc: test ResetInternalAny
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, ResetInternalAny, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");

    EXPECT_EQ(p->GetValue(), 0);
    p->SetDefaultValue(2);
    EXPECT_EQ(p->GetDefaultValue(), 2);
    EXPECT_EQ(p->GetValue(), 2);
    p->SetValue(3);
    EXPECT_EQ(p->GetValue(), 3);

    auto i = interface_cast<IPropertyInternalAny>(p.GetProperty());
    ASSERT_TRUE(i);
    i->SetInternalAny(IAny::Ptr(new Any<BASE_NS::string>));

    Property<BASE_NS::string> s(p.GetProperty());
    ASSERT_TRUE(s);

    EXPECT_EQ(s->GetValue(), "");
    s->SetDefaultValue("ab");
    EXPECT_EQ(s->GetDefaultValue(), "ab");
    EXPECT_EQ(s->GetValue(), "ab");
    s->SetValue("xx");
    EXPECT_EQ(s->GetValue(), "xx");
}

/**
 * @tc.name: RecursiveProperty
 * @tc.desc: test RecursiveProperty
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, RecursiveProperty, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto stack = interface_cast<IStackProperty>(p->GetProperty());
    EXPECT_EQ(p->PushValue(p.GetProperty()).GetError(), GenericError::RECURSIVE_CALL);

    auto p2 = ConstructProperty<int>("test2");
    p2->PushValue(p.GetProperty());
    EXPECT_EQ(p->PushValue(p2.GetProperty()).GetError(), GenericError::RECURSIVE_CALL);
}

/**
 * @tc.name: Reset
 * @tc.desc: test Reset
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, Reset, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");

    EXPECT_EQ(p->GetValue(), 0);
    p->SetDefaultValue(2);
    EXPECT_EQ(p->GetDefaultValue(), 2);
    EXPECT_EQ(p->GetValue(), 2);
    p->SetValue(3);
    EXPECT_EQ(p->GetValue(), 3);
    p->Reset();
    EXPECT_EQ(p->GetValue(), 2);
}

/**
 * @tc.name: ResetWithSharedPointer
 * @tc.desc: test ResetWithSharedPointer
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, ResetWithSharedPointer, TestSize.Level1)
{
    auto p = ConstructProperty<IObject::Ptr>("test");
    auto obj = CreateTestType<IObject>();
    BASE_NS::weak_ptr<IObject> w = obj;
    p->SetValue(obj);
    obj.reset();
    EXPECT_TRUE(w.lock());
    p->Reset();
    EXPECT_FALSE(w.lock());
}

/**
 * @tc.name: UserType
 * @tc.desc: test UserType
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, UserType, TestSize.Level1)
{
    auto p = ConstructProperty<TestType>("test");
    EXPECT_EQ(p->GetValue().i, 0);
    EXPECT_TRUE(p->SetValue(TestType { 1 }));
    EXPECT_EQ(p->GetValue().i, 1);
}

/**
 * @tc.name: BindingMultiDep
 * @tc.desc: test BindingMultiDep
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, BindingMultiDep, TestSize.Level1)
{
    auto p1 = ConstructProperty<int>("test1");
    auto p2 = ConstructProperty<int>("test2");
    auto p3 = ConstructProperty<int>("test3");
    auto p4 = ConstructProperty<int>("test4");

    p3->SetValue(2);

    EXPECT_TRUE(p1->SetBind(CreateBindFunction([&p3] { return p3->GetValue(); }),
        (BASE_NS::vector<INotifyOnChange::ConstPtr> { p3.GetProperty() })));
    EXPECT_TRUE(p2->SetBind(CreateBindFunction([&p3] { return p3->GetValue(); }),
        (BASE_NS::vector<INotifyOnChange::ConstPtr> { p3.GetProperty() })));

    EXPECT_TRUE(p4->SetBind(CreateBindFunction([&p1, &p2] { return p1->GetValue() + p2->GetValue(); }),
        (BASE_NS::vector<INotifyOnChange::ConstPtr> { p1.GetProperty(), p2.GetProperty() })));

    EXPECT_EQ(p4->GetValue(), 4);
    p3->SetValue(3);
    EXPECT_EQ(p4->GetValue(), 6);
}

/**
 * @tc.name: ObjectFlags
 * @tc.desc: test ObjectFlags
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, ObjectFlags, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test", 0, ObjectFlagBits::INTERNAL);
    EXPECT_TRUE(IsFlagSet(p.GetProperty(), ObjectFlagBits::INTERNAL));
    EXPECT_FALSE(IsFlagSet(p.GetProperty(), ObjectFlagBits::SERIALIZE));

    auto test = CreateTestType();
    EXPECT_TRUE(IsFlagSet(test->First().GetProperty(), DEFAULT_PROPERTY_FLAGS));
    EXPECT_TRUE(IsFlagSet(
        test->Second().GetProperty(), ObjectFlagBitsValue { ObjectFlagBits::INTERNAL } | ObjectFlagBits::NATIVE));
}

/**
 * @tc.name: Conversions
 * @tc.desc: test Conversions
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, Conversions, TestSize.Level1)
{
    Property<int> p = ConstructProperty<int>("test");
    ASSERT_TRUE(p.IsValid());
    Property<const int> constp = p;
    IProperty::Ptr ptrp = p;
    IProperty::ConstPtr constptrp = p;
    IProperty::ConstPtr constptrp2 = constp;
    IProperty::WeakPtr wp = p;
    IProperty::ConstWeakPtr constwp = p;
    IProperty::ConstWeakPtr constwp2 = constp;
    Property<const int> cp1 = ptrp;
    Property<const int> cp2 = constptrp;
    EXPECT_TRUE(interface_pointer_cast<IStackProperty>(p));
    EXPECT_TRUE(interface_cast<IStackProperty>(p));
}

/**
 * @tc.name: IncompatibleValue
 * @tc.desc: test IncompatibleValue
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, IncompatibleValue, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto s = ConstructProperty<BASE_NS::string>("test");
    EXPECT_EQ(p->PushValue(s.GetProperty()).GetError(), GenericError::INCOMPATIBLE_TYPES);
}

/**
 * @tc.name: CompatiblePtrValue
 * @tc.desc: test CompatiblePtrValue
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, CompatiblePtrValue, TestSize.Level1)
{
    auto p = ConstructProperty<IObject::Ptr>("test");
    auto obj = CreateTestType();

    EXPECT_TRUE(p->SetValueAny(Any<SharedPtrIInterface>(interface_pointer_cast<CORE_NS::IInterface>(obj))));
    EXPECT_EQ(p->GetValue(), interface_pointer_cast<IObject>(obj));
}

/**
 * @tc.name: SetValueSafeInit
 * @tc.desc: test SetValueSafeInit
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, SetValueSafeInit, TestSize.Level1)
{
    Property<int> p(NOCHECK, GetObjectRegistry().GetPropertyRegister().Create(META_NS::ClassId::StackProperty, "Test"));

    int i = 0;
    EXPECT_FALSE(p->GetValueAny().GetValue(i));

    EXPECT_TRUE(p->SetValue(2));
    EXPECT_EQ(p->GetValue(), 2);
    EXPECT_TRUE(Property<int>(p));
}

/**
 * @tc.name: ValuePtr
 * @tc.desc: test ValuePtr
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, ValuePtr, TestSize.Level1)
{
    auto obj = GetObjectRegistry().Create<ITestPtrValue>(ClassId::TestPtrValue);
    ASSERT_TRUE(obj);
    ASSERT_TRUE(obj->Text()->GetValue());
    EXPECT_EQ(obj->Text()->GetValue()->GetString(), "Some");
    EXPECT_TRUE(obj->Text()->SetValueAny(Any<BASE_NS::string>("Hips")));
    EXPECT_EQ(obj->Text()->GetValue()->GetString(), "Hips");

    auto str = GetObjectRegistry().Create<ITestString>(ClassId::TestString);
    ASSERT_TRUE(str);
    str->SetString("Hoo'o");

    EXPECT_TRUE(obj->Text()->SetValue(str));
    EXPECT_EQ(obj->Text()->GetValue()->GetString(), "Hoo'o");
}

/**
 * @tc.name: ValuePtrBind
 * @tc.desc: test ValuePtrBind
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, ValuePtrBind, TestSize.Level1)
{
    auto obj = GetObjectRegistry().Create<ITestString>(ClassId::TestString);
    ASSERT_TRUE(obj);
    obj->SetString("hahah");

    auto p = ConstructProperty<ValuePtr<ITestString, ClassId::TestString>>("Test", BASE_NS::string("empty"));
    auto strProp = ConstructProperty<BASE_NS::string>("Test", "huuhaa");
    auto strPtrProp = ConstructProperty<ITestString::Ptr>("Test", obj);

    EXPECT_EQ(p->GetValue()->GetString(), "empty");

    ASSERT_TRUE(p->SetBind(strProp));
    EXPECT_EQ(p->GetValue()->GetString(), "huuhaa");

    ASSERT_TRUE(p->SetBind(strPtrProp));
    EXPECT_EQ(p->GetValue()->GetString(), "hahah");

    auto strP = ConstructProperty<BASE_NS::string>("Test");
    ASSERT_TRUE(strP->SetBind(p));
    EXPECT_EQ(strP->GetValue(), "hahah");
}

/**
 * @tc.name: ValuePtrDefaultValue
 * @tc.desc: test ValuePtrDefaultValue
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, ValuePtrDefaultValue, TestSize.Level1)
{
    auto p = ConstructProperty<ValuePtr<ITestString, ClassId::TestString>>("Test", BASE_NS::string("hih"));

    int count = 0;
    p->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] { ++count; }));
    EXPECT_EQ(p->GetValue()->GetString(), "hih");
    ITestString::Ptr str;
    ASSERT_TRUE(p->GetDefaultValueAny().GetValue(str));
    ASSERT_TRUE(str);
    str->SetString("hips");
    EXPECT_EQ(p->GetValue()->GetString(), "hips");
    EXPECT_EQ(count, 1);
}

/**
 * @tc.name: MultiOnChangedWithSameHandle
 * @tc.desc: test MultiOnChangedWithSameHandle
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, MultiOnChangedWithSameHandle, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");

    int count = 0;
    EXPECT_TRUE(p->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] { ++count; }), uintptr_t(1)));
    EXPECT_TRUE(p->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] { ++count; }), uintptr_t(1)));

    p->SetValue(1);
    EXPECT_EQ(count, 1);

    EXPECT_TRUE(p->OnChanged()->RemoveHandler(uintptr_t(1)));

    p->SetValue(2);
    EXPECT_EQ(count, 2);

    EXPECT_TRUE(p->OnChanged()->RemoveHandler(uintptr_t(1)));

    p->SetValue(3);
    EXPECT_EQ(count, 2);
}

META_REGISTER_CLASS(PropertyOwnerTest, "2ab1b570-f44d-4fe3-9d40-4ed02294cee6", META_NS::ObjectCategoryBits::NO_CATEGORY)
class PropertyOwnerTest final : public IntroduceInterfaces<META_NS::ObjectFwd, IEmbeddedTestType, IPropertyOwner> {
    META_OBJECT(PropertyOwnerTest, ClassId::PropertyOwnerTest, IntroduceInterfaces)

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IEmbeddedTestType, int, Property)
    META_END_STATIC_DATA()
    META_IMPLEMENT_PROPERTY(int, Property)

    void OnPropertyChanged(const IProperty& p) override
    {
        changed_ = p.GetName() == "Property";
    }

public:
    bool changed_ {};
};

/**
 * @tc.name: PropertyOwner
 * @tc.desc: test PropertyOwner
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, PropertyOwner, TestSize.Level1)
{
    RegisterObjectType<PropertyOwnerTest>();
    auto object = GetObjectRegistry().Create(ClassId::PropertyOwnerTest);
    ASSERT_TRUE(object);

    interface_cast<IEmbeddedTestType>(object)->Property()->SetValue(1);
    PropertyOwnerTest* p = static_cast<PropertyOwnerTest*>(object.get());
    EXPECT_TRUE(p->changed_);

    UnregisterObjectType<PropertyOwnerTest>();
}

/**
 * @tc.name: DisableEventHandler
 * @tc.desc: test DisableEventHandler
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, DisableEventHandler, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    ASSERT_TRUE(p);

    int count = 0;
    auto token = p->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] { ++count; }));

    p->SetValue(1);
    EXPECT_EQ(count, 1);

    {
        ScopedDisableEventHandler scoped(p->OnChanged(), token);
        p->SetValue(2);
        EXPECT_EQ(count, 1);
    }
    p->SetValue(3);
    EXPECT_EQ(count, 2);
}

/**
 * @tc.name: PointerValueLifetime
 * @tc.desc: test PointerValueLifetime
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, PointerValueLifetime, TestSize.Level1)
{
    auto p = ConstructProperty<IObject::Ptr>("test");
    auto obj = CreateTestType<IObject>();
    IObject::WeakPtr w = obj;

    p->SetValue(obj);
    obj.reset();
    EXPECT_TRUE(p->GetValue());
    p->SetValue(nullptr);

    EXPECT_FALSE(w.lock());
}

/**
 * @tc.name: ValidPropertyLock
 * @tc.desc: test ValidPropertyLock
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, ValidPropertyLock, TestSize.Level1)
{
    {
        auto p = ConstructProperty<int>("p").GetProperty();
        PropertyLock lock { p };
        EXPECT_TRUE(lock);
        EXPECT_TRUE(lock.IsValid());
        EXPECT_TRUE(lock.GetProperty());

        TypedPropertyLock<int> typedLock { p };
        EXPECT_TRUE(typedLock);
        EXPECT_TRUE(typedLock.IsValid());
        EXPECT_TRUE(typedLock.GetProperty());

        TypedPropertyLock<float> bad { p };
        EXPECT_FALSE(bad);
        EXPECT_FALSE(bad.IsValid());
        EXPECT_FALSE(bad.GetProperty());
    }
    {
        auto p = ConstructArrayProperty<int>("p").GetProperty();
        PropertyLock lock { p };
        EXPECT_TRUE(lock);
        EXPECT_TRUE(lock.IsValid());
        EXPECT_TRUE(lock.GetProperty());

        TypedPropertyLock<int> typedLock { p };
        EXPECT_FALSE(typedLock);
        EXPECT_FALSE(typedLock.IsValid());
        EXPECT_FALSE(typedLock.GetProperty());

        TypedPropertyLock<float> bad { p };
        EXPECT_FALSE(bad);
        EXPECT_FALSE(bad.IsValid());
        EXPECT_FALSE(bad.GetProperty());
    }
}

/**
 * @tc.name: PropertyHelpers
 * @tc.desc: test PropertyHelpers
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, PropertyHelpers, TestSize.Level1)
{
    auto obj = CreateTestType<IObject>();
    auto p = ConstructProperty<IObject::Ptr>("test", obj);

    EXPECT_FALSE(IsGetPointerArray(p));
    EXPECT_FALSE(IsSetPointerArray(p));

    EXPECT_TRUE(IsGetPointer(p));
    EXPECT_EQ(GetPointer(p), interface_pointer_cast<CORE_NS::IInterface>(obj));
    EXPECT_EQ(GetPointer<IObject>(p), obj);

    auto other = CreateTestType<IObject>();

    EXPECT_TRUE(IsSetPointer(p));
    EXPECT_TRUE(SetPointer(p, other));
    EXPECT_EQ(GetPointer(p), interface_pointer_cast<CORE_NS::IInterface>(other));

    auto cp = ConstructProperty<IObject::ConstPtr>("test", obj);
    EXPECT_TRUE(IsGetPointer(cp));
    EXPECT_TRUE(IsSetPointer(cp));
    EXPECT_TRUE(SetPointer(cp, other));
    EXPECT_EQ(GetConstPointer(cp), interface_pointer_cast<CORE_NS::IInterface>(other));

    IObject::ConstPtr some = CreateTestType<IObject>();
    EXPECT_TRUE(SetPointer(cp, some));
    EXPECT_EQ(GetConstPointer(cp), interface_pointer_cast<CORE_NS::IInterface>(some));
}

/**
 * @tc.name: UnlockedOnChanged
 * @tc.desc: test UnlockedOnChanged
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, UnlockedOnChanged, TestSize.Level1)
{
    auto prop = ConstructProperty<int>("test");
    auto p = prop.GetProperty();

    int count = 0;
    prop->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] {
        ++count;
        EXPECT_EQ(GetValue<int>(p->GetValue()), count);
    }));

    p->SetValue(Any<int>(1));
}

/**
 * @tc.name: IsDefault
 * @tc.desc: test IsDefault
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, IsDefault, TestSize.Level1)
{
    auto prop = ConstructProperty<int>("test", 1);
    EXPECT_TRUE(prop->IsDefaultValue());
    EXPECT_EQ(prop->GetValue(), 1);
    prop->SetValue(2);
    EXPECT_FALSE(prop->IsDefaultValue());
    EXPECT_EQ(prop->GetValue(), 2);
    prop->ResetValue();
    EXPECT_TRUE(prop->IsDefaultValue());
    EXPECT_EQ(prop->GetValue(), 1);
}

META_REGISTER_CLASS(StaticValueTestMod, "3aff3770-0681-45b7-ab56-fe4487883a7a", ObjectCategory::NO_CATEGORY)

class StaticValueTestMod : public IntroduceInterfaces<MinimalObject, IModifier> {
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(ClassId::StaticValueTestMod)
public:
    EvaluationResult ProcessOnGet(IAny& value) override
    {
        value.SetValue<uint32_t>(4);
        return EVAL_VALUE_CHANGED;
    }
    EvaluationResult ProcessOnSet(IAny& value, const IAny& current) override
    {
        return EVAL_CONTINUE;
    }
    bool IsCompatible(const TypeId& id) const override
    {
        return id == TypeId(UidFromType<uint32_t>());
    }
};

/**
 * @tc.name: EvaluateAndStore
 * @tc.desc: test EvaluateAndStore
 * @tc.type: FUNC
 */
HWTEST_F(PropertyTest, EvaluateAndStore, TestSize.Level1)
{
    auto p = ConstructProperty<uint32_t>("test");
    auto stack = interface_cast<IStackProperty>(p->GetProperty());
    ASSERT_TRUE(stack);

    p->SetValue(2);

    ASSERT_TRUE(stack->AddModifier(IModifier::Ptr(new StaticValueTestMod)));
    EXPECT_EQ(p->GetValue(), 4);
    stack->EvaluateAndStore();

    auto v = stack->TopValue();
    ASSERT_TRUE(v);
    EXPECT_EQ(GetValue<uint32_t>(v->GetValue()), 4);
}

META_END_NAMESPACE()
