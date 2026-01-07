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

#include <base/containers/unordered_map.h>

#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/engine/intf_engine_data.h>
#include <meta/interface/intf_object_registry.h>

#include "helpers/test_data_helpers.h"
#include "helpers/testing_objects.h"
#include "helpers/util.h"
#include "test_framework.h"

META_BEGIN_NAMESPACE()
namespace UTest {

/**
 * @tc.name: BasicCompatibility
 * @tc.desc: Tests for Basic Compatibility. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AnyTest, BasicCompatibility, testing::ext::TestSize.Level1)
{
    Any<uint32_t> any { 1 };
    TypeId uid = UidFromType<uint32_t>();
    EXPECT_THAT(any.GetCompatibleTypes(CompatibilityDirection::BOTH), testing::UnorderedElementsAre(uid));
    EXPECT_TRUE(IsCompatible(any, uid));
    EXPECT_TRUE(IsCompatibleWith<uint32_t>(any));
    EXPECT_TRUE(IsSetCompatible(any, uid));
    EXPECT_TRUE(IsSetCompatibleWith<uint32_t>(any));
    EXPECT_TRUE(IsGetCompatible(any, uid));
    EXPECT_TRUE(IsGetCompatibleWith<uint32_t>(any));
    EXPECT_EQ(any.GetTypeId(), uid);
}

/**
 * @tc.name: SharedPointerCompatibility
 * @tc.desc: Tests for Shared Pointer Compatibility. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AnyTest, SharedPointerCompatibility, testing::ext::TestSize.Level1)
{
    Any<IObject::Ptr> any { CreateTestType<IObject>() };
    TypeId uid = UidFromType<IObject::Ptr>();
    EXPECT_THAT(any.GetCompatibleTypes(CompatibilityDirection::BOTH),
        testing::UnorderedElementsAre(uid, SharedPtrIInterfaceId));
    EXPECT_THAT(
        any.GetCompatibleTypes(CompatibilityDirection::SET), testing::UnorderedElementsAre(uid, SharedPtrIInterfaceId));
    EXPECT_THAT(any.GetCompatibleTypes(CompatibilityDirection::GET),
        testing::UnorderedElementsAre(uid, SharedPtrIInterfaceId, SharedPtrConstIInterfaceId));
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
 * @tc.name: WeakPointerCompatibility
 * @tc.desc: Tests for Weak Pointer Compatibility. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AnyTest, WeakPointerCompatibility, testing::ext::TestSize.Level1)
{
    IObject::Ptr p = CreateTestType<IObject>();
    Any<IObject::WeakPtr> any { p };
    TypeId uid = UidFromType<IObject::WeakPtr>();
    EXPECT_THAT(any.GetCompatibleTypes(CompatibilityDirection::BOTH),
        testing::UnorderedElementsAre(uid, SharedPtrIInterfaceId, WeakPtrIInterfaceId));
    EXPECT_THAT(any.GetCompatibleTypes(CompatibilityDirection::SET),
        testing::UnorderedElementsAre(uid, SharedPtrIInterfaceId, WeakPtrIInterfaceId));
    EXPECT_THAT(any.GetCompatibleTypes(CompatibilityDirection::GET),
        testing::UnorderedElementsAre(
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
 * @tc.name: GetSet
 * @tc.desc: Tests for Get Set. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AnyTest, GetSet, testing::ext::TestSize.Level1)
{
    Any<uint32_t> any { 1 };
    uint32_t v = 0;
    EXPECT_TRUE(any.GetValue(v));
    EXPECT_EQ(v, 1);
    EXPECT_EQ(GetValue<uint32_t>(any), 1);

    EXPECT_TRUE(any.SetValue(uint32_t(2)));
    EXPECT_EQ(GetValue<uint32_t>(any), 2);

    EXPECT_FALSE(any.SetValue(3.0f));
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
 * @tc.name: GetSetSharedPointer
 * @tc.desc: Tests for Get Set Shared Pointer. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AnyTest, GetSetSharedPointer, testing::ext::TestSize.Level1)
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
 * @tc.name: GetSetWeakPointer
 * @tc.desc: Tests for Get Set Weak Pointer. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AnyTest, GetSetWeakPointer, testing::ext::TestSize.Level1)
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
 * @tc.name: PtrCopyFrom
 * @tc.desc: Tests for Ptr Copy From. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AnyTest, PtrCopyFrom, testing::ext::TestSize.Level1)
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
 * @tc.name: BasicCompatibility
 * @tc.desc: Tests for Basic Compatibility. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AnyArrayTest, BasicCompatibility, testing::ext::TestSize.Level1)
{
    ArrayAny<uint32_t> any { 1, 2, 3 };
    using Type = uint32_t[];

    TypeId arrayUid = ArrayUidFromType<uint32_t>();
    TypeId vectorUid = UidFromType<BASE_NS::vector<uint32_t>>();

    EXPECT_THAT(
        any.GetCompatibleTypes(CompatibilityDirection::BOTH), testing::UnorderedElementsAre(arrayUid, vectorUid));
    EXPECT_TRUE(IsCompatible(any, arrayUid));
    EXPECT_TRUE(IsCompatibleWith<Type>(any));
    EXPECT_TRUE(IsSetCompatible(any, arrayUid));
    EXPECT_TRUE(IsSetCompatibleWith<Type>(any));
    EXPECT_TRUE(IsGetCompatible(any, arrayUid));
    EXPECT_TRUE(IsGetCompatibleWith<Type>(any));
    EXPECT_EQ(any.GetTypeId(), arrayUid);
}

/**
 * @tc.name: GetSetTypes
 * @tc.desc: Tests for Get Set Types. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AnyArrayTest, GetSetTypes, testing::ext::TestSize.Level1)
{
    static constexpr auto VALUE_COUNT = 3;
    const uint32_t valuesArr[VALUE_COUNT] = { 1, 2, 3 };
    const BASE_NS::vector<uint32_t> vectorArr = { 4, 5, 6 };

    auto arrayUid = UidFromType<uint32_t[]>();
    auto vectorUid = UidFromType<BASE_NS::vector<uint32_t>>();

    ArrayAny<uint32_t> any { valuesArr };

    uint32_t values[VALUE_COUNT];
    auto valuesSize = sizeof(uint32_t) * VALUE_COUNT;

    EXPECT_TRUE(any.GetData(arrayUid, values, valuesSize));
    EXPECT_THAT(values, testing::ElementsAreArray(valuesArr));

    BASE_NS::vector<uint32_t> valuesVec;
    EXPECT_TRUE(any.GetData(vectorUid, &valuesVec, sizeof(valuesVec)));
    EXPECT_THAT(values, testing::ElementsAreArray(valuesArr));

    EXPECT_TRUE(any.SetData(vectorUid, &vectorArr, sizeof(vectorArr)));
    EXPECT_TRUE(any.GetData(arrayUid, values, valuesSize));
    EXPECT_THAT(values, testing::ElementsAreArray(vectorArr));

    EXPECT_TRUE(any.SetData(arrayUid, vectorArr.data(), sizeof(uint32_t) * vectorArr.size()));
    EXPECT_TRUE(any.GetData(arrayUid, values, valuesSize));
    EXPECT_THAT(values, testing::ElementsAreArray(vectorArr));
}

/**
 * @tc.name: Types
 * @tc.desc: Tests for Types. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AnyTest, Types, testing::ext::TestSize.Level1)
{
    auto& reg = GetObjectRegistry().GetPropertyRegister();
    auto types = reg.GetAllRegisteredAnyTypes();
    const auto& invalid = reg.InvalidAny();
    auto invalidTarget = invalid.Clone();

    BASE_NS::unordered_map<TypeId, BASE_NS::vector<IAny::Ptr>> allTypes;
    // Instantiate all anys to a map for searching with type id
    for (auto&& id : types) {
        auto any = reg.ConstructAny(id);
        ASSERT_TRUE(any);
        auto& vec = allTypes[any->GetTypeId()];
        vec.push_back(any);
    }

    auto& data = GetObjectRegistry().GetEngineData();
    auto accesses = data.GetAllRegisteredValueAccess();
    ASSERT_FALSE(accesses.empty());
    for (auto&& access : accesses) {
        auto v = data.GetInternalValueAccess(access);
        CORE_NS::Property p;
        p.type = access;
        auto any = v->CreateAny(p);
        auto& vec = allTypes[any->GetTypeId()];
        vec.push_back(any);
    }

    auto getAllTypes = [&]() {
        BASE_NS::vector<TypeId> ids;
        ids.reserve(allTypes.size());
        for (auto&& t : allTypes) {
            ids.push_back(t.first);
        }
        return ids;
    };

    auto getAny = [&](TypeId id) -> BASE_NS::vector<IAny::Ptr> {
        BASE_NS::vector<IAny::Ptr> anys;
        // Return a clone of any with given type id
        if (auto it = allTypes.find(id); it != allTypes.end()) {
            for (auto&& a : it->second) {
                anys.push_back(a->Clone());
            }
        }
        return anys;
    };

    auto testAny = [&](const IAny::Ptr& any, TypeId t) {
        AnyCloneOptions opt;
        opt.value = CloneValueType::DEFAULT_VALUE;
        opt.role = TypeIdRole::CURRENT;
        auto clone = any->Clone(opt);
        ASSERT_TRUE(clone);
        auto cv = interface_cast<IValue>(clone);
        auto av = interface_cast<IValue>(any);
        if (cv && av) {
            EXPECT_TRUE(av->SetValue(cv->GetValue()));
            EXPECT_TRUE(any->ResetValue());
            EXPECT_TRUE(any->CopyFrom(cv->GetValue()));
            EXPECT_TRUE(av->IsCompatible(any->GetTypeId()));
            EXPECT_FALSE(av->IsCompatible({}));
        }
        EXPECT_FALSE(clone->CopyFrom(invalid));
        if (auto array = interface_cast<IArrayAny>(any)) {
            auto item = any->Clone({ CloneValueType::DEFAULT_VALUE, TypeIdRole::ITEM });
            EXPECT_EQ(array->GetSize(), 0);
            EXPECT_FALSE(array->SetAnyAt(0, *item));
            EXPECT_FALSE(array->InsertAnyAt(0, invalid));
            EXPECT_TRUE(array->InsertAnyAt(0, *item));
            EXPECT_TRUE(array->GetAnyAt(0, *item));
            EXPECT_FALSE(array->InsertAnyAt(10, invalid));
            EXPECT_TRUE(array->InsertAnyAt(10, *item));
            EXPECT_GT(array->GetSize(), 0);
            EXPECT_TRUE(array->SetAnyAt(0, *item));
            EXPECT_TRUE(array->InsertAnyAt(0, *item));
            EXPECT_FALSE(array->SetAnyAt(10, *item));
            EXPECT_FALSE(array->SetAnyAt(0, invalid));
            EXPECT_FALSE(array->SetAnyAt(10, invalid));
            EXPECT_FALSE(array->GetAnyAt(0, *invalidTarget));
            EXPECT_FALSE(array->GetAnyAt(10, *invalidTarget));
            EXPECT_FALSE(array->RemoveAt(10));
            EXPECT_TRUE(array->RemoveAt(0));
            EXPECT_FALSE(array->GetItemCompatibleTypes(CompatibilityDirection::GET).empty());
            EXPECT_FALSE(array->GetItemCompatibleTypes(CompatibilityDirection::SET).empty());
            EXPECT_FALSE(array->GetItemCompatibleTypes(CompatibilityDirection::BOTH).empty());
            array->RemoveAll();
            EXPECT_EQ(array->GetSize(), 0);
        }

        EXPECT_TRUE(any->ResetValue());
        EXPECT_FALSE(any->GetTypeIdString().empty());
        EXPECT_NE(any->GetClassId(), ObjectId {});
        EXPECT_NE(any->GetTypeId(), TypeId {});
        EXPECT_NE(any->GetTypeId(TypeIdRole::ARRAY), TypeId {});
        EXPECT_NE(any->GetTypeId(TypeIdRole::ITEM), TypeId {});
        EXPECT_NE(any->GetTypeId(TypeIdRole::CURRENT), TypeId {});
        // valid type, invalid ptr
        EXPECT_FALSE(any->GetData(t, nullptr, 0));
        EXPECT_FALSE(any->SetData(t, nullptr, 0));
        uint8_t d;
        // invalid type, valid ptr
        EXPECT_FALSE(any->GetData(TypeId {}, &d, 0));
        EXPECT_FALSE(any->SetData(TypeId {}, &d, 0));
        EXPECT_FALSE(any->CopyFrom(invalid));
        EXPECT_FALSE(invalidTarget->CopyFrom(*any));
    };

    auto testType = [&](const IAny::Ptr& v, CompatibilityDirection dir) {
        auto against = v->GetCompatibleTypes(dir);
        EXPECT_FALSE(against.empty());
        bool foundCompatible {};
        for (auto&& t : against) {
            auto anys = getAny(t);
            for (const auto& any : anys) {
                foundCompatible = true;
                testAny(any, t);
            }
            AnyCloneOptions opt;
            opt.value = CloneValueType::DEFAULT_VALUE;
            opt.role = TypeIdRole::ARRAY;
            if (auto any = v->Clone(opt)) {
                foundCompatible = true;
                testAny(any, t);
            }
            opt.role = TypeIdRole::ITEM;
            if (auto any = v->Clone(opt)) {
                foundCompatible = true;
                any = v->Clone(opt);
            }
        }
        EXPECT_TRUE(foundCompatible);
    };

    ASSERT_FALSE(types.empty());
    for (auto&& id : types) {
        auto any = reg.ConstructAny(id);
        ASSERT_TRUE(any);
        testAny(any, any->GetTypeId());
        AnyCloneOptions opt;
        opt.value = CloneValueType::DEFAULT_VALUE;
        opt.role = TypeIdRole::ARRAY;
        auto array = any->Clone(opt);
        testAny(array, array->GetTypeId());
        opt.role = TypeIdRole::ITEM;
        auto item = any->Clone(opt);
        testAny(item, item->GetTypeId());
        testType(any, CompatibilityDirection::GET);
        testType(any, CompatibilityDirection::SET);
        testType(any, CompatibilityDirection::BOTH);
    }
}

/**
 * @tc.name: GetSet
 * @tc.desc: Tests for Get Set. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_AnyArrayTest, GetSet, testing::ext::TestSize.Level1)
{
    using ArrayType = BASE_NS::vector<uint32_t>;

    BASE_NS::vector<uint32_t> vv { 1 };
    ArrayAny<uint32_t> any { vv };
    uint32_t v = 0;
    EXPECT_TRUE(any.GetValueAt(0, v));
    EXPECT_EQ(v, 1);
    EXPECT_EQ(GetValue<BASE_NS::vector<uint32_t>>(any), vv);

    EXPECT_FALSE(any.GetValueAt(1, v));
    EXPECT_FALSE(any.SetValueAt(1, v));
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

// clang-format off

const TestTypes AnyTestData(
    TType<bool> { true, false },
    TType<int8_t> { 2, -100 },
    TType<int16_t> { 4012, -10 },
    TType<int32_t> { 111111, -45114 },
    TType<int64_t> { 1234567890LL, -5555555LL },
    TType<uint8_t> { 2, 200 },
    TType<uint16_t> { 4012, 56000 },
    TType<uint32_t> { 115111, 455647114 },
    TType<uint64_t> { 5, 99999999999ULL },
    TType<float> { 1.0f, 2.33333f },
    TType<double> { 3.0, -4363.12455 },
    TType<TypeId> { TypeId{"00000000-0000-0000-0000-000000000000"}, TypeId{"10000000-0000-0000-0000-000000000000"} },
    TType<ObjectId> {
        ObjectId{"00000000-0000-0000-0000-000000000000"},
        ObjectId{"10000000-0000-0000-0000-000000000000"} },
    TType<InstanceId> {
        InstanceId{"00000000-0000-0000-0000-000000000000"},
        InstanceId{"10000000-0000-0000-0000-000000000000"} },
    TType<BASE_NS::Uid> {
        BASE_NS::Uid{"00000000-0000-0000-0000-000000000000"},
        BASE_NS::Uid{"10000000-0000-0000-0000-000000000000"} },
    TType<BASE_NS::string> { "1", "2" },
    TType<BASE_NS::Math::Vec2> { BASE_NS::Math::Vec2 { 1.3f, 5.0f },  BASE_NS::Math::Vec2 { -3.1f, 13.0f } },
    TType<BASE_NS::Math::Vec3> { BASE_NS::Math::Vec3 { 1.f, 2.f, 3.f }, BASE_NS::Math::Vec3 { 4.f, 1.2f, 0.88f } },
    TType<BASE_NS::Math::Vec4> {
        BASE_NS::Math::Vec4 { 4.f, 5.f, 6.f, 7.f },
        BASE_NS::Math::Vec4 { -0.005f, 0.13f, 13.f, 100.f } },
    TType<BASE_NS::Math::UVec2> { BASE_NS::Math::UVec2 { 1, 2 }, BASE_NS::Math::UVec2 { 6, 7 } },
    TType<BASE_NS::Math::UVec3> { BASE_NS::Math::UVec3 { 1, 2, 3 }, BASE_NS::Math::UVec3 { 6, 7, 8 } },
    TType<BASE_NS::Math::UVec4> { BASE_NS::Math::UVec4 { 1, 2, 3, 4 }, BASE_NS::Math::UVec4 { 6, 7, 8, 9 } },
    TType<BASE_NS::Math::IVec2> { BASE_NS::Math::IVec2 { -1, 1 }, BASE_NS::Math::IVec2 { 60, -50 } },
    TType<BASE_NS::Math::IVec3> { BASE_NS::Math::IVec3 { -1, 1, 0 }, BASE_NS::Math::IVec3 { 60, -50, 1 } },
    TType<BASE_NS::Math::IVec4> { BASE_NS::Math::IVec4 { -1, 1, 9, -50 }, BASE_NS::Math::IVec4 { 60, -50, 2, 3 } },
    TType<BASE_NS::Math::Quat> {
        BASE_NS::Math::Quat { 4.f, 5.f, 6.f, 7.f },
        BASE_NS::Math::Quat { -0.005f, 0.13f, 13.f, 100.f } },
    TType<BASE_NS::Math::Mat3X3> { BASE_NS::Math::Mat3X3 { BASE_NS::Math::Vec3 { 1.f, 2.f, 3.f },
                                   BASE_NS::Math::Vec3 { 4.f, 5.f, 6.f },
                                   BASE_NS::Math::Vec3 { 7.f, 8.f, 9.f } },
                                   BASE_NS::Math::Mat3X3 { 3.3f } },
    TType<BASE_NS::Math::Mat4X4> {
        BASE_NS::Math::Mat4X4 { 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f },
        BASE_NS::Math::Mat4X4 { 2.2f } },
    // Ptr types
    TType<SharedPtrIInterface> { SharedPtrIInterface{}, SharedPtrIInterface {} },
    TType<SharedPtrConstIInterface> { SharedPtrConstIInterface{}, SharedPtrConstIInterface {} },
    TType<IObject::Ptr> { IObject::Ptr{}, IObject::Ptr {} },
    TType<IObject::ConstPtr> { IObject::ConstPtr{}, IObject::ConstPtr {} },
    TType<IAttach::Ptr> { IAttach::Ptr{}, IAttach::Ptr {} },
    TType<IAttach::ConstPtr> { IAttach::ConstPtr{}, IAttach::ConstPtr {} },
    TType<IAny::Ptr> { IAny::Ptr {}, IAny::Ptr {} },
    TType<IAny::ConstPtr> { IAny::ConstPtr {}, IAny::ConstPtr {} },
    TType<IValue::Ptr> { IValue::Ptr {}, IValue::Ptr {} },
    TType<IValue::ConstPtr> { IValue::ConstPtr {}, IValue::ConstPtr {} },
    TType<IFunction::Ptr> { IFunction::Ptr {}, IFunction::Ptr {} },
    TType<IFunction::ConstPtr> { IFunction::ConstPtr {}, IFunction::ConstPtr {} },
    TType<IProperty::Ptr> { IProperty::Ptr {}, IProperty::Ptr {} },
    TType<IProperty::ConstPtr> { IProperty::ConstPtr {}, IProperty::ConstPtr {} }
    // Weak ptr types
    // TType<WeakPtrIInterface> { WeakPtrIInterface{}, WeakPtrIInterface {} },
    // TType<WeakPtrConstIInterface> { WeakPtrConstIInterface{}, WeakPtrConstIInterface {} },
    // TType<IObject::WeakPtr> { IObject::WeakPtr{}, IObject::WeakPtr {} },
    // TType<IObject::ConstWeakPtr> { IObject::ConstWeakPtr{}, IObject::ConstWeakPtr {} },
    // TType<IAttach::WeakPtr> { IAttach::WeakPtr{}, IAttach::WeakPtr {} },
    // TType<IAttach::ConstWeakPtr> { IAttach::ConstWeakPtr{}, IAttach::ConstWeakPtr {} },
    // TType<IFunction::WeakPtr> { IFunction::WeakPtr {}, IFunction::WeakPtr {} },
    // TType<IFunction::ConstWeakPtr> { IFunction::ConstWeakPtr {}, IFunction::ConstWeakPtr {} },
    // TType<IProperty::WeakPtr> { IProperty::WeakPtr {}, IProperty::WeakPtr {} },
    // TType<IProperty::ConstWeakPtr> { IProperty::ConstWeakPtr {}, IProperty::ConstWeakPtr {} }
);
// clang-format on

template<typename Type>
class API_TypedAnyTest : public ::testing::Test {
public:
    API_TypedAnyTest() : value1_(AnyTestData.GetValue<Type>(0)), value2_(AnyTestData.GetValue<Type>(1)) {}

    static constexpr bool IS_PTR_TYPE = IsSharedOrWeakPtr_v<Type>;

protected:
    void SetUp() override
    {
        typeId_ = UidFromType<Type>();
        vectorTypeId_ = UidFromType<BASE_NS::vector<Type>>();
        arrayTypeId_ = ArrayUidFromType<Type>();

        auto& reg = GetObjectRegistry();
        if constexpr (IS_PTR_TYPE) {
            if constexpr (BASE_NS::is_same_v<typename Type::element_type, IAny> ||
                          BASE_NS::is_same_v<typename Type::element_type, IValue>) {
                auto classId = Any<int>::StaticGetClassId();
                object1_ = reg.GetPropertyRegister().ConstructAny(classId);
                object2_ = reg.GetPropertyRegister().ConstructAny(classId);
            } else if constexpr (BASE_NS::is_same_v<typename Type::element_type, IProperty>) {
                object1_ = reg.GetPropertyRegister().Create(ClassId::StackProperty, "");
                object2_ = reg.GetPropertyRegister().Create(ClassId::StackProperty, "");
            } else {
                ObjectId classId = ClassId::Object;
                if constexpr (BASE_NS::is_same_v<typename Type::element_type, IFunction>) {
                    classId = ClassId::SettableFunction;
                }
                object1_ = reg.Create(classId);
                object2_ = reg.Create(classId);
            }
            ASSERT_TRUE(object1_);
            ASSERT_TRUE(object2_);
        }
    }
    void TearDown() override
    {
        // Make sure we don't leave anything alive which would interfere with other tests
        object1_.reset();
        object2_.reset();
        if constexpr (IS_PTR_TYPE) {
            value1_.reset();
            value2_.reset();
        }
        META_NS::GetObjectRegistry().Purge();
    }
    Type GetDefValue() const
    {
        return {};
    }
    Type GetValue1() const
    {
        if constexpr (IS_PTR_TYPE) {
            return interface_pointer_cast<typename Type::element_type>(object1_);
        }
        return value1_;
    }
    Type GetValue2() const
    {
        if constexpr (IS_PTR_TYPE) {
            return interface_pointer_cast<typename Type::element_type>(object2_);
        }
        return value2_;
    }
    TypeId GetTypeId() const
    {
        return typeId_;
    }
    TypeId GetVectorTypeId() const
    {
        return vectorTypeId_;
    }
    TypeId GetArrayTypeId() const
    {
        return arrayTypeId_;
    }
    size_t GetDataSize() const
    {
        return sizeof(Type);
    }

private:
    Type value1_;
    Type value2_;
    TypeId typeId_;
    TypeId vectorTypeId_;
    TypeId arrayTypeId_;
    BASE_NS::shared_ptr<CORE_NS::IInterface> object1_;
    BASE_NS::shared_ptr<CORE_NS::IInterface> object2_;
};

TYPED_TEST_SUITE(API_TypedAnyTest, decltype(AnyTestData)::Types);

TYPED_TEST(API_TypedAnyTest, GetSetData)
{
    auto& reg = GetObjectRegistry().GetPropertyRegister();
    auto types = reg.GetAllRegisteredAnyTypes();
    const auto& invalid = reg.InvalidAny();

    const auto typeId = this->GetTypeId();
    const auto value1 = this->GetValue1();
    const auto value2 = this->GetValue2();
    const auto dataSize = this->GetDataSize();
    constexpr auto anyObjectId = Any<TypeParam>::StaticGetClassId();
    auto any = reg.ConstructAny(anyObjectId);
    auto any2 = reg.ConstructAny(anyObjectId);
    auto defaultAny = reg.ConstructAny(anyObjectId);
    ASSERT_TRUE(any) << "Item " << BASE_NS::to_string(typeId.ToUid()).c_str();
    ASSERT_TRUE(any2);
    ASSERT_TRUE(defaultAny);

    EXPECT_FALSE(any->SetData({}, nullptr, 0));
    EXPECT_FALSE(any->SetData({}, &value1, dataSize));
    EXPECT_FALSE(any->SetData(typeId, nullptr, dataSize));
    EXPECT_FALSE(any->SetData({}, nullptr, dataSize));
    EXPECT_FALSE(any->SetData(typeId, &value1, 0));
    EXPECT_FALSE(any->SetData({}, &value1, 0));
    EXPECT_FALSE(any->SetData(typeId, &value1, dataSize + 1));
    EXPECT_EQ(any->SetData(typeId, &value1, dataSize), AnyReturnValue { AnyReturn::SUCCESS });
    EXPECT_EQ(any->SetData(typeId, &value1, dataSize), AnyReturnValue { AnyReturn::NOTHING_TO_DO });

    EXPECT_TRUE(any2->SetValue(value2));

    auto target = TypeParam {};
    EXPECT_FALSE(any->GetData({}, nullptr, 0));
    EXPECT_FALSE(any->GetData({}, &target, dataSize));
    EXPECT_FALSE(any->GetData(typeId, nullptr, dataSize));
    EXPECT_FALSE(any->GetData({}, nullptr, dataSize));
    EXPECT_FALSE(any->GetData(typeId, &target, 0));
    EXPECT_FALSE(any->GetData({}, &target, 0));
    EXPECT_FALSE(any->GetData(typeId, &target, dataSize + 1));
    EXPECT_TRUE(any->GetData(typeId, &target, dataSize));

    EXPECT_TRUE(any->CopyFrom(*any2));
    EXPECT_TRUE(any->CopyFrom(*defaultAny));
    EXPECT_TRUE(any->CopyFrom(*any2));
    EXPECT_TRUE(any->ResetValue());
    EXPECT_TRUE(any->CopyFrom(*any2));

    auto defaultValue = TypeParam {};
    EXPECT_TRUE(any->SetValue(value1));
    EXPECT_TRUE(any->SetValue(value2));
    EXPECT_TRUE(any->SetValue(defaultValue));
    EXPECT_TRUE(any->SetValue(value2));
    EXPECT_TRUE(any->SetValue(defaultValue));

    EXPECT_EQ(any->SetData(typeId, &defaultValue, dataSize), AnyReturnValue { AnyReturn::SUCCESS });

    auto testArray = [&](TypeId arrayTypeId, auto& arrayAny, auto& arrayValue1, auto& arraySize, auto* arrayTarget,
                         const auto* defaultArrayValue) {
        const bool isCArray = arrayTypeId == this->GetArrayTypeId();
        AnyReturnValue expectedSetSame =
            arrayTypeId == this->GetArrayTypeId() ? AnyReturn::SUCCESS : AnyReturn::NOTHING_TO_DO;
        AnyReturnValue expectedAccessTooBigSize =
            arrayTypeId == this->GetArrayTypeId() ? AnyReturn::SUCCESS : AnyReturn::INVALID_ARGUMENT;
        AnyReturnValue expectedAccessTooSmallSize =
            arrayTypeId == this->GetArrayTypeId() ? AnyReturn::INVALID_ARGUMENT : AnyReturn::SUCCESS;
        EXPECT_FALSE(arrayAny->SetData({}, &arrayValue1, arraySize));
        EXPECT_FALSE(arrayAny->SetData(arrayTypeId, nullptr, arraySize));
        EXPECT_FALSE(arrayAny->SetData({}, nullptr, arraySize));
        EXPECT_FALSE(arrayAny->SetData(arrayTypeId, &arrayValue1, 0));
        EXPECT_FALSE(arrayAny->SetData({}, &arrayValue1, 0));
        EXPECT_EQ(arrayAny->SetData(arrayTypeId, &arrayValue1, arraySize + 1), expectedAccessTooBigSize);
        EXPECT_TRUE(arrayAny->SetData(arrayTypeId, &arrayValue1, arraySize));
        EXPECT_EQ(arrayAny->SetData(arrayTypeId, &arrayValue1, arraySize), expectedSetSame);

        EXPECT_FALSE(arrayAny->GetData({}, arrayTarget, arraySize));
        EXPECT_FALSE(arrayAny->GetData(arrayTypeId, nullptr, arraySize));
        EXPECT_FALSE(arrayAny->GetData({}, nullptr, arraySize));
        EXPECT_FALSE(arrayAny->GetData(arrayTypeId, arrayTarget, 0));
        EXPECT_FALSE(arrayAny->GetData({}, arrayTarget, 0));
        if (arrayTypeId != this->GetArrayTypeId()) {
            EXPECT_EQ(arrayAny->GetData(arrayTypeId, arrayTarget, arraySize + 1), expectedAccessTooBigSize);
            EXPECT_TRUE(arrayAny->GetData(arrayTypeId, arrayTarget, arraySize));
        }

        EXPECT_FALSE(arrayAny->SetDataAt(0, {}, &value1, dataSize));
        EXPECT_FALSE(arrayAny->SetDataAt(0, typeId, nullptr, dataSize));
        EXPECT_FALSE(arrayAny->SetDataAt(0, {}, nullptr, dataSize));
        EXPECT_FALSE(arrayAny->SetDataAt(0, typeId, &value1, 0));
        EXPECT_FALSE(arrayAny->SetDataAt(0, {}, &value1, 0));
        EXPECT_FALSE(arrayAny->SetDataAt(0, typeId, &value1, dataSize + 1));
        EXPECT_FALSE(arrayAny->SetDataAt(2, {}, &value1, dataSize));
        EXPECT_FALSE(arrayAny->SetDataAt(2, typeId, nullptr, dataSize));
        EXPECT_FALSE(arrayAny->SetDataAt(2, {}, nullptr, dataSize));
        EXPECT_FALSE(arrayAny->SetDataAt(2, typeId, &value1, 0));
        EXPECT_FALSE(arrayAny->SetDataAt(2, {}, &value1, 0));
        EXPECT_FALSE(arrayAny->SetDataAt(2, typeId, &value1, dataSize + 1));
        EXPECT_FALSE(arrayAny->SetDataAt(2, typeId, &value2, dataSize));
        EXPECT_EQ(arrayAny->SetDataAt(0, typeId, &value2, dataSize), AnyReturn::SUCCESS);

        EXPECT_FALSE(arrayAny->GetDataAt(0, {}, arrayTarget, dataSize));
        EXPECT_FALSE(arrayAny->GetDataAt(0, arrayTypeId, nullptr, dataSize));
        EXPECT_FALSE(arrayAny->GetDataAt(0, {}, nullptr, dataSize));
        EXPECT_FALSE(arrayAny->GetDataAt(0, arrayTypeId, arrayTarget, 0));
        EXPECT_FALSE(arrayAny->GetDataAt(0, {}, arrayTarget, 0));
        EXPECT_FALSE(arrayAny->GetDataAt(0, arrayTypeId, arrayTarget, dataSize + 1));
        EXPECT_FALSE(arrayAny->GetDataAt(2, {}, arrayTarget, dataSize));
        EXPECT_FALSE(arrayAny->GetDataAt(2, arrayTypeId, nullptr, dataSize));
        EXPECT_FALSE(arrayAny->GetDataAt(2, {}, nullptr, dataSize));
        EXPECT_FALSE(arrayAny->GetDataAt(2, arrayTypeId, arrayTarget, 0));
        EXPECT_FALSE(arrayAny->GetDataAt(2, {}, arrayTarget, 0));
        EXPECT_FALSE(arrayAny->GetDataAt(2, arrayTypeId, arrayTarget, dataSize + 1));
        EXPECT_FALSE(arrayAny->GetDataAt(2, typeId, &target, dataSize));
        EXPECT_TRUE(arrayAny->GetDataAt(0, typeId, &target, dataSize));

        // Add 2
        EXPECT_TRUE(arrayAny->InsertAnyAt(0, *any));
        EXPECT_TRUE(arrayAny->InsertAnyAt(0, *any));
        // GetData to 1 size array
        if (arrayTypeId != this->GetArrayTypeId()) {
            EXPECT_EQ(arrayAny->GetData(arrayTypeId, arrayTarget, arraySize), expectedAccessTooSmallSize);
        }
        EXPECT_TRUE(arrayAny->template SetValueAt<TypeParam>(0, value1));
        EXPECT_FALSE(arrayAny->template SetValueAt<TypeParam>(10, value1));
        EXPECT_TRUE(arrayAny->template GetValueAt<TypeParam>(0, target));
        EXPECT_FALSE(arrayAny->template GetValueAt<TypeParam>(10, target));

        if (defaultArrayValue) {
            EXPECT_TRUE(arrayAny->SetData(arrayTypeId, defaultArrayValue, arraySize));
        }
    };

    AnyCloneOptions opt;
    opt.value = CloneValueType::DEFAULT_VALUE;
    opt.role = TypeIdRole::ARRAY;
    auto arrayAny = interface_pointer_cast<IArrayAny>(any->Clone(opt));

    ASSERT_TRUE(arrayAny) << "Array " << BASE_NS::to_string(typeId.ToUid()).c_str();

    BASE_NS::vector<TypeParam> vectorValue1 = { value1 };
    const auto vectorSize = sizeof(vectorValue1);
    const auto vectorTypeId = this->GetVectorTypeId();
    BASE_NS::vector<TypeParam> defaultVectorValue {};
    auto vectorTarget = BASE_NS::vector<TypeParam> {};
    testArray(vectorTypeId, arrayAny, vectorValue1, vectorSize, &vectorTarget, &defaultVectorValue);

    const TypeParam arrayValue1[] = { value1 };
    const auto arrayTypeId = this->GetArrayTypeId();
    TypeParam arrayTarget[] = { {} };
    const auto arraySize = sizeof(TypeParam);

    EXPECT_NE(vectorTypeId, arrayTypeId);

    testArray(arrayTypeId, arrayAny, arrayValue1, arraySize, &arrayTarget[0], static_cast<TypeParam*>(nullptr));
}

} // namespace UTest
META_END_NAMESPACE()
