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

#ifndef META_TEST_TESTING_OBJECTS_HEADER
#define META_TEST_TESTING_OBJECTS_HEADER

#include <meta/api/make_callback.h>
#include <meta/api/object.h>
#include <meta/base/time_span.h>
#include <meta/base/types.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_startable.h>
#include <meta/interface/object_macros.h>
#include <meta/interface/property/intf_property.h>
#include <meta/interface/property/property_events.h>

META_BEGIN_NAMESPACE()

// Allow creating object instances with CreateInstance in addition to CreateObjectInstance
inline auto CreateInstance(const META_NS::ClassInfo& clsi)
{
    return META_NS::CreateObjectInstance(clsi);
}

// compatibility
META_REGISTER_CLASS(EmbeddedTestType, "4a3bd491-1111-4a9a-bcad-92530493be09", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(TestType, "766fadc7-1767-42fd-a60a-f974e25e6cc9", ObjectCategoryBits::NO_CATEGORY, "Test type")
META_REGISTER_CLASS(
    TestAttachment, "0c45ff57-14f5-4d31-9a93-2e22eea99f75", ObjectCategoryBits::NO_CATEGORY, "Test attachment")
META_REGISTER_CLASS(
    TestStartable, "e9470f5b-97ce-404c-9d1e-b37a4f1ac594", ObjectCategoryBits::NO_CATEGORY, "Test startable")

META_REGISTER_CLASS(TestContainer, "bb8fe547-6f70-4552-b269-9bcb55c27f8a", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(
    TestContainerPreTransaction, "038e74f7-bb55-4637-a13f-3db86408663f", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(TestFlatContainer, "cc9fe547-6f70-4552-b269-9bcb55c27f8a", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(TestString, "96f87fd0-aabb-4223-8ccf-24662be1dec1", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(TestPtrValue, "f77b3c6b-312a-4102-a8ed-a2442b2a8872", ObjectCategoryBits::NO_CATEGORY)

namespace UTest {

// compatibility
META_REGISTER_INTERFACE(IEmbeddedTestType, "f66fbf28-ba2c-49dc-8521-c552cb104815")
META_REGISTER_INTERFACE(IEmbeddedTestType2, "f77fbf28-ba2c-49dc-8521-c552cb104815", "Some type")

META_REGISTER_INTERFACE(ITestType, "7c327d8f-3930-478f-b91b-8ddef8a52f3e", "This is test type")
META_REGISTER_INTERFACE(ITestContainer, "456195cf-f539-4572-ba9d-3fd87e75ff08")
META_REGISTER_INTERFACE(ITestAttachment, "0af32c00-9037-47f9-b7dd-3e43679ffdfa", "This is test attachment")
META_REGISTER_INTERFACE(ITestStartable, "bb8188a9-aeca-4414-98c3-7a989d54d430", "This is test startable")
META_REGISTER_INTERFACE(ITestPtrValue, "55a8c04d-ae8f-4dfe-b604-2f31ea07e23b", "This is test type")

using BASE_NS::string;

struct MyComparableTestType {
    float i = 0;

    constexpr bool operator==(float value) const
    {
        return i == value;
    }
    constexpr bool operator!=(float value) const
    {
        return i != value;
    }
    constexpr bool operator==(const MyComparableTestType& other) const
    {
        return i == other.i;
    }
    constexpr bool operator!=(const MyComparableTestType& other) const
    {
        return i != other.i;
    }
    constexpr bool operator<(const MyComparableTestType& other) const
    {
        return i < other.i;
    }
    constexpr bool operator>(const MyComparableTestType& other) const
    {
        return i > other.i;
    }
    constexpr MyComparableTestType& operator*=(float value)
    {
        i *= value;
        return *this;
    }
    constexpr MyComparableTestType& operator/=(float value)
    {
        i /= value;
        return *this;
    }
    constexpr MyComparableTestType& operator+=(float value)
    {
        i += value;
        return *this;
    }
    constexpr MyComparableTestType& operator-=(float value)
    {
        i -= value;
        return *this;
    }
    constexpr MyComparableTestType operator*(float value) const
    {
        return MyComparableTestType { i * value };
    }
    constexpr MyComparableTestType operator/(float value) const
    {
        return MyComparableTestType { i / value };
    }
    constexpr MyComparableTestType operator-(float value) const
    {
        return MyComparableTestType { i - value };
    }
    constexpr MyComparableTestType operator+(float value) const
    {
        return MyComparableTestType { i + value };
    }
    constexpr MyComparableTestType operator*(MyComparableTestType value) const
    {
        return MyComparableTestType { i * value.i };
    }
    constexpr MyComparableTestType operator/(MyComparableTestType value) const
    {
        return MyComparableTestType { i / value.i };
    }
    constexpr MyComparableTestType operator-(MyComparableTestType value) const
    {
        return MyComparableTestType { i - value.i };
    }
    constexpr MyComparableTestType operator+(MyComparableTestType value) const
    {
        return MyComparableTestType { i + value.i };
    }
};

class IEmbeddedTestType : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEmbeddedTestType)

public:
    META_PROPERTY(int, Property)
};

class ITestAttachment : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestAttachment)

public:
    META_PROPERTY(string, Name)
    virtual void SetName(const BASE_NS::string& name) = 0;
    virtual uint32_t GetAttachCount() const = 0;
    virtual uint32_t GetDetachCount() const = 0;
};

class ITestStartable : public ITestAttachment {
    META_INTERFACE(ITestAttachment, ITestStartable)

public:
    enum class Operation : uint32_t {
        ATTACH,
        START,
        STOP,
        DETACH,
        TICK,
    };

    virtual void StartRecording() = 0;
    virtual BASE_NS::vector<Operation> StopRecording() = 0;
    virtual BASE_NS::vector<Operation> GetOps() const = 0;
    virtual TimeSpan GetLastTick() const = 0;

    META_EVENT(IOnChanged, OnStarted)
    META_EVENT(IOnChanged, OnStopped)
    META_EVENT(IOnChanged, OnTicked)

    template<class Callback>
    void AddOnStarted(Callback&& callback)
    {
        OnStarted()->AddHandler(MakeCallback<IOnChanged>(callback));
    }
    template<class Callback>
    void AddOnStopped(Callback&& callback)
    {
        OnStopped()->AddHandler(MakeCallback<IOnChanged>(callback));
    }
    template<class Callback>
    void AddOnTicked(Callback&& callback)
    {
        OnTicked()->AddHandler(MakeCallback<IOnChanged>(callback));
    }
};

std::ostream& operator<<(std::ostream& os, const META_NS::UTest::ITestStartable::Operation& op);

class ITestType : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestType)

public:
    META_PROPERTY(int, First)
    META_PROPERTY(string, Second)
    META_READONLY_PROPERTY(int, Third)
    META_READONLY_PROPERTY(string, Fourth)

    META_PROPERTY(BASE_NS::Math::Vec3, Vec3Property1)
    META_PROPERTY(BASE_NS::Math::Vec3, Vec3Property2)
    META_PROPERTY(BASE_NS::Math::Vec3, Vec3Property3)

    META_PROPERTY(float, FloatProperty1)
    META_PROPERTY(float, FloatProperty2)

    META_PROPERTY(MyComparableTestType, MyTestTypeProperty1)
    META_PROPERTY(MyComparableTestType, MyTestTypeProperty2)

    META_PROPERTY(IEmbeddedTestType::Ptr, EmbeddedTestTypeProperty)

    META_ARRAY_PROPERTY(int, MyIntArray)
    META_READONLY_ARRAY_PROPERTY(int, MyConstIntArray)

    META_ARRAY_PROPERTY(IObject::Ptr, MyObjectArray)
    //    META_PROPERTY(int, NonSerialized)

    META_PROPERTY(string, Name)
    virtual void SetName(const BASE_NS::string& name) = 0;

    META_EVENT(IOnChanged, OnTest)

    virtual int NormalMember() = 0;
    virtual int OtherNormalMember(int, int) const = 0;
};

