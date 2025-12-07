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

#include <scene/interface/intf_light.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>

#include <3d/ecs/components/light_component.h>

#include "scene/scene_component_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginLightComponentTest : public ScenePluginComponentTest<CORE3D_NS::ILightComponentManager> {
protected:
    template<class T>
    void TestILightPropertyGetters(T* light)
    {
        EXPECT_TRUE(light->Color());
        EXPECT_TRUE(light->Intensity());
        EXPECT_TRUE(light->NearPlane());
        EXPECT_TRUE(light->ShadowEnabled());
        EXPECT_TRUE(light->ShadowStrength());
        EXPECT_TRUE(light->ShadowDepthBias());
        EXPECT_TRUE(light->ShadowNormalBias());
        EXPECT_TRUE(light->SpotInnerAngle());
        EXPECT_TRUE(light->SpotOuterAngle());
        EXPECT_TRUE(light->Type());
        EXPECT_TRUE(light->AdditionalFactor());
        EXPECT_TRUE(light->LightLayerMask());
        EXPECT_TRUE(light->ShadowLayerMask());
    }

    void TestILightFunctions(ILight* light)
    {
        ASSERT_TRUE(light);
        TestILightPropertyGetters<const ILight>(light);
        TestILightPropertyGetters<ILight>(light);
        TestInvalidComponentPropertyInit(light);
    }
};

/**
 * @tc.name: Members
 * @tc.desc: Tests for Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginLightComponentTest, Members, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test", ClassId::LightNode).GetResult();
    SetComponent(node, "LightComponent");

    TestEngineProperty<BASE_NS::Color>("Color", { 0.5, 0.6, 0.7, 0.0 }, nativeComponent.color);
    TestEngineProperty<float>("Intensity", 0.5, nativeComponent.intensity);
    TestEngineProperty<float>("NearPlane", 1.2345f, nativeComponent.nearPlane);
    TestEngineProperty<bool>("ShadowEnabled", true, nativeComponent.shadowEnabled);
    TestEngineProperty<float>("ShadowStrength", 0.5, nativeComponent.shadowStrength);
    TestEngineProperty<float>("ShadowDepthBias", 0.5, nativeComponent.shadowDepthBias);
    TestEngineProperty<float>("ShadowNormalBias", 0.5, nativeComponent.shadowNormalBias);
    TestEngineProperty<float>("SpotInnerAngle", 0.5, nativeComponent.spotInnerAngle);
    TestEngineProperty<float>("SpotOuterAngle", 0.5, nativeComponent.spotOuterAngle);
    TestEngineProperty<LightType>("Type", LightType::SPOT, nativeComponent.type);
    TestEngineProperty<BASE_NS::Math::Vec4>("AdditionalFactor", { 0.5, 1, 2, 0.6 }, nativeComponent.additionalFactor);
    TestEngineProperty<uint64_t>("LightLayerMask", 4, nativeComponent.lightLayerMask);
    TestEngineProperty<uint64_t>("ShadowLayerMask", 128, nativeComponent.shadowLayerMask);
}

/**
 * @tc.name: Functions
 * @tc.desc: Tests for Functions. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginLightComponentTest, Functions, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test", ClassId::LightNode).GetResult();
    SetComponent(node, "LightComponent");

    TestILightFunctions(interface_cast<ILight>(node));
    TestILightFunctions(interface_cast<ILight>(object));
    TestILightFunctions(interface_cast<ILight>(node));
    TestILightFunctions(interface_cast<ILight>(object));
}

} // namespace UTest

SCENE_END_NAMESPACE()
