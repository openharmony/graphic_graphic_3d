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

#include <base/math/float_packer.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/intf_render_data_store_post_process.h>
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
static constexpr size_t NUM_BYTES = IMAGE_SIZE * sizeof(uint16_t);
uint16_t imageData[IMAGE_SIZE];
float testDataTonemapper[IMAGE_SIZE];
static constexpr string_view INPUT_IMAGE_NAME_0 { "InputImage0" };
// NOTE: created in render node graph
static constexpr string_view OUTPUT_IMAGE_NAME_0 { "OutputImage0" };

constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
constexpr const string_view RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY = "RenderDataStoreDefaultGpuResourceDataCopy";

struct TestResources {
    RenderHandleReference inputImageHandle0;

    unique_ptr<ByteArray> byteArray;

    RenderHandleReference renderNodeGraph;
};

struct TestData {
    UTest::EngineResources engine;
    TestResources resources;
};

inline Math::Vec3 Mix(const Math::Vec3& a, const Math::Vec3& b, float t)
{
    return Math::Vec3(a.x * (1.0f - t) + b.x * t, a.y * (1.0f - t) + b.y * t, a.z * (1.0f - t) + b.z * t);
}
Math::Vec3 RunPbrTonemapper(const Math::Vec3& rgb)
{
    Math::Vec3 color = rgb;
    const float startCompression = 0.8f - 0.04f;
    const float desaturation = 0.15f;

    float x = Math::min(color.x, Math::min(color.y, color.z));
    float offset = x < 0.08f ? x - 6.25f * x * x : 0.04f;
    color.x -= offset;
    color.y -= offset;
    color.z -= offset;

    float peak = Math::max(color.x, Math::max(color.y, color.z));
    if (peak < startCompression)
        return color;

    const float d = 1.0f - startCompression;
    float newPeak = 1.0f - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    float g = 1.0f - 1.0f / (desaturation * (peak - newPeak) + 1.0f);
    return Mix(color, Math::Vec3(newPeak, newPeak, newPeak), g);
}

array_view<const uint8_t> CreateImageDataView(string type)
{
    if (type == "Bloom") {
        for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
            for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
                if (i < TEST_DATA_SIZE.x / 2) {
                    float value = 15.f;
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u] = BASE_NS::Math::F32ToF16(value); // R
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u] = BASE_NS::Math::F32ToF16(value); // G
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u] = BASE_NS::Math::F32ToF16(value); // B
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u] = BASE_NS::Math::F32ToF16(1.0f);  // A
                } else {
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u] = BASE_NS::Math::F32ToF16(0.0f); // R
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u] = BASE_NS::Math::F32ToF16(0.0f); // G
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u] = BASE_NS::Math::F32ToF16(0.0f); // B
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u] = BASE_NS::Math::F32ToF16(1.0f); // A
                }
            }
        }
#if RENDER_SAVE_TEST_IMAGES == 1
        // convert uint16_t data to float
        float* floatData = new float[IMAGE_SIZE];
        for (size_t ii = 0; ii < IMAGE_SIZE; ii++) {
            floatData[ii] = BASE_NS::Math::F16ToF32(imageData[ii]);
        }
        UTest::SaveHdrImage("SinglePostProcessBloomTestInput.png", TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, floatData);
        delete[] floatData;
#endif // RENDER_SAVE_TEST_IMAGES
    } else if (type == "Blur") {
        for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
            for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
                constexpr float value = 15.f;
                if (i < TEST_DATA_SIZE.x / 2 && (i + j) % 2 == 0) {
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u] = BASE_NS::Math::F32ToF16(value); // R
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u] = BASE_NS::Math::F32ToF16(0.0f);  // G
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u] = BASE_NS::Math::F32ToF16(0.0f);  // B
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u] = BASE_NS::Math::F32ToF16(1.0f);  // A
                } else {
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u] = BASE_NS::Math::F32ToF16(0.0f);  // R
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u] = BASE_NS::Math::F32ToF16(value); // G
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u] = BASE_NS::Math::F32ToF16(0.0f);  // B
                    imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u] = BASE_NS::Math::F32ToF16(1.0f);  // A
                }
            }
        }