class ITestContainer : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestContainer)

public:
    META_PROPERTY(string, Name)
    virtual void SetName(const BASE_NS::string& name) = 0;

    virtual void Increment() = 0;
    virtual int GetIncrement() const = 0;
};

class ITestString : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestString, "3f05057d-95b3-4599-a88d-5983287a48e6")

public:
    virtual BASE_NS::string GetString() const = 0;
    virtual void SetString(BASE_NS::string) = 0;
};

class ITestPtrValue : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestPtrValue)

public:
    META_PROPERTY(ITestString::Ptr, Text)
    META_PROPERTY(ITestString::Ptr, String)
};

void RegisterTestTypes();
void UnregisterTestTypes();
ITestType::Ptr CreateTestType(const BASE_NS::string_view name = {});
ITestContainer::Ptr CreateTestContainer(
    const BASE_NS::string_view name = {}, ClassInfo type = META_NS::ClassId::TestContainer);
ITestAttachment::Ptr CreateTestAttachment(const BASE_NS::string_view name = {});
ITestStartable::Ptr CreateTestStartable(
    const BASE_NS::string_view name = {}, StartBehavior behavior = StartBehavior::MANUAL);

template<class T>
typename T::Ptr CreateTestType(const BASE_NS::string_view name = {})
{
    return interface_pointer_cast<T>(CreateTestType(name));
}

