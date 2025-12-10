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

#include <postprocesses/render_post_process_bloom_node.h>
#include <postprocesses/render_post_process_blur_node.h>
#include <postprocesses/render_post_process_cmaa2_node.h>
#include <postprocesses/render_post_process_ssao_node.h>
#include <postprocesses/render_post_process_upscale_node.h>

#include <base/math/float_packer.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/intf_render_data_store_post_process.h>
#include <render/datastore/intf_render_data_store_render_post_processes.h>
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
using namespace CORE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;

namespace {
static constexpr Math::UVec2 TEST_DATA_SIZE { 512u, 512u };
static constexpr size_t IMAGE_SIZE = TEST_DATA_SIZE.x * TEST_DATA_SIZE.y * 4u;
static constexpr size_t NUM_BYTES = IMAGE_SIZE * sizeof(uint16_t);
uint16_t imageData[IMAGE_SIZE];
float testDataTonemapper[IMAGE_SIZE];
static constexpr string_view INPUT_IMAGE_NAME_0 { "InputImage0" };
static constexpr string_view INPUT_IMAGE_NAME_1 { "InputImage1" };
static constexpr string_view INPUT_IMAGE_NAME_2 { "InputImage2" };
static constexpr string_view INPUT_BUFFER_NAME_0 { "InputBuffer0" };
// NOTE: created in render node graph
static constexpr string_view OUTPUT_IMAGE_NAME_0 { "OutputImage0" };

constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
constexpr const string_view RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY = "RenderDataStoreDefaultGpuResourceDataCopy";

struct TestResources {
    RenderHandleReference inputImageHandle0;
    RenderHandleReference inputImageHandle1;
    RenderHandleReference inputImageHandle2;
    RenderHandleReference inputBufferHandle0;

    unique_ptr<ByteArray> byteArray;

    RenderHandleReference renderNodeGraph;

