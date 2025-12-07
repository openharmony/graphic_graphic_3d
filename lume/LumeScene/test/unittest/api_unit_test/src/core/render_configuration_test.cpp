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

#include <core/property/property_types.h>

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(BASE_NS::string);
CORE_END_NAMESPACE()

#include <scene/ext/intf_create_entity.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>

#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <core/property/property_handle_util.h>

#include <meta/api/metadata_util.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginRenderConfigurationTest : public ScenePluginTest {
public:
};

/**
 * @tc.name: Members
 * @tc.desc: Tests for Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginRenderConfigurationTest, Members, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto config = scene->RenderConfiguration()->GetValue();
    ASSERT_TRUE(config);

    auto env = scene->CreateObject<IEnvironment>(ClassId::Environment).GetResult();
    env->EnvironmentMapLodLevel()->SetValue(1.0);

    config->RenderNodeGraphUri()->SetValue("plapla");
    config->PostRenderNodeGraphUri()->SetValue("plapla");
    config->Environment()->SetValue(env);

    UpdateScene();

    EXPECT_EQ(config->RenderNodeGraphUri()->GetValue(), "plapla");
    EXPECT_EQ(config->PostRenderNodeGraphUri()->GetValue(), "plapla");
    auto ee = config->Environment()->GetValue();
    ASSERT_EQ(ee, env);
    EXPECT_EQ(ee->EnvironmentMapLodLevel()->GetValue(), 1.0);
}

/**
 * @tc.name: InvalidState
 * @tc.desc: Tests RenderConfiguration invalid state
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginRenderConfigurationTest, InvalidState, testing::ext::TestSize.Level1)
{
    auto& reg = META_NS::GetObjectRegistry();
    auto config = reg.Create(ClassId::RenderConfiguration);
    auto create = interface_cast<ICreateEntity>(config);
    ASSERT_TRUE(create);
    EXPECT_EQ(create->CreateEntity({}), CORE_NS::Entity {});
}

/**
 * @tc.name: ShadowResolution
 * @tc.desc: Test for shadow resolution override.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginRenderConfigurationTest, ShadowResolution, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto is = scene->GetInternalScene();
    ASSERT_TRUE(is);

    auto config = scene->RenderConfiguration()->GetValue();
    ASSERT_TRUE(config);

    BASE_NS::refcnt_ptr<CORE3D_NS::IRenderDataStoreDefaultLight> store;
    auto ecs = is->GetEcsContext().GetNativeEcs();
    if (auto preprocessor = CORE_NS::GetSystem<CORE3D_NS::IRenderPreprocessorSystem>(*ecs)) {
        auto systemProperties = preprocessor->GetProperties();
        if (auto dsLightName = CORE_NS::GetPropertyValue<BASE_NS::string>(systemProperties, "dataStoreLight");
            !dsLightName.empty()) {
            auto& manager = is->GetRenderContext().GetRenderDataStoreManager();
            store =
                BASE_NS::refcnt_ptr<CORE3D_NS::IRenderDataStoreDefaultLight>(manager.GetRenderDataStore(dsLightName));
        }
    }
    ASSERT_TRUE(store);

    auto getShadowResolution = [&]() { return store->GetShadowQualityResolution(); };
    auto setShadowResolution = [&](const BASE_NS::Math::UVec2& resolution) {
        META_NS::SetValue(config->ShadowResolution(), resolution);
        UpdateSceneAndRenderFrame(scene);
    };

    CORE3D_NS::IRenderDataStoreDefaultLight::ShadowQualityResolutions defaults;
    BASE_NS::Math::UVec2 setLow(128, 128);
    EXPECT_GT(defaults.normal.x, setLow.x);
    EXPECT_GT(defaults.normal.y, setLow.y);

    // Expect normal by default
    EXPECT_EQ(getShadowResolution(), defaults.normal);

    EXPECT_FALSE(config->ShadowResolution()->IsValueSet());
    EXPECT_EQ(META_NS::GetValue(config->ShadowResolution()), defaults.normal);

    // If either component is 0, should revert to default
    setShadowResolution({ 128, 0 });
    EXPECT_EQ(getShadowResolution(), defaults.normal);
    setShadowResolution({ 0, 128 });
    EXPECT_EQ(getShadowResolution(), defaults.normal);
    setShadowResolution({ 0, 0 });
    EXPECT_EQ(getShadowResolution(), defaults.normal);

    // Override shadow map resolution with 128x128
    setShadowResolution(setLow);
    EXPECT_EQ(getShadowResolution(), setLow);

    // Change quality while override set, should not affect resolution (for now)
    META_NS::SetValue(config->ShadowQuality(), SceneShadowQuality::HIGH);
    UpdateScene();
    EXPECT_EQ(getShadowResolution(), setLow);

    // Reset override resolution, shadow resolution should now switch to high (since we changed quality above)
    META_NS::ResetValue(config->ShadowResolution());
    UpdateScene();
    EXPECT_EQ(getShadowResolution(), defaults.high);

    META_NS::SetValue(config->ShadowQuality(), SceneShadowQuality::LOW);
    EXPECT_EQ(META_NS::GetValue(config->ShadowResolution()), defaults.low);
    UpdateScene();
    EXPECT_EQ(getShadowResolution(), defaults.low);

    META_NS::SetValue(config->ShadowQuality(), SceneShadowQuality::NORMAL);
    EXPECT_EQ(META_NS::GetValue(config->ShadowResolution()), defaults.normal);
    UpdateScene();
    EXPECT_EQ(getShadowResolution(), defaults.normal);

    META_NS::SetValue(config->ShadowQuality(), SceneShadowQuality::HIGH);
    EXPECT_EQ(META_NS::GetValue(config->ShadowResolution()), defaults.high);
    UpdateScene();
    EXPECT_EQ(getShadowResolution(), defaults.high);

    META_NS::SetValue(config->ShadowQuality(), SceneShadowQuality::ULTRA);
    EXPECT_EQ(META_NS::GetValue(config->ShadowResolution()), defaults.ultra);
    UpdateScene();
    EXPECT_EQ(getShadowResolution(), defaults.ultra);
}

} // namespace UTest

SCENE_END_NAMESPACE()