#if RENDER_SAVE_TEST_IMAGES == 1
        // convert uint16_t data to float
        float* floatData = new float[IMAGE_SIZE];
        for (size_t ii = 0; ii < IMAGE_SIZE; ii++) {
            floatData[ii] = BASE_NS::Math::F16ToF32(imageData[ii]);
        }
        UTest::SaveHdrImage("SinglePostProcessBlurTestInput.png", TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, floatData);
        delete[] floatData;
#endif // RENDER_SAVE_TEST_IMAGES
    } else if (type == "PbrTonemapper") {
        float w = float(TEST_DATA_SIZE.x);
        float h = float(TEST_DATA_SIZE.y);
        for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
            for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
                float x = (float)i;
                float y = (float)j;

                float r = x / w;
                float g = y / h;
                float b = (x / w + y / h) / 2.0f;

                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u] = BASE_NS::Math::F32ToF16(r);    // R
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u] = BASE_NS::Math::F32ToF16(g);    // G
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u] = BASE_NS::Math::F32ToF16(b);    // B
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u] = BASE_NS::Math::F32ToF16(1.0f); // A

                // Fill the validation test image
                Math::Vec3 newRgb = RunPbrTonemapper(Math::Vec3(r, g, b));
                testDataTonemapper[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u] = newRgb.x;
                testDataTonemapper[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u] = newRgb.y;
                testDataTonemapper[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u] = newRgb.z;
                testDataTonemapper[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u] = 1.0f;
            }
        }
#if RENDER_SAVE_TEST_IMAGES == 1
        // convert uint16_t data to float
        float* floatData = new float[IMAGE_SIZE];
        for (size_t ii = 0; ii < IMAGE_SIZE; ii++) {
            floatData[ii] = BASE_NS::Math::F16ToF32(imageData[ii]);
        }
        UTest::SaveHdrImage("SinglePostProcessPbrTonemapperInput.png", TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, floatData);
        delete[] floatData;
#endif // RENDER_SAVE_TEST_IMAGES
    }
    return array_view<const uint8_t> { reinterpret_cast<const uint8_t*>(imageData), sizeof(imageData) };
}