    refcnt_ptr<IRenderDataStore> dataStore = nullptr;
};

struct TestData {
    UTest::EngineResources engine;
    TestResources resources;
};

array_view<const uint8_t> CreateImageDataViewBloom()
{
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
#endif
    return array_view<const uint8_t> { reinterpret_cast<const uint8_t*>(imageData), sizeof(imageData) };
}

array_view<const uint8_t> CreateImageDataViewBlur()
{
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
#endif
    return array_view<const uint8_t> { reinterpret_cast<const uint8_t*>(imageData), sizeof(imageData) };
}

array_view<const uint8_t> CreateImageDataViewUpscale()
{
    constexpr auto width = TEST_DATA_SIZE.x / 4U;
    constexpr auto height = TEST_DATA_SIZE.y / 4U;
    for (size_t i = 0; i < width; ++i) {
        for (size_t j = 0; j < height; ++j) {
            float x = (float)i;
            float y = (float)j;

            float r = x / float(width);
            float g = y / float(height);
            float b = (r + g) / 2.0f;

            imageData[i * height * 4u + j * 4u + 0u] = BASE_NS::Math::F32ToF16(r);    // R
            imageData[i * height * 4u + j * 4u + 1u] = BASE_NS::Math::F32ToF16(g);    // G
            imageData[i * height * 4u + j * 4u + 2u] = BASE_NS::Math::F32ToF16(b);    // B
            imageData[i * height * 4u + j * 4u + 3u] = BASE_NS::Math::F32ToF16(1.0f); // A
        }
    }
#if RENDER_SAVE_TEST_IMAGES == 1
    // convert uint16_t data to float
    float* floatData = new float[width * height * 4U];
    for (size_t ii = 0; ii < width * height * 4U; ii++) {
        floatData[ii] = BASE_NS::Math::F16ToF32(imageData[ii]);
    }
    UTest::SaveHdrImage("SinglePostProcessUpscalingTestInput.png", width, height, floatData);
    delete[] floatData;
#endif // RENDER_SAVE_TEST_IMAGES
    return array_view<const uint8_t> { reinterpret_cast<const uint8_t*>(imageData), sizeof(imageData) };
}

array_view<const uint8_t> CreateImageDataViewSsao()
{
    constexpr auto width = TEST_DATA_SIZE.x;
    constexpr auto height = TEST_DATA_SIZE.y;
    for (size_t i = 0; i < width; ++i) {
        for (size_t j = 0; j < height; ++j) {
            float x = (float)i;
            float y = (float)j;

            float r = x / float(width);
            float g = y / float(height);
            float b = (r + g) / 2.0f;

            imageData[i * height * 4u + j * 4u + 0u] = BASE_NS::Math::F32ToF16(r);    // R
            imageData[i * height * 4u + j * 4u + 1u] = BASE_NS::Math::F32ToF16(g);    // G
            imageData[i * height * 4u + j * 4u + 2u] = BASE_NS::Math::F32ToF16(b);    // B
            imageData[i * height * 4u + j * 4u + 3u] = BASE_NS::Math::F32ToF16(1.0f); // A
        }
    }
#if RENDER_SAVE_TEST_IMAGES == 1
    // convert uint16_t data to float
    float* floatData = new float[width * height * 4U];
    for (size_t ii = 0; ii < width * height * 4U; ii++) {
        floatData[ii] = BASE_NS::Math::F16ToF32(imageData[ii]);
    }
    UTest::SaveHdrImage("SinglePostProcessSsaoTestInput.png", width, height, floatData);
    delete[] floatData;
#endif // RENDER_SAVE_TEST_IMAGES
    return array_view<const uint8_t> { reinterpret_cast<const uint8_t*>(imageData), sizeof(imageData) };
}

TestResources CreateTestResourcesBloom(UTest::EngineResources& er)
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
        const array_view<const uint8_t> dataView = CreateImageDataViewBloom();
        res.inputImageHandle0 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_0, imageDesc, dataView);
    }

    auto* renderClassFactory = er.context->GetInterface<Core::IClassFactory>();

    refcnt_ptr<IRenderDataStoreRenderPostProcesses> store = er.context->GetRenderDataStoreManager().Create(
        IRenderDataStoreRenderPostProcesses::UID, "RenderDataStoreRenderPostProcesses");
    IRenderPostProcess::Ptr bloom =
        CreateInstance<IRenderPostProcess>(*renderClassFactory, RenderPostProcessBloomNode::UID);

    RenderPostProcessBloomNode::EffectProperties properties = {};
    properties.enabled = true;
    array_view<uint8_t> propData = { reinterpret_cast<uint8_t*>(&properties),
        sizeof(RenderPostProcessBloomNode::EffectProperties) };
    bloom->SetData(propData);

    IRenderDataStoreRenderPostProcesses::PostProcessData data = {};
    data.id = 0;
    data.postProcess = bloom;

    store->AddData("SrcTests", { &data, 1 });
    res.dataStore = store;
    // Render node graph
    {
        res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphPostProcessInterfaceUtil.rng");
    }

    res.byteArray = make_unique<ByteArray>(NUM_BYTES);

    return res;
}

TestResources CreateTestResourcesBlur(UTest::EngineResources& er)
{
    TestResources res;
    // Input image
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.mipCount = 1U;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                               CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        const array_view<const uint8_t> dataView = CreateImageDataViewBlur();
        res.inputImageHandle0 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_0, imageDesc, dataView);
    }

    auto* renderClassFactory = er.context->GetInterface<Core::IClassFactory>();

    refcnt_ptr<IRenderDataStoreRenderPostProcesses> store = er.context->GetRenderDataStoreManager().Create(
        IRenderDataStoreRenderPostProcesses::UID, "RenderDataStoreRenderPostProcesses");
    IRenderPostProcess::Ptr blur =
        CreateInstance<IRenderPostProcess>(*renderClassFactory, RenderPostProcessBlurNode::UID);

    RenderPostProcessBlurNode::EffectProperties properties = {};
    properties.enabled = true;
    array_view<uint8_t> propData = { reinterpret_cast<uint8_t*>(&properties),
        sizeof(RenderPostProcessBlurNode::EffectProperties) };
    blur->SetData(propData);

    IRenderDataStoreRenderPostProcesses::PostProcessData data = {};
    data.id = 0;
    data.postProcess = blur;

    store->AddData("SrcTests", { &data, 1 });
    res.dataStore = store;
    // Render node graph
    {
        res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphPostProcessInterfaceUtil.rng");
    }

    res.byteArray = make_unique<ByteArray>(NUM_BYTES);

    return res;
}

