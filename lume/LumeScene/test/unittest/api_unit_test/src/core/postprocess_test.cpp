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

#include <scene/api/scene.h>
#include <scene/ext/intf_engine_property_init.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>

#include <3d/ecs/components/post_process_component.h>

#include <meta/interface/intf_class_registry.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class ScenePluginPostProcessTest : public ScenePluginTest {
public:
    CORE_NS::ScopedHandle<CORE3D_NS::PostProcessComponent> GetNativeComponent(const IPostProcess::Ptr& pp)
    {
        auto ecsObject = interface_cast<IEcsObjectAccess>(pp)->GetEcsObject();
        auto ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
        auto manager = CORE_NS::GetManager<CORE3D_NS::IPostProcessComponentManager>(*ecs);
        return manager->Write(ecsObject->GetEntity());
    }

    struct PP {
        Scene scene { nullptr };
        PostProcess pp { nullptr };
    };

    PP SetupScene()
    {
        PP pp;
        pp.scene = Scene(CreateEmptyScene());
        if (auto is = pp.scene.GetPtr<IScene>()) {
            pp.pp = PostProcess(is->CreateObject<IPostProcess>(ClassId::PostProcess).GetResult());
            if (!pp.pp) {
                pp.scene.Release();
            }
        }
        return pp;
    }
};

