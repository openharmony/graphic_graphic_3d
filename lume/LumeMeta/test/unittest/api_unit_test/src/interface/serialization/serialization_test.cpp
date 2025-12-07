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

#include <test_framework.h>

#include <base/math/matrix.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>

#include <meta/api/object.h>
#include <meta/base/meta_types.h>
#include <meta/ext/any_builder.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object_fwd.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_iterable.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_startable.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_ser_output.h>
#include <meta/interface/serialization/intf_serializable.h>

#include "helpers/serialisation_utils.h"
#include "helpers/test_data_helpers.h"
#include "helpers/test_utils.h"
#include "helpers/util.h"

META_BEGIN_NAMESPACE()
namespace UTest {

// ----------- Basic Serialisation of Types -------------

template<typename Type>
class IBasicSerType : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IBasicSerType<Type>, MakeUid<Type>("BasicSer"))

public:
    virtual Type GetValue() const = 0;
    virtual void SetValue(Type) = 0;
    virtual BASE_NS::vector<Type> GetVector() const = 0;
    virtual void SetVector(BASE_NS::vector<Type>) = 0;
};

template<typename Type>
inline constexpr ClassInfo BASIC_SER_TYPE_INFO { MakeUid<Type>("BasicTyp"), "BasicType",
    ObjectCategoryBits::NO_CATEGORY, false };

template<typename Type>
struct BasicSerType : public IntroduceInterfaces<BaseObjectFwd, IBasicSerType<Type>, ISerializable> {
    using Intro = IntroduceInterfaces<BaseObjectFwd, IBasicSerType<Type>, ISerializable>;
    META_OBJECT(BasicSerType<Type>, BASIC_SER_TYPE_INFO<Type>, Intro)
    ReturnError Export(IExportContext& c) const override
    {
        return Serializer(c) & NamedValue("value", value) & NamedValue("vector", vector);
    }
    ReturnError Import(IImportContext& c) override
    {
        return Serializer(c) & NamedValue("value", value) & NamedValue("vector", vector);
    }
    Type GetValue() const override
    {
        return value;
    }
    void SetValue(Type i) override
    {
        value = i;
    }
    BASE_NS::vector<Type> GetVector() const override
    {
        return vector;
    }
    void SetVector(BASE_NS::vector<Type> s) override
    {
        vector = s;
    }

    Type value {};
    BASE_NS::vector<Type> vector;
};