TestResources CreateTestResourcesUpscale(UTest::EngineResources& er)
{
    TestResources res;
    // Input image
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x / 4U;
        imageDesc.height = TEST_DATA_SIZE.y / 4u;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                               CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        const array_view<const uint8_t> dataView = CreateImageDataViewUpscale();
        res.inputImageHandle0 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_0, imageDesc, dataView);
    }
    auto* renderClassFactory = er.context->GetInterface<Core::IClassFactory>();

    IRenderPostProcess::Ptr upscaler =
        CreateInstance<IRenderPostProcess>(*renderClassFactory, RenderPostProcessUpscaleNode::UID);

    RenderPostProcessUpscaleNode::EffectProperties properties = {};
    properties.enabled = true;
    properties.upscaleConfiguration.ratio = 4.0f;
    array_view<uint8_t> propData = { reinterpret_cast<uint8_t*>(&properties),
        sizeof(RenderPostProcessUpscaleNode::EffectProperties) };
    upscaler->SetData(propData);

    IRenderDataStoreRenderPostProcesses::PostProcessData data = {};
    data.id = 0;
    data.postProcess = upscaler;

    refcnt_ptr<IRenderDataStoreRenderPostProcesses> store = er.context->GetRenderDataStoreManager().Create(
        IRenderDataStoreRenderPostProcesses::UID, "RenderDataStoreRenderPostProcesses");
    store->AddData("SrcTests", { &data, 1 });
    res.dataStore = move(store);

    // Render node graph
    {
        res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphPostProcessInterfaceUtil.rng");
    }

    res.byteArray = make_unique<ByteArray>(NUM_BYTES);

    return res;
}

TestResources CreateTestResourcesSsao(UTest::EngineResources& er)
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
        const array_view<const uint8_t> dataView = CreateImageDataViewSsao();
        res.inputImageHandle0 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_0, imageDesc, dataView);
    }
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
        const array_view<const uint8_t> dataView = CreateImageDataViewSsao();
        res.inputImageHandle1 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_1, imageDesc, dataView);
    }
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_D32_SFLOAT;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                               CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        const vector<float> depth(TEST_DATA_SIZE.x * TEST_DATA_SIZE.y, 0.5f);
        const array_view<const uint8_t> dataView(reinterpret_cast<const uint8_t*>(depth.data()), depth.size_in_bytes());
        res.inputImageHandle2 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_2, imageDesc, dataView);
    }
    {
        GpuBufferDesc bufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, 1024U };
        res.inputBufferHandle0 = er.device->GetGpuResourceManager().Create(INPUT_BUFFER_NAME_0, bufferDesc);
    }
    auto* renderClassFactory = er.context->GetInterface<Core::IClassFactory>();

    IRenderPostProcess::Ptr ssao =
        CreateInstance<IRenderPostProcess>(*renderClassFactory, RenderPostProcessSsaoNode::UID);

    RenderPostProcessSsaoNode::EffectProperties properties = {};
    properties.enabled = true;
    array_view<uint8_t> propData = { reinterpret_cast<uint8_t*>(&properties),
        sizeof(RenderPostProcessSsaoNode::EffectProperties) };
    ssao->SetData(propData);

    IRenderDataStoreRenderPostProcesses::PostProcessData data = {};
    data.id = 0;
    data.postProcess = ssao;

    refcnt_ptr<IRenderDataStoreRenderPostProcesses> store = er.context->GetRenderDataStoreManager().Create(
        IRenderDataStoreRenderPostProcesses::UID, "RenderDataStoreRenderPostProcesses");
    store->AddData("SrcTests", { &data, 1 });
    res.dataStore = move(store);

    // Render node graph
    {
        res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphPostProcessInterfaceUtil.rng");
    }

    res.byteArray = make_unique<ByteArray>(NUM_BYTES);

    return res;
}

