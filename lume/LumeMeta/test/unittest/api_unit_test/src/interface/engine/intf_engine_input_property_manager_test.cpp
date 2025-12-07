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

#include <meta/interface/builtin_objects.h>
#include <meta/interface/engine/intf_engine_input_property_manager.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_registry.h>

#include "helpers/testing_objects.h"
#include "helpers/util.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

META_BEGIN_NAMESPACE()
namespace UTest {

class API_EngineInputPropertyManagerTest : public ::testing::Test {
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
 * @tc.name: Property
 * @tc.desc: Tests for Property. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineInputPropertyManagerTest, Property, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager =
        GetObjectRegistry().Create<IEngineInputPropertyManager>(META_NS::ClassId::EngineInputPropertyManager);
    ASSERT_TRUE(manager);

    EXPECT_TRUE(manager->GetValueManager()->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));

    auto p = manager->ConstructProperty<int32_t>("value");
    ASSERT_TRUE(p);

    p->SetValue(2);
    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 0));
    EXPECT_TRUE(manager->Sync());
    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 2));

    auto source = ConstructProperty<int32_t>("source");
    p->SetBind(source);
    source->SetValue(3);
    EXPECT_TRUE(manager->Sync());
    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 3));

    auto other = ConstructProperty<int32_t>("source");
    manager->TieProperty(other, "value");
    p->SetValue(1);
    EXPECT_TRUE(manager->Sync());
    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 3));

    other->SetValue(7);
    EXPECT_TRUE(manager->Sync());
    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 7));
}

/**
 * @tc.name: PropertyExceptions
 * @tc.desc: Test exceptional cases for property construction.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineInputPropertyManagerTest, PropertyExceptions, testing::ext::TestSize.Level1)
{
    static constexpr BASE_NS::string_view PNAME = "floats";
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };
    auto manager =
        GetObjectRegistry().Create<IEngineInputPropertyManager>(META_NS::ClassId::EngineInputPropertyManager);
    ASSERT_TRUE(manager);
    EXPECT_TRUE(manager->GetValueManager()->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));
    auto floats = manager->ConstructProperty(PNAME);
    ASSERT_TRUE(floats);
    // Again
    EXPECT_EQ(manager->ConstructProperty(PNAME), floats);
    EXPECT_TRUE(manager->TieProperty(floats, ""));
    // Again
    EXPECT_TRUE(manager->TieProperty(floats, ""));
    // Property not in manager
    auto newProp = META_NS::ConstructProperty<int>("NewProp", 0);
    EXPECT_FALSE(manager->TieProperty(newProp, ""));
    // Remove and reconstruct
    manager->RemoveProperty(PNAME);
    auto floats2 = manager->ConstructProperty(PNAME);
    ASSERT_TRUE(floats2);
    EXPECT_NE(floats2, floats);
}

/**
 * @tc.name: GetAllProperties
 * @tc.desc: Test EngineInputPropertyManager::GetAllProperties.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineInputPropertyManagerTest, GetAllProperties, testing::ext::TestSize.Level1)
{
    static constexpr BASE_NS::string_view PNAME = "floats";
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };
    auto manager =
        GetObjectRegistry().Create<IEngineInputPropertyManager>(META_NS::ClassId::EngineInputPropertyManager);
    ASSERT_TRUE(manager);
    EXPECT_TRUE(manager->GetValueManager()->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));
    auto all = manager->GetAllProperties();
    EXPECT_TRUE(all.empty());
    auto floats = manager->ConstructProperty(PNAME);
    EXPECT_TRUE(floats);
    all = manager->GetAllProperties();
    ASSERT_EQ(all.size(), 1);
    EXPECT_EQ(all[0], floats);
}

/**
 * @tc.name: PopulateMetadata
 * @tc.desc: Tests for Populate Metadata. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineInputPropertyManagerTest, PopulateMetadata, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager =
        GetObjectRegistry().Create<IEngineInputPropertyManager>(META_NS::ClassId::EngineInputPropertyManager);
    ASSERT_TRUE(manager);
    EXPECT_TRUE(manager->GetValueManager()->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));

    auto m = GetObjectRegistry().Create<IMetadata>(ClassId::Object);
    ASSERT_TRUE(m);

    EXPECT_TRUE(manager->PopulateProperties(*m));

    auto cont = interface_cast<IAttach>(m)->GetAttachmentContainer(true);
    EXPECT_EQ(cont->GetSize(), prop1::TEST_PROPERTY_COUNT);
    auto p = m->GetProperty<int32_t>("value");
    ASSERT_TRUE(p);
    auto fp = m->GetArrayProperty<float>("floats");
    ASSERT_TRUE(fp);

    p->SetValue(2);
    EXPECT_TRUE(manager->Sync());
    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 2));
}

/**
 * @tc.name: PopulateMetadataTypeClash
 * @tc.desc: Tests for Populate Metadata with an invalid type.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineInputPropertyManagerTest, PopulateMetadataTypeClash, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager =
        GetObjectRegistry().Create<IEngineInputPropertyManager>(META_NS::ClassId::EngineInputPropertyManager);
    ASSERT_TRUE(manager);
    EXPECT_TRUE(manager->GetValueManager()->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));

    auto m = GetObjectRegistry().Create<IMetadata>(ClassId::Object);
    ASSERT_TRUE(m);

    auto sameTypeP = ConstructProperty<int>("value", 0);
    auto differentTypeP = ConstructProperty<int>("floats", 0);
    EXPECT_TRUE(m->AddProperty(sameTypeP));
    EXPECT_TRUE(m->AddProperty(differentTypeP));

    EXPECT_TRUE(manager->PopulateProperties(*m));
}

/**
 * @tc.name: DefaultValueFromEngineValue
 * @tc.desc: Tests for Default Value From Engine Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EngineInputPropertyManagerTest, DefaultValueFromEngineValue, testing::ext::TestSize.Level1)
{
    TestComponentManager<prop1::EngineTestProp> cman { prop1::ENGINE_TESTPROP_METADATA };

    auto manager =
        GetObjectRegistry().Create<IEngineInputPropertyManager>(META_NS::ClassId::EngineInputPropertyManager);
    ASSERT_TRUE(manager);
    EXPECT_TRUE(manager->GetValueManager()->ConstructValues(EnginePropertyHandle { &cman, cman.entityRef }));

    EXPECT_TRUE(CORE_NS::SetPropertyValue(*cman.GetData(0), "value", 2));
    manager->GetValueManager()->Sync(EngineSyncDirection::FROM_ENGINE);

    auto p = manager->ConstructProperty<int32_t>("value");
    ASSERT_TRUE(p);
    EXPECT_EQ(p->GetValue(), 2);
}

} // namespace UTest
META_END_NAMESPACE()