TestResources CreateTestResources(UTest::EngineResources& er, bool useCompute, string type)
{
    TestResources res;
    // Input image
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                               CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        const array_view<const uint8_t> dataView = CreateImageDataView(type);
        res.inputImageHandle0 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_0, imageDesc, dataView);
    }
    // Post processing configuration
    {
        refcnt_ptr<IRenderDataStorePod> dataStorePod =
            er.context->GetRenderDataStoreManager().GetRenderDataStore("RenderDataStorePod");
        PostProcessConfiguration ppConf;

        if (type == "Bloom") {
            ppConf.enableFlags = PostProcessConfiguration::ENABLE_BLOOM_BIT;
        } else if (type == "normal_blur" || type == "vertical_blur" || type == "horizontal_blur") {
            ppConf.enableFlags = PostProcessConfiguration::ENABLE_BLUR_BIT;
        }

        ppConf.bloomConfiguration.bloomQualityType = BloomConfiguration::BloomQualityType::QUALITY_TYPE_NORMAL;
        ppConf.bloomConfiguration.useCompute = useCompute;

        ppConf.blurConfiguration.blurQualityType = BlurConfiguration::QUALITY_TYPE_HIGH;
        if (type == "normal_blur") {
            ppConf.blurConfiguration.blurType = BlurConfiguration::TYPE_NORMAL;
        } else if (type == "vertical_blur") {
            ppConf.blurConfiguration.blurType = BlurConfiguration::TYPE_VERTICAL;
        } else {
            ppConf.blurConfiguration.blurType = BlurConfiguration::TYPE_HORIZONTAL;
        }
        ppConf.blurConfiguration.filterSize = 10.f;
        ppConf.blurConfiguration.maxMipLevel = 5;

        ppConf.fxaaConfiguration.quality = FxaaConfiguration::Quality::HIGH;
        ppConf.fxaaConfiguration.sharpness = FxaaConfiguration::Sharpness::SHARP;

        ppConf.taaConfiguration.quality = TaaConfiguration::Quality::HIGH;
        ppConf.taaConfiguration.sharpness = TaaConfiguration::Sharpness::SHARP;

        const array_view<const uint8_t> dataView = { reinterpret_cast<const uint8_t*>(&ppConf), sizeof(ppConf) };
        if (type == "Bloom") {
            dataStorePod->CreatePod("RenderDataStorePod", "PostProcessConfiguration", dataView);
        } else {
            dataStorePod->CreatePod("PostProcessConfiguration", "PostProcessConfiguration", dataView);
        }

        {
            refcnt_ptr<IRenderDataStorePostProcess> dataStorePostProcess =
                er.context->GetRenderDataStoreManager().Create(
                    IRenderDataStorePostProcess::UID, "RenderDataStorePostProcess");
            dataStorePostProcess->Create("PostProcessConfiguration");
            IRenderDataStorePostProcess::PostProcess::Variables variables;
            variables.enabled = true;
            if (type == "Bloom") {
                dataStorePostProcess->Create("PostProcessConfiguration", "render_bloom", {});
                dataStorePostProcess->Set("PostProcessConfiguration", "render_bloom", variables);
            } else if (type == "normal_blur" || type == "vertical_blur" || type == "horizontal_blur") {
                dataStorePostProcess->Create("PostProcessConfiguration", "render_blur", {});
                dataStorePostProcess->Set("PostProcessConfiguration", "render_blur", variables);
            } else { // Used for both CustomShader and PbrTonemap shader
                dataStorePostProcess->Create("PostProcessConfiguration", "render_custom", {});
                dataStorePostProcess->Set("PostProcessConfiguration", "render_custom", variables);
            }
        }
    }
    // Render node graph
    {
        if (type == "Bloom") {
            res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
                IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
                "test://renderNodeGraphGfxBloomSinglePostProcessRenderNodeTest.rng");
        } else if (type == "normal_blur" || type == "vertical_blur" || type == "horizontal_blur") {
            res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
                IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
                "test://renderNodeGraphGfxBlurSinglePostProcessRenderNodeTest.rng");
        } else if (type == "CustomShader") {
            res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
                IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
                "test://renderNodeGraphGfxCustomShaderSinglePostProcessRenderNodeTest.rng");
        } else if (type == "PbrTonemapper") {
            res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
                IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
                "test://renderNodeGraphGfxPbrTonemapperRenderNodeTest.rng");
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
            er.context->GetRenderer().RenderDeferred({ &tr.renderNodeGraph, 1u });
            er.context->GetRenderer().RenderDeferredFrame();
        } else {
            er.context->GetRenderer().RenderFrame({});
        }

        if (idx == 1) {
            IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
            const RenderHandleReference outputImageHandle0 = gpuResourceMgr.GetImageHandle(OUTPUT_IMAGE_NAME_0);
            const IRenderFrameUtil::FrameCopyData& data = rfUtil.GetFrameCopyData(outputImageHandle0);
            const GpuBufferDesc desc = gpuResourceMgr.GetBufferDescriptor(data.bufferHandle);

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

string GetBloomFileName(const UTest::EngineResources& er, bool useCompute)
{
    if (er.backend == DeviceBackendType::VULKAN) {
        if (useCompute) {
            return "SinglePostProcessBloomTestOutputVulkanCompute.png";
        } else {
            return "SinglePostProcessBloomTestOutputVulkanGraphics.png";
        }
    } else {
        if (useCompute) {
            return "SinglePostProcessBloomTestOutputOpenGLCompute.png";
        } else {
            return "SinglePostProcessBloomTestOutputOpenGLGraphics.png";
        }
    }
}

string GetBlurFileName(const UTest::EngineResources& er)
{
    if (er.backend == DeviceBackendType::VULKAN) {
        return "SinglePostProcessBlurTestOutputVulkan.png";
    } else {
        return "SinglePostProcessBlurTestOutputOpenGL.png";
    }
}

string GetCustomShaderFileName(const UTest::EngineResources& er)
{
    if (er.backend == DeviceBackendType::VULKAN) {
        return "SinglePostProcessCustomShaderTestOutputVulkan.png";
    } else {
        return "SinglePostProcessCustomShaderTestOutputOpenGL.png";
    }
}

string GetPbrTonemapperShaderFileName(const UTest::EngineResources& er)
{
    if (er.backend == DeviceBackendType::VULKAN) {
        return "SinglePostProcessPbrTonemapperShaderTestOutputVulkan.png";
    } else {
        return "SinglePostProcessPbrTonemapperShaderTestOutputOpenGL.png";
    }
}

void ValidateBloom(const TestData& td, bool useCompute)
{
    ASSERT_EQ(NUM_BYTES, td.resources.byteArray->GetData().size());
    uint16_t* outputImageData = reinterpret_cast<uint16_t*>(td.resources.byteArray->GetData().data());
#if RENDER_SAVE_TEST_IMAGES == 1
    // convert uint16_t data to float
    float* floatData = new float[IMAGE_SIZE];
    for (size_t ii = 0; ii < IMAGE_SIZE; ii++) {
        floatData[ii] = BASE_NS::Math::F16ToF32(outputImageData[ii]);
    }
    UTest::SaveHdrImage(GetBloomFileName(td.engine, useCompute), TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, floatData);
    delete[] floatData;
#endif // RENDER_SAVE_TEST_IMAGES
    for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
            float R = UTest::FromHDR(BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u]));
            float G = UTest::FromHDR(BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u]));
            float B = UTest::FromHDR(BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u]));
            float A = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u]);

            if (i < TEST_DATA_SIZE.x / 2) {
                ASSERT_LE(0.9f, R);
                ASSERT_LE(0.9f, G);
                ASSERT_LE(0.9f, B);
            } else {
                float threshold = static_cast<float>(TEST_DATA_SIZE.x - i) / (TEST_DATA_SIZE.x / 2) - 0.1f;
                ASSERT_LE(threshold, R);
                ASSERT_LE(threshold, G);
                ASSERT_LE(threshold, B);
            }

            ASSERT_EQ(1.0f, A);
        }
    }
}