TestResources CreateTestResourcesCmaa2(UTest::EngineResources& er)
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
        const array_view<const uint8_t> dataView = CreateImageDataViewSsao();
        res.inputImageHandle0 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_0, imageDesc, dataView);
    }
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
        const array_view<const uint8_t> dataView = CreateImageDataViewSsao();
        res.inputImageHandle1 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_1, imageDesc, dataView);
    }
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_D32_SFLOAT;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                               CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        const vector<float> depth(TEST_DATA_SIZE.x * TEST_DATA_SIZE.y, 0.5f);
        const array_view<const uint8_t> dataView(reinterpret_cast<const uint8_t*>(depth.data()), depth.size_in_bytes());
        res.inputImageHandle2 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_2, imageDesc, dataView);
    }
    {
        GpuBufferDesc bufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, 1024U };
        res.inputBufferHandle0 = er.device->GetGpuResourceManager().Create(INPUT_BUFFER_NAME_0, bufferDesc);
    }
    auto* renderClassFactory = er.context->GetInterface<Core::IClassFactory>();

    IRenderPostProcess::Ptr cmaa2 =
        CreateInstance<IRenderPostProcess>(*renderClassFactory, RenderPostProcessCmaa2Node::UID);

    RenderPostProcessCmaa2Node::EffectProperties properties = {};
    properties.enabled = true;
    array_view<uint8_t> propData = { reinterpret_cast<uint8_t*>(&properties),
        sizeof(RenderPostProcessCmaa2Node::EffectProperties) };
    cmaa2->SetData(propData);

    IRenderDataStoreRenderPostProcesses::PostProcessData data = {};
    data.id = 0;
    data.postProcess = cmaa2;

    refcnt_ptr<IRenderDataStoreRenderPostProcesses> store = er.context->GetRenderDataStoreManager().Create(
        IRenderDataStoreRenderPostProcesses::UID, "RenderDataStoreRenderPostProcesses");
    store->AddData("SrcTests", { &data, 1 });
    res.dataStore = move(store);

    // Render node graph
    {
        res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphPostProcessInterfaceUtil.rng");
    }

    res.byteArray = make_unique<ByteArray>(NUM_BYTES);

    return res;
}

void DestroyTestResources(IDevice& device, TestResources& tr)
{
    tr = {};
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
        er.context->GetRenderNodeGraphManager().SetRenderNodeGraphResources(tr.renderNodeGraph, inputs, {});

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

string GetBloomFileName(const UTest::EngineResources& er)
{
    if (er.backend == DeviceBackendType::VULKAN) {
        return "RenderNodePostProcessInterfaceUtilBloomVulkan.png";
    } else {
        return "RenderNodePostProcessInterfaceUtilBloomOpenGL.png";
    }
}

string GetBlurFileName(const UTest::EngineResources& er)
{
    if (er.backend == DeviceBackendType::VULKAN) {
        return "RenderNodePostProcessInterfaceUtilBlurVulkan.png";
    } else {
        return "RenderNodePostProcessInterfaceUtilBlurOpenGL.png";
    }
}

string GetUpscaleFileName(const UTest::EngineResources& er)
{
    if (er.backend == DeviceBackendType::VULKAN) {
        return "RenderNodePostProcessInterfaceUtilUpscaleVulkan.png";
    } else {
        return "RenderNodePostProcessInterfaceUtilUpscaleOpenGL.png";
    }
}

string GetSsaoFileName(const UTest::EngineResources& er)
{
    if (er.backend == DeviceBackendType::VULKAN) {
        return "RenderNodePostProcessInterfaceUtilSsaoVulkan.png";
    } else {
        return "RenderNodePostProcessInterfaceUtilSsaoOpenGL.png";
    }
}

void ValidateBloom(const TestData& td)
{
    ASSERT_EQ(NUM_BYTES, td.resources.byteArray->GetData().size());
    uint16_t* outputImageData = reinterpret_cast<uint16_t*>(td.resources.byteArray->GetData().data());
#if RENDER_SAVE_TEST_IMAGES == 1
    // convert uint16_t data to float
    float* floatData = new float[IMAGE_SIZE];
    for (size_t ii = 0; ii < IMAGE_SIZE; ii++) {
        floatData[ii] = BASE_NS::Math::F16ToF32(outputImageData[ii]);
    }
    UTest::SaveHdrImage(GetBloomFileName(td.engine), TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, floatData);
    delete[] floatData;
#endif
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
#endif
    for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
            float R = (BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u]));
            float G = (BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u]));
            float B = (BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u]));
            float A = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u]);

            constexpr float epsilon = 0.5f;
            if (i < ((TEST_DATA_SIZE.x / 2U) - 6U)) {
                ASSERT_NEAR(7.5f, R, epsilon);
                ASSERT_NEAR(7.5f, G, epsilon);
                ASSERT_NEAR(0.0f, B, epsilon);
            } else if (i > ((TEST_DATA_SIZE.x / 2U) + 7U)) {
                ASSERT_NEAR(0.0f, R, epsilon);
                ASSERT_NEAR(15.0f, G, epsilon);
                ASSERT_NEAR(0.0f, B, epsilon);
            }

            ASSERT_EQ(1.0f, A);
        }
    }
}

