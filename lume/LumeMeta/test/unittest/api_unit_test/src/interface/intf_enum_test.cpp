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

#include <meta/api/object.h>
#include <meta/ext/any_builder.h>
#include <meta/interface/detail/enum.h>
#include <meta/interface/enum_macros.h>
#include <meta/interface/intf_object_registry.h>

#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"
#include "helpers/util.h"

META_BEGIN_NAMESPACE()
enum class MyEnum {
    MyA = 1,
    MyB = 2,
    MyC = 4,
};

META_BEGIN_ENUM(MyEnum, "Nice Enum", MyEnum::MyA)
META_ENUM_VALUE(MyEnum::MyA, "MyA", "My A enumerator")
META_ENUM_VALUE(MyEnum::MyB, "MyB", "My B enumerator")
META_ENUM_VALUE(MyEnum::MyC, "MyC", "My C enumerator")
META_END_ENUM()

META_ENUM_TYPE(MyEnum)

enum class MyBitFieldEnum : uint32_t {
    MyA = 1,
    MyB = 2,
    MyC = 4,
};

META_BEGIN_BITFIELD_ENUM(MyBitFieldEnum, "Nice Enum", MyBitFieldEnum(0))
META_ENUM_VALUE(MyBitFieldEnum::MyA, "MyA", "My A enumerator")
META_ENUM_VALUE(MyBitFieldEnum::MyB, "MyB", "My B enumerator")
META_ENUM_VALUE(MyBitFieldEnum::MyC, "MyC", "My C enumerator")
META_END_ENUM()

META_ENUM_TYPE(MyBitFieldEnum)

enum class PlainEnum : uint32_t { Hops = 1 };

META_TYPE(PlainEnum)

