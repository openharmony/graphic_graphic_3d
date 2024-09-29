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

#include <meta/interface/detail/any.h>
#include <meta/interface/intf_object_registry.h>

#include "src/test_runner.h"
#include "src/testing_objects.h"
#include "src/util.h"

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

class IntfAnyTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        ResetTest();
    }
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: IntfAnyTest
 * @tc.desc: test BasicCompatibility001 function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfAnyTest, BasicCompatibility001, TestSize.Level1)
{
    Any<uint32_t> any { 1 };
    TypeId uid = UidFromType<uint32_t>();
    EXPECT_THAT(any.GetCompatibleTypes(CompatibilityDirection::BOTH), UnorderedElementsAre(uid));
    EXPECT_TRUE(IsCompatible(any, uid));
    EXPECT_TRUE(IsCompatibleWith<uint32_t>(any));
    EXPECT_TRUE(IsSetCompatible(any, uid));
    EXPECT_TRUE(IsSetCompatibleWith<uint32_t>(any));
    EXPECT_TRUE(IsGetCompatible(any, uid));
    EXPECT_TRUE(IsGetCompatibleWith<uint32_t>(any));
    EXPECT_EQ(any.GetTypeId(), uid);
}

/**
 * @tc.name: IntfAnyTest
 * @tc.desc: test SharedPointerCompatibility function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfAnyTest, SharedPointerCompatibility, TestSize.Level1)
{
    Any<IObject::Ptr> any { CreateTestType<IObject>() };
    TypeId uid = UidFromType<IObject::Ptr>();
    EXPECT_THAT(any.GetCompatibleTypes(CompatibilityDirection::BOTH), UnorderedElementsAre(uid, SharedPtrIInterfaceId));
    EXPECT_THAT(any.GetCompatibleTypes(CompatibilityDirection::SET), UnorderedElementsAre(uid, SharedPtrIInterfaceId));
    EXPECT_THAT(any.GetCompatibleTypes(CompatibilityDirection::GET),
        UnorderedElementsAre(uid, SharedPtrIInterfaceId, SharedPtrConstIInterfaceId));
    EXPECT_TRUE(IsCompatible(any, uid));
    EXPECT_TRUE(IsCompatibleWith<IObject::Ptr>(any));
    EXPECT_TRUE(IsSetCompatible(any, uid));
    EXPECT_TRUE(IsSetCompatibleWith<IObject::Ptr>(any));
    EXPECT_TRUE(IsSetCompatibleWith<SharedPtrIInterface>(any));
    EXPECT_TRUE(IsGetCompatible(any, uid));
    EXPECT_TRUE(IsGetCompatibleWith<IObject::Ptr>(any));
    EXPECT_TRUE(IsGetCompatibleWith<SharedPtrIInterface>(any));
    EXPECT_TRUE(IsGetCompatibleWith<SharedPtrConstIInterface>(any));
    EXPECT_EQ(any.GetTypeId(), uid);

    Any<IObject::ConstPtr> constAny { CreateTestType<IObject>() };
    EXPECT_FALSE(IsGetCompatibleWith<SharedPtrIInterface>(constAny));
    EXPECT_TRUE(IsGetCompatibleWith<SharedPtrConstIInterface>(constAny));
}

/**
 * @tc.name: IntfAnyTest
 * @tc.desc: test WeakPointerCompatibility function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfAnyTest, WeakPointerCompatibility, TestSize.Level1)
{
    IObject::Ptr p = CreateTestType<IObject>();
    Any<IObject::WeakPtr> any { p };
    TypeId uid = UidFromType<IObject::WeakPtr>();
    EXPECT_THAT(any.GetCompatibleTypes(CompatibilityDirection::BOTH),
        UnorderedElementsAre(uid, SharedPtrIInterfaceId, WeakPtrIInterfaceId));
    EXPECT_THAT(any.GetCompatibleTypes(CompatibilityDirection::SET),
        UnorderedElementsAre(uid, SharedPtrIInterfaceId, WeakPtrIInterfaceId));
    EXPECT_THAT(any.GetCompatibleTypes(CompatibilityDirection::GET),
        UnorderedElementsAre(
            uid, SharedPtrIInterfaceId, SharedPtrConstIInterfaceId, WeakPtrIInterfaceId, WeakPtrConstIInterfaceId));
    EXPECT_TRUE(IsCompatible(any, uid));
    EXPECT_TRUE(IsCompatibleWith<IObject::WeakPtr>(any));
    EXPECT_TRUE(IsSetCompatible(any, uid));
    EXPECT_TRUE(IsSetCompatibleWith<IObject::WeakPtr>(any));
    EXPECT_TRUE(IsSetCompatibleWith<SharedPtrIInterface>(any));
    EXPECT_TRUE(IsSetCompatibleWith<WeakPtrIInterface>(any));
    EXPECT_TRUE(IsGetCompatible(any, uid));
    EXPECT_TRUE(IsGetCompatibleWith<IObject::WeakPtr>(any));
    EXPECT_TRUE(IsGetCompatibleWith<SharedPtrIInterface>(any));
    EXPECT_TRUE(IsGetCompatibleWith<SharedPtrConstIInterface>(any));
    EXPECT_TRUE(IsGetCompatibleWith<WeakPtrIInterface>(any));
    EXPECT_TRUE(IsGetCompatibleWith<WeakPtrConstIInterface>(any));
    EXPECT_EQ(any.GetTypeId(), uid);
}

/**
 * @tc.name: IntfAnyTest
 * @tc.desc: test GetSet001 function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfAnyTest, GetSet001, TestSize.Level1)
{
    Any<uint32_t> any { 1 };
    uint32_t v = 0;
    EXPECT_TRUE(any.GetValue(v));
    EXPECT_EQ(v, 1);
    EXPECT_EQ(GetValue<uint32_t>(any), 1);

    EXPECT_TRUE(any.SetValue((uint32_t(2))));
    EXPECT_EQ(GetValue<uint32_t>(any), 2);

    EXPECT_FALSE(any.SetValue(float(3.0f)));
    EXPECT_EQ(GetValue<float>(any), 0);

    Any<uint32_t> other;
    EXPECT_TRUE(other.CopyFrom(any));
    EXPECT_EQ(GetValue<uint32_t>(other), 2);

    auto c1 = other.Clone(false);
    ASSERT_TRUE(c1);
    EXPECT_EQ(GetValue<uint32_t>(*c1), 0);
    auto c2 = other.Clone(true);
    ASSERT_TRUE(c2);
    EXPECT_EQ(GetValue<uint32_t>(*c2), 2);

    Any<float> otherType;
    EXPECT_FALSE(otherType.CopyFrom(any));
}

/**
 * @tc.name: IntfAnyTest
 * @tc.desc: test GetSetSharedPointer function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfAnyTest, GetSetSharedPointer, TestSize.Level1)
{
    auto p = CreateTestType<IObject>();
    auto p2 = CreateTestType<IObject>();
    {
        Any<IObject::Ptr> any { p };
        IObject::Ptr v;
        EXPECT_TRUE(any.GetValue(v));
        EXPECT_EQ(v, p);
        EXPECT_EQ(GetValue<IObject::Ptr>(any), p);
        EXPECT_TRUE(any.SetValue(interface_pointer_cast<CORE_NS::IInterface>(p2)));
        EXPECT_EQ(GetValue<IObject::Ptr>(any), p2);
    }
    {
        Any<IObject::Ptr> any { p };
        SharedPtrIInterface v;
        EXPECT_TRUE(any.GetValue(v));
        EXPECT_EQ(interface_pointer_cast<IObject>(v), p);
        EXPECT_EQ(interface_pointer_cast<IObject>(GetValue<SharedPtrIInterface>(any)), p);
        EXPECT_EQ(interface_pointer_cast<IObject>(GetValue<SharedPtrConstIInterface>(any)), p);
        EXPECT_TRUE(any.SetValue(interface_pointer_cast<CORE_NS::IInterface>(p2)));
        EXPECT_EQ(interface_pointer_cast<IObject>(GetValue<SharedPtrConstIInterface>(any)), p2);
    }
    {
        auto p = CreateTestType<IObject>();
        Any<IObject::ConstPtr> constAny { p };
        SharedPtrConstIInterface v;
        EXPECT_TRUE(constAny.GetValue(v));
        EXPECT_EQ(interface_pointer_cast<IObject>(v), p);
        EXPECT_NE(interface_pointer_cast<IObject>(GetValue<SharedPtrIInterface>(constAny)), p);
        EXPECT_EQ(interface_pointer_cast<IObject>(GetValue<SharedPtrConstIInterface>(constAny)), p);
        EXPECT_TRUE(constAny.SetValue(interface_pointer_cast<CORE_NS::IInterface>(p2)));
        EXPECT_EQ(interface_pointer_cast<IObject>(GetValue<SharedPtrConstIInterface>(constAny)), p2);
        EXPECT_TRUE(constAny.SetValue(interface_pointer_cast<const CORE_NS::IInterface>(p)));
        EXPECT_EQ(interface_pointer_cast<IObject>(GetValue<SharedPtrConstIInterface>(constAny)), p);
    }
}

/**
 * @tc.name: IntfAnyTest
 * @tc.desc: test GetSetWeakPointer function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfAnyTest, GetSetWeakPointer, TestSize.Level1)
{
    auto p = CreateTestType<IObject>();
    auto p2 = CreateTestType<IObject>();
    WeakPtrIInterface weak = interface_pointer_cast<CORE_NS::IInterface>(p2);

    {
        Any<IObject::WeakPtr> any { p };
        IObject::WeakPtr v;
        EXPECT_TRUE(any.GetValue(v));
        EXPECT_EQ(v.lock(), p);
        EXPECT_EQ(GetValue<IObject::WeakPtr>(any).lock(), p);
        EXPECT_EQ(interface_pointer_cast<IObject>(GetValue<WeakPtrIInterface>(any)), p);
        EXPECT_EQ(interface_pointer_cast<IObject>(GetValue<WeakPtrConstIInterface>(any)), p);
        EXPECT_TRUE(any.SetValue(weak));
        EXPECT_EQ(GetValue<IObject::WeakPtr>(any).lock(), p2);
    }
    {
        Any<IObject::WeakPtr> any { p };
        SharedPtrIInterface v;
        EXPECT_TRUE(any.GetValue(v));
        EXPECT_EQ(interface_pointer_cast<IObject>(v), p);
        EXPECT_EQ(interface_pointer_cast<IObject>(GetValue<SharedPtrIInterface>(any)), p);
    }
}

/**
 * @tc.name: IntfAnyTest
 * @tc.desc: test PtrCopyFrom function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfAnyTest, PtrCopyFrom, TestSize.Level1)
{
    auto p1 = CreateTestType<IObject>();
    auto p2 = CreateTestType<IObject>();
    {
        Any<IObject::Ptr> any { p1 };
        Any<IObject::Ptr> source { p2 };
        EXPECT_TRUE(any.CopyFrom(source));
        EXPECT_EQ(GetValue<IObject::Ptr>(any), p2);
    }
    {
        Any<IObject::Ptr> any { p1 };
        Any<SharedPtrIInterface> source { interface_pointer_cast<CORE_NS::IInterface>(p2) };
        EXPECT_TRUE(any.CopyFrom(source));
        EXPECT_EQ(GetValue<IObject::Ptr>(any), p2);
    }
    {
        Any<IObject::Ptr> any { p1 };
        Any<ITestType::Ptr> source { interface_pointer_cast<ITestType>(p2) };
        EXPECT_TRUE(any.CopyFrom(source));
        EXPECT_EQ(GetValue<IObject::Ptr>(any), p2);
    }
    {
        Any<IObject::ConstPtr> any { p1 };
        Any<IObject::Ptr> source { p2 };
        EXPECT_TRUE(any.CopyFrom(source));
        EXPECT_EQ(GetValue<IObject::ConstPtr>(any), p2);
    }
    {
        Any<IObject::ConstPtr> any { p1 };
        Any<SharedPtrIInterface> source { interface_pointer_cast<CORE_NS::IInterface>(p2) };
        EXPECT_TRUE(any.CopyFrom(source));
        EXPECT_EQ(GetValue<IObject::ConstPtr>(any), p2);
    }
    {
        Any<IObject::ConstPtr> any { p1 };
        Any<ITestType::Ptr> source { interface_pointer_cast<ITestType>(p2) };
        EXPECT_TRUE(any.CopyFrom(source));
        EXPECT_EQ(GetValue<IObject::ConstPtr>(any), p2);
    }
    {
        Any<IObject::ConstPtr> any { p1 };
        Any<IObject::ConstPtr> source { p2 };
        EXPECT_TRUE(any.CopyFrom(source));
        EXPECT_EQ(GetValue<IObject::ConstPtr>(any), p2);
    }
    {
        Any<IObject::ConstPtr> any { p1 };
        Any<SharedPtrConstIInterface> source { interface_pointer_cast<CORE_NS::IInterface>(p2) };
        EXPECT_TRUE(any.CopyFrom(source));
        EXPECT_EQ(GetValue<IObject::ConstPtr>(any), p2);
    }
    {
        Any<IObject::ConstPtr> any { p1 };
        Any<ITestType::ConstPtr> source { interface_pointer_cast<ITestType>(p2) };
        EXPECT_TRUE(any.CopyFrom(source));
        EXPECT_EQ(GetValue<IObject::ConstPtr>(any), p2);
    }
}

/**
 * @tc.name: IntfAnyTest
 * @tc.desc: test BasicCompatibility002 function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfAnyTest, BasicCompatibility002, TestSize.Level1)
{
    ArrayAny<uint32_t> any { 1, 2, 3 };
    using Type = uint32_t[];

    TypeId arrayUid = ArrayUidFromType<uint32_t>();
    TypeId vectorUid = UidFromType<BASE_NS::vector<uint32_t>>();

    EXPECT_THAT(any.GetCompatibleTypes(CompatibilityDirection::BOTH), UnorderedElementsAre(arrayUid, vectorUid));
    EXPECT_TRUE(IsCompatible(any, arrayUid));
    EXPECT_TRUE(IsCompatibleWith<Type>(any));
    EXPECT_TRUE(IsSetCompatible(any, arrayUid));
    EXPECT_TRUE(IsSetCompatibleWith<Type>(any));
    EXPECT_TRUE(IsGetCompatible(any, arrayUid));
    EXPECT_TRUE(IsGetCompatibleWith<Type>(any));
    EXPECT_EQ(any.GetTypeId(), arrayUid);
}

/**
 * @tc.name: IntfAnyTest
 * @tc.desc: test GetSetTypes function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfAnyTest, GetSetTypes, TestSize.Level1)
{
    static constexpr auto valueCount = 3;
    const uint32_t valuesArr[valueCount] = { 1, 2, 3 };
    const BASE_NS::vector<uint32_t> vectorArr = { 4, 5, 6 };

    auto arrayUid = UidFromType<uint32_t[]>();
    auto vectorUid = UidFromType<BASE_NS::vector<uint32_t>>();

    ArrayAny<uint32_t> any { valuesArr };

    uint32_t values[valueCount];
    auto valuesSize = sizeof(uint32_t) * valueCount;

    EXPECT_TRUE(any.GetData(arrayUid, values, valuesSize));
    EXPECT_THAT(values, ::ElementsAreArray(valuesArr));

    BASE_NS::vector<uint32_t> valuesVec;
    EXPECT_TRUE(any.GetData(vectorUid, &valuesVec, sizeof(valuesVec)));
    EXPECT_THAT(values, ::ElementsAreArray(valuesArr));

    EXPECT_TRUE(any.SetData(vectorUid, &vectorArr, sizeof(vectorArr)));
    EXPECT_TRUE(any.GetData(arrayUid, values, valuesSize));
    EXPECT_THAT(values, ::ElementsAreArray(vectorArr));

    EXPECT_TRUE(any.SetData(arrayUid, vectorArr.data(), sizeof(uint32_t) * vectorArr.size()));
    EXPECT_TRUE(any.GetData(arrayUid, values, valuesSize));
    EXPECT_THAT(values, ::ElementsAreArray(vectorArr));
}

/**
 * @tc.name: IntfAnyTest
 * @tc.desc: test GetSet002 function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfAnyTest, GetSet002, TestSize.Level1)
{
    using ArrayType = BASE_NS::vector<uint32_t>;

    BASE_NS::vector<uint32_t> vv { 1 };
    ArrayAny<uint32_t> any { vv };
    uint32_t v = 0;
    EXPECT_TRUE(any.GetValueAt(0, v));
    EXPECT_EQ(v, 1);
    EXPECT_EQ(GetValue<BASE_NS::vector<uint32_t>>(any), vv);

    EXPECT_FALSE(any.GetValue(1, v));
    EXPECT_FALSE(any.SetValue(1, v));
    EXPECT_EQ(GetValue<ArrayType>(any), vv);

    EXPECT_TRUE(any.SetValueAt(0, static_cast<uint32_t>(42)));
    EXPECT_TRUE(any.GetValueAt(0, v));
    EXPECT_EQ(v, 42);

    vv = { 1, 2 };
    EXPECT_TRUE(any.SetValue(vv));
    EXPECT_EQ(GetValue<ArrayType>(any), vv);

    EXPECT_FALSE(any.SetValue(3.0f));
    EXPECT_EQ(GetValue<ArrayType>(any), vv);

    ArrayAny<uint32_t> other;
    EXPECT_TRUE(other.CopyFrom(any));
    EXPECT_EQ(GetValue<ArrayType>(other), vv);

    auto c1 = interface_pointer_cast<IArrayAny>(other.Clone(false));
    ASSERT_TRUE(c1);
    EXPECT_EQ(c1->GetSize(), 0);
    auto c2 = interface_pointer_cast<IArrayAny>(other.Clone(true));
    ASSERT_TRUE(c2);
    EXPECT_EQ(GetValue<ArrayType>(*c2), vv);

    ArrayAny<float> otherType;
    EXPECT_FALSE(otherType.CopyFrom(any));
}

META_END_NAMESPACE()