// clang-format off
const TestTypes BasicTestData(
        TType<bool>                {true, false},
        TType<int8_t>              {2, -100},
        TType<int16_t>             {4012, -10},
        TType<int32_t>             {111111, -45114},
        TType<int64_t>             {1234567890LL, -5555555LL},
        TType<uint8_t>             {2, 200},
        TType<uint16_t>            {4012, 56000},
        TType<uint32_t>            {115111, 455647114},
        TType<uint64_t>            {5, 99999999999ULL},
        TType<float>               {1.0f, 2.33333f},
        TType<double>              {3.0, -4363.12455},
        TType<BASE_NS::string>     {"abc", "hips hops haps"},
        TType<RefUri>              {RefUri("ref://Some/$Other"), RefUri("ref://@Context/Other")},
        TType<ITestType::Ptr>      {[]{return CreateTestType("Test1");}, []{return CreateTestType("Test2");}},
        TType<ITestType::ConstPtr> {[]{return CreateTestType("Test1");}, []{return CreateTestType("Test2");}},
        TType<IObject::Ptr>        {IObject::Ptr{}, IObject::Ptr{}},
        TType<IAny::Ptr>           {[]{return ConstructAny(1);}, []{return ConstructAny(2);}},
        TType<IArrayAny::Ptr>      {[]{return ConstructArrayAny<uint32_t>({1,2,1});}, []{return ConstructArrayAny<uint32_t>({2,3,4});}},
        TType<BASE_NS::Math::Vec2> {BASE_NS::Math::Vec2{1.3f, 5.0f}, BASE_NS::Math::Vec2{-3.1f, 13.0f}},
        TType<BASE_NS::Math::Vec3> {BASE_NS::Math::Vec3{1.f, 2.f ,3.f},BASE_NS::Math::Vec3{4.f, 1.2f, 0.88f}},
        TType<BASE_NS::Math::Vec4> {BASE_NS::Math::Vec4{4.f, 5.f, 6.f, 7.f},BASE_NS::Math::Vec4{-0.005f, 0.13f, 13.f, 100.f}},
        TType<BASE_NS::Math::UVec2>{BASE_NS::Math::UVec2{1,2},BASE_NS::Math::UVec2{6,7}},
        TType<BASE_NS::Math::UVec3>{BASE_NS::Math::UVec3{1,2,3},BASE_NS::Math::UVec3{6,7,8}},
        TType<BASE_NS::Math::UVec4>{BASE_NS::Math::UVec4{1,2,3,4},BASE_NS::Math::UVec4{6,7,8,9}},
        TType<BASE_NS::Math::IVec2>{BASE_NS::Math::IVec2{-1, 1},BASE_NS::Math::IVec2{60, -50}},
        TType<BASE_NS::Math::IVec3>{BASE_NS::Math::IVec3{-1, 1, 0},BASE_NS::Math::IVec3{60, -50, 1}},
        TType<BASE_NS::Math::IVec4>{BASE_NS::Math::IVec4{-1, 1, 9, -50},BASE_NS::Math::IVec4{60, -50, 2, 3}},
        TType<BASE_NS::Math::Quat> {BASE_NS::Math::Quat{4.f, 5.f, 6.f, 7.f},BASE_NS::Math::Quat{-0.005f, 0.13f, 13.f, 100.f}},
        TType<BASE_NS::Math::Mat3X3>{BASE_NS::Math::Mat3X3{BASE_NS::Math::Vec3{1.f, 2.f, 3.f}, BASE_NS::Math::Vec3{4.f, 5.f, 6.f}, BASE_NS::Math::Vec3{7.f, 8.f, 9.f}},BASE_NS::Math::Mat3X3{3.3f}},
        TType<BASE_NS::Math::Mat4X4>{BASE_NS::Math::Mat4X4{1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f},BASE_NS::Math::Mat4X4{2.2f}},
        TType<TraversalType>       {TraversalType::FULL_HIERARCHY, TraversalType::BREADTH_FIRST_ORDER},
        TType<StartBehavior>       {StartBehavior::MANUAL, StartBehavior::AUTOMATIC},
        TType<StartableState>      {StartableState::ATTACHED, StartableState::STARTED},
        TType<TimeSpan>            {TimeSpan::Microseconds(1234567), TimeSpan::Microseconds(97)},
        TType<BASE_NS::Uid>        {BASE_NS::StringToUid("9e6d554e-3647-4040-a5c4-ea299fa85b8f"), BASE_NS::StringToUid("d3c49aa9-9314-4680-b23a-7d2a24a0af30")}
        );

// clang-format on

template<typename T>
class API_BasicSerialisationTest : public ::testing::Test {
public:
    using Type = T;
    Type value1;
    Type value2;

    API_BasicSerialisationTest() : value1(BasicTestData.GetValue<Type>(0)), value2(BasicTestData.GetValue<Type>(1)) {}

    Type GetDefValue() const
    {
        return {};
    }
    Type GetValue1() const
    {
        return value1;
    }
    Type GetValue2() const
    {
        return value2;
    }
    BASE_NS::vector<Type> GetDefVector() const
    {
        return {};
    }
    BASE_NS::vector<Type> GetVector1() const
    {
        return { GetValue1(), GetValue2(), GetValue1() };
    }
    BASE_NS::vector<Type> GetVector2() const
    {
        return { GetValue2(), GetValue1(), GetValue2() };
    }
    typename IBasicSerType<Type>::Ptr CreateBasicType()
    {
        return GetObjectRegistry().Create<IBasicSerType<Type>>(BASIC_SER_TYPE_INFO<Type>);
    }
    void SetUp() override
    {
        ::testing::Test::SetUp();
        auto& reg = GetObjectRegistry();
        reg.RegisterObjectType<BasicSerType<Type>>();
    }
    void TearDown() override
    {
        auto& reg = GetObjectRegistry();
        reg.UnregisterObjectType<BasicSerType<Type>>();
        ::testing::Test::TearDown();
    }
};

