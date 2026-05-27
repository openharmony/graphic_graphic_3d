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

#include <scene/ext/intf_component.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_transform.h>

#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/transform_component.h>
#include <render/datastore/render_data_store_render_pods.h>

#include <meta/api/util.h>
#include <meta/interface/intf_object.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginGenericComponentTest : public ScenePluginTest {
public:
    META_NS::IProperty::Ptr GetProperty(IGenericComponent::Ptr component, BASE_NS::string propName) const
    {
        const auto ecsObj = interface_cast<IEcsObjectAccess>(component)->GetEcsObject();
        const auto evManager = ecsObj->GetEngineValueManager();
        return META_NS::PropertyFromEngineValue(propName, evManager->GetEngineValue(propName));
    }
};

/**
 * @tc.name: ReadFromUnregisteredComponent
 * @tc.desc: Tests for Read From Unregistered Component. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginGenericComponentTest, ReadFromUnregisteredComponent, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    scene->GetInternalScene()->UnregisterComponent(CORE3D_NS::ITransformComponentManager::UID);
    const auto node = scene->CreateNode("//test").GetResult();

    const auto att = interface_cast<META_NS::IAttach>(node);
    ASSERT_TRUE(att);
    EXPECT_TRUE(att->GetAttachments<ITransform>().empty());
    const auto component = att->GetAttachmentContainer()->FindByName<IGenericComponent>("TransformComponent");
    ASSERT_TRUE(component);

    EXPECT_TRUE(interface_cast<IComponent>(component)->PopulateAllProperties());
    const auto prop = GetProperty(component, "TransformComponent.scale");
    ASSERT_TRUE(prop);

    const auto magicScale = BASE_NS::Math::Vec3{1.2345f, 2.3456f, 3.4567f};
    auto scale = BASE_NS::Math::Vec3{magicScale};
    prop->GetValue().GetValue(scale);
    EXPECT_NE(scale, magicScale);
}

/**
 * @tc.name: EnumerateProperties
 * @tc.desc: Text EnumerateProperties API
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginGenericComponentTest, EnumerateProperties, testing::ext::TestSize.Level1)
{
    auto getEntity = [](const BASE_NS::shared_ptr<CORE_NS::IInterface>& ptr) {
        auto ecso = interface_cast<IEcsObjectAccess>(ptr);
        return ecso ? ecso->GetEcsObject()->GetEntity() : CORE_NS::Entity{};
    };

    auto getPPManager = [](const BASE_NS::shared_ptr<CORE_NS::IInterface>& ptr) {
        CORE3D_NS::IPostProcessComponentManager* mgr{};
        if (auto ecsoa = interface_cast<IEcsObjectAccess>(ptr)) {
            if (auto ecso = ecsoa->GetEcsObject()) {
                auto ecs = ecso->GetScene()->GetEcsContext().GetNativeEcs();
                mgr = static_cast<CORE3D_NS::IPostProcessComponentManager*>(
                    ecs->GetComponentManager(CORE3D_NS::IPostProcessComponentManager::UID));
            }
        }
        return mgr;
    };

    auto scene = CreateEmptyScene();
    auto node = interface_pointer_cast<INode>(scene->CreateNode("//test", ClassId::CameraNode).GetResult());

    auto component = scene->CreateComponent(node, "PostProcessComponent").GetResult();
    ASSERT_TRUE(component);
    auto entity = getEntity(component);
    ASSERT_TRUE(CORE_NS::EntityUtil::IsValid(entity));

    // Enumerate all properties
    META_NS::EnginePropertyInfoConfig config;
    config.traverseMemberProperties = true;
    config.includeStructs = false;
    auto result = component->EnumerateProperties(config);
    ASSERT_FALSE(result.empty());

    auto notExpected = "PostProcessComponent.motionBlurConfiguration";
    auto notExpected2 = "PostProcessComponent.fxaaConfiguration.sharpness.value";
    auto expected = "PostProcessComponent.motionBlurConfiguration.velocityCoefficient";
    auto expected2 = "PostProcessComponent.fxaaConfiguration.sharpness";
    uint32_t found = 0;
    uint32_t notFound = 0;
    for (auto&& i : result) {
        if (i.name == expected || i.name == expected2) {
            found++;
        }
        if (i.name == notExpected || i.name == notExpected2) {
            notFound++;
        }
        if (found == 2u) {
            break;
        }
    }
    ASSERT_EQ(found, 2u);
    ASSERT_EQ(notFound, 0u);
    auto meta = interface_cast<META_NS::IMetadata>(component);
    ASSERT_TRUE(meta);
    auto resolved = meta->GetProperty<float>(expected);
    ASSERT_TRUE(resolved);
    EXPECT_EQ(META_NS::GetValue(resolved), 1.f);

    auto resolved2 = meta->GetProperty<int64_t>(expected2);
    ASSERT_TRUE(resolved2);
    EXPECT_EQ(META_NS::GetValue(resolved2), int64_t(RENDER_NS::FxaaConfiguration::Sharpness::SHARP));

    META_NS::SetValue(resolved, 42.f);
    META_NS::SetValue(resolved2, int64_t(RENDER_NS::FxaaConfiguration::Sharpness::MEDIUM));

    UpdateScene();
    auto* ppm = getPPManager(component);
    ASSERT_TRUE(ppm);
    auto rh = ppm->Read(entity);
    ASSERT_TRUE(rh);
    EXPECT_EQ(rh->motionBlurConfiguration.velocityCoefficient, 42.f);
    EXPECT_EQ(rh->fxaaConfiguration.sharpness, RENDER_NS::FxaaConfiguration::Sharpness::MEDIUM);
}

}  // namespace UTest

SCENE_END_NAMESPACE()