template<class T>
typename T::Ptr CreateTestContainer(
    const BASE_NS::string_view name = {}, ClassInfo type = META_NS::ClassId::TestContainer)
{
    return interface_pointer_cast<T>(CreateTestContainer(name, type));
}

template<class T>
typename T::Ptr CreateTestAttachment(
    const BASE_NS::string_view name = {}, ClassInfo type = META_NS::ClassId::TestContainer)
{
    return interface_pointer_cast<T>(CreateTestAttachment(name));
}

template<class T>
typename T::Ptr CreateTestStartable(
    const BASE_NS::string_view name = {}, ClassInfo type = META_NS::ClassId::TestStartable)
{
    return interface_pointer_cast<T>(CreateTestStartable(name));
}

struct MyTestType {
    int i = 0;
};

enum TestEnum { TestEnumA = 1, TestEnumB = 2 };

} // namespace UTest
META_END_NAMESPACE()

#define META_INTERFACE_TYPE_IMP(a, n)           \
    META_TYPE_IMPL(a::Ptr, n "::Ptr")           \
    META_TYPE_IMPL(a::ConstPtr, n "::ConstPtr") \
    META_TYPE_IMPL(a::WeakPtr, n "::WeakPtr")   \
    META_TYPE_IMPL(a::ConstWeakPtr, n "::ConstWeakPtr")

META_INTERFACE_TYPE_IMP(META_NS::UTest::ITestString, "META_NS::Test::ITestString")
META_INTERFACE_TYPE_IMP(META_NS::UTest::ITestPtrValue, "META_NS::Test::ITestPtrValue")
META_INTERFACE_TYPE_IMP(META_NS::UTest::ITestType, "META_NS::Test::ITestType")
META_INTERFACE_TYPE_IMP(META_NS::UTest::IEmbeddedTestType, "META_NS::Test::IEmbeddedTestType")
META_TYPE_IMPL(META_NS::UTest::MyTestType, "META_NS::Test::MyTestType")
META_TYPE_IMPL(META_NS::UTest::MyComparableTestType, "META_NS::Test::MyComparableTestType")
META_TYPE_IMPL(META_NS::UTest::TestEnum, "META_NS::Test::TestEnum")

#endif // META_TEST_TESTING_OBJECTS_HEADER
