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

#include <meta/api/function.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/metadata_helpers.h>
#include <meta/ext/object.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/serialization/intf_serializable.h>

#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"
#include "helpers/util.h"

META_BEGIN_NAMESPACE()
namespace UTest {

/**
 * @tc.name: Functions
 * @tc.desc: Tests for Functions. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MetadataTest, Functions, testing::ext::TestSize.Level1)
{
    auto p = CreateTestType();
    ITestType::ConstPtr cp = p;

    auto m = interface_cast<IMetadata>(p);
    ASSERT_TRUE(m);

    {
        auto f = m->GetFunction("NormalMember");
        ASSERT_TRUE(f);
        auto res = CallMetaFunction<void>(f);
        ASSERT_TRUE(res);
    }
    {
        auto f = m->GetFunction("OtherNormalMember");
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
 * @tc.name: Events
 * @tc.desc: Tests for Events. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MetadataTest, Events, testing::ext::TestSize.Level1)
{
    auto p = CreateTestType();
    auto m = interface_cast<IMetadata>(p);
    ASSERT_TRUE(m);

    auto e = m->GetEvent("OnTest");
    ASSERT_TRUE(e);
}

template<typename Set, typename Get>
void TestGetSetValue(Set sp, Get gp)
{
    ASSERT_TRUE(SetValue(sp, "First", 1));
    ASSERT_EQ(GetValue(gp, "First", 2), 1);

    ASSERT_TRUE(SetValue<int>(sp, "First", 2));
    ASSERT_EQ(GetValue<int>(gp, "First"), 2);
    ASSERT_EQ(GetValue<int>(gp, "First", 1), 2);

    ASSERT_FALSE(SetValue<float>(sp, "First", 2));

    ASSERT_EQ(GetValue<float>(gp, "First"), 0);
    ASSERT_EQ(GetValue<float>(gp, "First", 1), 1);

    ASSERT_FALSE(SetValue(Set {}, "First", 3));
    Get nil {};
    ASSERT_EQ(GetValue(nil, "First", 3), 3);
    ASSERT_EQ(GetValue<int>(nil, "First", 3), 3);
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
 * @tc.name: GetSetValue
 * @tc.desc: Tests for Get Set Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MetadataTest, GetSetValue, testing::ext::TestSize.Level1)
{
    auto p = CreateTestType();

    TestGetSetValue<ITestType>(p);
    TestGetSetValue<IMetadata>(interface_pointer_cast<IMetadata>(p));
    TestGetSetValue<CORE_NS::IInterface>(interface_pointer_cast<CORE_NS::IInterface>(p));
}

/**
 * @tc.name: StaticMetadata
 * @tc.desc: Tests for Static Metadata. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MetadataTest, StaticMetadata, testing::ext::TestSize.Level1)
{
    EXPECT_EQ(META_NS::UTest::InterfaceId::ITestType.ReadableName(), "This is test type");
    EXPECT_EQ(META_NS::UTest::InterfaceId::ITestContainer.ReadableName(), "ITestContainer");

    auto fac = GetObjectRegistry().GetObjectFactory(META_NS::ClassId::TestType);
    ASSERT_TRUE(fac);
    auto sm = fac->GetClassStaticMetadata();

    ASSERT_TRUE(sm);
    auto p = FindStaticMetadata(*sm, "First", MetadataType::PROPERTY);
    ASSERT_TRUE(p);
    ASSERT_TRUE(p->interfaceInfo.IsValid());
    ASSERT_TRUE(p->type == MetadataType::PROPERTY);
    EXPECT_EQ(p->interfaceInfo.ReadableName(), "This is test type");

    auto e = FindStaticMetadata(*sm, "OnTest", MetadataType::EVENT);
    ASSERT_TRUE(e);
    ASSERT_TRUE(e->interfaceInfo.IsValid());
    ASSERT_TRUE(e->type == MetadataType::EVENT);
    EXPECT_EQ(e->interfaceInfo.ReadableName(), "This is test type");

    auto f = FindStaticMetadata(*sm, "NormalMember", MetadataType::FUNCTION);
    ASSERT_TRUE(f);
    ASSERT_TRUE(f->interfaceInfo.IsValid());
    ASSERT_TRUE(f->type == MetadataType::FUNCTION);
    EXPECT_EQ(f->interfaceInfo.ReadableName(), "This is test type");
}

struct MData {
    const StaticMetadata* data {};
    CORE_NS::IInterface* pointer {};
};

class IMetadataTest : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMetadataTest, "751c6067-4fb8-404b-a240-4fa6f32c727c")

public:
    META_PROPERTY(int, Value)
    META_READONLY_PROPERTY(int, ReadValue)
    META_EVENT(IOnChanged, OnChanged)
    virtual void Func() = 0;

    virtual BASE_NS::vector<MData> GetDatas() const = 0;
};

META_REGISTER_CLASS(MetadataTest, "751c6067-4fb8-404b-a240-4fa6f32c726b", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(EmptyMetadataTest, "5d726c98-89e5-445e-a830-4a634c69c012", ObjectCategoryBits::NO_CATEGORY)

class MetadataTest : public IntroduceInterfaces<META_NS::MetaObject, IMetadataTest, IMetadataOwner> {
    META_OBJECT(MetadataTest, ClassId::MetadataTest, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IMetadataTest, int, Value)
    META_STATIC_PROPERTY_DATA(IMetadataTest, int, ReadValue)
    META_STATIC_EVENT_DATA(IMetadataTest, IOnChanged, OnChanged)
    META_STATIC_FUNCTION_DATA(IMetadataTest, Func)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(int, Value)
    META_IMPLEMENT_READONLY_PROPERTY(int, ReadValue)
    META_IMPLEMENT_EVENT(IOnChanged, OnChanged)

    void Func() override {}

    void OnMetadataConstructed(const META_NS::StaticMetadata& m, CORE_NS::IInterface& i) override
    {
        datas_.push_back(MData { &m, &i });
    }

    BASE_NS::vector<MData> GetDatas() const override
    {
        return datas_;
    }

    BASE_NS::vector<MData> datas_;
};

class EmptyMetadataTest : public MetadataTest {
    META_OBJECT(EmptyMetadataTest, ClassId::EmptyMetadataTest, MetadataTest)
};

template<typename Type>
void TestMetadata(const ClassInfo& info)
{
    auto& r = META_NS::GetObjectRegistry();
    r.RegisterObjectType<Type>();

    auto obj = r.Create<IMetadata>(info);
    ASSERT_TRUE(obj);

    MetadataInfo valueMetaInfo { MetadataType::PROPERTY, "Value", IMetadataTest::UID, false,
        TypeId(UidFromType<int>()) };
    MetadataInfo readValueMetaInfo { MetadataType::PROPERTY, "ReadValue", IMetadataTest::UID, true,
        TypeId(UidFromType<int>()) };
    MetadataInfo onChangedMetaInfo { MetadataType::EVENT, "OnChanged", IMetadataTest::UID };
    MetadataInfo funcMetaInfo { MetadataType::FUNCTION, "Func", IMetadataTest::UID };

    {
        auto vec = obj->GetAllMetadatas(MetadataType::PROPERTY);
        ASSERT_EQ(vec.size(), 2);
        EXPECT_THAT(vec, testing::Contains(valueMetaInfo));
        EXPECT_THAT(vec, testing::Contains(readValueMetaInfo));
    }
    {
        auto vec = obj->GetAllMetadatas(MetadataType::EVENT);
        ASSERT_EQ(vec.size(), 1);
        EXPECT_EQ(vec[0], onChangedMetaInfo);
        EXPECT_EQ(vec[0].interfaceId, IMetadataTest::UID);
    }
    {
        auto vec = obj->GetAllMetadatas(MetadataType::FUNCTION);
        ASSERT_EQ(vec.size(), 1);
        EXPECT_EQ(vec[0], funcMetaInfo);
        EXPECT_EQ(vec[0].interfaceId, IMetadataTest::UID);
    }
    {
        auto vec = obj->GetAllMetadatas(MetadataType::PROPERTY | MetadataType::EVENT | MetadataType::FUNCTION);
        ASSERT_EQ(vec.size(), 4);
        EXPECT_THAT(vec, testing::Contains(valueMetaInfo));
        EXPECT_THAT(vec, testing::Contains(readValueMetaInfo));
        EXPECT_THAT(vec, testing::Contains(onChangedMetaInfo));
        EXPECT_THAT(vec, testing::Contains(funcMetaInfo));
    }
    {
        auto p = ConstructProperty<float>("Test");
        obj->AddProperty(p);
        auto vec = obj->GetAllMetadatas(MetadataType::PROPERTY);
        ASSERT_EQ(vec.size(), 3);
        EXPECT_THAT(vec, testing::Contains(valueMetaInfo));
        EXPECT_THAT(vec, testing::Contains(readValueMetaInfo));
        EXPECT_THAT(vec, testing::Contains(
                             MetadataInfo { MetadataType::PROPERTY, "Test", {}, false, TypeId(UidFromType<float>()) }));
    }
    {
        interface_cast<IMetadataTest>(obj)->Value()->SetValue(1);
        auto vec = obj->GetAllMetadatas(MetadataType::PROPERTY);
        ASSERT_EQ(vec.size(), 3);
        EXPECT_THAT(vec, testing::Contains(valueMetaInfo));
        EXPECT_THAT(vec, testing::Contains(readValueMetaInfo));
        EXPECT_THAT(vec, testing::Contains(
                             MetadataInfo { MetadataType::PROPERTY, "Test", {}, false, TypeId(UidFromType<float>()) }));
    }
    {
        auto m = obj->GetMetadata(MetadataType::PROPERTY, "Value");
        EXPECT_EQ(m, valueMetaInfo);
    }
    {
        auto m = obj->GetMetadata(MetadataType::PROPERTY, "ReadValue");
        EXPECT_EQ(m, readValueMetaInfo);
    }
    {
        auto m = obj->GetMetadata(MetadataType::EVENT, "OnChanged");
        EXPECT_EQ(m, onChangedMetaInfo);
    }
    {
        auto m = obj->GetMetadata(MetadataType::FUNCTION, "Func");
        EXPECT_EQ(m, funcMetaInfo);
    }
    {
        auto m = obj->GetMetadata(MetadataType::FUNCTION, "Value");
        EXPECT_FALSE(m.IsValid());
    }

    r.UnregisterObjectType<Type>();
}

/**
 * @tc.name: MetadataInfo
 * @tc.desc: Tests for Metadata Info. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MetadataTest, MetadataInfo, testing::ext::TestSize.Level1)
{
    TestMetadata<MetadataTest>(ClassId::MetadataTest);
}

/**
 * @tc.name: EmptyMetadataInfo
 * @tc.desc: Tests for Empty Metadata Info. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MetadataTest, EmptyMetadataInfo, testing::ext::TestSize.Level1)
{
    TestMetadata<EmptyMetadataTest>(ClassId::EmptyMetadataTest);
}

/**
 * @tc.name: MetadataConstructor
 * @tc.desc: Tests for Metadata Constructor. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MetadataTest, MetadataConstructor, testing::ext::TestSize.Level1)
{
    auto& r = META_NS::GetObjectRegistry();
    r.RegisterObjectType<MetadataTest>();
    auto obj = r.Create<IMetadataTest>(ClassId::MetadataTest);
    auto meta = interface_cast<IMetadata>(obj);
    ASSERT_TRUE(meta);
    auto smeta = interface_cast<IStaticMetadata>(meta);
    ASSERT_TRUE(smeta);
    auto sm = smeta->GetStaticMetadata();
    ASSERT_TRUE(sm);

    EXPECT_EQ(obj->GetDatas().size(), 0);
    ASSERT_TRUE(meta->GetProperty("Value"));
    {
        auto d = obj->GetDatas();
        ASSERT_EQ(d.size(), 1);
        EXPECT_EQ(d[0].data, FindStaticMetadata(*sm, "Value", MetadataType::PROPERTY));
        auto p = interface_cast<IProperty>(d[0].pointer);
        ASSERT_TRUE(p);
        EXPECT_EQ(p->GetName(), "Value");
    }
    ASSERT_TRUE(meta->GetEvent("OnChanged"));
    {
        auto d = obj->GetDatas();
        ASSERT_EQ(d.size(), 2);
        EXPECT_EQ(d[1].data, FindStaticMetadata(*sm, "OnChanged", MetadataType::EVENT));
        auto p = interface_cast<IEvent>(d[1].pointer);
        ASSERT_TRUE(p);
        EXPECT_EQ(p->GetName(), "OnChanged");
    }
    ASSERT_TRUE(meta->GetFunction("Func"));
    {
        auto d = obj->GetDatas();
        ASSERT_EQ(d.size(), 3);
        EXPECT_EQ(d[2].data, FindStaticMetadata(*sm, "Func", MetadataType::FUNCTION));
        auto p = interface_cast<IFunction>(d[2].pointer);
        ASSERT_TRUE(p);
        EXPECT_EQ(p->GetName(), "Func");
    }

    r.UnregisterObjectType<MetadataTest>();
}

class MetadataTestAggr : public MetaObject {
    META_OBJECT_NO_CLASSINFO(MetadataTestAggr, MetaObject, ClassId::MetadataTest)
};

META_REGISTER_CLASS(MetadataTestAggrDerived, "ef139359-8e47-4558-b032-f96dd473baee", ObjectCategoryBits::NO_CATEGORY)

class MetadataTestAggrDerived : public MetadataTestAggr {
    // override the aggregate base
    META_OBJECT(MetadataTestAggrDerived, META_NS::UTest::ClassId::MetadataTestAggrDerived, MetadataTestAggr,
        META_NS::ClassId::Object)
};

/**
 * @tc.name: AggregateOverride
 * @tc.desc: Tests for Aggregate Override. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MetadataTest, AggregateOverride, testing::ext::TestSize.Level1)
{
    auto& r = META_NS::GetObjectRegistry();
    r.RegisterObjectType<MetadataTestAggrDerived>();

    auto fac = GetObjectRegistry().GetObjectFactory(META_NS::UTest::ClassId::MetadataTestAggrDerived);
    ASSERT_TRUE(fac);
    auto sm = fac->GetClassStaticMetadata();

    ASSERT_TRUE(sm);
    // the aggregate was overridden so we should not find this
    EXPECT_FALSE(FindStaticMetadata(*sm, "Value", MetadataType::PROPERTY));

    r.UnregisterObjectType<MetadataTestAggrDerived>();
}

META_REGISTER_CLASS(MetadataForwardTest, "947c9ac4-2116-4971-a544-d34e85b51005", ObjectCategoryBits::NO_CATEGORY)

class MetadataForwardTest : public IntroduceInterfaces<META_NS::MetaObject, IMetadataTest, ISerializable> {
    META_OBJECT(MetadataForwardTest, META_NS::UTest::ClassId::MetadataForwardTest, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_FORWARDED_PROPERTY_DATA(IMetadataTest, int, Value)
    META_STATIC_FORWARDED_PROPERTY_DATA(IMetadataTest, int, ReadValue)
    META_STATIC_FORWARDED_EVENT_DATA(IMetadataTest, IOnChanged, OnChanged)
    META_STATIC_FUNCTION_DATA(IMetadataTest, Func)
    META_END_STATIC_DATA()

    META_FORWARD_PROPERTY(int, Value, object_->Value())
    META_FORWARD_READONLY_PROPERTY(int, ReadValue, object_->ReadValue())
    META_FORWARD_EVENT(IOnChanged, OnChanged, object_->EventOnChanged)

    void Func() override {}

    bool Build(const IMetadata::Ptr& d) override
    {
        if (Super::Build(d)) {
            object_ = GetObjectRegistry().Create<IMetadataTest>(ClassId::MetadataTest);
        }
        return object_ != nullptr;
    }

    BASE_NS::vector<MData> GetDatas() const override
    {
        return {};
    }

    ReturnError Export(IExportContext& c) const override
    {
        return Serializer(c) & AutoSerialize() & NamedValue("object", interface_pointer_cast<IObject>(object_));
    }
    ReturnError Import(IImportContext& c) override
    {
        IObject::Ptr p = interface_pointer_cast<IObject>(object_);
        return Serializer(c) & AutoSerialize() & NamedValue("object", p);
    }

private:
    IMetadataTest::Ptr object_;
};

/**
 * @tc.name: StaticDataWithForwarding
 * @tc.desc: Tests for Static Data With Forwarding. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MetadataTest, StaticDataWithForwarding, testing::ext::TestSize.Level1)
{
    GetObjectRegistry().RegisterObjectType<MetadataTest>();
    TestMetadata<MetadataForwardTest>(ClassId::MetadataForwardTest);
    GetObjectRegistry().UnregisterObjectType<MetadataTest>();
}

/**
 * @tc.name: StaticDataWithForwardingSerialise
 * @tc.desc: Tests for Static Data With Forwarding Serialise. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MetadataTest, StaticDataWithForwardingSerialise, testing::ext::TestSize.Level1)
{
    auto& r = GetObjectRegistry();
    r.RegisterObjectType<MetadataForwardTest>();
    r.RegisterObjectType<MetadataTest>();

    TestSerialiser ser;
    {
        auto obj = r.Create<IMetadataTest>(ClassId::MetadataForwardTest);
        obj->Value()->SetValue(1);
        ASSERT_TRUE(ser.Export(obj));
    }
    ser.Dump("app://forwarded_static.json");

    auto meta = ser.Import<IMetadata>();
    ASSERT_TRUE(meta);
    auto obj = interface_cast<IMetadataTest>(meta);

    EXPECT_EQ(obj->Value()->GetValue(), 1);
    EXPECT_EQ(obj->Value().GetProperty(), meta->GetProperty("Value"));

    r.UnregisterObjectType<MetadataTest>();
    r.UnregisterObjectType<MetadataForwardTest>();
}

} // namespace UTest
META_END_NAMESPACE()
