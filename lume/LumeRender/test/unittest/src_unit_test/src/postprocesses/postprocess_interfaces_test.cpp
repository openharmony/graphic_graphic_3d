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

#include <nodecontext/render_node_post_process_util.h>
#include <render_context.h>

#include <core/property/property_handle_util.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/intf_plugin.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

/**
 * @tc.name: Interfaces
 * @tc.desc: Tests for different post process propery interfaces by verifying the data was passed to the
 * RenderPostProcess* correctly.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_Postprocesses, Interfaces, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    // Bloom interface
    {
        RenderPostProcessBloomNode bloom;
        auto* pp = static_cast<IRenderPostProcess*>(bloom.GetInterface(IRenderPostProcess::UID));
        ASSERT_EQ(BASE_NS::Uid("89b83608-c910-433e-9fd8-22f3ef64eabd"), pp->GetRenderPostProcessNodeUid());

        RenderPostProcessBloomNode::EffectProperties properties;
        properties.enabled = true;
        properties.bloomConfiguration.bloomType = BloomConfiguration::TYPE_NORMAL;
        properties.bloomConfiguration.bloomQualityType = BloomConfiguration::QUALITY_TYPE_HIGH;
        properties.bloomConfiguration.thresholdHard = 0.8f;
        properties.bloomConfiguration.thresholdSoft = 1.5f;
        properties.bloomConfiguration.amountCoefficient = 0.3f;
        properties.bloomConfiguration.dirtMaskCoefficient = 0.1f;
        properties.bloomConfiguration.scatter = 0.9f;
        properties.bloomConfiguration.scaleFactor = 0.75f;
        properties.bloomConfiguration.useCompute = true;
        const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&properties);
        BASE_NS::array_view<const uint8_t> dataView(dataPtr, sizeof(RenderPostProcessBloomNode::EffectProperties));
        pp->SetData(dataView);
        BASE_NS::array_view<const uint8_t> getDataView = pp->GetData();
        ASSERT_EQ(0, memcmp(dataView.data(), getDataView.data(), dataView.size()));
        auto* propertyHandle = pp->GetProperties();
        EXPECT_TRUE(CORE_NS::GetPropertyValue<bool>(propertyHandle, "enabled"));
        EXPECT_FLOAT_EQ(CORE_NS::GetPropertyValue<float>(propertyHandle, "bloomConfiguration.scatter"),
            properties.bloomConfiguration.scatter);

        auto* node = static_cast<IRenderPostProcessNode*>(pp->GetInterface(IRenderPostProcessNode::UID));
        node->SetRenderAreaRequest(
            IRenderPostProcessNode::RenderAreaRequest { RenderPassDesc::RenderArea { 0, 0, 1, 1 } });
        EXPECT_EQ(IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE, node->GetExecuteFlags());
        EXPECT_NE(nullptr, node->GetRenderOutputProperties());
        EXPECT_NE(nullptr, node->GetRenderInputProperties());
    }
    // Blur interface
    {
        RenderPostProcessBlurNode blur;
        auto* pp = static_cast<IRenderPostProcess*>(blur.GetInterface(IRenderPostProcess::UID));
        ASSERT_EQ(BASE_NS::Uid("65ec929c-c38c-4d4e-b0a0-62fffb55d338"), pp->GetRenderPostProcessNodeUid());

        RenderPostProcessBlurNode::EffectProperties properties;
        properties.enabled = true;
        properties.blurShaderType = 5;
        properties.blurShaderScaleType = 2;
        properties.blurConfiguration.blurType = BlurConfiguration::TYPE_HORIZONTAL;
        properties.blurConfiguration.blurQualityType = BlurConfiguration::QUALITY_TYPE_HIGH;
        properties.blurConfiguration.filterSize = 3.0f;
        properties.blurConfiguration.maxMipLevel = 5U;
        const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&properties);
        BASE_NS::array_view<const uint8_t> dataView(dataPtr, sizeof(RenderPostProcessBlurNode::EffectProperties));
        pp->SetData(dataView);
        BASE_NS::array_view<const uint8_t> getDataView = pp->GetData();
        ASSERT_EQ(0, memcmp(dataView.data(), getDataView.data(), dataView.size()));
        auto* propertyHandle = pp->GetProperties();
        EXPECT_TRUE(CORE_NS::GetPropertyValue<bool>(propertyHandle, "enabled"));
        EXPECT_FLOAT_EQ(CORE_NS::GetPropertyValue<uint32_t>(propertyHandle, "blurConfiguration.maxMipLevel"),
            properties.blurConfiguration.maxMipLevel);

        auto* node = static_cast<IRenderPostProcessNode*>(pp->GetInterface(IRenderPostProcessNode::UID));
        node->SetRenderAreaRequest(
            IRenderPostProcessNode::RenderAreaRequest { RenderPassDesc::RenderArea { 0, 0, 1, 1 } });
        EXPECT_EQ(IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE, node->GetExecuteFlags());
        EXPECT_NE(nullptr, node->GetRenderOutputProperties());
        EXPECT_NE(nullptr, node->GetRenderInputProperties());
    }
    // Combined interface
    {
        RenderPostProcessCombinedNode combined;
        auto* pp = static_cast<IRenderPostProcess*>(combined.GetInterface(IRenderPostProcess::UID));
        ASSERT_EQ(BASE_NS::Uid("e3ac92f0-ccb2-4573-9536-ac1f24d8e47b"), pp->GetRenderPostProcessNodeUid());

        RenderPostProcessCombinedNode::EffectProperties properties;
        properties.enabled = true;
        properties.glOptimizedLayerCopyEnabled = true;
        properties.postProcessConfiguration.tonemapConfiguration.exposure = 1.0f;
        const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&properties);
        BASE_NS::array_view<const uint8_t> dataView(dataPtr, sizeof(RenderPostProcessCombinedNode::EffectProperties));
        pp->SetData(dataView);
        BASE_NS::array_view<const uint8_t> getDataView = pp->GetData();
        ASSERT_EQ(0, memcmp(dataView.data(), getDataView.data(), dataView.size()));
        auto* propertyHandle = pp->GetProperties();
        EXPECT_TRUE(CORE_NS::GetPropertyValue<bool>(propertyHandle, "enabled"));
        EXPECT_FLOAT_EQ(
            CORE_NS::GetPropertyValue<float>(propertyHandle, "postProcessConfiguration.tonemapConfiguration.exposure"),
            properties.postProcessConfiguration.tonemapConfiguration.exposure);

        auto* node = static_cast<IRenderPostProcessNode*>(pp->GetInterface(IRenderPostProcessNode::UID));
        node->SetRenderAreaRequest(
            IRenderPostProcessNode::RenderAreaRequest { RenderPassDesc::RenderArea { 0, 0, 1, 1 } });
        EXPECT_EQ(IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE, node->GetExecuteFlags());
        EXPECT_NE(nullptr, node->GetRenderOutputProperties());
        EXPECT_NE(nullptr, node->GetRenderInputProperties());
    }
    // Dof interface
    {
        RenderPostProcessDofNode dof;
        auto* pp = static_cast<IRenderPostProcess*>(dof.GetInterface(IRenderPostProcess::UID));
        ASSERT_EQ(BASE_NS::Uid("134815e1-915e-40d9-b230-2467fb946cf7"), pp->GetRenderPostProcessNodeUid());

        RenderPostProcessDofNode::EffectProperties properties;
        properties.enabled = true;
        properties.dofConfiguration.farBlur = 120.0f;
        properties.dofConfiguration.nearBlur = 4.0f;
        properties.dofConfiguration.farPlane = 200.0f;
        properties.dofConfiguration.farTransitionRange = 40.0f;
        properties.dofConfiguration.focusPoint = 10.0f;
        properties.dofConfiguration.focusRange = 5.0f;
        properties.dofConfiguration.nearPlane = 0.1f;
        properties.dofConfiguration.nearTransitionRange = 3.0f;
        const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&properties);
        BASE_NS::array_view<const uint8_t> dataView(dataPtr, sizeof(RenderPostProcessDofNode::EffectProperties));
        pp->SetData(dataView);
        BASE_NS::array_view<const uint8_t> getDataView = pp->GetData();
        ASSERT_EQ(0, memcmp(dataView.data(), getDataView.data(), dataView.size()));
        auto* propertyHandle = pp->GetProperties();
        EXPECT_TRUE(CORE_NS::GetPropertyValue<bool>(propertyHandle, "enabled"));
        EXPECT_FLOAT_EQ(CORE_NS::GetPropertyValue<float>(propertyHandle, "dofConfiguration.farBlur"),
            properties.dofConfiguration.farBlur);

        auto* node = static_cast<IRenderPostProcessNode*>(pp->GetInterface(IRenderPostProcessNode::UID));
        node->SetRenderAreaRequest(
            IRenderPostProcessNode::RenderAreaRequest { RenderPassDesc::RenderArea { 0, 0, 1, 1 } });
        EXPECT_EQ(IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE, node->GetExecuteFlags());
        EXPECT_NE(nullptr, node->GetRenderOutputProperties());
        EXPECT_NE(nullptr, node->GetRenderInputProperties());
    }
    // Flare interface
    {
        RenderPostProcessFlareNode flare;
        auto* pp = static_cast<IRenderPostProcess*>(flare.GetInterface(IRenderPostProcess::UID));
        ASSERT_EQ(BASE_NS::Uid("eeea01ce-c094-4559-a909-ea44172c7d43"), pp->GetRenderPostProcessNodeUid());

        RenderPostProcessFlareNode::EffectProperties properties;
        properties.enabled = true;
        properties.flarePos = BASE_NS::Math::Vec3(0.1, 0.2, 0.3);
        properties.intensity = 13.37;
        const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&properties);
        BASE_NS::array_view<const uint8_t> dataView(dataPtr, sizeof(RenderPostProcessFlareNode::EffectProperties));
        pp->SetData(dataView);
        BASE_NS::array_view<const uint8_t> getDataView = pp->GetData();
        ASSERT_EQ(0, memcmp(dataView.data(), getDataView.data(), dataView.size()));
        auto* propertyHandle = pp->GetProperties();
        EXPECT_TRUE(CORE_NS::GetPropertyValue<bool>(propertyHandle, "enabled"));
        EXPECT_FLOAT_EQ(CORE_NS::GetPropertyValue<float>(propertyHandle, "intensity"), properties.intensity);

        auto* node = static_cast<IRenderPostProcessNode*>(pp->GetInterface(IRenderPostProcessNode::UID));
        node->SetRenderAreaRequest(
            IRenderPostProcessNode::RenderAreaRequest { RenderPassDesc::RenderArea { 0, 0, 1, 1 } });
        EXPECT_EQ(IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE, node->GetExecuteFlags());
        EXPECT_NE(nullptr, node->GetRenderOutputProperties());
        EXPECT_NE(nullptr, node->GetRenderInputProperties());
    }
    // FXAA interface
    {
        RenderPostProcessFxaaNode fxaa;
        auto* pp = static_cast<IRenderPostProcess*>(fxaa.GetInterface(IRenderPostProcess::UID));
        ASSERT_EQ(BASE_NS::Uid("7ae334d0-4da8-4e17-842a-d4fe3a4ef61d"), pp->GetRenderPostProcessNodeUid());
        RenderPostProcessFxaaNode::EffectProperties properties;
        properties.enabled = true;
        properties.targetSize = BASE_NS::Math::Vec4(0.1, 0.2, 0.3, 0.4);
        properties.fxaaConfiguration.quality = FxaaConfiguration::Quality::HIGH;
        properties.fxaaConfiguration.sharpness = FxaaConfiguration::Sharpness::SHARP;
        const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&properties);
        BASE_NS::array_view<const uint8_t> dataView(dataPtr, sizeof(RenderPostProcessFxaaNode::EffectProperties));
        pp->SetData(dataView);
        BASE_NS::array_view<const uint8_t> getDataView = pp->GetData();
        ASSERT_EQ(0, memcmp(dataView.data(), getDataView.data(), dataView.size()));
        auto* propertyHandle = pp->GetProperties();
        EXPECT_TRUE(CORE_NS::GetPropertyValue<bool>(propertyHandle, "enabled"));
        EXPECT_EQ(CORE_NS::GetPropertyValue<BASE_NS::Math::Vec4>(propertyHandle, "targetSize"), properties.targetSize);

        auto* node = static_cast<IRenderPostProcessNode*>(pp->GetInterface(IRenderPostProcessNode::UID));
        node->SetRenderAreaRequest(
            IRenderPostProcessNode::RenderAreaRequest { RenderPassDesc::RenderArea { 0, 0, 1, 1 } });
        EXPECT_EQ(IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE, node->GetExecuteFlags());
        EXPECT_NE(nullptr, node->GetRenderOutputProperties());
        EXPECT_NE(nullptr, node->GetRenderInputProperties());
    }
    // Motion blur interface
    {
        RenderPostProcessMotionBlurNode motionBlur;
        auto* pp = static_cast<IRenderPostProcess*>(motionBlur.GetInterface(IRenderPostProcess::UID));
        ASSERT_EQ(BASE_NS::Uid("b77a0280-8669-4492-a9a1-96a16c79e64f"), pp->GetRenderPostProcessNodeUid());

        RenderPostProcessMotionBlurNode::EffectProperties properties;
        properties.enabled = true;
        properties.size = BASE_NS::Math::UVec2(512, 512);
        properties.motionBlurConfiguration.alpha = 1.0f;
        properties.motionBlurConfiguration.velocityCoefficient = 0.777;
        properties.motionBlurConfiguration.quality = MotionBlurConfiguration::Quality::MEDIUM;
        properties.motionBlurConfiguration.sharpness = MotionBlurConfiguration::Sharpness::SOFT;
        const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&properties);
        BASE_NS::array_view<const uint8_t> dataView(dataPtr, sizeof(RenderPostProcessMotionBlurNode::EffectProperties));
        pp->SetData(dataView);
        BASE_NS::array_view<const uint8_t> getDataView = pp->GetData();
        ASSERT_EQ(0, memcmp(dataView.data(), getDataView.data(), dataView.size()));
        auto* propertyHandle = pp->GetProperties();
        EXPECT_TRUE(CORE_NS::GetPropertyValue<bool>(propertyHandle, "enabled"));
        EXPECT_EQ(CORE_NS::GetPropertyValue<BASE_NS::Math::UVec2>(propertyHandle, "size"), properties.size);

        auto* node = static_cast<IRenderPostProcessNode*>(pp->GetInterface(IRenderPostProcessNode::UID));
        node->SetRenderAreaRequest(
            IRenderPostProcessNode::RenderAreaRequest { RenderPassDesc::RenderArea { 0, 0, 1, 1 } });
        EXPECT_EQ(IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE, node->GetExecuteFlags());
        EXPECT_NE(nullptr, node->GetRenderOutputProperties());
        EXPECT_NE(nullptr, node->GetRenderInputProperties());
    }
    // Screen space ambien occlusion interface
    {
        RenderPostProcessSsaoNode ssao;
        auto* pp = static_cast<IRenderPostProcess*>(ssao.GetInterface(IRenderPostProcess::UID));
        ASSERT_EQ(BASE_NS::Uid("6f7db0df-648a-467d-a695-1ca13af4c712"), pp->GetRenderPostProcessNodeUid());

        RenderPostProcessSsaoNode::EffectProperties properties;
        properties.enabled = true;
        properties.radius = 0.25f;
        properties.intensity = 0.5f;
        const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&properties);
        BASE_NS::array_view<const uint8_t> dataView(dataPtr, sizeof(RenderPostProcessSsaoNode::EffectProperties));
        pp->SetData(dataView);
        BASE_NS::array_view<const uint8_t> getDataView = pp->GetData();
        ASSERT_EQ(0, memcmp(dataView.data(), getDataView.data(), dataView.size()));
        auto* propertyHandle = pp->GetProperties();
        EXPECT_TRUE(CORE_NS::GetPropertyValue<bool>(propertyHandle, "enabled"));
        EXPECT_FLOAT_EQ(CORE_NS::GetPropertyValue<float>(propertyHandle, "radius"), properties.radius);

        auto* node = static_cast<IRenderPostProcessNode*>(pp->GetInterface(IRenderPostProcessNode::UID));
        node->SetRenderAreaRequest(
            IRenderPostProcessNode::RenderAreaRequest { RenderPassDesc::RenderArea { 0, 0, 1, 1 } });
        EXPECT_EQ(IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE, node->GetExecuteFlags());
        EXPECT_NE(nullptr, node->GetRenderOutputProperties());
        EXPECT_NE(nullptr, node->GetRenderInputProperties());
    }
    // TAA interface
    {
        RenderPostProcessTaaNode taa;
        auto* pp = static_cast<IRenderPostProcess*>(taa.GetInterface(IRenderPostProcess::UID));
        ASSERT_EQ(BASE_NS::Uid("8589eded-49de-4128-852a-f3e521561f93"), pp->GetRenderPostProcessNodeUid());

        RenderPostProcessTaaNode::EffectProperties properties;
        properties.enabled = true;
        properties.taaConfiguration.ignoreBicubicEdges = true;
        properties.taaConfiguration.useBicubic = true;
        properties.taaConfiguration.useVarianceClipping = false;
        properties.taaConfiguration.useyCoCG = false;
        properties.taaConfiguration.quality = TaaConfiguration::Quality::HIGH;
        properties.taaConfiguration.sharpness = TaaConfiguration::Sharpness::SHARP;
        const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&properties);
        BASE_NS::array_view<const uint8_t> dataView(dataPtr, sizeof(RenderPostProcessTaaNode::EffectProperties));
        pp->SetData(dataView);
        BASE_NS::array_view<const uint8_t> getDataView = pp->GetData();
        ASSERT_EQ(0, memcmp(dataView.data(), getDataView.data(), dataView.size()));
        auto* propertyHandle = pp->GetProperties();
        EXPECT_TRUE(CORE_NS::GetPropertyValue<bool>(propertyHandle, "enabled"));
        EXPECT_EQ(CORE_NS::GetPropertyValue<bool>(propertyHandle, "taaConfiguration.ignoreBicubicEdges"),
            properties.taaConfiguration.ignoreBicubicEdges);

        auto* node = static_cast<IRenderPostProcessNode*>(pp->GetInterface(IRenderPostProcessNode::UID));
        node->SetRenderAreaRequest(
            IRenderPostProcessNode::RenderAreaRequest { RenderPassDesc::RenderArea { 0, 0, 1, 1 } });
        EXPECT_EQ(IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE, node->GetExecuteFlags());
        EXPECT_NE(nullptr, node->GetRenderOutputProperties());
        EXPECT_NE(nullptr, node->GetRenderInputProperties());
    }
    // Upscale interface
    {
        RenderPostProcessUpscaleNode upscale;
        auto* pp = static_cast<IRenderPostProcess*>(upscale.GetInterface(IRenderPostProcess::UID));
        ASSERT_EQ(BASE_NS::Uid("81321991-8314-4c74-bc6a-487eed297550"), pp->GetRenderPostProcessNodeUid());

        RenderPostProcessUpscaleNode::EffectProperties properties;
        properties.enabled = true;
        properties.upscaleConfiguration.ratio = 0.5;
        const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&properties);
        BASE_NS::array_view<const uint8_t> dataView(dataPtr, sizeof(RenderPostProcessUpscaleNode::EffectProperties));
        pp->SetData(dataView);
        BASE_NS::array_view<const uint8_t> getDataView = pp->GetData();
        ASSERT_EQ(0, memcmp(dataView.data(), getDataView.data(), dataView.size()));
        auto* propertyHandle = pp->GetProperties();
        EXPECT_TRUE(CORE_NS::GetPropertyValue<bool>(propertyHandle, "enabled"));
        EXPECT_FLOAT_EQ(CORE_NS::GetPropertyValue<float>(propertyHandle, "upscaleConfiguration.ratio"),
            properties.upscaleConfiguration.ratio);

        auto* node = static_cast<IRenderPostProcessNode*>(pp->GetInterface(IRenderPostProcessNode::UID));
        node->SetRenderAreaRequest(
            IRenderPostProcessNode::RenderAreaRequest { RenderPassDesc::RenderArea { 0, 0, 1, 1 } });
        EXPECT_EQ(IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE, node->GetExecuteFlags());
        EXPECT_NE(nullptr, node->GetRenderOutputProperties());
        EXPECT_NE(nullptr, node->GetRenderInputProperties());
    }
    UTest::DestroyEngine(engine);
}
