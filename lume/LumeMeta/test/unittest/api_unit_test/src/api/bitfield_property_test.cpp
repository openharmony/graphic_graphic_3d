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

#include <meta/api/util.h>
#include <meta/base/bit_field.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_registry.h>

#include "test_framework.h"

enum TestFlagBits : uint32_t {
    FLAG1 = 1,
    FLAG2 = 2,
    FLAG4 = 4,
};

using TestFlags = META_NS::EnumBitField<TestFlagBits, uint64_t>;
META_TYPE(TestFlags)

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ITestFlagsInterface, "eba09b49-6efe-4b97-b809-981569a935e7")

namespace UTest {

class ITestFlagsInterface : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestFlagsInterface)
public:
    META_PROPERTY(TestFlags, Flags)
};

META_REGISTER_CLASS(
    BitfieldPropertyObject, "e62b0ab3-b07d-4423-b2bc-d4a522db8ded", META_NS::ObjectCategoryBits::NO_CATEGORY)

class BitfieldPropertyObject final : public IntroduceInterfaces<META_NS::ObjectFwd, ITestFlagsInterface> {
    META_OBJECT(BitfieldPropertyObject, ClassId::BitfieldPropertyObject, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(ITestFlagsInterface, TestFlags, Flags, TestFlagBits::FLAG2)
    META_END_STATIC_DATA()
    META_IMPLEMENT_PROPERTY(TestFlags, Flags)
};
/*
template<class T>
static void RegisterUIntSerialization()
{
    using namespace Serialization;
    auto& imp = GetObjectRegistry().GetImportContext();
    auto& exp = GetObjectRegistry().GetExportContext();

    exp.RegisterValueExporter(CreateValueSerialiser<T>(
        [](IExportContext& c, const T& v) { return c.ExportUnsignedInteger(static_cast<uint64_t>(v)); }));

    imp.RegisterValueImporter(CreateValueDeserialiser<T>([](IImportContext& c, const Primitive& p, T& v) {
        if (c.CanImportUnsignedInteger(p)) {
            v = static_cast<T>(c.ImportUnsignedInteger(p));
            return true;
        }
        return false;
    }));
}
*/
class API_BitfieldPropertyTest : public ::testing::Test {
public:
    static void SetUpTestSuite()
    {
        // RegisterPropertyType<TestFlags>();
        RegisterObjectType<BitfieldPropertyObject>();
        // RegisterUIntSerialization<TestFlags>();
    }
    static void TearDownTestSuite()
    {
        // GetObjectRegistry().GetExportContext().UnregisterValueExporter(GetPropertyUid(TestFlags));
        // GetObjectRegistry().GetImportContext().UnregisterValueImporter(GetPropertyUid(TestFlags));
        UnregisterObjectType<BitfieldPropertyObject>();
        // UnRegisterPropertyType<TestFlags>();
    }