void ValidateBlur(const TestData& td)
{
    ASSERT_EQ(NUM_BYTES, td.resources.byteArray->GetData().size());
    uint16_t* outputImageData = reinterpret_cast<uint16_t*>(td.resources.byteArray->GetData().data());
#if RENDER_SAVE_TEST_IMAGES == 1
    // convert uint16_t data to float
    float* floatData = new float[IMAGE_SIZE];
    for (size_t ii = 0; ii < IMAGE_SIZE; ii++) {
        floatData[ii] = BASE_NS::Math::F16ToF32(outputImageData[ii]);
    }
    UTest::SaveHdrImage(GetBlurFileName(td.engine), TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, floatData);
    delete[] floatData;
#endif // RENDER_SAVE_TEST_IMAGES
}

void ValidateCustomShader(const TestData& td)
{
    ASSERT_EQ(NUM_BYTES, td.resources.byteArray->GetData().size());
    uint16_t* outputImageData = reinterpret_cast<uint16_t*>(td.resources.byteArray->GetData().data());
#if RENDER_SAVE_TEST_IMAGES == 1
    // convert uint16_t data to float
    float* floatData = new float[IMAGE_SIZE];
    for (size_t ii = 0; ii < IMAGE_SIZE; ii++) {
        floatData[ii] = BASE_NS::Math::F16ToF32(outputImageData[ii]);
    }
    UTest::SaveHdrImage(GetCustomShaderFileName(td.engine), TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, floatData);
    delete[] floatData;
#endif // RENDER_SAVE_TEST_IMAGES
    for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
            constexpr size_t stride = TEST_DATA_SIZE.y;
            float R = UTest::FromHDR(BASE_NS::Math::F16ToF32(outputImageData[i * stride * 4u + j * 4u + 0u]));
            float G = UTest::FromHDR(BASE_NS::Math::F16ToF32(outputImageData[i * stride * 4u + j * 4u + 1u]));
            float B = UTest::FromHDR(BASE_NS::Math::F16ToF32(outputImageData[i * stride * 4u + j * 4u + 2u]));
            float A = BASE_NS::Math::F16ToF32(outputImageData[i * stride * 4u + j * 4u + 3u]);

            ASSERT_EQ(1.0f, A);
        }
    }
}
void ValidatePbrTonemapperShader(const TestData& td)
{
    ASSERT_EQ(NUM_BYTES, td.resources.byteArray->GetData().size());
    uint16_t* outputImageData = reinterpret_cast<uint16_t*>(td.resources.byteArray->GetData().data());
#if RENDER_SAVE_TEST_IMAGES == 1
    // convert uint16_t data to float
    float* floatData = new float[IMAGE_SIZE];
    for (size_t ii = 0; ii < IMAGE_SIZE; ii++) {
        floatData[ii] = BASE_NS::Math::F16ToF32(outputImageData[ii]);
    }
    UTest::SaveHdrImage(GetPbrTonemapperShaderFileName(td.engine), TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, floatData);
    delete[] floatData;
#endif // RENDER_SAVE_TEST_IMAGES
    for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
            float R = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u]);
            float G = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u]);
            float B = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u]);
            float A = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u]);

            float expectedR = testDataTonemapper[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u];
            float expectedG = testDataTonemapper[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u];
            float expectedB = testDataTonemapper[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u];
            float expectedA = testDataTonemapper[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u];

            float epsilon = 0.001f;
            ASSERT_NEAR(R, expectedR, epsilon);
            ASSERT_NEAR(G, expectedG, epsilon);
            ASSERT_NEAR(B, expectedB, epsilon);

            ASSERT_EQ(1.0f, A);
        }
    }
}

