/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include <meta/base/meta_types.h>
#include <meta/ext/any_builder.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object_fwd.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_ser_output.h>
#include <meta/interface/serialization/intf_serializable.h>

#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"
#include "helpers/util.h"

META_BEGIN_NAMESPACE()
namespace UTest {

META_REGISTER_CLASS(ExplicitSerType, "b142e2d3-8cfa-4028-ae8d-91f006ebd6fd", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(AutoSerType, "be2ae3df-66f4-4603-adeb-0ff7a751bb15", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(EnumValueSerType, "1111e3df-66f4-4603-adeb-0ff7a751bb15", ObjectCategoryBits::NO_CATEGORY)

class ISerType : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISerType, "bd602a50-25c3-4eb6-a07a-8dfa96f313eb")
public:
    virtual int32_t GetValue() const = 0;
    virtual void SetValue(int32_t) = 0;
    virtual BASE_NS::vector<BASE_NS::string> GetStrings() const = 0;
    virtual void SetStrings(BASE_NS::vector<BASE_NS::string>) = 0;

    META_PROPERTY(uint32_t, IntProp)
};

struct SerType : public IntroduceInterfaces<ObjectFwd, ISerType, ISerializable> {
    META_OBJECT_NO_CLASSINFO(SerType, IntroduceInterfaces)

    int32_t GetValue() const override
    {
        return value;
    }
    void SetValue(int32_t i) override
    {
        value = i;
    }
    BASE_NS::vector<BASE_NS::string> GetStrings() const override
    {
        return strings;
    }
    void SetStrings(BASE_NS::vector<BASE_NS::string> s) override
    {
        strings = s;
    }

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(ISerType, uint32_t, IntProp)
    META_END_STATIC_DATA()
    META_IMPLEMENT_PROPERTY(uint32_t, IntProp)

    int32_t value { 0 };
    BASE_NS::vector<BASE_NS::string> strings;
};

struct ExplicitSerType : public SerType {
    META_OBJECT(ExplicitSerType, ClassId::ExplicitSerType, SerType, META_NS::ClassId::Object)
    ReturnError Export(IExportContext& c) const override
    {
        return Serializer(c) & NamedValue("value", value) & NamedValue("strings", strings) &
               NamedValue("IntProp", META_ACCESS_PROPERTY(IntProp));
    }
    ReturnError Import(IImportContext& c) override
    {
        return Serializer(c) & NamedValue("value", value) & NamedValue("strings", strings) &
               NamedValue("IntProp", META_ACCESS_PROPERTY(IntProp));
    }
};

struct AutoSerType : public SerType {
    META_OBJECT(AutoSerType, ClassId::AutoSerType, SerType, META_NS::ClassId::Object)
    ReturnError Export(IExportContext& c) const override
    {
        return Serializer(c) & AutoSerialize() & NamedValue("value", value) & NamedValue("strings", strings);
    }
    ReturnError Import(IImportContext& c) override
    {
        return Serializer(c) & AutoSerialize() & NamedValue("value", value) & NamedValue("strings", strings);
    }
};

enum class SetTypeTestEnum { VALUE_A = 1, VALUE_B = 2 };

struct EnumValueSerType : public SerType {
    META_OBJECT(EnumValueSerType, ClassId::EnumValueSerType, SerType, META_NS::ClassId::Object)
    ReturnError Export(IExportContext& c) const override
    {
        return Serializer(c) & NamedValue("enum", enum_);
    }
    ReturnError Import(IImportContext& c) override
    {
        return Serializer(c) & NamedValue("enum", enum_);
    }

    int32_t GetValue() const override
    {
        return static_cast<int32_t>(enum_);
    }
    void SetValue(int32_t i) override
    {
        enum_ = static_cast<SetTypeTestEnum>(i);
    }

    SetTypeTestEnum enum_;
};

/**
 * @tc.name: EnumValueSer
 * @tc.desc: Tests for Enum Value Ser. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, EnumValueSer, testing::ext::TestSize.Level1)
{
    auto& reg = GetObjectRegistry();
    reg.RegisterObjectType<EnumValueSerType>();

    TestSerialiser ser;
    {
        auto obj = reg.Create<ISerType>(ClassId::EnumValueSerType);
        ASSERT_TRUE(obj);
        obj->SetValue(2);

        ASSERT_TRUE(ser.Export(obj));
        ser.Dump("app://test.json");
    }
    auto obj = ser.Import<ISerType>();
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->GetValue(), 2);

    reg.UnregisterObjectType<EnumValueSerType>();
}

/**
 * @tc.name: JsonExportExplicitSer
 * @tc.desc: Tests for Json Export Explicit Ser. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, JsonExportExplicitSer, testing::ext::TestSize.Level1)
{
    auto& reg = GetObjectRegistry();
    reg.RegisterObjectType<ExplicitSerType>();

    TestSerialiser ser;
    {
        auto obj = reg.Create<ISerType>(ClassId::ExplicitSerType);
        ASSERT_TRUE(obj);
        obj->SetValue(2);
        obj->SetStrings({ "hips", "hops" });
        obj->IntProp()->SetDefaultValue(2);
        obj->IntProp()->SetValue(3);

        ASSERT_TRUE(ser.Export(obj));
        ser.Dump("app://test.json");
    }
    auto obj = ser.Import<ISerType>();
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->GetValue(), 2);
    EXPECT_EQ(obj->GetStrings(), (BASE_NS::vector<BASE_NS::string> { "hips", "hops" }));
    EXPECT_EQ(obj->IntProp()->GetDefaultValue(), 2);
    EXPECT_EQ(obj->IntProp()->GetValue(), 3);

    reg.UnregisterObjectType<ExplicitSerType>();
}

/**
 * @tc.name: JsonExportAutoSer
 * @tc.desc: Tests for Json Export Auto Ser. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, JsonExportAutoSer, testing::ext::TestSize.Level1)
{
    auto& reg = GetObjectRegistry();
    reg.RegisterObjectType<AutoSerType>();

    TestSerialiser ser;
    {
        auto obj = reg.Create<ISerType>(ClassId::AutoSerType);
        ASSERT_TRUE(obj);
        obj->SetValue(2);
        obj->SetStrings({ "hips", "hops" });
        obj->IntProp()->SetDefaultValue(2);
        obj->IntProp()->SetValue(3);

        ASSERT_TRUE(ser.Export(obj));
        ser.Dump("app://test.json");
    }
    auto obj = ser.Import<ISerType>();
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->GetValue(), 2);
    EXPECT_EQ(obj->GetStrings(), (BASE_NS::vector<BASE_NS::string> { "hips", "hops" }));
    EXPECT_EQ(obj->IntProp()->GetDefaultValue(), 2);
    EXPECT_EQ(obj->IntProp()->GetValue(), 3);

    reg.UnregisterObjectType<AutoSerType>();
}

} // namespace UTest
META_END_NAMESPACE()