    void SetUp() override
    {
        object_ = GetObjectRegistry().Create<ITestFlagsInterface>(ClassId::BitfieldPropertyObject);
        ASSERT_NE(object_, nullptr);
    }
    void TearDown() override
    {
        object_.reset();
    }

protected:
    ITestFlagsInterface::Ptr object_;
};

MATCHER_P(ExactBitsSet, n, "")
{
    return arg == n;
}

/**
 * @tc.name: GetValue
 * @tc.desc: Tests for Get Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BitfieldPropertyTest, GetValue, testing::ext::TestSize.Level1)
{
    EXPECT_EQ(GetValue(object_->Flags()), TestFlagBits::FLAG2);
}

/**
 * @tc.name: SetValue
 * @tc.desc: Tests for Set Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BitfieldPropertyTest, SetValue, testing::ext::TestSize.Level1)
{
    SetValue(object_->Flags(), TestFlagBits::FLAG1 | TestFlagBits::FLAG4);
    EXPECT_EQ(GetValue(object_->Flags()), TestFlagBits::FLAG1 | TestFlagBits::FLAG4);
}

/**
 * @tc.name: SetValueOp
 * @tc.desc: Tests for Set Value Op. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BitfieldPropertyTest, SetValueOp, testing::ext::TestSize.Level1)
{
    TestFlags value1 = TestFlagBits::FLAG2;
    SetValue(object_->Flags(), value1 | TestFlagBits::FLAG1 | TestFlagBits::FLAG4);
    EXPECT_EQ(GetValue(object_->Flags()), TestFlagBits::FLAG1 | TestFlagBits::FLAG2 | TestFlagBits::FLAG4);
}

/**
 * @tc.name: Serialisation
 * @tc.desc: Tests for Serialisation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST_F(API_BitfieldPropertyTest, Serialisation, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    object_->Flags()->SetValue(TestFlagBits::FLAG4);
    ASSERT_TRUE(ser.Export(object_));
    ser.Dump("app://bitfield.json");
    auto obj = ser.Import<ITestFlagsInterface>();
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->Flags()->GetValue(), TestFlagBits::FLAG4);
}
*/
UNIT_TEST(API_BitfieldTest, Or, testing::ext::TestSize.Level1)
{
    EnumBitField<TestFlagBits> value1(TestFlagBits::FLAG1);
    EnumBitField<TestFlagBits> value2(TestFlagBits::FLAG2 | TestFlagBits::FLAG4);
    EXPECT_THAT(
        (value1 | value2).GetValue(), ExactBitsSet(TestFlagBits::FLAG1 | TestFlagBits::FLAG2 | TestFlagBits::FLAG4));
}

/**
 * @tc.name: And
 * @tc.desc: Tests for And. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_BitfieldTest, And, testing::ext::TestSize.Level1)
{
    EnumBitField<TestFlagBits> value1(TestFlagBits::FLAG1);
    EnumBitField<TestFlagBits> value2(TestFlagBits::FLAG1 | TestFlagBits::FLAG2 | TestFlagBits::FLAG4);
    EXPECT_THAT((value1 & value2).GetValue(), ExactBitsSet(TestFlagBits::FLAG1));
}

/**
 * @tc.name: Xor
 * @tc.desc: Tests for Xor. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_BitfieldTest, Xor, testing::ext::TestSize.Level1)
{
    EnumBitField<TestFlagBits> value1(TestFlagBits::FLAG1);
    EnumBitField<TestFlagBits> value2(TestFlagBits::FLAG1 | TestFlagBits::FLAG2 | TestFlagBits::FLAG4);
    EXPECT_THAT((value1 ^ value2).GetValue(), ExactBitsSet(TestFlagBits::FLAG2 | TestFlagBits::FLAG4));
}

/**
 * @tc.name: Not
 * @tc.desc: Tests for Not. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_BitfieldTest, Not, testing::ext::TestSize.Level1)
{
    EnumBitField<TestFlagBits> value1(TestFlagBits::FLAG1);
    EXPECT_THAT((~value1).GetValue(), ExactBitsSet(~TestFlagBits::FLAG1));
}

/**
 * @tc.name: SetAndClear
 * @tc.desc: Tests for Set And Clear. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_BitfieldTest, SetAndClear, testing::ext::TestSize.Level1)
{
    EnumBitField<TestFlagBits> value(TestFlagBits::FLAG1);
    EXPECT_TRUE(value.IsSet(TestFlagBits::FLAG1));
    EXPECT_FALSE(value.IsSet(TestFlagBits::FLAG2));
    value.Set(TestFlagBits::FLAG2);
    EXPECT_TRUE(value.IsSet(TestFlagBits::FLAG2));
    value.Clear(TestFlagBits::FLAG1);
    EXPECT_FALSE(value.IsSet(TestFlagBits::FLAG1));
    EXPECT_TRUE(value.IsSet(TestFlagBits::FLAG2));
}

enum ExtTestFlagBits : uint32_t {
    FLAG1 = 1,
    FLAG2 = 2,
    FLAG4 = 4,
};

using ExtTestFlags = META_NS::SubEnumBitField<ExtTestFlagBits, TestFlags, 16, 16>;

/**
 * @tc.name: Value
 * @tc.desc: Tests for Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SubBitfieldTest, Value, testing::ext::TestSize.Level1)
{
    TestFlags base(TestFlagBits::FLAG1);
    ExtTestFlags ext(ExtTestFlagBits::FLAG2);

    TestFlags res = base | ext;
    EXPECT_THAT(res.GetValue(), ExactBitsSet(TestFlagBits::FLAG1 | (ExtTestFlagBits::FLAG2 << 16)));

    ExtTestFlags sub(res);
    EXPECT_EQ(ext, sub);
    EXPECT_THAT(sub.GetValue(), ExactBitsSet(ExtTestFlagBits::FLAG2 << 16));
    EXPECT_THAT((ExtTestFlagBits)sub, ExactBitsSet(ExtTestFlagBits::FLAG2));
}

/**
 * @tc.name: Or
 * @tc.desc: Tests for Or. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SubBitfieldTest, Or, testing::ext::TestSize.Level1)
{
    TestFlags base(TestFlagBits::FLAG1);
    ExtTestFlags ext(ExtTestFlagBits::FLAG2);
    EXPECT_THAT(base | ext, ExactBitsSet(TestFlagBits::FLAG1 | (ExtTestFlagBits::FLAG2 << 16)));
}

/**
 * @tc.name: And
 * @tc.desc: Tests for And. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SubBitfieldTest, And, testing::ext::TestSize.Level1)
{
    TestFlags base(TestFlagBits::FLAG1);
    ExtTestFlags ext(ExtTestFlagBits::FLAG2);
    TestFlags res = base | ext;
    EXPECT_THAT(res & ext, ExactBitsSet(ExtTestFlagBits::FLAG2 << 16));
    EXPECT_THAT(res & base, ExactBitsSet(TestFlagBits::FLAG1));
}

/**
 * @tc.name: Xor
 * @tc.desc: Tests for Xor. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SubBitfieldTest, Xor, testing::ext::TestSize.Level1)
{
    TestFlags base(TestFlagBits::FLAG1);
    ExtTestFlags ext(ExtTestFlagBits::FLAG2);
    TestFlags res = base | ext;
    EXPECT_THAT(res ^ ext, ExactBitsSet(TestFlagBits::FLAG1));
    EXPECT_THAT(res ^ base, ExactBitsSet(ExtTestFlagBits::FLAG2 << 16));
}

/**
 * @tc.name: Not
 * @tc.desc: Tests for Not. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SubBitfieldTest, Not, testing::ext::TestSize.Level1)
{
    TestFlags base(TestFlagBits::FLAG1);
    ExtTestFlags ext(ExtTestFlagBits::FLAG2);
    TestFlags res = base | ext;
    EXPECT_THAT(res & ~ext, ExactBitsSet(TestFlagBits::FLAG1));
    EXPECT_THAT(res & ~base, ExactBitsSet(ExtTestFlagBits::FLAG2 << 16));
}

/**
 * @tc.name: SetAndClear
 * @tc.desc: Tests for Set And Clear. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SubBitfieldTest, SetAndClear, testing::ext::TestSize.Level1)
{
    TestFlags base(TestFlagBits::FLAG1);
    ExtTestFlags ext(ExtTestFlagBits::FLAG2);

    EXPECT_FALSE(ext.IsSet(ExtTestFlagBits::FLAG1));
    EXPECT_TRUE(ext.IsSet(ExtTestFlagBits::FLAG2));
    ext.Set(ExtTestFlagBits::FLAG1);
    EXPECT_TRUE(ext.IsSet(ExtTestFlagBits::FLAG1));
    ext.Clear(ExtTestFlagBits::FLAG2);
    EXPECT_FALSE(ext.IsSet(ExtTestFlagBits::FLAG2));
    EXPECT_TRUE(ext.IsSet(ExtTestFlagBits::FLAG1));

    TestFlags value = base | ext;
    EXPECT_TRUE(value.IsSet(TestFlagBits::FLAG1));
    EXPECT_FALSE(value.IsSet(TestFlagBits::FLAG2));

    value.Clear(TestFlagBits::FLAG1);
    ExtTestFlags ext2 = value;
    EXPECT_FALSE(ext2.IsSet(ExtTestFlagBits::FLAG2));
    EXPECT_TRUE(ext2.IsSet(ExtTestFlagBits::FLAG1));
}

/**
 * @tc.name: SetSubBits
 * @tc.desc: Tests for Set Sub Bits. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SubBitfieldTest, SetSubBits, testing::ext::TestSize.Level1)
{
    TestFlags base(TestFlagBits::FLAG1);
    ExtTestFlags ext(ExtTestFlagBits::FLAG2);

    base.SetSubBits(ext);

    EXPECT_TRUE(base.IsSet(TestFlagBits::FLAG1));
    EXPECT_FALSE(base.IsSet(TestFlagBits::FLAG2));

    ExtTestFlags ext2 = base;
    EXPECT_TRUE(ext2.IsSet(ExtTestFlagBits::FLAG2));
    EXPECT_FALSE(ext2.IsSet(ExtTestFlagBits::FLAG1));
}

} // namespace UTest

META_END_NAMESPACE()
