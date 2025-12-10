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

#include <scene/ext/intf_ecs_object.h>

#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <core/plugin/intf_plugin_register.h>

#include "meta/interface/intf_class_registry.h"
#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginEcsObjectTest : public ScenePluginTest {
public:
    META_NS::IMetadata::Ptr metadata_;

    // Use node and name components, as they carry only a few properties.
    CORE_NS::IComponentManager* nameManager_ { nullptr };
    CORE_NS::IComponentManager* nodeManager_ { nullptr };
    const BASE_NS::vector<BASE_NS::string> nodeProps_ {
        "NodeComponent.parent",             //
        "NodeComponent.enabled",            //
        "NodeComponent.effectivelyEnabled", //
        "NodeComponent.exported",           //
        /*"NodeComponent.parent.id",          // by default only first level handled */
        "NodeComponent.sceneId" //
    };
    const BASE_NS::vector<BASE_NS::string> nameProps_ {
        "NameComponent.name" //
    };

    IEcsObject::Ptr CreateEcsObject(const BASE_NS::vector<CORE_NS::IComponentManager*>& managers) const
    {
        auto& context = scene->GetInternalScene()->GetEcsContext();
        const auto ecs = context.GetNativeEcs();

        const auto entity = ecs->GetEntityManager().Create();
        for (const auto& manager : managers) {
            manager->Create(entity);
        }

        return context.GetEcsObject(entity);
    }

    void CheckProps(const META_NS::IMetadata::Ptr& container, const BASE_NS::vector<BASE_NS::string>& expected) const
    {
        const auto allProps = container->GetProperties();
        EXPECT_EQ(allProps.size(), expected.size());
        for (const auto& name : expected) {
            EXPECT_TRUE(container->GetProperty(name, META_NS::MetadataQuery::EXISTING));
        }
    }

protected:
    void SetUp() override
    {
        ScenePluginTest::SetUp();

        CreateEmptyScene();
        auto ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
        nameManager_ = CORE_NS::GetManager<CORE3D_NS::INameComponentManager>(*ecs);
        nodeManager_ = CORE_NS::GetManager<CORE3D_NS::INodeComponentManager>(*ecs);
        metadata_ = META_NS::GetObjectRegistry().Create<META_NS::IMetadata>(META_NS::ClassId::Object);
        ASSERT_TRUE(metadata_ && nameManager_ && nodeManager_);
    }

    void TearDown() override
    {
        metadata_.reset();
        ScenePluginTest::TearDown();
    }
};

