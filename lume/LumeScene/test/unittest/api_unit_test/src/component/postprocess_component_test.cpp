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

#include <cstdint>
#include <scene/ext/component_util.h>
#include <scene/interface/intf_scene.h>
#include <test_framework.h>

#include <3d/ecs/components/post_process_component.h>
#include <3d/namespace.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/namespace.h>

#include <meta/interface/intf_class_registry.h>

#include "scene/scene_component_test.h"

META_TYPE(RENDER_NS::BloomConfiguration);
META_TYPE(RENDER_NS::BlurConfiguration);
META_TYPE(RENDER_NS::ColorConversionConfiguration);
META_TYPE(RENDER_NS::ColorFringeConfiguration);
META_TYPE(RENDER_NS::DitherConfiguration);
META_TYPE(RENDER_NS::DofConfiguration);
META_TYPE(RENDER_NS::FxaaConfiguration);
META_TYPE(RENDER_NS::MotionBlurConfiguration);
META_TYPE(RENDER_NS::TaaConfiguration);
META_TYPE(RENDER_NS::TonemapConfiguration);
META_TYPE(RENDER_NS::VignetteConfiguration);
META_TYPE(RENDER_NS::UpscaleConfiguration);

using RENDER_NS::BloomConfiguration;
using RENDER_NS::BlurConfiguration;
using RENDER_NS::ColorConversionConfiguration;
using RENDER_NS::ColorFringeConfiguration;
using RENDER_NS::DitherConfiguration;
using RENDER_NS::DofConfiguration;
using RENDER_NS::FxaaConfiguration;
using RENDER_NS::MotionBlurConfiguration;
using RENDER_NS::TaaConfiguration;
using RENDER_NS::TonemapConfiguration;
using RENDER_NS::UpscaleConfiguration;
using RENDER_NS::VignetteConfiguration;

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginPostprocessComponentTest
    : public ScenePluginComponentTest<CORE3D_NS::IPostProcessComponentManager> {
public:
protected:
    template<typename Interface>
    void TestInterface(Interface* interface)
    {
        ASSERT_TRUE(interface);
    }

    template<>
    void TestInterface(IBloom* interface)
    {
        ASSERT_TRUE(interface);
        EXPECT_TRUE(interface->Enabled());
        EXPECT_TRUE(interface->Type());
        EXPECT_TRUE(interface->Quality());
        EXPECT_TRUE(interface->ThresholdHard());
        EXPECT_TRUE(interface->ThresholdSoft());
        EXPECT_TRUE(interface->AmountCoefficient());
        EXPECT_TRUE(interface->DirtMaskCoefficient());
        EXPECT_TRUE(interface->DirtMaskImage());
        EXPECT_TRUE(interface->UseCompute());
        EXPECT_TRUE(interface->Scatter());
        EXPECT_TRUE(interface->ScaleFactor());
    }

    template<>
    void TestInterface(IBlur* interface)
    {
        ASSERT_TRUE(interface);
        EXPECT_TRUE(interface->Type());
        EXPECT_TRUE(interface->Quality());
        EXPECT_TRUE(interface->FilterSize());
        EXPECT_TRUE(interface->MaxMipmapLevel());
    }
    template<>
    void TestInterface(IColorConversion* interface)
    {
        EXPECT_TRUE(interface->Function());
        EXPECT_TRUE(interface->MultiplyWithAlpha());
    }

    template<>
    void TestInterface(IColorFringe* interface)
    {
        EXPECT_TRUE(interface->DistanceCoefficient());
    }
    template<>
    void TestInterface(IDepthOfField* interface)
    {
        EXPECT_TRUE(interface->FocusPoint());
        EXPECT_TRUE(interface->FocusRange());
        EXPECT_TRUE(interface->NearTransitionRange());
        EXPECT_TRUE(interface->FarTransitionRange());
        EXPECT_TRUE(interface->NearBlur());
        EXPECT_TRUE(interface->FarBlur());
        EXPECT_TRUE(interface->NearPlane());
        EXPECT_TRUE(interface->FarPlane());
    }
    template<>
    void TestInterface(IFxaa* interface)
    {
        EXPECT_TRUE(interface->Quality());
        EXPECT_TRUE(interface->Sharpness());
    }
    template<>
    void TestInterface(ILensFlare* interface)
    {
        EXPECT_TRUE(interface->Quality());
        EXPECT_TRUE(interface->Intensity());
        EXPECT_TRUE(interface->FlarePosition());
    }
    template<>
    void TestInterface(IMotionBlur* interface)
    {
        EXPECT_TRUE(interface->Quality());
        EXPECT_TRUE(interface->Sharpness());
        EXPECT_TRUE(interface->Alpha());
        EXPECT_TRUE(interface->VelocityCoefficient());
    }
    template<>
    void TestInterface(ITaa* interface)
    {
        EXPECT_TRUE(interface->Quality());
        EXPECT_TRUE(interface->Sharpness());
    }
    template<>
    void TestInterface(ITonemap* interface)
    {
        EXPECT_TRUE(interface->Type());
        EXPECT_TRUE(interface->Exposure());
    }
    template<>
    void TestInterface(IUpscale* interface)
    {
        EXPECT_TRUE(interface->SmoothScale());
        EXPECT_TRUE(interface->StructureSensitivity());
        EXPECT_TRUE(interface->EdgeSharpness());
        EXPECT_TRUE(interface->Ratio());
    }
    template<>
    void TestInterface(IVignette* interface)
    {
        EXPECT_TRUE(interface->Coefficient());
        EXPECT_TRUE(interface->Power());
    }

    template<typename Interface>
    void TestPPComponent(META_NS::ObjectId classId)
    {
        auto& reg = META_NS::GetObjectRegistry();
        auto pp = reg.Create(classId);
        ASSERT_TRUE(pp);
        // Call twice to cover the cases where property gets created and when it's already there
        Interface* b = interface_cast<Interface>(pp);
        TestInterface<Interface>(b);
        TestInterface<const Interface>(b);
        TestInterface<Interface>(b);
        TestInterface<const Interface>(b);
    }
};