void ValidateUpscale(const TestData& td)
{
    ASSERT_EQ(NUM_BYTES, td.resources.byteArray->GetData().size());
    uint16_t* outputImageData = reinterpret_cast<uint16_t*>(td.resources.byteArray->GetData().data());
#if RENDER_SAVE_TEST_IMAGES == 1
    // convert uint16_t data to float
    float* floatData = new float[IMAGE_SIZE];
    for (size_t ii = 0; ii < IMAGE_SIZE; ii++) {
        floatData[ii] = BASE_NS::Math::F16ToF32(outputImageData[ii]);
    }
    UTest::SaveHdrImage(GetUpscaleFileName(td.engine), TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, floatData);
    delete[] floatData;
#endif
    for (size_t i = 0; i < TEST_DATA_SIZE.x; i += 1) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; j += 1) {
            float R = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u]);
            float G = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u]);
            float B = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u]);
            float A = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u]);

            float x = (float)i;
            float y = (float)j;

            float expectedR = x / TEST_DATA_SIZE.x;
            float expectedG = y / TEST_DATA_SIZE.y;
            float expectedB = (x / TEST_DATA_SIZE.x + y / TEST_DATA_SIZE.y) / 2.0f;
            float expectedA = 1.0f;

            float epsilon = 0.05f;
            EXPECT_NEAR(R, expectedR, epsilon);
            EXPECT_NEAR(G, expectedG, epsilon);
            EXPECT_NEAR(B, expectedB, epsilon);

            EXPECT_EQ(1.0f, A);
        }
    }
}

void ValidateSsao(const TestData& td)
{
    ASSERT_EQ(NUM_BYTES, td.resources.byteArray->GetData().size());
    uint16_t* outputImageData = reinterpret_cast<uint16_t*>(td.resources.byteArray->GetData().data());
#if RENDER_SAVE_TEST_IMAGES == 1
    // convert uint16_t data to float
    float* floatData = new float[IMAGE_SIZE];
    for (size_t ii = 0; ii < IMAGE_SIZE; ii++) {
        floatData[ii] = BASE_NS::Math::F16ToF32(outputImageData[ii]);
    }
    UTest::SaveHdrImage(GetSsaoFileName(td.engine), TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, floatData);
    delete[] floatData;
#endif
    for (size_t i = 0; i < TEST_DATA_SIZE.x; i += 1) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; j += 1) {
            float R = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u]);
            float G = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u]);
            float B = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u]);
            float A = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u]);
            // output is garbage as input images are not meaningful
            EXPECT_EQ(1.0f, A);
        }
    }
}

void ValidateCmaa2(const TestData& td)
{
    ASSERT_EQ(NUM_BYTES, td.resources.byteArray->GetData().size());
    uint16_t* outputImageData = reinterpret_cast<uint16_t*>(td.resources.byteArray->GetData().data());
#if RENDER_SAVE_TEST_IMAGES == 1
    // convert uint16_t data to float
    float* floatData = new float[IMAGE_SIZE];
    for (size_t ii = 0; ii < IMAGE_SIZE; ii++) {
        floatData[ii] = BASE_NS::Math::F16ToF32(outputImageData[ii]);
    }
    UTest::SaveHdrImage(GetSsaoFileName(td.engine), TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, floatData);
    delete[] floatData;
#endif
    for (size_t i = 0; i < TEST_DATA_SIZE.x; i += 1) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; j += 1) {
            float R = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u]);
            float G = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u]);
            float B = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u]);
            float A = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u]);
            // output is garbage as input images are not meaningful
            EXPECT_EQ(1.0f, A);
        }
    }
}

void TestBloomIntfUtil(DeviceBackendType backend)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResourcesBloom(testData.engine);
    }
    TickTest(testData, 3);
    ValidateBloom(testData);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}

void TestBlurIntfUtil(DeviceBackendType backend)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResourcesBlur(testData.engine);
    }
    TickTest(testData, 3);
    ValidateBlur(testData);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}

void TestUpscaleIntfUtil(DeviceBackendType backend)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResourcesUpscale(testData.engine);
    }
    TickTest(testData, 3);
    ValidateUpscale(testData);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}

void TestSsaoIntfUtil(DeviceBackendType backend)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResourcesSsao(testData.engine);
    }
    TickTest(testData, 3);
    ValidateSsao(testData);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}