TYPED_TEST_SUITE(API_BasicSerialisationTest, decltype(BasicTestData)::Types);

TYPED_TEST(API_BasicSerialisationTest, ExportImportValue)
{
    TestSerialiser ser;
    {
        auto obj = this->CreateBasicType();
        ASSERT_TRUE(obj);
        obj->SetValue(this->GetValue1());
        obj->SetVector(this->GetVector1());

        ASSERT_TRUE(ser.Export(obj));
        ser.Dump("app://test.json");
    }
    auto obj = ser.Import<IBasicSerType<TypeParam>>();
    ASSERT_TRUE(obj);
    EXPECT_TRUE(IsEqual(obj->GetValue(), this->GetValue1()));
    EXPECT_TRUE(IsEqual(obj->GetVector(), this->GetVector1()));
}

/**
 * @tc.name: AutoSerialisationIMetadata
 * @tc.desc: Tests for Auto Serialisation Imetadata. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, AutoSerialisationIMetadata, testing::ext::TestSize.Level1)
{
    auto original = CreateTestType();
    TestSerialiser ser;
    {
        ASSERT_TRUE(ser.Export(original));
        ser.Dump("app://test.json");
    }
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    EXPECT_TRUE(IsEqual(interface_pointer_cast<IMetadata>(original), obj));
}

/**
 * @tc.name: AutoSerialisationIContainer
 * @tc.desc: Tests for Auto Serialisation Icontainer. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, AutoSerialisationIContainer, testing::ext::TestSize.Level1)
{
    auto original = CreateTestContainer<IContainer>();
    TestSerialiser ser;
    {
        original->Add(CreateTestType<IObject>("Test1"));
        original->Add(CreateTestType<IObject>("Test2"));
        original->Add(CreateTestType<IObject>("Test3"));
        original->Add(CreateTestType<IObject>("Test4"));
        ASSERT_TRUE(ser.Export(original));
        ser.Dump("app://test.json");
    }
    auto obj = ser.Import<IContainer>();
    ASSERT_TRUE(obj);
    EXPECT_TRUE(IsEqual(original, obj));
}

/**
 * @tc.name: AutoSerialisationIAttach
 * @tc.desc: Tests for Auto Serialisation Iattach. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, AutoSerialisationIAttach, testing::ext::TestSize.Level1)
{
    AttachmentContainer original(CreateInstance(ClassId::Object));
    Object other(CreateInstance(ClassId::Object));
    TestSerialiser ser;
    {
        original.Attach(CreateTestAttachment<IAttachment>("Test1"));
        original.Attach(CreateTestAttachment<IAttachment>("Test2"));
        original.Attach(CreateTestAttachment<IAttachment>("Test3"));
        // use non-iattachment object
        original.Attach(other);
        ASSERT_TRUE(ser.Export(Object(original)));
        ser.Dump("app://test.json");
    }
    auto obj = ser.Import<IAttach>();
    ASSERT_TRUE(obj);
    EXPECT_TRUE(IsEqual(interface_pointer_cast<IAttach>(original), obj));
}

/**
 * @tc.name: AutoSerialisationIObjectFlags
 * @tc.desc: Tests for Auto Serialisation Iobject Flags. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, AutoSerialisationIObjectFlags, testing::ext::TestSize.Level1)
{
    {
        Object obj(CreateInstance(ClassId::Object));
        auto flags = interface_cast<IObjectFlags>(obj)->GetObjectFlags();
        TestSerialiser ser;
        ASSERT_TRUE(ser.Export(obj));
        auto imp = ser.Import<IObjectFlags>();
        ASSERT_TRUE(imp);
        EXPECT_EQ(imp->GetObjectFlags(), flags);
    }
    {
        Object obj(CreateInstance(ClassId::Object));
        SetObjectFlags(obj, ObjectFlagBits::INTERNAL, true);
        auto flags = interface_cast<IObjectFlags>(obj)->GetObjectFlags();
        TestSerialiser ser;
        ASSERT_TRUE(ser.Export(obj));
        ser.Dump("app://test.json");
        auto imp = ser.Import<IObjectFlags>();
        ASSERT_TRUE(imp);
        EXPECT_EQ(imp->GetObjectFlags(), flags);
    }
}

/**
 * @tc.name: EscapingNames
 * @tc.desc: Tests for Escaping Names. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, EscapingNames, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;

    BASE_NS::string contName = " Co\"ntai\"ne'r'.S/g\\Hop \n\t\b\fHii";
    BASE_NS::string rect1Name = " Rect1\"ntai\"ne'r'.S/g\\Hop \n\t\b\fHii";
    BASE_NS::string rect2Name = " Rect2\"ntai\"ne'r'.S/g\\Hop \n\t\b\fHii";
    BASE_NS::string prop1Name = " Prop1\"ntai\"ne'r'.S/g\\Hop \n\t\b\fHii";
    BASE_NS::string prop2Name = " Prop2\"ntai\"ne'r'.S/g\\Hop \n\t\b\fHii";
    BASE_NS::string prop1Value = " ?Pr@£$€%¤€[{[]]}¨~[]{}!!|<>*^-.,,;:op1\"ntai\"ne'r'.S/g\\Hop \n\t\b\fHii";
    BASE_NS::string prop2Value = "SomeSomeSomeSomeSome";
    // add null character to the middle (this is currently not escaped and outputs invalid json)
    // prop2Value[prop2Value.size()/2] = 0;

    {
        auto c = CreateTestContainer<IContainer>(contName);
        c->Add(CreateTestType<IObject>(rect1Name));
        c->Add(CreateTestType<IObject>(rect2Name));

        auto prop1 = META_NS::ConstructProperty<BASE_NS::string>(prop1Name, {});
        auto prop2 = META_NS::ConstructProperty<BASE_NS::string>(prop2Name, {});

        auto data = interface_cast<META_NS::IMetadata>(c);
        ASSERT_TRUE(data);

        prop1->SetValue(prop1Value);
        prop2->SetValue(prop2Value);

        // Add properties to our object, they should get serialized along with the object
        data->AddProperty(prop1);
        data->AddProperty(prop2);

        // Export the object
        ASSERT_TRUE(ser.Export(c));
    }

    META_NS::IObject::Ptr object = ser.Import();
    ASSERT_TRUE(object);

    EXPECT_EQ(object->GetName(), contName);

    auto cont = interface_cast<META_NS::IContainer>(object);
    ASSERT_TRUE(cont);

    auto data = interface_cast<META_NS::IMetadata>(object);
    ASSERT_TRUE(data);

    auto children = cont->GetAll();
    ASSERT_EQ(children.size(), 2);

    for (auto c : children) {
        const auto name = c->GetName();
        EXPECT_TRUE(name == rect1Name || name == rect2Name);
    }

    auto prop1 = data->GetProperty<BASE_NS::string>(prop1Name);
    ASSERT_TRUE(prop1);
    EXPECT_EQ(prop1->GetName(), prop1Name);
    EXPECT_EQ(prop1->GetValue(), prop1Value);

    auto prop2 = data->GetProperty<BASE_NS::string>(prop2Name);
    ASSERT_TRUE(prop2);
    EXPECT_EQ(prop2->GetName(), prop2Name);
    EXPECT_EQ(prop2->GetValue(), prop2Value);
}

/**
 * @tc.name: InstanceIdMapping
 * @tc.desc: Tests for Instance Id Mapping. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, InstanceIdMapping, testing::ext::TestSize.Level1)
{
    TestSerialiser ser1;
    {
        auto original = CreateTestType();
        ASSERT_TRUE(ser1.Export(original));
    }
    auto obj1 = ser1.Import<IMetadata>();

    TestSerialiser ser2;
    ser2.SetInstanceIdMapping(ser1.GetInstanceIdMapping());
    {
        ASSERT_TRUE(ser2.Export(obj1));
    }
    auto obj2 = ser2.Import<IMetadata>();

    auto map1 = ser1.GetInstanceIdMapping();
    auto map2 = ser2.GetInstanceIdMapping();
    BASE_NS::unordered_map<InstanceId, InstanceId> revMap;
    for (auto&& v : map2) {
        revMap[v.second] = v.first;
    }

    for (auto&& v : map1) {
        EXPECT_TRUE(revMap.count(v.second));
    }

    // ser1.Dump("app://ids1.json");
    // ser2.Dump("app://ids2.json");
}

META_REGISTER_CLASS(TestMetadata, "95d0b30e-5c77-4d4f-8741-04a7b2b11ad2", ObjectCategoryBits::NO_CATEGORY)

class TestMetadata : public IntroduceInterfaces<MetaObject, ISerializable> {
    META_OBJECT(TestMetadata, ClassId::TestMetadata, IntroduceInterfaces)
public:
    ReturnError Export(IExportContext& c) const override
    {
        EXPECT_EQ(SerMetadataValues(c.GetMetadata()).GetVersion(), Version(1, 2));
        return GenericError::SUCCESS;
    }
    ReturnError Import(IImportContext& c) override
    {
        EXPECT_EQ(SerMetadataValues(c.GetMetadata()).GetVersion(), Version(1, 2));
        return GenericError::SUCCESS;
    }
};

/**
 * @tc.name: Metadata
 * @tc.desc: Tests for Metadata. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, Metadata, testing::ext::TestSize.Level1)
{
    auto& reg = GetObjectRegistry();
    reg.RegisterObjectType<TestMetadata>();

    TestSerialiser ser;
    {
        ser.SetMetadata(SerMetadataValues().SetVersion(Version(1, 2)).SetType("test"));
        auto original = CreateTestType();
        ASSERT_TRUE(ser.Export(original));
    }
    auto obj = ser.Import<IMetadata>();
    SerMetadataValues m(ser.GetMetadata());
    EXPECT_EQ(m.GetVersion(), Version(1, 2));
    EXPECT_EQ(m.GetType(), "test");

    reg.UnregisterObjectType<TestMetadata>();
}

/**
 * @tc.name: NonSerializablePropertyInProperty
 * @tc.desc: Tests for Non Serializable Property In Property. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, NonSerializablePropertyInProperty, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    {
        Object obj(CreateInstance(META_NS::ClassId::Object));

        auto p1 = ConstructProperty<int>("p1");
        auto p2 = ConstructProperty<int>("p2");
        auto vp2 = interface_pointer_cast<IValue>(p2.GetProperty());
        ASSERT_TRUE(vp2);

        SetObjectFlags(p2.GetProperty(), ObjectFlagBits::SERIALIZE, false);

        p1->SetValue(3);
        EXPECT_TRUE(p1->PushValue(vp2));
        p1->SetValue(4);
        EXPECT_EQ(p1->GetValue(), 4);

        Metadata md(obj);
        md.AddProperty(p1);

        ASSERT_TRUE(ser.Export(obj));
        ser.Dump("app://test.json");
    }
    auto obj = ser.Import<IMetadata>();

    auto p = obj->GetProperty<int>("p1");
    EXPECT_EQ(p->GetValue(), 3);
}

} // namespace UTest
META_END_NAMESPACE()