/**
 * @tc.name: ChangeTonemap
 * @tc.desc: Tests for Change Tonemap. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(ScenePluginPostProcessTest, ChangeTonemap, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto pp = scene->CreateObject<IPostProcess>(ClassId::PostProcess).GetResult();
    ASSERT_TRUE(pp);

    auto tm = pp->Tonemap()->GetValue();
    ASSERT_TRUE(tm);
    tm->Enabled()->SetValue(true);
    tm->Type()->SetValue(TonemapType::FILMIC);
    tm->Exposure()->SetValue(1.0);
    UpdateScene();
    {
        auto nc = GetNativeComponent(pp);
        ASSERT_TRUE(nc);
        EXPECT_TRUE(nc->enableFlags & CORE3D_NS::PostProcessComponent::TONEMAP_BIT);
        EXPECT_EQ(nc->tonemapConfiguration.tonemapType, RENDER_NS::TonemapConfiguration::TONEMAP_FILMIC);
        EXPECT_FLOAT_EQ(nc->tonemapConfiguration.exposure, 1.0);

        nc->tonemapConfiguration.exposure = 0.1;
        nc->enableFlags = 0;
    }

    UpdateScene();
    WaitForUserQueue();

    EXPECT_FALSE(tm->Enabled()->GetValue());
    EXPECT_FLOAT_EQ(tm->Exposure()->GetValue(), 0.1);

    WaitForUserQueue();
}

/**
 * @tc.name: ChangeBloom
 * @tc.desc: Tests for Change Bloom. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(ScenePluginPostProcessTest, ChangeBloom, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto pp = scene->CreateObject<IPostProcess>(ClassId::PostProcess).GetResult();
    ASSERT_TRUE(pp);

    auto b = pp->Bloom()->GetValue();
    ASSERT_TRUE(b);
    b->Enabled()->SetValue(true);
    b->Type()->SetValue(BloomType::BILATERAL);
    b->Quality()->SetValue(EffectQualityType::HIGH);
    b->ThresholdHard()->SetValue(0.5);
    b->ThresholdSoft()->SetValue(1.0);
    b->AmountCoefficient()->SetValue(0.5);
    b->DirtMaskCoefficient()->SetValue(0.5);
    auto bitmap = CreateTestBitmap();
    b->DirtMaskImage()->SetValue(bitmap);
    b->UseCompute()->SetValue(true);
    b->Scatter()->SetValue(0.5);
    b->ScaleFactor()->SetValue(0.5);

    UpdateScene();
    {
        auto nc = GetNativeComponent(pp);
        ASSERT_TRUE(nc);
        EXPECT_TRUE(nc->enableFlags & CORE3D_NS::PostProcessComponent::BLOOM_BIT);
        EXPECT_EQ(nc->bloomConfiguration.bloomType, RENDER_NS::BloomConfiguration::TYPE_BILATERAL);
        EXPECT_EQ(nc->bloomConfiguration.bloomQualityType, RENDER_NS::BloomConfiguration::QUALITY_TYPE_HIGH);
        EXPECT_FLOAT_EQ(nc->bloomConfiguration.thresholdHard, 0.5);
        EXPECT_FLOAT_EQ(nc->bloomConfiguration.thresholdSoft, 1.0);
        EXPECT_FLOAT_EQ(nc->bloomConfiguration.amountCoefficient, 0.5);
        EXPECT_FLOAT_EQ(nc->bloomConfiguration.dirtMaskCoefficient, 0.5);
        EXPECT_TRUE(RENDER_NS::RenderHandleUtil::IsValid(nc->bloomConfiguration.dirtMaskImage));
        EXPECT_TRUE(nc->bloomConfiguration.useCompute);
        EXPECT_FLOAT_EQ(nc->bloomConfiguration.scatter, 0.5);
        EXPECT_FLOAT_EQ(nc->bloomConfiguration.scaleFactor, 0.5);

        nc->bloomConfiguration.bloomQualityType = RENDER_NS::BloomConfiguration::QUALITY_TYPE_LOW;
        nc->enableFlags = 0;
    }

    UpdateScene();
    WaitForUserQueue();

    EXPECT_FALSE(b->Enabled()->GetValue());
    EXPECT_EQ(b->Quality()->GetValue(), EffectQualityType::LOW);

    WaitForUserQueue();
}

/**
 * @tc.name: ChangeBlur
 * @tc.desc: Tests for Change Blur. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(ScenePluginPostProcessTest, ChangeBlur, testing::ext::TestSize.Level1)
{
    auto pp = SetupScene();
    ASSERT_TRUE(scene);
    auto b = pp.pp.GetBlur();
    ASSERT_TRUE(b);
    b.SetFilterSize(0.5f)
        .SetMaxMipmapLevel(42)
        .SetQuality(EffectQualityType::LOW)
        .SetType(BlurType::HORIZONTAL)
        .SetEnabled(true);

    UpdateScene();
    {
        auto nc = GetNativeComponent(pp.pp);
        ASSERT_TRUE(nc);
        EXPECT_TRUE(nc->enableFlags & CORE3D_NS::PostProcessComponent::BLUR_BIT);
        EXPECT_EQ(nc->blurConfiguration.maxMipLevel, 42);
        EXPECT_EQ(nc->blurConfiguration.blurQualityType, RENDER_NS::BlurConfiguration::QUALITY_TYPE_LOW);
        EXPECT_EQ(nc->blurConfiguration.filterSize, 0.5f);
        EXPECT_EQ(nc->blurConfiguration.blurType, RENDER_NS::BlurConfiguration::TYPE_HORIZONTAL);

        nc->blurConfiguration.blurQualityType = RENDER_NS::BlurConfiguration::QUALITY_TYPE_HIGH;
        nc->enableFlags = 0;
    }
    UpdateScene();
    WaitForUserQueue();

    EXPECT_FALSE(b.GetEnabled());
    EXPECT_EQ(b.GetQuality(), EffectQualityType::HIGH);
}

/**
 * @tc.name: ChangeMotionBlur
 * @tc.desc: Tests for Change Motion Blur. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(ScenePluginPostProcessTest, ChangeMotionBlur, testing::ext::TestSize.Level1)
{
    auto pp = SetupScene();
    ASSERT_TRUE(scene);
    auto b = pp.pp.GetMotionBlur();
    ASSERT_TRUE(b);
    b.SetAlpha(0.5f)
        .SetSharpness(EffectSharpnessType::SHARP)
        .SetVelocityCoefficient(12.f)
        .SetQuality(EffectQualityType::LOW)
        .SetEnabled(true);

    UpdateScene();
    {
        auto nc = GetNativeComponent(pp.pp);
        ASSERT_TRUE(nc);
        EXPECT_TRUE(nc->enableFlags & CORE3D_NS::PostProcessComponent::MOTION_BLUR_BIT);
        EXPECT_EQ(nc->motionBlurConfiguration.alpha, 0.5f);
        EXPECT_EQ(nc->motionBlurConfiguration.quality, RENDER_NS::MotionBlurConfiguration::Quality::LOW);
        EXPECT_EQ(nc->motionBlurConfiguration.sharpness, RENDER_NS::MotionBlurConfiguration::Sharpness::SHARP);
        EXPECT_EQ(nc->motionBlurConfiguration.velocityCoefficient, 12.f);

        nc->motionBlurConfiguration.quality = RENDER_NS::MotionBlurConfiguration::Quality::HIGH;
        nc->enableFlags = 0;
    }
    UpdateScene();
    WaitForUserQueue();

    EXPECT_FALSE(b.GetEnabled());
    EXPECT_EQ(b.GetQuality(), EffectQualityType::HIGH);
}

/**
 * @tc.name: ChangeColorConversion
 * @tc.desc: Tests for Change Color Conversion. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(ScenePluginPostProcessTest, ChangeColorConversion, testing::ext::TestSize.Level1)
{
    auto pp = SetupScene();
    ASSERT_TRUE(scene);
    auto b = pp.pp.GetColorConversion();
    ASSERT_TRUE(b);
    b.SetFunction(ColorConversionFunctionType::LINEAR).SetMultiplyWithAlpha(true).SetEnabled(true);

    UpdateScene();
    {
        auto nc = GetNativeComponent(pp.pp);
        ASSERT_TRUE(nc);
        EXPECT_TRUE(nc->enableFlags & CORE3D_NS::PostProcessComponent::COLOR_CONVERSION_BIT);
        EXPECT_FALSE(nc->colorConversionConfiguration.conversionFunctionType &
                     RENDER_NS::ColorConversionConfiguration::ConversionFunctionType::CONVERSION_LINEAR_TO_SRGB);
        EXPECT_TRUE(nc->colorConversionConfiguration.conversionFunctionType &
                    RENDER_NS::ColorConversionConfiguration::ConversionFunctionType::CONVERSION_MULTIPLY_WITH_ALPHA);

        nc->colorConversionConfiguration.conversionFunctionType |=
            RENDER_NS::ColorConversionConfiguration::ConversionFunctionType::CONVERSION_LINEAR_TO_SRGB;
        nc->enableFlags = 0;
    }
    UpdateScene();
    WaitForUserQueue();

    EXPECT_FALSE(b.GetEnabled());
    EXPECT_EQ(b.GetFunction(), ColorConversionFunctionType::LINEAR_TO_SRGB);
    EXPECT_TRUE(b.GetMultiplyWithAlpha());

    b.SetMultiplyWithAlpha(true);
    UpdateScene();

    {
        auto nc = GetNativeComponent(pp.pp);
        ASSERT_TRUE(nc);
        EXPECT_TRUE(nc->colorConversionConfiguration.conversionFunctionType &
                    RENDER_NS::ColorConversionConfiguration::ConversionFunctionType::CONVERSION_LINEAR_TO_SRGB);
        EXPECT_TRUE(nc->colorConversionConfiguration.conversionFunctionType &
                    RENDER_NS::ColorConversionConfiguration::ConversionFunctionType::CONVERSION_MULTIPLY_WITH_ALPHA);
    }
}

/**
 * @tc.name: ChangeColorFringe
 * @tc.desc: Tests for Change Color Fringe. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(ScenePluginPostProcessTest, ChangeColorFringe, testing::ext::TestSize.Level1)
{
    auto pp = SetupScene();
    ASSERT_TRUE(scene);
    auto b = pp.pp.GetColorFringe();
    ASSERT_TRUE(b);
    b.SetDistanceCoefficient(24.f).SetEnabled(true);

    UpdateScene();
    {
        auto nc = GetNativeComponent(pp.pp);
        ASSERT_TRUE(nc);
        EXPECT_TRUE(nc->enableFlags & CORE3D_NS::PostProcessComponent::COLOR_FRINGE_BIT);
        EXPECT_EQ(nc->colorFringeConfiguration.distanceCoefficient, 24.f);

        nc->colorFringeConfiguration.distanceCoefficient = 6.f;
        nc->enableFlags = 0;
    }
    UpdateScene();
    WaitForUserQueue();

    EXPECT_FALSE(b.GetEnabled());
    EXPECT_EQ(b.GetDistanceCoefficient(), 6.f);
}

/**
 * @tc.name: ChangeDepthOfField
 * @tc.desc: Tests for Change Depth Of Field. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(ScenePluginPostProcessTest, ChangeDepthOfField, testing::ext::TestSize.Level1)
{
    auto pp = SetupScene();
    ASSERT_TRUE(scene);
    auto b = pp.pp.GetDepthOfField();
    ASSERT_TRUE(b);
    b.SetFarBlur(11.f)
        .SetFarPlane(12.f)
        .SetFarTransitionRange(13.f)
        .SetFocusPoint(14.f)
        .SetFocusRange(15.f)
        .SetNearBlur(16.f)
        .SetNearPlane(17.f)
        .SetNearTransitionRange(18.f)
        .SetEnabled(true);

    UpdateScene();
    {
        auto nc = GetNativeComponent(pp.pp);
        ASSERT_TRUE(nc);
        EXPECT_TRUE(nc->enableFlags & CORE3D_NS::PostProcessComponent::DOF_BIT);
        EXPECT_EQ(nc->dofConfiguration.farBlur, 11.f);
        EXPECT_EQ(nc->dofConfiguration.farPlane, 12.f);
        EXPECT_EQ(nc->dofConfiguration.farTransitionRange, 13.f);
        EXPECT_EQ(nc->dofConfiguration.focusPoint, 14.f);
        EXPECT_EQ(nc->dofConfiguration.focusRange, 15.f);
        EXPECT_EQ(nc->dofConfiguration.nearBlur, 16.f);
        EXPECT_EQ(nc->dofConfiguration.nearPlane, 17.f);
        EXPECT_EQ(nc->dofConfiguration.nearTransitionRange, 18.f);

        nc->dofConfiguration.focusPoint = 10.f;
        nc->enableFlags = 0;
    }
    UpdateScene();
    WaitForUserQueue();

    EXPECT_FALSE(b.GetEnabled());
    EXPECT_EQ(b.GetFocusPoint(), 10.f);
}

/**
 * @tc.name: ChangeFxaa
 * @tc.desc: Tests for Change Fxaa. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(ScenePluginPostProcessTest, ChangeFxaa, testing::ext::TestSize.Level1)
{
    auto pp = SetupScene();
    ASSERT_TRUE(scene);
    auto b = pp.pp.GetFxaa();
    ASSERT_TRUE(b);
    b.SetQuality(EffectQualityType::LOW).SetSharpness(EffectSharpnessType::MEDIUM).SetEnabled(true);

    UpdateScene();
    {
        auto nc = GetNativeComponent(pp.pp);
        ASSERT_TRUE(nc);
        EXPECT_TRUE(nc->enableFlags & CORE3D_NS::PostProcessComponent::FXAA_BIT);
        EXPECT_EQ(nc->fxaaConfiguration.quality, RENDER_NS::FxaaConfiguration::Quality::LOW);
        EXPECT_EQ(nc->fxaaConfiguration.sharpness, RENDER_NS::FxaaConfiguration::Sharpness::MEDIUM);

        nc->fxaaConfiguration.sharpness = RENDER_NS::FxaaConfiguration::Sharpness::SHARP;
        nc->enableFlags = 0;
    }
    UpdateScene();
    WaitForUserQueue();

    EXPECT_FALSE(b.GetEnabled());
    EXPECT_EQ(b.GetSharpness(), EffectSharpnessType::SHARP);
}

/**
 * @tc.name: ChangeTaa
 * @tc.desc: Tests for Change Taa. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(ScenePluginPostProcessTest, ChangeTaa, testing::ext::TestSize.Level1)
{
    auto pp = SetupScene();
    ASSERT_TRUE(scene);
    auto b = pp.pp.GetTaa();
    ASSERT_TRUE(b);
    b.SetQuality(EffectQualityType::LOW).SetSharpness(EffectSharpnessType::MEDIUM).SetEnabled(true);

    UpdateScene();
    {
        auto nc = GetNativeComponent(pp.pp);
        ASSERT_TRUE(nc);
        EXPECT_TRUE(nc->enableFlags & CORE3D_NS::PostProcessComponent::TAA_BIT);
        EXPECT_EQ(nc->taaConfiguration.quality, RENDER_NS::TaaConfiguration::Quality::LOW);
        EXPECT_EQ(nc->taaConfiguration.sharpness, RENDER_NS::TaaConfiguration::Sharpness::MEDIUM);

        nc->taaConfiguration.sharpness = RENDER_NS::TaaConfiguration::Sharpness::SHARP;
        nc->enableFlags = 0;
    }
    UpdateScene();
    WaitForUserQueue();

    EXPECT_FALSE(b.GetEnabled());
    EXPECT_EQ(b.GetSharpness(), EffectSharpnessType::SHARP);
}

/**
 * @tc.name: ChangeVignette
 * @tc.desc: Tests for Change Vignette. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(ScenePluginPostProcessTest, ChangeVignette, testing::ext::TestSize.Level1)
{
    auto pp = SetupScene();
    ASSERT_TRUE(scene);
    auto b = pp.pp.GetVignette();
    ASSERT_TRUE(b);
    b.SetCoefficient(10.f).SetPower(20.f).SetEnabled(true);

    UpdateScene();
    {
        auto nc = GetNativeComponent(pp.pp);
        ASSERT_TRUE(nc);
        EXPECT_TRUE(nc->enableFlags & CORE3D_NS::PostProcessComponent::VIGNETTE_BIT);
        EXPECT_EQ(nc->vignetteConfiguration.coefficient, 10.f);
        EXPECT_EQ(nc->vignetteConfiguration.power, 20.f);

        nc->vignetteConfiguration.coefficient = 30.f;
        nc->enableFlags = 0;
    }
    UpdateScene();
    WaitForUserQueue();

    EXPECT_FALSE(b.GetEnabled());
    EXPECT_EQ(b.GetCoefficient(), 30.f);
}

/**
 * @tc.name: ChangeLensFlare
 * @tc.desc: Tests for Change Lens Flare. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(ScenePluginPostProcessTest, ChangeLensFlare, testing::ext::TestSize.Level1)
{
    auto pp = SetupScene();
    ASSERT_TRUE(scene);
    auto b = pp.pp.GetLensFlare();
    ASSERT_TRUE(b);
    auto position = BASE_NS::Math::Vec3(42.f, 0.f, 0.f);

    b.SetQuality(EffectQualityType::LOW).SetIntensity(42.f).SetFlarePosition(position).SetEnabled(true);

    UpdateScene();
    {
        auto nc = GetNativeComponent(pp.pp);
        ASSERT_TRUE(nc);
        EXPECT_TRUE(nc->enableFlags & CORE3D_NS::PostProcessComponent::LENS_FLARE_BIT);
        EXPECT_EQ(nc->lensFlareConfiguration.quality, RENDER_NS::LensFlareConfiguration::Quality::LOW);
        EXPECT_EQ(nc->lensFlareConfiguration.intensity, 42.f);
        EXPECT_EQ(nc->lensFlareConfiguration.flarePosition, position);

        nc->lensFlareConfiguration.quality = RENDER_NS::LensFlareConfiguration::Quality::HIGH;
        nc->enableFlags = 0;
    }
    UpdateScene();
    WaitForUserQueue();

    EXPECT_FALSE(b.GetEnabled());
    EXPECT_EQ(b.GetQuality(), EffectQualityType::HIGH);
}

/**
 * @tc.name: ChangeMultipleEnabled
 * @tc.desc: Tests for Change Multiple Enabled. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(ScenePluginPostProcessTest, ChangeMultipleEnabled, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto pp = scene->CreateObject<IPostProcess>(ClassId::PostProcess).GetResult();
    ASSERT_TRUE(pp);

    auto b = pp->Bloom()->GetValue();
    ASSERT_TRUE(b);
    auto tm = pp->Tonemap()->GetValue();
    ASSERT_TRUE(tm);

    b->Enabled()->SetValue(true);
    tm->Enabled()->SetValue(true);
    b->Enabled()->SetValue(false);
    tm->Enabled()->SetValue(true);
    b->Enabled()->SetValue(true);

    EXPECT_TRUE(b->Enabled()->GetValue());
    EXPECT_TRUE(tm->Enabled()->GetValue());

    UpdateScene();
    WaitForUserQueue();

    EXPECT_TRUE(b->Enabled()->GetValue());
    EXPECT_TRUE(tm->Enabled()->GetValue());
    {
        auto nc = GetNativeComponent(pp);
        ASSERT_TRUE(nc);
        EXPECT_TRUE(nc->enableFlags &
                    (CORE3D_NS::PostProcessComponent::BLOOM_BIT | CORE3D_NS::PostProcessComponent::TONEMAP_BIT));
        nc->enableFlags = CORE3D_NS::PostProcessComponent::BLOOM_BIT;
    }
    UpdateScene();
    WaitForUserQueue();
    EXPECT_TRUE(b->Enabled()->GetValue());
    EXPECT_FALSE(tm->Enabled()->GetValue());

    WaitForUserQueue();
}

/**
 * @tc.name: ChangeUpscale
 * @tc.desc: Tests for Change Upscale. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(ScenePluginPostProcessTest, ChangeUpscale, testing::ext::TestSize.Level1)
{
    auto pp = SetupScene();
    ASSERT_TRUE(scene);
    auto b = pp.pp.GetUpscale();
    ASSERT_TRUE(b);
    b.SetRatio(0.9f).SetEdgeSharpness(0.9f).SetSmoothScale(0.9f).SetStructureSensitivity(0.9f).SetEnabled(true);

    UpdateScene();
    {
        auto nc = GetNativeComponent(pp.pp);
        ASSERT_TRUE(nc);
        EXPECT_TRUE(nc->enableFlags & CORE3D_NS::PostProcessComponent::UPSCALE_BIT);
        EXPECT_EQ(nc->upscaleConfiguration.ratio, 0.9f);
        EXPECT_EQ(nc->upscaleConfiguration.edgeSharpness, 0.9f);
        EXPECT_EQ(nc->upscaleConfiguration.smoothScale, 0.9f);
        EXPECT_EQ(nc->upscaleConfiguration.structureSensitivity, 0.9f);

        nc->upscaleConfiguration.ratio = 0.5f;
        nc->upscaleConfiguration.edgeSharpness = 0.5f;
        nc->upscaleConfiguration.smoothScale = 0.5f;
        nc->upscaleConfiguration.structureSensitivity = 0.5f;
        nc->enableFlags = 0;
    }
    UpdateScene();
    WaitForUserQueue();

    EXPECT_FALSE(b.GetEnabled());
    EXPECT_EQ(b.GetRatio(), 0.5f);
    EXPECT_EQ(b.GetEdgeSharpness(), 0.5f);
    EXPECT_EQ(b.GetSmoothScale(), 0.5f);
    EXPECT_EQ(b.GetStructureSensitivity(), 0.5f);
}

/**
 * @tc.name: InvalidPostInit
 * @tc.desc: Tests for Invalid Post Init. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(ScenePluginPostProcessTest, InvalidPostInit, testing::ext::TestSize.Level1)
{
    auto& reg = META_NS::GetObjectRegistry();
    auto pp = reg.Create(ClassId::PostProcess);
    auto init = interface_cast<IEnginePropertyInit>(pp);
    ASSERT_TRUE(init);

    auto testProperty = [&](BASE_NS::string_view name) {
        EXPECT_FALSE(init->InitDynamicProperty({}, "invalid"));
        auto property = reg.GetPropertyRegister().Create(META_NS::ClassId::StackProperty, name);
        EXPECT_FALSE(init->InitDynamicProperty(property, "invalid"));
    };

    testProperty("Tonemap");
    testProperty("Bloom");
    testProperty("Blur");
    testProperty("MotionBlur");
    testProperty("ColorConversion");
    testProperty("ColorFringe");
    testProperty("DepthOfField");
    testProperty("Fxaa");
    testProperty("Taa");
    testProperty("Vignette");
    testProperty("LensFlare");
    testProperty("Upscale");
}
} // namespace UTest

SCENE_END_NAMESPACE()