void TestBloom(DeviceBackendType backend, bool useCompute)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResources(testData.engine, useCompute, "Bloom");
    }
    TickTest(testData, 3);
    ValidateBloom(testData, useCompute);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}

void TestBlur(DeviceBackendType backend, string type)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResources(testData.engine, false, type + "_blur");
    }
    TickTest(testData, 3);
    ValidateBlur(testData);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}

void TestCustomShader(DeviceBackendType backend)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResources(testData.engine, false, "CustomShader");
    }
    TickTest(testData, 3);
    ValidateCustomShader(testData);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}

void TestPbrTonemapperShader(DeviceBackendType backend)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResources(testData.engine, false, "PbrTonemapper");
    }
    TickTest(testData, 3);
    ValidatePbrTonemapperShader(testData);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: SinglePostProcessRenderNodeGfxBloomTestVulkan
 * @tc.desc: Tests for a single bloom post process in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(
    API_GfxSinglePostProcessRenderNode, SinglePostProcessRenderNodeGfxBloomTestVulkan, testing::ext::TestSize.Level1)
{
    TestBloom(DeviceBackendType::VULKAN, false);
}
/**
 * @tc.name: SinglePostProcessRenderNodeComputeBloomTestVulkan
 * @tc.desc: Tests for a single bloom post process using compute in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxSinglePostProcessRenderNode, SinglePostProcessRenderNodeComputeBloomTestVulkan,
    testing::ext::TestSize.Level1)
{
    TestBloom(DeviceBackendType::VULKAN, true);
}
/**
 * @tc.name: SinglePostProcessRenderNodeBlurTestVulkan
 * @tc.desc: Tests for a single normal blur post process in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxSinglePostProcessRenderNode, SinglePostProcessRenderNodeBlurTestVulkan, testing::ext::TestSize.Level1)
{
    TestBlur(DeviceBackendType::VULKAN, "normal");
}
/**
 * @tc.name: SinglePostProcessRenderNodeVerticalBlurTestVulkan
 * @tc.desc: Tests for a single vertical blur post process in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxSinglePostProcessRenderNode, SinglePostProcessRenderNodeVerticalBlurTestVulkan,
    testing::ext::TestSize.Level1)
{
    TestBlur(DeviceBackendType::VULKAN, "vertical");
}
/**
 * @tc.name: SinglePostProcessRenderNodeHorizontalBlurTestVulkan
 * @tc.desc: Tests for a single horizontal blur post process in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxSinglePostProcessRenderNode, SinglePostProcessRenderNodeHorizontalBlurTestVulkan,
    testing::ext::TestSize.Level1)
{
    TestBlur(DeviceBackendType::VULKAN, "horizontal");
}
/**
 * @tc.name: SinglePostProcessRenderNodeCustomShaderTestVulkan
 * @tc.desc: Tests for a single custom post processing pass in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxSinglePostProcessRenderNode, SinglePostProcessRenderNodeCustomShaderTestVulkan,
    testing::ext::TestSize.Level1)
{
    TestCustomShader(DeviceBackendType::VULKAN);
}
/**
 * @tc.name: SinglePostProcessRenderNodePbrTonemapperShaderTestVulkan
 * @tc.desc: Tests for single PBR tonemapping post process in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxSinglePostProcessRenderNode, SinglePostProcessRenderNodePbrTonemapperShaderTestVulkan,
    testing::ext::TestSize.Level1)
{
    TestPbrTonemapperShader(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: SinglePostProcessRenderNodeGfxBloomTestOpenGL
 * @tc.desc: Tests for a single bloom post process in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(
    API_GfxSinglePostProcessRenderNode, SinglePostProcessRenderNodeGfxBloomTestOpenGL, testing::ext::TestSize.Level1)
{
    TestBloom(UTest::GetOpenGLBackend(), false);
}
#ifdef DISABLED_TESTS_ON
/**
 * @tc.name: SinglePostProcessRenderNodeComputeBloomTestOpenGL
 * @tc.desc: Tests for a single bloom post process using compute in OpenGL.
 * @tc.type: FUNC
 */
