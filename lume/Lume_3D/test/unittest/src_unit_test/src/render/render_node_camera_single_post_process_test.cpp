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
#include <render/datastore/intf_render_data_store_post_process.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace {
static constexpr Math::UVec2 TEST_DATA_SIZE { 512u, 512u };
static constexpr size_t IMAGE_SIZE = TEST_DATA_SIZE.x * TEST_DATA_SIZE.y * 4u;
static constexpr size_t NUM_BYTES = IMAGE_SIZE * sizeof(float) / 4u;
float imageData[IMAGE_SIZE];
static constexpr string_view INPUT_IMAGE_NAME_0 { "InputImage0" };
static constexpr string_view COPY_BUFFER_NAME_0 { "CopyBuffer0" };
// NOTE: created in render node graph
static constexpr string_view OUTPUT_IMAGE_NAME_0 { "OutputImage0" };

constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
constexpr const string_view RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY = "RenderDataStoreDefaultGpuResourceDataCopy";

struct TestResources {
    RenderHandleReference inputImageHandle0;
    RenderHandleReference copyBufferHandle0;

    unique_ptr<ByteArray> byteArray;

    RenderHandleReference renderNodeGraph;
};

TestResources CreateTestResources(UTest::TestContext* testContext)
{
    TestResources res;
    // Render node graph
    {
        res.renderNodeGraph = testContext->renderContext->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://RenderNodeCameraSinglePostProcessTest.rng");
    }
    // Input image
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R32G32B32A32_SFLOAT;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags =
            CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT;
        const array_view<const uint8_t> dataView { reinterpret_cast<const uint8_t*>(imageData), sizeof(imageData) };
        res.inputImageHandle0 = testContext->renderContext->GetDevice().GetGpuResourceManager().Create(
            INPUT_IMAGE_NAME_0, imageDesc, dataView);
    }
    // Copy buffer
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = NUM_BYTES;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.copyBufferHandle0 =
            testContext->renderContext->GetDevice().GetGpuResourceManager().Create(COPY_BUFFER_NAME_0, bufferDesc);
    }
    // Post process
    {
        auto dsPp =
            refcnt_ptr<IRenderDataStorePostProcess>(testContext->renderContext->GetRenderDataStoreManager().Create(
                IRenderDataStorePostProcess::UID, "renderDataStore"));
        IRenderDataStorePostProcess::PostProcess::Variables vars;
        vars.enabled = true;
        dsPp->Create("configuration", "postProcess", {});
        dsPp->Set("configuration", "postProcess", vars);
    }
    res.byteArray = make_unique<ByteArray>(NUM_BYTES);
    return res;
}

void DestroyTestResources(TestResources& tr)
{
    tr.byteArray.reset();
    tr.copyBufferHandle0 = {};
    tr.inputImageHandle0 = {};
    tr.renderNodeGraph = {};
}

void TickTest(UTest::TestContext* testContext, TestResources& tr, int32_t frameCountToTick)
{
    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        if (idx == 1) {
            auto& rdsMgr = testContext->renderContext->GetRenderDataStoreManager();
            auto dsStaging = refcnt_ptr<IRenderDataStoreDefaultStaging>(
                rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING));
            if (dsStaging) {
                BufferImageCopy bufferImageCopy;
                bufferImageCopy.bufferOffset = 0;
                bufferImageCopy.bufferRowLength = TEST_DATA_SIZE.x / 2u;
                bufferImageCopy.bufferImageHeight = TEST_DATA_SIZE.y / 2u;
                bufferImageCopy.imageSubresource =
                    ImageSubresourceLayers { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1u };
                bufferImageCopy.imageOffset = { 0, 0, 0 };
                bufferImageCopy.imageExtent = { TEST_DATA_SIZE.x / 2u, TEST_DATA_SIZE.y / 2u, 1u };

                IGpuResourceManager& gpuResourceMgr = testContext->renderContext->GetDevice().GetGpuResourceManager();
                const RenderHandleReference outputImageHandle0 = gpuResourceMgr.GetImageHandle(OUTPUT_IMAGE_NAME_0);

                dsStaging->CopyImageToBuffer(outputImageHandle0, tr.copyBufferHandle0, bufferImageCopy);
            }
        }

        if (idx == 2) {
            auto& rdsMgr = testContext->renderContext->GetRenderDataStoreManager();
            auto dataStoreDataCopy = refcnt_ptr<IRenderDataStoreDefaultGpuResourceDataCopy>(
                rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY));
            if (dataStoreDataCopy) {
                IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                dataCopy.gpuHandle = tr.copyBufferHandle0;
                dataCopy.byteArray = tr.byteArray.get();
                dataStoreDataCopy->AddCopyOperation(dataCopy);
            }
        }

        testContext->engine->TickFrame();
        const RenderHandleReference inputs[] = { tr.inputImageHandle0 };
        testContext->renderContext->GetRenderNodeGraphManager().SetRenderNodeGraphResources(
            tr.renderNodeGraph, { inputs, countof(inputs) }, {});

        testContext->renderContext->GetRenderer().RenderFrame({ &tr.renderNodeGraph, 1u });
    }
}
} // namespace

/**
 * @tc.name: BasicTest
 * @tc.desc: Tests for Basic Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeCameraSinglePostProcessTest, BasicTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    TestResources res = CreateTestResources(testContext);
    TickTest(testContext, res, 3);
    ASSERT_NE(nullptr, res.byteArray.get());
    DestroyTestResources(res);
}