namespace UTest {

/**
 * @tc.name: SingleValue
 * @tc.desc: Tests for Single Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, SingleValue, testing::ext::TestSize.Level1)
{
    Enum<MyEnumMetadata> v;

    EXPECT_EQ(v.GetName(), "MyEnum");
    EXPECT_EQ(v.GetDescription(), "Nice Enum");
    EXPECT_EQ(v.GetEnumType(), EnumType::SINGLE_VALUE);
    auto members = v.GetValues();
    ASSERT_EQ(members.size(), 3);
    EXPECT_EQ(members[0].name, "MyA");
    EXPECT_EQ(members[0].desc, "My A enumerator");
    EXPECT_EQ(members[1].name, "MyB");
    EXPECT_EQ(members[1].desc, "My B enumerator");
    EXPECT_EQ(members[2].name, "MyC");
    EXPECT_EQ(members[2].desc, "My C enumerator");
    EXPECT_EQ(v.GetValueIndex(), 0);
    EXPECT_TRUE(v.IsValueSet(0));
    EXPECT_FALSE(v.IsValueSet(1));
    EXPECT_FALSE(v.IsValueSet(6));
    EXPECT_TRUE(v.SetValueIndex(1));
    EXPECT_EQ(v.GetValueIndex(), 1);
    EXPECT_TRUE(v.IsValueSet(1));
    EXPECT_TRUE(v.SetValueIndex(2));
    EXPECT_EQ(v.GetValueIndex(), 2);
    EXPECT_TRUE(v.IsValueSet(2));
    EXPECT_FALSE(v.SetValueIndex(3));
}

/**
 * @tc.name: BitField
 * @tc.desc: Tests for Bit Field. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, BitField, testing::ext::TestSize.Level1)
{
    Enum<MyBitFieldEnumMetadata> v;

    EXPECT_EQ(v.GetValueIndex(), -1);
    EXPECT_FALSE(v.IsValueSet(0));
    EXPECT_FALSE(v.IsValueSet(1));
    EXPECT_FALSE(v.IsValueSet(6));
    EXPECT_TRUE(v.SetValueIndex(1));
    EXPECT_EQ(v.GetValueIndex(), 1);
    EXPECT_TRUE(v.IsValueSet(1));
    EXPECT_TRUE(v.SetValueIndex(2));
    EXPECT_EQ(v.GetValueIndex(), 2);
    EXPECT_TRUE(v.IsValueSet(2));
    EXPECT_FALSE(v.SetValueIndex(3));

    EXPECT_TRUE(v.FlipValue(2, false));
    EXPECT_EQ(v.GetValueIndex(), -1);

    EXPECT_TRUE(v.FlipValue(0, true));
    EXPECT_EQ(v.GetValueIndex(), 0);
    EXPECT_TRUE(v.FlipValue(1, true));
    EXPECT_EQ(v.GetValueIndex(), -1);
    EXPECT_TRUE(v.IsValueSet(0));
    EXPECT_TRUE(v.IsValueSet(1));
    EXPECT_FALSE(v.IsValueSet(2));
}

/**
 * @tc.name: Compatibility
 * @tc.desc: Tests for Compatibility. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, Compatibility, testing::ext::TestSize.Level1)
{
    Enum<MyEnumMetadata> v1;
    EXPECT_TRUE(IsCompatible(v1, UidFromType<MyEnum>()));
    EXPECT_TRUE(IsCompatible(v1, UidFromType<int64_t>()));
    Enum<MyBitFieldEnumMetadata> v2;
    EXPECT_TRUE(IsCompatible(v2, UidFromType<MyBitFieldEnum>()));
    EXPECT_TRUE(IsCompatible(v2, UidFromType<uint64_t>()));
}

/**
 * @tc.name: GetSetData
 * @tc.desc: Tests for Get Set Data. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, GetSetData, testing::ext::TestSize.Level1)
{
    {
        Enum<MyEnumMetadata> v;
        EXPECT_TRUE(v.SetValue(MyEnum::MyB));
        EXPECT_EQ(GetValue<MyEnum>(v), MyEnum::MyB);
        EXPECT_EQ(GetValue<int64_t>(v), 2);
        EXPECT_TRUE(v.SetValue<int64_t>(4));
        EXPECT_EQ(GetValue<MyEnum>(v), MyEnum::MyC);
        EXPECT_FALSE(v.SetValue<int64_t>(5));
    }
    {
        Enum<MyBitFieldEnumMetadata> v;
        EXPECT_TRUE(v.SetValue(MyBitFieldEnum::MyB));
        EXPECT_EQ(GetValue<MyBitFieldEnum>(v), MyBitFieldEnum::MyB);
        EXPECT_EQ(GetValue<uint64_t>(v), 2);
        EXPECT_TRUE(v.SetValue<uint64_t>(4));
        EXPECT_EQ(GetValue<MyBitFieldEnum>(v), MyBitFieldEnum::MyC);
        EXPECT_TRUE(v.SetValue<uint64_t>(5));
        EXPECT_EQ(GetValue<uint64_t>(v), 5);
    }
}

/**
 * @tc.name: CopyFrom
 * @tc.desc: Tests for Copy From. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, CopyFrom, testing::ext::TestSize.Level1)
{
    Enum<MyEnumMetadata> v;
    auto p = v.Clone();
    EXPECT_EQ(GetValue<int64_t>(*p), 1);
    EXPECT_TRUE(v.SetValue(MyEnum::MyB));
    EXPECT_TRUE(p->CopyFrom(v));
    EXPECT_EQ(GetValue<int64_t>(*p), 2);

    Any<int64_t> any { 4 };
    EXPECT_TRUE(p->CopyFrom(any));
    EXPECT_EQ(GetValue<int64_t>(*p), 4);
    EXPECT_TRUE(any.SetValue<int64_t>(5));
    EXPECT_FALSE(p->CopyFrom(any));
    EXPECT_EQ(GetValue<int64_t>(*p), 4);

    EXPECT_TRUE(any.CopyFrom(*p));
    EXPECT_EQ(GetValue<int64_t>(any), 4);
}

/**
 * @tc.name: ArrayEnum
 * @tc.desc: Tests for Array Enum. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, ArrayEnum, testing::ext::TestSize.Level1)
{
    ArrayEnum<MyEnumMetadata> arr { MyEnum::MyA, MyEnum::MyB, MyEnum::MyC };

    auto any = arr.Clone({ CloneValueType::COPY_VALUE, TypeIdRole::ITEM });
    ASSERT_TRUE(any);
    EXPECT_TRUE(arr.GetAnyAt(0, *any));
    EXPECT_EQ(GetValue<int64_t>(*any), 1);
    EXPECT_EQ(GetValue<MyEnum>(*any), MyEnum::MyA);
}

/**
 * @tc.name: Serialise
 * @tc.desc: Tests for Serialise. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, Serialise, testing::ext::TestSize.Level1)
{
    RegisterUserAny<Enum<MyEnumMetadata>>();
    TestSerialiser ser;
    {
        Object obj(CreateInstance(ClassId::Object));
        Metadata(obj).AddProperty(ConstructProperty<MyEnum>("test"));
        ASSERT_TRUE(ser.Export(obj));
    }
    ser.Dump("app://enum.json");

    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto p = obj->GetProperty<MyEnum>("test");
    ASSERT_TRUE(p);

    EXPECT_EQ(p->GetValue(), MyEnum::MyA);

    UnregisterUserAny<Enum<MyEnumMetadata>>();
}

/**
 * @tc.name: SerialiseBitField
 * @tc.desc: Tests for Serialise Bit Field. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, SerialiseBitField, testing::ext::TestSize.Level1)
{
    RegisterUserAny<Enum<MyBitFieldEnumMetadata>>();
    TestSerialiser ser;
    {
        Object obj(CreateInstance(ClassId::Object));
        auto p = ConstructProperty<MyBitFieldEnum>("test");
        auto en = interface_pointer_cast<IEnum>(p->GetValueAny().Clone());
        ASSERT_TRUE(en);
        en->FlipValue(0, true);
        en->FlipValue(2, true);
        p->SetValueAny(*interface_cast<IAny>(en));
        Metadata(obj).AddProperty(p);
        ASSERT_TRUE(ser.Export(obj));
    }
    ser.Dump("app://enum_bitfield.json");

    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto p = obj->GetProperty<MyBitFieldEnum>("test");
    ASSERT_TRUE(p);

    EXPECT_EQ(p->GetValue(), MyBitFieldEnum(5));

    UnregisterUserAny<Enum<MyBitFieldEnumMetadata>>();
}

/**
 * @tc.name: PlainEnum
 * @tc.desc: Tests for Plain Enum. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EnumTest, PlainEnum, testing::ext::TestSize.Level1)
{
    auto p = ConstructProperty<PlainEnum>("test");
    ASSERT_TRUE(p);
    Property<uint64_t> prop { p.GetProperty() };
    ASSERT_TRUE(prop);
    prop->SetValue(1);
    EXPECT_EQ(prop->GetValue(), 1);
}

} // namespace UTest
META_END_NAMESPACE()
