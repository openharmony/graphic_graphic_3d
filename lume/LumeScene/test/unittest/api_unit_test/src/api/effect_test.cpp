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

#include <gmock/gmock.h>

#include <core/property/property_types.h>

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(float);
DECLARE_PROPERTY_TYPE(bool);
CORE_END_NAMESPACE()

#include <scene/api/ecs_scene.h>
#include <scene/api/effect.h>
#include <scene/api/scene.h>
#include <scene/interface/resource/types.h>

#include <3d/ecs/components/post_process_effect_component.h>
#include <core/property/property_handle_util.h>

#include <meta/api/util.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_EffectTest : public ScenePluginTest {
    using Super = ScenePluginTest;
    void SetUp() override
    {
        Super::SetUp();

        scene_ = Scene(CreateEmptyScene());
        ASSERT_TRUE(scene_);
        parent_ = scene_.GetResourceFactory().CreateNode("//root");
        ASSERT_TRUE(parent_);
    }

    void TearDown() override
    {
        scene_.Release();
        Super::TearDown();
    }

protected:
    Node& GetParent()
    {
        return parent_;
    }
    Scene& GetScene()
    {
        return scene_;
    }
    BASE_NS::string GetExpectedPath(BASE_NS::string_view nodeName)
    {
        return "//root/" + nodeName;
    }
    void CheckShader(Shader& shader)
    {
        EXPECT_TRUE(shader);
        EXPECT_EQ(shader.GetResourceType(), ::SCENE_NS::ClassId::ShaderResource.Id().ToUid());
    }

    static constexpr auto VALID_SHADER_URI = "test://shaders/test.shader";
    static constexpr auto INVALID_SHADER_URI = "test://this/is/a/nonexistent.shader";

private:
    Node parent_ { nullptr };
    Scene scene_ { nullptr };
};

using META_NS::Async;
using META_NS::CreateNew;
using META_NS::GetValue;
using META_NS::SetValue;