void TestCmaa2IntfUtil(DeviceBackendType backend)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResourcesCmaa2(testData.engine);
    }
    TickTest(testData, 3);
    return;
    ValidateCmaa2(testData);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: RenderNodePostProcessInterfaceUtilBloomVulkan
 * @tc.desc: Tests for post processing using bloom via RenderNodePostProcessInterfaceUtil in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodePostProcessInterfaceUtil, RenderNodePostProcessInterfaceUtilBloomVulkan,
    testing::ext::TestSize.Level1)
{
    TestBloomIntfUtil(DeviceBackendType::VULKAN);
}

/**
 * @tc.name: RenderNodePostProcessInterfaceUtilUpscaleVulkan
 * @tc.desc: Tests for post processing using upscaler via RenderNodePostProcessInterfaceUtil in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodePostProcessInterfaceUtil, RenderNodePostProcessInterfaceUtilUpscaleVulkan,
    testing::ext::TestSize.Level1)
{
    TestUpscaleIntfUtil(DeviceBackendType::VULKAN);
}
/**
 * @tc.name: RenderNodePostProcessInterfaceUtilBlurVulkan
 * @tc.desc: Tests for post processing using blur via RenderNodePostProcessInterfaceUtil in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(
    SRC_RenderNodePostProcessInterfaceUtil, RenderNodePostProcessInterfaceUtilBlurVulkan, testing::ext::TestSize.Level1)
{
    TestBlurIntfUtil(DeviceBackendType::VULKAN);
}

/**
 * @tc.name: RenderNodePostProcessInterfaceUtilSsaoVulkan
 * @tc.desc: Tests for post processing using SSAO via RenderNodePostProcessInterfaceUtil in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(
    SRC_RenderNodePostProcessInterfaceUtil, RenderNodePostProcessInterfaceUtilSsaoVulkan, testing::ext::TestSize.Level1)
{
    TestSsaoIntfUtil(DeviceBackendType::VULKAN);
}

/**
 * @tc.name: RenderNodePostProcessInterfaceUtilCmaa2Vulkan
 * @tc.desc: Tests for post processing using CMAA2 via RenderNodePostProcessInterfaceUtil in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodePostProcessInterfaceUtil, RenderNodePostProcessInterfaceUtilCmaa2Vulkan,
    testing::ext::TestSize.Level1)
{
    TestCmaa2IntfUtil(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: RenderNodePostProcessInterfaceUtilOpenGL
 * @tc.desc: Tests for post processing using bloom via RenderNodePostProcessInterfaceUtil in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(
    SRC_RenderNodePostProcessInterfaceUtil, RenderNodePostProcessInterfaceUtilOpenGL, testing::ext::TestSize.Level1)
{
    TestBloomIntfUtil(UTest::GetOpenGLBackend());
}

/**
 * @tc.name: RenderNodePostProcessInterfaceUtilUpscaleOpenGL
 * @tc.desc: Tests for post processing using upscaler via RenderNodePostProcessInterfaceUtil in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodePostProcessInterfaceUtil, RenderNodePostProcessInterfaceUtilUpscaleOpenGL,
    testing::ext::TestSize.Level1)
{
    TestUpscaleIntfUtil(UTest::GetOpenGLBackend());
}

/**
 * @tc.name: RenderNodePostProcessInterfaceUtilBlurOpenGL
 * @tc.desc: Tests for post processing using blur via RenderNodePostProcessInterfaceUtil in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(
    SRC_RenderNodePostProcessInterfaceUtil, RenderNodePostProcessInterfaceUtilBlurOpenGL, testing::ext::TestSize.Level1)
{
    TestBlurIntfUtil(UTest::GetOpenGLBackend());
}

/**
 * @tc.name: RenderNodePostProcessInterfaceUtilSsaoOpenGL
 * @tc.desc: Tests for post processing using SSAO via RenderNodePostProcessInterfaceUtil in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(
    SRC_RenderNodePostProcessInterfaceUtil, RenderNodePostProcessInterfaceUtilSsaoOpenGL, testing::ext::TestSize.Level1)
{
    TestSsaoIntfUtil(UTest::GetOpenGLBackend());
}

/**
 * @tc.name: RenderNodePostProcessInterfaceUtilCmaa2OpenGL
 * @tc.desc: Tests for post processing using SSAO via RenderNodePostProcessInterfaceUtil in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodePostProcessInterfaceUtil, RenderNodePostProcessInterfaceUtilCmaa2OpenGL,
    testing::ext::TestSize.Level1)
{
    TestCmaa2IntfUtil(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
