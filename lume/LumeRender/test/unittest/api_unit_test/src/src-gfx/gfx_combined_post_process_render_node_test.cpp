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

#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/util/intf_render_frame_util.h>
#include <render/util/intf_render_util.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;

namespace {
static constexpr Math::UVec2 TEST_DATA_SIZE { 512u, 512u };
static constexpr size_t IMAGE_SIZE = TEST_DATA_SIZE.x * TEST_DATA_SIZE.y * 4u;
static constexpr size_t NUM_BYTES = IMAGE_SIZE * sizeof(float) / 4u;
float imageData[IMAGE_SIZE];
static constexpr string_view INPUT_IMAGE_NAME_0 { "InputImage0" };
// NOTE: created in render node graph
static constexpr string_view OUTPUT_IMAGE_NAME_0 { "OutputImage0" };

constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
constexpr const string_view RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY = "RenderDataStoreDefaultGpuResourceDataCopy";

struct TestResources {
    RenderHandleReference inputImageHandle0;

    unique_ptr<ByteArray> byteArray;

    RenderHandleReference renderNodeGraph;

    refcnt_ptr<IRenderDataStorePod> dataStorePod;
};

struct TestData {
    UTest::EngineResources engine;
    TestResources resources;
};

array_view<const uint8_t> CreateImageDataView()
{
    for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
            constexpr float value = 15.f;
            if (i < TEST_DATA_SIZE.x / 2 && (i + j) % 2 == 0) {
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u] = value; // R
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u] = 0.f;   // G
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u] = 0.f;   // B
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u] = 1.f;   // A
            } else {
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u] = 0.f;   // R
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u] = value; // G
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u] = 0.f;   // B
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u] = 1.f;   // A
            }
        }
    }
#if RENDER_SAVE_TEST_IMAGES == 1
    UTest::SaveHdrImage("CombinedPostProcessTestInput.png", TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, imageData);
#endif // RENDER_SAVE_TEST_IMAGES
    return array_view<const uint8_t> { reinterpret_cast<const uint8_t*>(imageData), sizeof(imageData) };
}

TestResources CreateTestResources(
    UTest::EngineResources& er, bool useCompute, bool blitOnly, bool useAa, bool useMipmaps)
{
    TestResources res;
    // Input image
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        if (useMipmaps) {
            imageDesc.mipCount = 6;
        }
        imageDesc.format = BASE_FORMAT_R32G32B32A32_SFLOAT;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags =
            CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT;
        const array_view<const uint8_t> dataView = CreateImageDataView();
        res.inputImageHandle0 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_0, imageDesc, dataView);
    }
    // Post processing configuration
    {
        res.dataStorePod = refcnt_ptr<IRenderDataStorePod>(
            er.context->GetRenderDataStoreManager().Create(IRenderDataStorePod::UID, "PostProcessDataStore"));
        PostProcessConfiguration ppConf;

        if (blitOnly) {
            ppConf.enableFlags = 0;
        } else {
            ppConf.enableFlags = PostProcessConfiguration::ENABLE_BLOOM_BIT | PostProcessConfiguration::ENABLE_BLUR_BIT;

            if (useAa) {
                ppConf.enableFlags |=
                    PostProcessConfiguration::ENABLE_FXAA_BIT | PostProcessConfiguration::ENABLE_TAA_BIT;
            }

            ppConf.bloomConfiguration.bloomQualityType = BloomConfiguration::BloomQualityType::QUALITY_TYPE_LOW;
            ppConf.bloomConfiguration.useCompute = useCompute;

            ppConf.blurConfiguration.blurQualityType = BlurConfiguration::QUALITY_TYPE_HIGH;
            ppConf.blurConfiguration.blurType = BlurConfiguration::TYPE_NORMAL;
            ppConf.blurConfiguration.filterSize = 10.f;
            if (useMipmaps) {
                ppConf.blurConfiguration.maxMipLevel = 4;
            }

            ppConf.fxaaConfiguration.quality = FxaaConfiguration::Quality::HIGH;
            ppConf.fxaaConfiguration.sharpness = FxaaConfiguration::Sharpness::SHARP;

            ppConf.taaConfiguration.quality = TaaConfiguration::Quality::HIGH;
            ppConf.taaConfiguration.sharpness = TaaConfiguration::Sharpness::SHARP;
        }

        const array_view<const uint8_t> dataView = { reinterpret_cast<const uint8_t*>(&ppConf), sizeof(ppConf) };
        res.dataStorePod->CreatePod("PostProcessConfiguration", "PostProcessConfiguration", dataView);
    }
    // Render node graph
    {
        if (useMipmaps) {
            res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
                IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
                "test://renderNodeGraphGfxCombinedPostProcessRenderNodeMipmapTest.rng");
        } else {
            res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
                IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
                "test://renderNodeGraphGfxCombinedPostProcessRenderNodeTest.rng");
        }
    }

    res.byteArray = make_unique<ByteArray>(NUM_BYTES);

    return res;
}