/**
 * @tc.name: InvalidEffet
 * @tc.desc: Tests for Invalid Effet. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EffectTest, InvalidEffet, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();

    EffectResourceParameters p;
    ASSERT_FALSE(factory.CreateEffect(p));

    p.effectClassId = ::SCENE_NS::ClassId::Material;
    ASSERT_FALSE(factory.CreateEffect(p));
}

/**
 * @tc.name: ChangeEffectClassId
 * @tc.desc: Tests for Change Effect Class Id. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EffectTest, ChangeEffectClassId, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    EffectResourceParameters p;
    p.effectClassId = DefaultEffects::LENS_FLARE_EFFECT_ID;
    auto effect = factory.CreateEffect(p);
    ASSERT_TRUE(effect);
    EXPECT_EQ(effect.GetEffectClassId(), DefaultEffects::LENS_FLARE_EFFECT_ID);
    // Effect was already initialized so this should fail
    EXPECT_FALSE(effect.GetPtr<IEffect>()->InitializeEffect(scene, DefaultEffects::BLOOM_EFFECT_ID).GetResult());
    EXPECT_EQ(effect.GetEffectClassId(), DefaultEffects::LENS_FLARE_EFFECT_ID);
    EXPECT_TRUE(effect.GetPtr<IEffect>()->GetEffect());
}

/**
 * @tc.name: Effect
 * @tc.desc: Tests for Effect. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EffectTest, Effect, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();

    EffectResourceParameters p;
    p.effectClassId = DefaultEffects::LENS_FLARE_EFFECT_ID;
    auto effect = factory.CreateEffect(p);
    ASSERT_TRUE(effect);
    EXPECT_EQ(effect.GetEffectClassId(), ::SCENE_NS::DefaultEffects::LENS_FLARE_EFFECT_ID);

    auto intensity = effect.GetProperty<float>("intensity");
    ASSERT_TRUE(intensity);
    EXPECT_TRUE(effect.GetProperty<BASE_NS::Math::Vec3>("flarePos"));
    EXPECT_EQ(META_NS::GetValue(intensity), 1.f);
    EXPECT_EQ(
        META_NS::GetValue(effect.GetProperty<BASE_NS::Math::Vec3>("flarePos")), BASE_NS::Math::Vec3(0.f, 0.f, 0.f));
    EXPECT_FALSE(effect.GetEnabled());

    META_NS::SetValue(intensity, 0.5f);
    effect.SetEnabled(true);
    UpdateScene();
    auto re = effect.GetPtr<IEffect>()->GetEffect();
    ASSERT_TRUE(re);
    auto props = re->GetProperties();
    ASSERT_TRUE(props);

    auto ph = CORE_NS::MakeScopedHandle<const float>(props, "intensity");
    ASSERT_TRUE(ph);
    EXPECT_EQ(*ph, 0.5f);

    auto pe = CORE_NS::MakeScopedHandle<const bool>(props, "enabled");
    ASSERT_TRUE(ph);
    EXPECT_EQ(*pe, true);
}

/**
 * @tc.name: KnownEffectType
 * @tc.desc: Tests for Known Effect Type. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EffectTest, KnownEffectType, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();

    EffectResourceParameters p;
    p.effectClassId = DefaultEffects::BLOOM_EFFECT_ID;
    auto effect = factory.CreateEffect(p);

    auto th = effect.GetProperty<float>("thresholdHard");
    EXPECT_TRUE(th);
    auto ts = effect.GetProperty<float>("thresholdSoft");
    EXPECT_TRUE(ts);
}

/**
 * @tc.name: CameraEffect
 * @tc.desc: Tests for Camera Effect. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_EffectTest, CameraEffect, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto camera = factory.CreateCameraNode({ "//camera" });
    ASSERT_TRUE(camera);

    EcsScene ecss(scene);
    EcsObject ecso(camera);
    ASSERT_TRUE(ecss);
    ASSERT_TRUE(ecso);
    auto entity = ecso.GetEntity();

    auto hasEffectComponent = [&]() {
        auto ppm = ecss.GetEcs()->GetComponentManager(CORE3D_NS::IPostProcessEffectComponentManager::UID);
        return ppm->HasComponent(entity);
    };

    auto getEffects = [&]() {
        auto ppm = static_cast<CORE3D_NS::IPostProcessEffectComponentManager*>(
            ecss.GetEcs()->GetComponentManager(CORE3D_NS::IPostProcessEffectComponentManager::UID));
        auto ef = ppm->Read(entity);
        return ef ? ef->effects : BASE_NS::vector<RENDER_NS::IRenderPostProcess::Ptr> {};
    };

    EXPECT_FALSE(hasEffectComponent());
    EXPECT_TRUE(getEffects().empty());

    auto lf = factory.CreateEffect({ "", DefaultEffects::LENS_FLARE_EFFECT_ID });
    auto bl = factory.CreateEffect({ "", DefaultEffects::BLOOM_EFFECT_ID });
    ASSERT_TRUE(lf);
    ASSERT_TRUE(bl);

    camera.SetEffects({ lf, bl });
    BASE_NS::vector<RENDER_NS::IRenderPostProcess::Ptr> renderEffects { lf.GetPtr<IEffect>()->GetEffect(),
        bl.GetPtr<IEffect>()->GetEffect() };

    UpdateScene();
    EXPECT_TRUE(hasEffectComponent());
    auto ecsEffects = getEffects();
    ASSERT_EQ(ecsEffects.size(), 2);
    EXPECT_EQ(ecsEffects[0], renderEffects[0]);
    EXPECT_EQ(ecsEffects[1], renderEffects[1]);

    camera.SetEffects({});
    UpdateScene();
    EXPECT_FALSE(hasEffectComponent());
    EXPECT_TRUE(getEffects().empty());
}

} // namespace UTest
SCENE_END_NAMESPACE()
