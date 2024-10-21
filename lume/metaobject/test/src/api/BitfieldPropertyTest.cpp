/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/api/util.h>
#include <meta/base/bit_field.h>
#include <meta/ext/object.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_registry.h>

#include "src/test_runner.h"

namespace {
    const int TESTVALUE = 16;
}

enum TestFlagBits : uint32_t {
    FLAG1 = 1,
    FLAG2 = 2,
    FLAG4 = 4,
};

using TestFlags = META_NS::EnumBitField<TestFlagBits, uint64_t>;
META_TYPE(TestFlags)

META_BEGIN_NAMESPACE()

using namespace testing::ext;

META_REGISTER_INTERFACE(ITestFlagsInterface, "eba09b49-6efe-4b97-b809-981569a935e7")

class ITestFlagsInterface : public CORE_NS::IInterface {
    META_INTERFACE2(CORE_NS::IInterface, ITestFlagsInterface)
public:
    META_PROPERTY(TestFlags, Flags);
};

META_REGISTER_CLASS(
    BitfieldPropertyObject, "e62b0ab3-b07d-4423-b2bc-d4a522db8ded", META_NS::ObjectCategoryBits::NO_CATEGORY)

class BitfieldPropertyObject final
    : public META_NS::MetaObjectFwd<BitfieldPropertyObject, ClassId::BitfieldPropertyObject,
        META_NS::ClassId::MetaObject, ITestFlagsInterface> {
    META_IMPLEMENT_PROPERTY(TestFlags, Flags, TestFlagBits::FLAG2)
};


class BitfieldPropertyTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
        RegisterObjectType<BitfieldPropertyObject>();
    }
    static void TearDownTestSuite()
    {
        UnregisterObjectType<BitfieldPropertyObject>();
        ResetTest();
    }
    void SetUp() override
    {
        object_ = Meta::GetObjectRegistry().Create<ITestFlagsInterface> (ClassId::BitfieldPropertyObject);
    }
    void TearDown() override
    {
        object_.reset();
    }
protected:
    ITestFlagsInterface::Ptr object_;
};

MATCHER_P (ExactBitsSet, n, "")
{
    return arg == n;
}

/**
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test GetValue function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, GetValue, TestSize.Level1)
{
    EXPECT_EQ(GetValue(object_->Flags()), TestFlagBits::FLAG2);
}

/**
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test SetValue function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, SetValue, TestSize.Level1)
{
    SetValue(object_->Flags(), TestFlagBits::FLAG1 | TestFlagBits::FLAG4);
    EXPECT_EQ(GetValue(object_->Flags()), TestFlagBits::FLAG1 | TestFlagBits::FLAG4);
}

/**
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test SetValue function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, SetValueOp, TestSize.Level1)
{
    TestFlags value1 = TestFlagBits::FLAG2;
    SetValue(object_->Flags(), value1 | TestFlagBits::FLAG1 | TestFlagBits::FLAG4);
    EXPECT_EQ(GetValue(object_->Flags()), TestFlagBits::FLAG1 | TestFlagBits::FLAG2 | TestFlagBits::FLAG4);
}

/**
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test Or function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, Or, TestSize.Level1)
{
    EnumBitField<TestFlagBits> value1(TestFlagBits::FLAG1);
    EnumBitField<TestFlagBits> value2(TestFlagBits::FLAG2 | TestFlagBits::FLAG4);
    EXPECT_THAT(
        (value1 | value2).GetValue(), ExactBitsSet(TestFlagBits::FLAG1 | TestFlagBits::FLAG2 | TestFlagBits::FLAG4));
}

/**
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test And function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, And, TestSize.Level1)
{
    EnumBitField<TestFlagBits> value1(TestFlagBits::FLAG1);
    EnumBitField<TestFlagBits> value2(TestFlagBits::FLAG1 | TestFlagBits::FLAG2 | TestFlagBits::FLAG4);
    EXPECT_THAT((value1 & value2).GetValue(), ExactBitsSet(TestFlagBits::FLAG1));
}

/**
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test Xor function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, Xor, TestSize.Level1)
{
    EnumBitField<TestFlagBits> value1(TestFlagBits::FLAG1);
    EnumBitField<TestFlagBits> value2(TestFlagBits::FLAG1 | TestFlagBits::FLAG2 | TestFlagBits::FLAG4);
    EXPECT_THAT((value1 ^ value2).GetValue(), ExactBitsSet(TestFlagBits::FLAG2 | TestFlagBits::FLAG4));
}

/**
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test Not function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, Not, TestSize.Level1)
{
    EnumBitField<TestFlagBits> value1(TestFlagBits::FLAG1);
    EXPECT_THAT((~value1).GetValue(), ExactBitsSet(~TestFlagBits::FLAG1));
}

/**
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test SetAndClear function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, SetAndClear, TestSize.Level1)
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

using ExtTestFlags = META_NS::SubEnumBitField<ExtTestFlagBits, TestFlags, TESTVALUE, TESTVALUE>;

/**
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test Value function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, Value, TestSize.Level1)
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
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test Or1 function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, Or1, TestSize.Level1)
{
    TestFlags base(TestFlagBits::FLAG1);
    ExtTestFlags ext(ExtTestFlagBits::FLAG2);

    EXPECT_THAT(base | ext, ExactBitsSet(TestFlagBits::FLAG1 | (ExtTestFlagBits::FLAG2 << 16)));
}

/**
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test And1 function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, And1, TestSize.Level1)
{
    TestFlags base(TestFlagBits::FLAG1);
    ExtTestFlags ext(ExtTestFlagBits::FLAG2);
    TestFlags res = base | ext;
    EXPECT_THAT(res & ext, ExactBitsSet(ExtTestFlagBits::FLAG2 << 16));
    EXPECT_THAT(res & base, ExactBitsSet(TestFlagBits::FLAG1));
}

/**
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test Xor1 function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, Xor1, TestSize.Level1)
{
    TestFlags base(TestFlagBits::FLAG1);
    ExtTestFlags ext(ExtTestFlagBits::FLAG2);
    TestFlags res = base | ext;
    EXPECT_THAT(res ^ ext, ExactBitsSet(TestFlagBits::FLAG1));
    EXPECT_THAT(res ^ base, ExactBitsSet(ExtTestFlagBits::FLAG2 << 16));
}

/**
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test Not1 function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, Not1, TestSize.Level1)
{
    TestFlags base(TestFlagBits::FLAG1);
    ExtTestFlags ext(ExtTestFlagBits::FLAG2);
    TestFlags res = base | ext;
    EXPECT_THAT(res & ~ext, ExactBitsSet(TestFlagBits::FLAG1));
    EXPECT_THAT(res & ~base, ExactBitsSet(ExtTestFlagBits::FLAG2 << 16));
}

/**
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test SetAndClear1 function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, SetAndClear1, TestSize.Level1)
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
 * @tc.name: BitfieldPropertyTest
 * @tc.desc: test SetAndClear1 function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BitfieldPropertyTest, SetSubBits, TestSize.Level1)
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
META_END_NAMESPACE()