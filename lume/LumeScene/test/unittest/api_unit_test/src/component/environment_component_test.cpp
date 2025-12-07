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

#include <scene/ext/component_util.h>
#include <scene/interface/intf_scene.h>

#include <3d/ecs/components/environment_component.h>
#include <3d/namespace.h>
#include <base/math/quaternion.h>

#include "scene/interface/intf_image.h"
#include "scene/scene_component_test.h"
#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginEnvironmentComponentTest
    : public ScenePluginComponentTest<CORE3D_NS::IEnvironmentComponentManager> {
protected:
    template<class T>
    void TestIEnvironmentPropertyGetters(T* env)
    {
        EXPECT_TRUE(env->Background());
        EXPECT_TRUE(env->IndirectDiffuseFactor());
        EXPECT_TRUE(env->IndirectSpecularFactor());
        EXPECT_TRUE(env->EnvMapFactor());
        EXPECT_TRUE(env->RadianceImage());
        EXPECT_TRUE(env->RadianceCubemapMipCount());
        EXPECT_TRUE(env->EnvironmentImage());
        EXPECT_TRUE(env->EnvironmentMapLodLevel());
        EXPECT_TRUE(env->IrradianceCoefficients());
        EXPECT_TRUE(env->EnvironmentRotation());
    }

    void TestIEnvironmentFunctions(IEnvironment* env)
    {
        ASSERT_TRUE(env);
        TestIEnvironmentPropertyGetters<const IEnvironment>(env);
        TestIEnvironmentPropertyGetters<IEnvironment>(env);
        TestIEnvironmentPropertyGetters<const IEnvironment>(env);
        TestIEnvironmentPropertyGetters<IEnvironment>(env);
        TestInvalidComponentPropertyInit(env);
    }
};

/**
 * @tc.name: Members
 * @tc.desc: Tests for Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginEnvironmentComponentTest, Members, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(AddComponent<CORE3D_NS::IEnvironmentComponentManager>(node));
    SetComponent(node, "EnvironmentComponent");

    BASE_NS::Math::Vec4 vec4 { 1.0f, 2.0f, 3.0f, 4.0f };
    BASE_NS::Math::Quat quat { 1.0f, 2.0f, 3.0f, 4.0f };

    TestEngineProperty<EnvBackgroundType>("Background", EnvBackgroundType::CUBEMAP, nativeComponent.background);
    TestEngineProperty<BASE_NS::Math::Vec4>("IndirectDiffuseFactor", vec4, nativeComponent.indirectDiffuseFactor);
    TestEngineProperty<BASE_NS::Math::Vec4>("IndirectSpecularFactor", vec4, nativeComponent.indirectSpecularFactor);
    TestEngineProperty<BASE_NS::Math::Vec4>("EnvMapFactor", vec4, nativeComponent.envMapFactor);
    TestEngineProperty<BASE_NS::Math::Quat>("EnvironmentRotation", quat, nativeComponent.environmentRotation);
    TestEngineProperty<uint32_t>("RadianceCubemapMipCount", 1, nativeComponent.radianceCubemapMipCount);
    TestEngineProperty<float>("EnvironmentMapLodLevel", 0.5, nativeComponent.envMapLodLevel);

    auto bitmap = CreateTestBitmap();
    for (BASE_NS::string_view propName : { "RadianceImage", "EnvironmentImage" }) {
        auto prop = GetProperty<IImage::Ptr>(propName);
        prop->SetValue(bitmap);
        UpdateComponentMembers();
        auto propValue = prop->GetValue();
        EXPECT_EQ(propValue, bitmap);
    }
    // fix?
    //    auto entity = interface_cast<IEcsResource>(bitmap)->GetEntity();
    //    EXPECT_EQ(nativeComponent.radianceCubemap, entity);
    //    EXPECT_EQ(nativeComponent.envMap, entity);

    auto prop = GetArrayProperty<BASE_NS::Math::Vec3>("IrradianceCoefficients");
    BASE_NS::Math::Vec3 coeffs[9] = {
        { 1.1f, 1.2f, 1.3f }, { 2.1f, 2.2f, 2.3f }, { 3.1f, 3.2f, 3.3f }, //
        { 4.1f, 4.2f, 4.3f }, { 5.1f, 5.2f, 5.3f }, { 6.1f, 6.2f, 6.3f }, //
        { 7.1f, 7.2f, 7.3f }, { 8.1f, 8.2f, 8.3f }, { 9.1f, 9.2f, 9.3f }  //
    };
    prop->SetValue(coeffs);
    UpdateComponentMembers();
    auto propValues = prop->GetValue();
    ASSERT_EQ(propValues.size(), 9);
    for (int i = 0; i < 9; ++i) {
        ASSERT_EQ(propValues[i], nativeComponent.irradianceCoefficients[i]);
    }
}

/**
 * @tc.name: Functions
 * @tc.desc: Tests for Functions. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginEnvironmentComponentTest, Functions, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(AddComponent<CORE3D_NS::IEnvironmentComponentManager>(node));
    SetComponent(node, "EnvironmentComponent");

    TestIEnvironmentFunctions(interface_cast<IEnvironment>(object));
}

} // namespace UTest

SCENE_END_NAMESPACE()
