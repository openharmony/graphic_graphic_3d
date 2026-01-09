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

// clang-format off
#include "engine_test_property.h"
// clang-format on

#include <test_framework.h>

#include <core/property/property_handle_util.h>

#include <meta/api/engine/util.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/engine/intf_engine_value_manager.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_task_queue_registry.h>

#include "helpers/testing_objects.h"
#include "helpers/util.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

META_BEGIN_NAMESPACE()
namespace UTest {

class API_EngineManagerTest : public ::testing::Test {
public:
    void SetUp() override
    {
        engine_ = GetTestEnv()->engine.get();
        RegisterEngineTestType();
    }
    void TearDown() override
    {
        UnregisterEngineTestType();
    }

private:
    CORE_NS::IEngine* engine_;
};

/**
 * @tc.name: Values
 * @tc.desc: Tests for Values. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, Values, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    EXPECT_TRUE(manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_PROPERTY_COUNT);
    auto p = manager->ConstructProperty<int32_t>("value");
    ASSERT_TRUE(p);
    EXPECT_EQ(p->GetValue(), 0);

    auto ev = interface_cast<IValue>(GetEngineValueFromProperty(p));
    ASSERT_TRUE(ev);
    auto& any = ev->GetValue();

    auto info = interface_cast<IInfo>(&any);
    ASSERT_TRUE(info);
    EXPECT_EQ(info->GetName(), "value");
    EXPECT_EQ(info->GetDescription(), "Value it is");

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 1));
    EXPECT_EQ(p->GetValue(), 0);
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(p->GetValue(), 1);

    EXPECT_TRUE(p->SetValue(2));
    EXPECT_EQ(p->GetValue(), 2);
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));

    EXPECT_EQ(p->GetValue(), 2);
    EXPECT_EQ(CORE_NS::GetPropertyValue<int32_t>(*cman.GetData(0), "value"), 2);

    // update
    {
        EXPECT_TRUE(manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));
        EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_PROPERTY_COUNT);
        auto p = manager->ConstructProperty<int32_t>("value");
        ASSERT_TRUE(p);
    }

    {
        manager->RemoveValue("value");
        EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_PROPERTY_COUNT - 1);
        EXPECT_TRUE(manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));
        EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_PROPERTY_COUNT);
        auto p = manager->ConstructProperty<int32_t>("value");
        ASSERT_TRUE(p);
    }

    {
        manager->RemoveAll();
        EXPECT_EQ(manager->GetAllEngineValues().size(), 0);
        EXPECT_TRUE(manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));
        EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_PROPERTY_COUNT);
        auto p = manager->ConstructProperty<int32_t>("value");
        ASSERT_TRUE(p);
    }
}

/**
 * @tc.name: ArrayValues
 * @tc.desc: Tests for Array Values. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, ArrayValues, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    EXPECT_TRUE(manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));
    auto p = manager->ConstructArrayProperty<float>("floats");
    ASSERT_TRUE(p);
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<float> {}));

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "floats", (BASE_NS::vector<float> { 1, 2 })));
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<float> {}));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<float> { 1, 2 }));

    EXPECT_TRUE(p->SetValue((BASE_NS::vector<float> { 3, 4, 5 })));
    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<float> { 3, 4, 5 }));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));

    EXPECT_EQ(p->GetValue(), (BASE_NS::vector<float> { 3, 4, 5 }));
    EXPECT_EQ(CORE_NS::GetPropertyValue<BASE_NS::vector<float>>(*cman.GetData(0), "floats"),
        (BASE_NS::vector<float> { 3, 4, 5 }));
}

/**
 * @tc.name: AddValuesRecursively
 * @tc.desc: Tests for Add Values Recursively. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, AddValuesRecursively, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    EngineValueOptions options;
    options.recurseKnownStructs = true;
    AddEngineValuesRecursively(manager, EnginePropertyHandle { &cman, cman.entityRef }, options);
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_RECURSIVE_PROPERTY_COUNT);
    auto p = manager->ConstructProperty<BASE_NS::Math::IVec2>("vec2");
    ASSERT_TRUE(p);
    auto x = manager->ConstructProperty<int32_t>("vec2.x");
    ASSERT_TRUE(x);
    auto y = manager->ConstructProperty<int32_t>("vec2.y");
    ASSERT_TRUE(y);

    EXPECT_EQ(p->GetValue(), BASE_NS::Math::IVec2());
    EXPECT_EQ(x->GetValue(), 0);
    EXPECT_EQ(y->GetValue(), 0);

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "vec2", BASE_NS::Math::IVec2(1, 2)));
    EXPECT_EQ(p->GetValue(), BASE_NS::Math::IVec2(0, 0));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(p->GetValue(), BASE_NS::Math::IVec2(1, 2));
    EXPECT_EQ(x->GetValue(), 1);
    EXPECT_EQ(y->GetValue(), 2);

    EXPECT_TRUE(p->SetValue(BASE_NS::Math::IVec2(2, 3)));
    EXPECT_EQ(p->GetValue(), BASE_NS::Math::IVec2(2, 3));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));

    EXPECT_EQ(p->GetValue(), BASE_NS::Math::IVec2(2, 3));
    // call again to ensure the x and y are updated from engine, as the update depends on the order of values
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(x->GetValue(), 2);
    EXPECT_EQ(y->GetValue(), 3);
    EXPECT_EQ(CORE_NS::GetPropertyValue<BASE_NS::Math::IVec2>(*cman.GetData(0), "vec2"), BASE_NS::Math::IVec2(2, 3));

    EXPECT_TRUE(x->SetValue(11));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(x->GetValue(), 11);
    // call again to ensure the x and y are updated from engine, as the update depends on the order of values
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_EQ(p->GetValue(), BASE_NS::Math::IVec2(11, 3));

    auto typex = manager->ConstructProperty<int32_t>("type.vec.x");
    ASSERT_TRUE(typex);
    EXPECT_TRUE(typex->SetValue(9));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));
    auto type = manager->ConstructProperty<BASE_NS::Math::IVec2>("type.vec");
    ASSERT_TRUE(type);
    EXPECT_EQ(type->GetValue(), BASE_NS::Math::IVec2(9, 0));
}

/**
 * @tc.name: AddValuesRecursivelyWithHandle
 * @tc.desc: Tests for Add Values Recursively With Handle. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, AddValuesRecursivelyWithHandle, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop1::EngineTestProp> cman1 { prop1::ENGINE_TESTPROP_METADATA };
    TestComponentManager<prop2::EngineTestProp> cman2 { prop2::ENGINE_TESTPROP_METADATA };

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman1.GetData(0), "handle", cman2.GetData(0)));

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    EngineValueOptions options;
    options.recurseKnownStructs = true;
    AddEngineValuesRecursively(manager, EnginePropertyHandle { &cman1, cman1.entityRef }, options);
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_RECURSIVE_PROPERTY_COUNT + prop2::TEST_PROPERTY_COUNT);

    auto handle = manager->ConstructProperty<CORE_NS::IPropertyHandle*>("handle");
    ASSERT_TRUE(handle);
    EXPECT_EQ(handle->GetValue(), cman2.GetData(0));

    auto value = manager->ConstructProperty<int>("handle.value");
    ASSERT_TRUE(value);
    EXPECT_EQ(value->GetValue(), 1);

    auto array = manager->ConstructProperty<BASE_NS::vector<uint32_t>>("handle.array");
    ASSERT_TRUE(array);
    EXPECT_EQ(array->GetValue(), (BASE_NS::vector<uint32_t> { 1, 1 }));
}

/**
 * @tc.name: SyncWithInvalidParentHandle
 * @tc.desc: Tests for Sync With Invalid Parent Handle. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, SyncWithInvalidParentHandle, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop1::EngineTestProp> cman1 { prop1::ENGINE_TESTPROP_METADATA };
    TestComponentManager<prop2::EngineTestProp> cman2 { prop2::ENGINE_TESTPROP_METADATA };

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman1.GetData(0), "handle", cman2.GetData(0)));

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    EngineValueOptions options;
    options.recurseKnownStructs = true;
    AddEngineValuesRecursively(manager, EnginePropertyHandle { &cman1, cman1.entityRef }, options);
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_RECURSIVE_PROPERTY_COUNT + prop2::TEST_PROPERTY_COUNT);

    auto handle = manager->ConstructProperty<CORE_NS::IPropertyHandle*>("handle");
    ASSERT_TRUE(handle);
    EXPECT_EQ(handle->GetValue(), cman2.GetData(0));

    auto value = manager->ConstructProperty<int>("handle.value");
    ASSERT_TRUE(value);
    EXPECT_EQ(value->GetValue(), 1);

    // remove handles
    cman1.Destroy(cman1.ent);
    cman2.Destroy(cman2.ent);

    EXPECT_FALSE(manager->Sync(EngineSyncDirection::AUTO));

    EXPECT_EQ(value->GetValue(), 1);
}

/**
 * @tc.name: ChangeHandle
 * @tc.desc: Tests for Change Handle. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, ChangeHandle, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop1::EngineTestProp> cman1 { prop1::ENGINE_TESTPROP_METADATA };
    TestComponentManager<prop2::EngineTestProp> cman2 { prop2::ENGINE_TESTPROP_METADATA };
    TestComponentManager<prop2::EngineTestProp> cman3 { prop2::ENGINE_TESTPROP_METADATA };

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman1.GetData(0), "handle", cman2.GetData(0)));

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    EngineValueOptions options;
    options.recurseKnownStructs = true;
    AddEngineValuesRecursively(manager, EnginePropertyHandle { &cman1, cman1.entityRef }, options);
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_RECURSIVE_PROPERTY_COUNT + prop2::TEST_PROPERTY_COUNT);

    auto handle = manager->ConstructProperty<CORE_NS::IPropertyHandle*>("handle");
    ASSERT_TRUE(handle);
    EXPECT_EQ(handle->GetValue(), cman2.GetData(0));

    auto value = manager->ConstructProperty<int>("handle.value");
    ASSERT_TRUE(value);
    EXPECT_EQ(value->GetValue(), 1);

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman3.GetData(0), "value", 9));
    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman1.GetData(0), "handle", cman3.GetData(0)));

    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));

    EXPECT_EQ(value->GetValue(), 9);
    EXPECT_EQ(handle->GetValue(), cman3.GetData(0));
}

template<typename Type>
void TestWithSingle(BASE_NS::string_view path, EngineValueOptions ops = {})
{
    TestComponentManager<prop1::EngineTestProp> cman1 { prop1::ENGINE_TESTPROP_METADATA };
    TestComponentManager<prop2::EngineTestProp> cman2 { prop2::ENGINE_TESTPROP_METADATA };

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman1.GetData(0), "handle", cman2.GetData(0)));

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    manager->ConstructValue(EnginePropertyHandle { &cman1, cman1.entityRef }, path, ops);
    EXPECT_TRUE(manager->GetAllEngineValues().size() >= 1);

    auto name = ops.namePrefix;
    if (!name.empty()) {
        name += ".";
    }
    name += path;

    EXPECT_TRUE(manager->ConstructProperty<Type>(name));
}

/**
 * @tc.name: AddSingleValue
 * @tc.desc: Tests for Add Single Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, AddSingleValue, testing::ext::TestSize.Level1)
{
    TestWithSingle<int>("value");
    TestWithSingle<CORE_NS::IPropertyHandle*>("handle");
    TestWithSingle<int>("handle.value");
    TestWithSingle<BASE_NS::vector<uint32_t>>("handle.array");
    TestWithSingle<BASE_NS::vector<EngineTestType>>("handle.complexVec");

    TestWithSingle<uint32_t>("handle.array[0]");
    TestWithSingle<uint32_t>("handle.array[1]");
    TestWithSingle<BASE_NS::vector<EngineTestType>>("handle.complex");
    TestWithSingle<EngineTestType>("handle.complex[1]");
    TestWithSingle<BASE_NS::Math::IVec2>("handle.complex[1].vec");
    TestWithSingle<int32_t>("handle.complex[2].vec.x");
    TestWithSingle<int32_t>("handle.complex[2].vec.y");
    TestWithSingle<int32_t>("handle.complexVec[0].vec.x");
    TestWithSingle<int32_t>("handle.complexVec[1].vec.y");

    TestWithSingle<BASE_NS::Math::IVec2>("type.vec");
    TestWithSingle<int32_t>("type.vec.x");

    TestWithSingle<int32_t>("type.vec.x", EngineValueOptions { "TestPrefix" });
}

// check the overlapping values don't overwrite each other if not altered
/**
 * @tc.name: Overlapping
 * @tc.desc: Tests for Overlapping. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, Overlapping, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    EngineValueOptions options;
    options.recurseKnownStructs = true;
    AddEngineValuesRecursively(manager, EnginePropertyHandle { &cman, cman.entityRef }, options);
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop1::TEST_RECURSIVE_PROPERTY_COUNT);

    auto p = manager->ConstructProperty<BASE_NS::Math::IVec2>("vec2");
    ASSERT_TRUE(p);
    auto x = manager->ConstructProperty<int32_t>("vec2.x");
    ASSERT_TRUE(x);

    x->SetValue(2);
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::TO_ENGINE));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::FROM_ENGINE));
    EXPECT_EQ(p->GetValue(), (BASE_NS::Math::IVec2 { 2, 0 }));
    EXPECT_EQ(x->GetValue(), 2);

    p->SetValue({ 3, 0 });
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::TO_ENGINE));
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::FROM_ENGINE));
    EXPECT_EQ(p->GetValue(), (BASE_NS::Math::IVec2 { 3, 0 }));
    EXPECT_EQ(x->GetValue(), 3);
}

/**
 * @tc.name: Enum
 * @tc.desc: Tests for Enum. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, Enum, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop3::EngineTestProp> cman { prop3::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef });
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop3::TEST_PROPERTY_COUNT);

    auto p = manager->ConstructProperty<EngineTestEnum>("enum1");
    ASSERT_TRUE(p);

    auto ev = interface_cast<IValue>(GetEngineValueFromProperty(p));
    ASSERT_TRUE(ev);
    auto& any = ev->GetValue();

    auto v = any.Clone();
    ASSERT_TRUE(interface_cast<IEnum>(&any));
    auto ei = interface_cast<IEnum>(v);
    ASSERT_TRUE(ei);
    ASSERT_TRUE(interface_cast<IInfo>(v));
    auto et = interface_cast<IEngineType>(v);
    ASSERT_TRUE(et);

    EXPECT_EQ(et->GetTypeDecl(), MetaType<EngineTestEnum>::coreType);
    EXPECT_EQ(ei->GetName(), "enum1");
    EXPECT_EQ(ei->GetDescription(), "My Enum");
    EXPECT_EQ(ei->GetEnumType(), EnumType::SINGLE_VALUE);
    auto view = ei->GetValues();
    ASSERT_EQ(view.size(), 3);
    EXPECT_EQ(view[0].name, "MY_VALUE_1");
    EXPECT_EQ(view[0].desc, "Value 1");
    EXPECT_EQ(view[1].name, "MY_VALUE_2");
    EXPECT_EQ(view[1].desc, "Value 2");
    EXPECT_EQ(view[2].name, "MY_VALUE_3");
    EXPECT_EQ(view[2].desc, "Value 3");

    EXPECT_EQ(ei->GetValueIndex(), 0);
    EXPECT_TRUE(ei->IsValueSet(0));

    EXPECT_TRUE(ei->SetValueIndex(1));
    EXPECT_EQ(ei->GetValueIndex(), 1);
    EXPECT_TRUE(ei->IsValueSet(1));
    EXPECT_EQ(GetValue<EngineTestEnum>(*v), EngineTestEnum::MY_VALUE_2);

    EXPECT_TRUE(ei->SetValueIndex(2));
    EXPECT_EQ(ei->GetValueIndex(), 2);
    EXPECT_EQ(GetValue<EngineTestEnum>(*v), EngineTestEnum::MY_VALUE_3);
    EXPECT_EQ(GetValue<int64_t>(*v), 5);

    EXPECT_FALSE(ei->SetValueIndex(3));
}

/**
 * @tc.name: BitField
 * @tc.desc: Tests for Bit Field. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, BitField, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop3::EngineTestProp> cman { prop3::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef });
    EXPECT_EQ(manager->GetAllEngineValues().size(), prop3::TEST_PROPERTY_COUNT);

    auto p = manager->ConstructProperty<EngineTestBitField>("enum2");
    ASSERT_TRUE(p);

    auto ev = interface_cast<IValue>(GetEngineValueFromProperty(p));
    ASSERT_TRUE(ev);
    auto& any = ev->GetValue();

    auto v = any.Clone();
    ASSERT_TRUE(interface_cast<IEnum>(&any));
    auto ei = interface_cast<IEnum>(v);
    ASSERT_TRUE(ei);
    ASSERT_TRUE(interface_cast<IInfo>(v));
    auto et = interface_cast<IEngineType>(v);
    ASSERT_TRUE(et);

    EXPECT_EQ(et->GetTypeDecl(), MetaType<EngineTestBitField>::coreType);
    EXPECT_EQ(ei->GetName(), "enum2");
    EXPECT_EQ(ei->GetDescription(), "My Bitfield");
    EXPECT_EQ(ei->GetEnumType(), EnumType::BIT_FIELD);
    auto view = ei->GetValues();
    ASSERT_EQ(view.size(), 4);
    EXPECT_EQ(view[0].name, "MY_VALUE_1");
    EXPECT_EQ(view[0].desc, "Value 1");
    EXPECT_EQ(view[1].name, "MY_VALUE_2");
    EXPECT_EQ(view[1].desc, "Value 2");
    EXPECT_EQ(view[2].name, "MY_VALUE_3");
    EXPECT_EQ(view[2].desc, "Value 3");
    EXPECT_EQ(view[3].name, "MY_VALUE_4");
    EXPECT_EQ(view[3].desc, "Value 4");

    EXPECT_EQ(ei->GetValueIndex(), -1);
    EXPECT_FALSE(ei->IsValueSet(0));

    EXPECT_TRUE(ei->SetValueIndex(0));
    EXPECT_EQ(ei->GetValueIndex(), 0);
    EXPECT_TRUE(ei->IsValueSet(0));
    EXPECT_EQ(GetValue<EngineTestBitField>(*v), EngineTestBitField::MY_VALUE_1);

    EXPECT_TRUE(ei->FlipValue(1, true));
    EXPECT_TRUE(ei->IsValueSet(1));
    EXPECT_EQ(GetValue<int64_t>(*v), int64_t(EngineTestBitField::MY_VALUE_1) | int64_t(EngineTestBitField::MY_VALUE_2));

    EXPECT_TRUE(ei->FlipValue(0, false));
    EXPECT_FALSE(ei->IsValueSet(0));
    EXPECT_TRUE(ei->IsValueSet(1));
    EXPECT_EQ(GetValue<int64_t>(*v), int64_t(EngineTestBitField::MY_VALUE_2));
}

/**
 * @tc.name: ContainerSubscription
 * @tc.desc: Tests for Container Subscription. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, ContainerSubscription, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop2::EngineTestProp> cman { prop2::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    ASSERT_TRUE(manager);

    bool arrres =
        manager->ConstructValue(EnginePropertyHandle { &cman, cman.entityRef }, "complexVec", EngineValueOptions {});
    ASSERT_TRUE(arrres);

    auto arr = manager->ConstructArrayProperty<EngineTestType>("complexVec");
    ASSERT_TRUE(arr);

    {
        bool res = manager->ConstructValue(
            EnginePropertyHandle { &cman, cman.entityRef }, "complexVec[2]", EngineValueOptions {});
        ASSERT_FALSE(res);
    }

    bool res0 =
        manager->ConstructValue(EnginePropertyHandle { &cman, cman.entityRef }, "complexVec[0]", EngineValueOptions {});
    ASSERT_TRUE(res0);

    auto v0 = manager->ConstructProperty<EngineTestType>("complexVec[0]");
    ASSERT_TRUE(v0);
    EXPECT_EQ(v0->GetValue(), (EngineTestType { { 1, 2 } }));

    bool res1 =
        manager->ConstructValue(EnginePropertyHandle { &cman, cman.entityRef }, "complexVec[1]", EngineValueOptions {});
    ASSERT_TRUE(res1);

    auto v1 = manager->ConstructProperty<EngineTestType>("complexVec[1]");
    ASSERT_TRUE(v1);
    EXPECT_EQ(v1->GetValue(), (EngineTestType { { 8, 8 } }));

    arr->SetValue({ EngineTestType { { 4, 4 } } });
    EXPECT_TRUE(manager->Sync(EngineSyncDirection::AUTO));

    EXPECT_EQ(v0->GetValue(), (EngineTestType { { 4, 4 } }));
    v0->SetValue({ { 5, 6 } });
    // old value is returned since there is nothing behind it any more
    EXPECT_EQ(v1->GetValue(), (EngineTestType { { 8, 8 } }));
    v1->SetValue({ { 3, 3 } });

    manager->Sync(EngineSyncDirection::AUTO);

    {
        bool res = manager->ConstructValue(
            EnginePropertyHandle { &cman, cman.entityRef }, "complexVec[0].vec.x", EngineValueOptions {});
        ASSERT_TRUE(res);

        auto v = manager->ConstructProperty<int32_t>("complexVec[0].vec.x");
        ASSERT_TRUE(v);
        EXPECT_EQ(v->GetValue(), 5);
        v->SetValue(7);
        manager->Sync(EngineSyncDirection::AUTO);
        EXPECT_EQ(v->GetValue(), 7);
    }
    EXPECT_EQ(v0->GetValue(), (EngineTestType { { 7, 6 } }));

    v0->ResetValue();
}

/**
 * @tc.name: InvalidState
 * @tc.desc: Tests EngineValue invalid state.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, InvalidState, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop2::EngineTestProp> cman { prop2::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(ClassId::EngineValueManager);
    ASSERT_TRUE(manager);
    EXPECT_TRUE(manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));

    auto value = manager->GetEngineValue("value");
    ASSERT_TRUE(value);
    auto internal = interface_cast<IEngineValueInternal>(value);
    ASSERT_TRUE(internal);
    // Invalidate state
    EnginePropertyParams p;
    p.pushValueToEngineDirectly = true;
    EXPECT_TRUE(internal->SetPropertyParams(p));
    // Calls should start failing now
    EXPECT_EQ(value->Sync(EngineSyncDirection::AUTO), AnyReturn::INVALID_ARGUMENT);
    auto any = Any<int>(0);
    EXPECT_EQ(value->SetValue(any), AnyReturn::INVALID_ARGUMENT);
    EXPECT_EQ(value->GetValue().GetClassId(), ObjectId {});
}

/**
 * @tc.name: SetValueDirect
 * @tc.desc: Tests engine value SetValue when pushValueToEngineDirectly = true.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, SetValueDirect, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop2::EngineTestProp> cman { prop2::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(ClassId::EngineValueManager);
    ASSERT_TRUE(manager);
    EXPECT_TRUE(manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));

    Any<int> v(5);
    Any<float> f(1.f);
    auto value = manager->GetEngineValue("value");
    ASSERT_TRUE(value);
    auto internal = interface_cast<IEngineValueInternal>(value);
    ASSERT_TRUE(internal);
    auto p = internal->GetPropertyParams();
    p.pushValueToEngineDirectly = true;
    EXPECT_TRUE(internal->SetPropertyParams(p));
    EXPECT_TRUE(value->SetValue(v));
    EXPECT_FALSE(value->SetValue(f));
}

/**
 * @tc.name: SetValueDirect
 * @tc.desc: Test engine value manager value construction with raw property handle pointer.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, ConstructPtr, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop2::EngineTestProp> cman { prop2::ENGINE_TESTPROP_METADATA };

    auto manager = GetObjectRegistry().Create<IEngineValueManager>(ClassId::EngineValueManager);
    ASSERT_TRUE(manager);
    EnginePropertyHandle eph;
    EXPECT_FALSE(manager->ConstructValues(eph, {}));
    EXPECT_FALSE(manager->ConstructValues(nullptr, {}));
    auto ph = EnginePropertyHandle { &cman, cman.entityRef };
    EXPECT_TRUE(manager->ConstructValues(ph.Handle(), {}));
    auto count = manager->GetAllEngineValues().size();
    EXPECT_GT(count, 0);
    EXPECT_TRUE(manager->ConstructValues(ph.Handle(), {}));
    EXPECT_EQ(count, manager->GetAllEngineValues().size());
    EXPECT_TRUE(manager->ConstructValue(EnginePropertyParams {}, {}));
    EXPECT_FALSE(manager->ConstructAllProperties().empty());
}

/**
 * @tc.name: SetValueDirect
 * @tc.desc: Test engine value manager notifications.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineManagerTest, Notification, testing::ext::TestSize.Level1)
{
    auto queue = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto queueId = interface_cast<IObjectInstance>(queue)->GetInstanceId().ToUid();
    auto& tr = GetTaskQueueRegistry();
    tr.RegisterTaskQueue(queue, queueId);

    TestComponentManager<prop2::EngineTestProp> cman { prop2::ENGINE_TESTPROP_METADATA };

    uint32_t onChangedCount = 0;
    {
        auto manager = GetObjectRegistry().Create<IEngineValueManager>(ClassId::EngineValueManager);
        ASSERT_TRUE(manager);
        manager->SetNotificationQueue(queue);

        EXPECT_TRUE(manager->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));
        auto p = manager->ConstructProperty<int>("value");

        ASSERT_TRUE(p);
        p->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&]() { onChangedCount++; }));
        p->SetValue(42);
        EXPECT_EQ(onChangedCount, 1);
        manager->Sync(EngineSyncDirection::AUTO);

        EXPECT_EQ(onChangedCount, 1);
        queue->ProcessTasks();
        EXPECT_EQ(onChangedCount, 2);

        // Set + sync again but leave the notify pending in the queue
        p->SetValue(43);
        EXPECT_EQ(onChangedCount, 3);
        manager->Sync(EngineSyncDirection::AUTO);
        // Expect EngineValueManager to cancel the task at destruction
    }
    EXPECT_EQ(onChangedCount, 3);
    queue->ProcessTasks();
    EXPECT_EQ(onChangedCount, 3);

    tr.UnregisterTaskQueue(queueId);
}

} // namespace UTest
META_END_NAMESPACE()