void DestroyTestResources(IDevice& device, TestResources& tr)
{
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    tr.inputImageHandle0 = {};
    tr.renderNodeGraph = {};

    tr.byteArray.reset();
}

void TickTest(TestData& td, int32_t frameCountToTick)
{
    UTest::EngineResources& er = td.engine;
    TestResources& tr = td.resources;
    IRenderFrameUtil& rfUtil = er.context->GetRenderUtil().GetRenderFrameUtil();

    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        if (idx == 1) {
            IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
            const RenderHandleReference outputImageHandle0 = gpuResourceMgr.GetImageHandle(OUTPUT_IMAGE_NAME_0);
            const IRenderFrameUtil::CopyFlags copyFlags = IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE;
            rfUtil.CopyToCpu(outputImageHandle0, copyFlags);
        }

        er.engine->TickFrame();
        const RenderHandleReference inputs[] = { tr.inputImageHandle0 };
        er.context->GetRenderNodeGraphManager().SetRenderNodeGraphResources(
            tr.renderNodeGraph, { inputs, countof(inputs) }, {});

        if (idx == 0) {
            er.context->GetRenderer().RenderFrame({ &tr.renderNodeGraph, 1u });
        } else {
            er.context->GetRenderer().RenderFrame({});
        }

        if (idx == 1) {
            IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
            const RenderHandleReference outputImageHandle0 = gpuResourceMgr.GetImageHandle(OUTPUT_IMAGE_NAME_0);
            const IRenderFrameUtil::FrameCopyData& data = rfUtil.GetFrameCopyData(outputImageHandle0);
            const GpuBufferDesc desc = gpuResourceMgr.GetBufferDescriptor(data.bufferHandle);
            const uint32_t minSize = Math::min(desc.byteSize, static_cast<uint32_t>(data.byteBuffer->GetData().size()));

            EXPECT_EQ(NUM_BYTES, data.byteBuffer->GetData().size());
            if (data.byteBuffer && (data.copyFlags & IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE)) {
                const uint32_t minSize =
                    Math::min(desc.byteSize, static_cast<uint32_t>(data.byteBuffer->GetData().size()));
                // map the GPU buffer memory and copy to CPU ByteArray buffer
                if (void* mem = gpuResourceMgr.MapBufferMemory(data.bufferHandle); mem) {
                    BASE_NS::CloneData(
                        td.resources.byteArray->GetData().data(), data.byteBuffer->GetData().size(), mem, minSize);
                }
            }
        }
    }
}

string GetFileName(const UTest::EngineResources& er, bool onlyBlit, bool useAa)
{
    string fileName = "CombinesPostProcessTestOutput";

    if (onlyBlit) {
        fileName += "OnlyBlit";
    }

    if (er.backend == DeviceBackendType::VULKAN) {
        fileName += "Vulkan";
    } else {
        fileName += "OpenGL";
    }

    if (useAa) {
        fileName += "WithAa";
    }

    fileName += ".png";
    return fileName;
}

void ValidateDataTest(const TestData& td, bool onlyBlit, bool useAa)
{
    ASSERT_EQ(NUM_BYTES, td.resources.byteArray->GetData().size());
    float* outputImageData = reinterpret_cast<float*>(td.resources.byteArray->GetData().data());
    // Saving the output image
    // UTest::SaveHdrImage(
    // GetFileName(td.engine, onlyBlit, useAa), TEST_DATA_SIZE.x / 2u, TEST_DATA_SIZE.y / 2u, outputImageData);
#if RENDER_SAVE_TEST_IMAGES == 1
    UTest::SaveHdrImage(
        GetFileName(td.engine, onlyBlit, useAa), TEST_DATA_SIZE.x / 2u, TEST_DATA_SIZE.y / 2u, outputImageData);
#endif // RENDER_SAVE_TEST_IMAGES
    for (size_t i = 0; i < TEST_DATA_SIZE.x / 2u; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y / 2u; ++j) {
            constexpr size_t stride = TEST_DATA_SIZE.y / 2u;
            float R = UTest::FromHDR(outputImageData[i * stride * 4u + j * 4u + 0u]);
            float G = UTest::FromHDR(outputImageData[i * stride * 4u + j * 4u + 1u]);
            float B = UTest::FromHDR(outputImageData[i * stride * 4u + j * 4u + 2u]);
            float A = outputImageData[i * stride * 4u + j * 4u + 3u];

            if (!useAa) {
                if (i < TEST_DATA_SIZE.x / 4u) {
                    if (!onlyBlit) {
                        constexpr float threshold = 0.1f;
                        ASSERT_GE(threshold, abs(R - G));
                    }
                } else {
                    if (!onlyBlit && i < TEST_DATA_SIZE.x / 4u + 64u) {
                        ASSERT_LT(0.0f, R);
                    }
                    ASSERT_LE(0.9f, G);
                }

                ASSERT_EQ(0.0f, B);
                ASSERT_EQ(1.0f, A);
            }
        }
    }
}