/**
 * @tc.name: Members
 * @tc.desc: Tests for Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginPostprocessComponentTest, Members, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    const auto node = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(AddComponent<CORE3D_NS::IPostProcessComponentManager>(node));
    SetComponent(node, "PostProcessComponent");

    TestEngineProperty<uint32_t>("EnableFlags", 42, nativeComponent.enableFlags);

    // Helpers to shorten the actual test lines.
    auto& convConf = nativeComponent.colorConversionConfiguration;
    auto& colorFringe = nativeComponent.colorFringeConfiguration;
    // These need to be non-default values for the members (for the tests to test anything).
    auto taaSoft = TaaConfiguration::Sharpness::SOFT;
    auto fxaaSoft = FxaaConfiguration::Sharpness::SOFT;
    auto srgb = ColorConversionConfiguration::CONVERSION_LINEAR_TO_SRGB;
    auto f = 1.23456f;

    TEST_COMPLEX_PROP(BloomConfiguration, "Bloom", nativeComponent.bloomConfiguration, scatter, f);
    TEST_COMPLEX_PROP(BlurConfiguration, "Blur", nativeComponent.blurConfiguration, filterSize, f);
    TEST_COMPLEX_PROP(ColorConversionConfiguration, "ColorConversion", convConf, conversionFunctionType, srgb);
    TEST_COMPLEX_PROP(ColorFringeConfiguration, "ColorFringe", colorFringe, coefficient, f);
    TEST_COMPLEX_PROP(DitherConfiguration, "Dither", nativeComponent.ditherConfiguration, amountCoefficient, f);
    TEST_COMPLEX_PROP(DofConfiguration, "Dof", nativeComponent.dofConfiguration, farBlur, f);
    TEST_COMPLEX_PROP(FxaaConfiguration, "Fxaa", nativeComponent.fxaaConfiguration, sharpness, fxaaSoft);
    TEST_COMPLEX_PROP(MotionBlurConfiguration, "MotionBlur", nativeComponent.motionBlurConfiguration, alpha, f);
    TEST_COMPLEX_PROP(TaaConfiguration, "Taa", nativeComponent.taaConfiguration, sharpness, taaSoft);
    TEST_COMPLEX_PROP(TonemapConfiguration, "Tonemap", nativeComponent.tonemapConfiguration, exposure, f);
    TEST_COMPLEX_PROP(VignetteConfiguration, "Vignette", nativeComponent.vignetteConfiguration, power, f);
    TEST_COMPLEX_PROP(UpscaleConfiguration, "Upscale", nativeComponent.upscaleConfiguration, ratio, f);
}

// From src/postprocess/util.h
class IPPEffectInit : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPPEffectInit, "ee72a777-45ff-42ff-b816-5f169b4453b0")
public:
    virtual bool Init(const META_NS::IProperty::Ptr& flags) = 0;
};

/**
 * @tc.name: InvalidInit
 * @tc.desc: Test for invalid initialization of post process components.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginPostprocessComponentTest, InvalidInit, testing::ext::TestSize.Level1)
{
    auto& reg = META_NS::GetObjectRegistry();
    auto allPPs = reg.GetClassRegistry().GetAllTypes({ IPostProcessEffect::UID });
    ASSERT_FALSE(allPPs.empty());
    for (auto&& ppType : allPPs) {
        const auto clsi = ppType->GetClassInfo();
        auto pp = reg.Create(clsi);
        auto meta = interface_cast<META_NS::IMetadata>(pp);
        auto init = interface_cast<IEnginePropertyInit>(pp);
        ASSERT_TRUE(meta);
        ASSERT_TRUE(init);
        auto props = meta->GetAllMetadatas(META_NS::MetadataType::PROPERTY);
        ASSERT_FALSE(props.empty());
        for (auto&& p : props) {
            EXPECT_FALSE(init->InitDynamicProperty({}, "invalid"));
            auto property = reg.GetPropertyRegister().Create(META_NS::ClassId::StackProperty, p.name);
            EXPECT_FALSE(init->InitDynamicProperty(property, "invalid"));
            // Again
            EXPECT_FALSE(init->InitDynamicProperty({}, "invalid"));
            EXPECT_FALSE(init->InitDynamicProperty(property, "invalid"));
        }
        auto ppinit = interface_cast<IPPEffectInit>(pp);
        ASSERT_TRUE(ppinit);
        EXPECT_FALSE(ppinit->Init({}));
        auto flags = reg.GetPropertyRegister().Create(META_NS::ClassId::StackProperty, "EnableFlags");
        EXPECT_FALSE(ppinit->Init(flags));
        auto property = reg.GetPropertyRegister().Create(META_NS::ClassId::StackProperty, "Enabled");
        EXPECT_FALSE(init->InitDynamicProperty(property, "invalid"));
        // Again
        EXPECT_FALSE(ppinit->Init({}));
        EXPECT_FALSE(ppinit->Init(flags));
        EXPECT_FALSE(init->InitDynamicProperty(property, "invalid"));
    }
}

/**
 * @tc.name: BloomComponent
 * @tc.desc: Bloom component interface test.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginPostprocessComponentTest, BloomComponent, testing::ext::TestSize.Level1)
{
    static constexpr META_NS::ObjectId id { "6718b07d-c3d1-4036-bd0f-88d1380b846a" }; // ClassId::Bloom
    TestPPComponent<IBloom>(id);
}

/**
 * @tc.name: BlurComponent
 * @tc.desc: Blur component interface test.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginPostprocessComponentTest, BlurComponent, testing::ext::TestSize.Level1)
{
    static constexpr META_NS::ObjectId id { "cf09372e-223a-4272-9063-38c0f07b2f4b" }; // ClassId::Blur
    TestPPComponent<IBlur>(id);
}

/**
 * @tc.name: ColorConversionComponent
 * @tc.desc: Color converstion component interface test.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginPostprocessComponentTest, ColorConversionComponent, testing::ext::TestSize.Level1)
{
    static constexpr META_NS::ObjectId id { "f532feeb-0e71-454b-a1ac-99a05933c506" }; // ClassId::ColorConversion
    TestPPComponent<IColorConversion>(id);
}

/**
 * @tc.name: ColorFringeComponent
 * @tc.desc: Color fringe component interface test.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginPostprocessComponentTest, ColorFringeComponent, testing::ext::TestSize.Level1)
{
    static constexpr META_NS::ObjectId id { "3e403293-8e0c-43bf-ae30-cb45d6ad2d65" }; // ClassId::ColorFringe
    TestPPComponent<IColorFringe>(id);
}

/**
 * @tc.name: DoFComponent
 * @tc.desc: Depth of field component interface test.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginPostprocessComponentTest, DoFComponent, testing::ext::TestSize.Level1)
{
    static constexpr META_NS::ObjectId id { "4165f797-a08b-4c81-af0b-14642eb1ed9c" }; // ClassId::DepthOfField
    TestPPComponent<IDepthOfField>(id);
}

/**
 * @tc.name: FxaaComponent
 * @tc.desc: FXAA component interface test.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginPostprocessComponentTest, FxaaComponent, testing::ext::TestSize.Level1)
{
    static constexpr META_NS::ObjectId id { "40e2b7ee-0e27-416f-9481-742eea521937" }; // ClassId::Fxaa
    TestPPComponent<IFxaa>(id);
}

/**
 * @tc.name: LensFlareComponent
 * @tc.desc: Lens flare component interface test.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginPostprocessComponentTest, LensFlareComponent, testing::ext::TestSize.Level1)
{
    static constexpr META_NS::ObjectId id { "72693514-26dc-4df4-86a7-cdaaa2d023f9" }; // ClassId::LensFlare
    TestPPComponent<ILensFlare>(id);
}

/**
 * @tc.name: MotionBlurComponent
 * @tc.desc: Motion blur component interface test.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginPostprocessComponentTest, MotionBlurComponent, testing::ext::TestSize.Level1)
{
    static constexpr META_NS::ObjectId id { "c2a00f4e-690c-4a76-9a32-92a3d9b76de2" }; // ClassId::MotionBlur
    TestPPComponent<IMotionBlur>(id);
}

/**
 * @tc.name: TaaComponent
 * @tc.desc: TAA component interface test.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginPostprocessComponentTest, TaaComponent, testing::ext::TestSize.Level1)
{
    static constexpr META_NS::ObjectId id { "004fb0b2-4248-464b-bd43-dcab43ddb8d8" }; // ClassId::Taa
    TestPPComponent<ITaa>(id);
}

/**
 * @tc.name: TonemapComponent
 * @tc.desc: Tonemap component interface test.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginPostprocessComponentTest, TonemapComponent, testing::ext::TestSize.Level1)
{
    static constexpr META_NS::ObjectId id { "56c1fc6e-90aa-486d-846e-c9a5c780a90a" }; // ClassId::Tonemap
    TestPPComponent<ITonemap>(id);
}

/**
 * @tc.name: UpscaleComponent
 * @tc.desc: Upscale component interface test.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginPostprocessComponentTest, UpscaleComponent, testing::ext::TestSize.Level1)
{
    static constexpr META_NS::ObjectId id { "4bb7b52d-9ccc-40a5-8f47-510946410fb5" }; // ClassId::Upscale
    TestPPComponent<IUpscale>(id);
}

/**
 * @tc.name: VignetteComponent
 * @tc.desc: Vignette component interface test.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginPostprocessComponentTest, VignetteComponent, testing::ext::TestSize.Level1)
{
    static constexpr META_NS::ObjectId id { "af3eb8f6-b271-4fb5-b077-a4644942be89" }; // ClassId::Vignette
    TestPPComponent<IVignette>(id);
}

} // namespace UTest

SCENE_END_NAMESPACE()