/**
 * @tc.name: AddAllEnginePropertiesCreation
 * @tc.desc: Tests for Add All Engine Properties Creation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginEcsObjectTest, AddAllEnginePropertiesCreation, testing::ext::TestSize.Level1)
{
    ASSERT_TRUE(CreateEcsObject({}));
    ASSERT_TRUE(CreateEcsObject({ nameManager_ }));
    ASSERT_TRUE(CreateEcsObject({ nodeManager_, nameManager_ }));
}

/**
 * @tc.name: AddAllEnginePropertiesSimple
 * @tc.desc: Tests for Add All Engine Properties Simple. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginEcsObjectTest, AddAllEnginePropertiesSimple, testing::ext::TestSize.Level1)
{
    auto obj = CreateEcsObject({ nameManager_ });
    EXPECT_TRUE(obj->AddAllProperties(metadata_, "NameComponent").GetResult());
    CheckProps(metadata_, nameProps_);
}

/**
 * @tc.name: AddAllEnginePropertiesNoComponents
 * @tc.desc: Tests for Add All Engine Properties No Components. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginEcsObjectTest, AddAllEnginePropertiesNoComponents, testing::ext::TestSize.Level1)
{
    auto obj = CreateEcsObject({});
    EXPECT_TRUE(obj->AddAllProperties(metadata_, "NameComponent").GetResult());
    CheckProps(metadata_, {});
}

/**
 * @tc.name: AddAllEnginePropertiesAlreadyExisting
 * @tc.desc: Tests for Add All Engine Properties Already Existing. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginEcsObjectTest, AddAllEnginePropertiesAlreadyExisting, testing::ext::TestSize.Level1)
{
    auto obj = CreateEcsObject({ nameManager_ });
    EXPECT_TRUE(obj->AddAllProperties(metadata_, "NameComponent").GetResult());
    EXPECT_TRUE(obj->AddAllProperties(metadata_, "NameComponent").GetResult());
    CheckProps(metadata_, nameProps_);
}

/**
 * @tc.name: AddAllEnginePropertiesAccumulate
 * @tc.desc: Tests for Add All Engine Properties Accumulate. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginEcsObjectTest, AddAllEnginePropertiesAccumulate, testing::ext::TestSize.Level1)
{
    auto obj = CreateEcsObject({ nodeManager_, nameManager_ });
    CheckProps(metadata_, {});

    EXPECT_TRUE(obj->AddAllProperties(metadata_, "NodeComponent").GetResult());
    auto expectedProps = nodeProps_;
    CheckProps(metadata_, expectedProps);

    EXPECT_TRUE(obj->AddAllProperties(metadata_, "NameComponent").GetResult());
    expectedProps.append(&nameProps_.front(), &nameProps_.back() + 1);
    CheckProps(metadata_, expectedProps);
}

/**
 * @tc.name: AddAllEnginePropertiesBadComponentName
 * @tc.desc: Tests for Add All Engine Properties Bad Component Name. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginEcsObjectTest, AddAllEnginePropertiesBadComponentName, testing::ext::TestSize.Level1)
{
    auto obj = CreateEcsObject({ nodeManager_, nameManager_ });
    EXPECT_FALSE(obj->AddAllProperties(metadata_, "NonExistentComponent").GetResult());
    CheckProps(metadata_, {});
}

/**
 * @tc.name: AddAllEnginePropertiesAtOnce
 * @tc.desc: Tests for Add All Engine Properties At Once. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginEcsObjectTest, AddAllEnginePropertiesAtOnce, testing::ext::TestSize.Level1)
{
    auto obj = CreateEcsObject({ nodeManager_, nameManager_ });
    EXPECT_TRUE(obj->AddAllProperties(metadata_, "").GetResult());
    auto expectedProps = nodeProps_;
    expectedProps.append(nameProps_.begin(), nameProps_.end());
    CheckProps(metadata_, expectedProps);
}

/**
 * @tc.name: AddAllEnginePropertiesAtOnceAccumulate
 * @tc.desc: Tests for Add All Engine Properties At Once Accumulate. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginEcsObjectTest, AddAllEnginePropertiesAtOnceAccumulate, testing::ext::TestSize.Level1)
{
    auto obj = CreateEcsObject({ nodeManager_, nameManager_ });
    EXPECT_TRUE(obj->AddAllProperties(metadata_, "NodeComponent").GetResult());
    auto expectedProps = nodeProps_;
    CheckProps(metadata_, expectedProps);

    EXPECT_TRUE(obj->AddAllProperties(metadata_, "").GetResult());
    expectedProps.append(nameProps_.begin(), nameProps_.end());
    CheckProps(metadata_, expectedProps);
}

/**
 * @tc.name: AddAllEnginePropertiesAtOnceNoComponents
 * @tc.desc: Tests for Add All Engine Properties At Once No Components. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginEcsObjectTest, AddAllEnginePropertiesAtOnceNoComponents, testing::ext::TestSize.Level1)
{
    auto obj = CreateEcsObject({});
    EXPECT_TRUE(obj->AddAllProperties(metadata_, "").GetResult());
    CheckProps(metadata_, {});
}

/**
 * @tc.name: ComponentDestroy
 * @tc.desc: Tests for Component Destroy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginEcsObjectTest, ComponentDestroy, testing::ext::TestSize.Level1)
{
    auto& plug = CORE_NS::GetPluginRegister();

    auto obj = CreateEcsObject({ nameManager_ });
    ASSERT_TRUE(obj);

    EXPECT_TRUE(obj->AddAllProperties(metadata_, "NameComponent").GetResult());
    auto p = metadata_->GetProperty<BASE_NS::string>("NameComponent.name");
    ASSERT_TRUE(p);
    auto sync = interface_cast<META_NS::IEnginePropertySync>(obj);
    ASSERT_TRUE(sync);

    p->SetValue("test");
    sync->SyncProperties();

    // this leaves things in very bad state, only for the test
    plug.UnregisterTypeInfo(
        CORE_NS::ComponentManagerTypeInfo { { CORE_NS::ComponentManagerTypeInfo::UID }, nameManager_->GetUid() });
    UpdateScene();

    p->SetValue("something else");
    sync->SyncProperties();
}

/**
 * @tc.name: TestAllEcsObjectAccess
 * @tc.desc: Tests for Test All Ecs Object Access. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginEcsObjectTest, TestAllEcsObjectAccess, testing::ext::TestSize.Level1)
{
    auto& reg = META_NS::GetObjectRegistry();
    const auto allAccess = reg.GetClassRegistry().GetAllTypes({ IEcsObjectAccess::UID });
    ASSERT_FALSE(allAccess.empty());

    for (const auto& t : allAccess) {
        auto clsi = t->GetClassInfo();
        auto object = reg.Create(clsi);
        if (clsi.id == META_NS::ObjectId("1eb22ecd-b8dd-4a1c-a491-1d4ed83c5179")) {
            // ClassId::GenericComponent should fail to build without valid build args
            ASSERT_FALSE(object);
            continue;
        }
        auto ecso = interface_cast<IEcsObjectAccess>(object);
        ASSERT_TRUE(ecso);
        EXPECT_FALSE(ecso->GetEcsObject());
        EXPECT_FALSE(ecso->SetEcsObject({}));
        EXPECT_EQ(clsi.name, object->GetClassName());
    }
}

/**
 * @tc.name: TestAllEcsObject
 * @tc.desc: Tests for Test All Ecs Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginEcsObjectTest, TestAllEcsObject, testing::ext::TestSize.Level1)
{
    auto& reg = META_NS::GetObjectRegistry();
    const auto allAccess = reg.GetClassRegistry().GetAllTypes({ IEcsObject::UID });
    ASSERT_FALSE(allAccess.empty());

    auto entityObject = CreateEcsObject({ nameManager_ });
    ASSERT_TRUE(entityObject);

    auto validArgs = reg.Create<META_NS::IMetadata>(META_NS::ClassId::Object);
    auto invalidArgs = reg.Create<META_NS::IMetadata>(META_NS::ClassId::Object);
    auto invalidArgsNoScene = reg.Create<META_NS::IMetadata>(META_NS::ClassId::Object);
    auto invalidArgsNoEntity = reg.Create<META_NS::IMetadata>(META_NS::ClassId::Object);
    auto sceneProp = META_NS::ConstructProperty<IInternalScene::Ptr>("Scene", scene->GetInternalScene());
    auto entityProp = META_NS::ConstructProperty<CORE_NS::Entity>("Entity", entityObject->GetEntity());

    validArgs->AddProperty(sceneProp);
    invalidArgsNoEntity->AddProperty(sceneProp);
    validArgs->AddProperty(entityProp);
    invalidArgsNoScene->AddProperty(entityProp);

    for (const auto& t : allAccess) {
        auto clsi = t->GetClassInfo();
        EXPECT_FALSE(reg.Create(clsi));
        EXPECT_FALSE(reg.Create(clsi, invalidArgs));
        EXPECT_FALSE(reg.Create(clsi, invalidArgsNoScene));
        EXPECT_FALSE(reg.Create(clsi, invalidArgsNoEntity));
        auto object = reg.Create(clsi, validArgs);
        ASSERT_TRUE(object);

        auto ecso = interface_cast<IEcsObject>(object);
        ASSERT_TRUE(ecso);
        EXPECT_EQ(clsi.name, object->GetClassName());
    }
}

} // namespace UTest

SCENE_END_NAMESPACE()