#ifdef __OHOS__
// Harmony OS compute bloom with GLES fail. Fails because the threshold is exceeded.
UNIT_TEST(API_GfxSinglePostProcessRenderNode, DISABLED_SinglePostProcessRenderNodeComputeBloomTestOpenGL,
    testing::ext::TestSize.Level1)
#else
UNIT_TEST(API_GfxSinglePostProcessRenderNode, SinglePostProcessRenderNodeComputeBloomTestOpenGL,
    testing::ext::TestSize.Level1)
#endif // __OHOS__
{
    TestBloom(UTest::GetOpenGLBackend(), true);
}
#endif // DISABLED_TESTS_ON
/**
 * @tc.name: SinglePostProcessRenderNodeBlurTestOpenGL
 * @tc.desc: Tests for a single normal blur post process in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxSinglePostProcessRenderNode, SinglePostProcessRenderNodeBlurTestOpenGL, testing::ext::TestSize.Level1)
{
    TestBlur(UTest::GetOpenGLBackend(), "normal");
}
/**
 * @tc.name: SinglePostProcessRenderNodeVerticalBlurTestOpenGL
 * @tc.desc: Tests for a single vertical blur post process in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxSinglePostProcessRenderNode, SinglePostProcessRenderNodeVerticalBlurTestOpenGL,
    testing::ext::TestSize.Level1)
{
    TestBlur(UTest::GetOpenGLBackend(), "vertical");
}
/**
 * @tc.name: SinglePostProcessRenderNodeHorizontalBlurTestOpenGL
 * @tc.desc: Tests for a single horizontal blur post process in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxSinglePostProcessRenderNode, SinglePostProcessRenderNodeHorizontalBlurTestOpenGL,
    testing::ext::TestSize.Level1)
{
    TestBlur(UTest::GetOpenGLBackend(), "horizontal");
}
/**
 * @tc.name: SinglePostProcessRenderNodeCustomShaderTestOpenGL
 * @tc.desc: Tests for a single custom post processing pass in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxSinglePostProcessRenderNode, SinglePostProcessRenderNodeCustomShaderTestOpenGL,
    testing::ext::TestSize.Level1)
{
    TestCustomShader(UTest::GetOpenGLBackend());
}
/**
 * @tc.name: SinglePostProcessRenderNodePbrTonemapperShaderTestOpenGL
 * @tc.desc: Tests for single PBR tonemapping post process in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxSinglePostProcessRenderNode, SinglePostProcessRenderNodePbrTonemapperShaderTestOpenGL,
    testing::ext::TestSize.Level1)
{
    TestPbrTonemapperShader(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