void TestCombinedPostProcessRenderNode(DeviceBackendType backend, bool onlyBlit, bool useAa, bool useMipmaps)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResources(testData.engine, false, onlyBlit, useAa, useMipmaps);
    }
    TickTest(testData, 3);
    ValidateDataTest(testData, onlyBlit, useAa);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: BlitTestVulkan
 * @tc.desc: Tests RenderNodeCombinedPostProcess with Blit only in Vulkan. RenderNodeCombinedPostProcess allows to
 * enable multiple post processes at the same time.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxCombinedPostProcessRenderNode, BlitTestVulkan, testing::ext::TestSize.Level1)
{
    TestCombinedPostProcessRenderNode(DeviceBackendType::VULKAN, true, false, false);
}
/**
 * @tc.name: CombinedPostProcessBlurTestVulkan
 * @tc.desc: Tests RenderNodeCombinedPostProcess with a Blur post process in Vulkan. RenderNodeCombinedPostProcess
 * allows to enable multiple post processes at the same time.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxCombinedPostProcessRenderNode, CombinedPostProcessBlurTestVulkan, testing::ext::TestSize.Level1)
{
    TestCombinedPostProcessRenderNode(DeviceBackendType::VULKAN, false, false, false);
}
/**
 * @tc.name: MipmapBlurTestVulkan
 * @tc.desc: Tests RenderNodeCombinedPostProcess with a Blur post process and mipmaps in Vulkan.
 * RenderNodeCombinedPostProcess allows to enable multiple post processes at the same time.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxCombinedPostProcessRenderNode, MipmapBlurTestVulkan, testing::ext::TestSize.Level1)
{
    TestCombinedPostProcessRenderNode(DeviceBackendType::VULKAN, false, false, true);
}
/**
 * @tc.name: AntiAliasingTestVulkan
 * @tc.desc: Tests for Anti Aliasing Test Vulkan. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxCombinedPostProcessRenderNode, AntiAliasingTestVulkan, testing::ext::TestSize.Level1)
{
    TestCombinedPostProcessRenderNode(DeviceBackendType::VULKAN, false, true, false);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: BlitTestOpenGL
 * @tc.desc: Tests RenderNodeCombinedPostProcess with Blit only in OpenGL. RenderNodeCombinedPostProcess allows to
 * enable multiple post processes at the same time.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxCombinedPostProcessRenderNode, BlitTestOpenGL, testing::ext::TestSize.Level1)
{
    TestCombinedPostProcessRenderNode(UTest::GetOpenGLBackend(), true, false, false);
}
/**
 * @tc.name: CombinedPostProcessBlurTestOpenGL
 * @tc.desc: Tests RenderNodeCombinedPostProcess with a Blur post process in OpenGL. RenderNodeCombinedPostProcess
 * allows to enable multiple post processes at the same time.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxCombinedPostProcessRenderNode, CombinedPostProcessBlurTestOpenGL, testing::ext::TestSize.Level1)
{
    TestCombinedPostProcessRenderNode(UTest::GetOpenGLBackend(), false, false, false);
}
/**
 * @tc.name: MipmapBlurTestOpenGL
 * @tc.desc: Tests RenderNodeCombinedPostProcess with a Blur post process and mipmaps in OpenGL.
 * RenderNodeCombinedPostProcess allows to enable multiple post processes at the same time.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxCombinedPostProcessRenderNode, MipmapBlurTestOpenGL, testing::ext::TestSize.Level1)
{
    TestCombinedPostProcessRenderNode(UTest::GetOpenGLBackend(), false, false, true);
}
#ifdef DISABLED_TESTS_ON
#if NDEBUG
/**
 * @tc.name: AntiAliasingTestOpenGL
 * @tc.desc: Tests for Anti Aliasing Test Open Gl. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxCombinedPostProcessRenderNode, AntiAliasingTestOpenGL, testing::ext::TestSize.Level1)
#else
/**
 * @tc.name: AntiAliasingTestOpenGL
 * @tc.desc: Tests for Anti Aliasing Test Open Gl. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxCombinedPostProcessRenderNode, DISABLED_AntiAliasingTestOpenGL, testing::ext::TestSize.Level1)
#endif // NDEBUG
{
    TestCombinedPostProcessRenderNode(UTest::GetOpenGLBackend(), false, true, false);
}
#endif // DISABLED_TESTS_ON
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
