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
static constexpr uint32_t TEST_ELEMENT_ELEMENT_COUNT { 8u };
static constexpr uint32_t TEST_INPUT_BUFFER_COLOR[] = { 4u, 1u, 1u, 0u, 1u, 4u, 11u, 0u };
static constexpr uint32_t BUFFER_BYTE_SIZE { sizeof(uint32_t) * TEST_ELEMENT_ELEMENT_COUNT };
static constexpr string_view INPUT_BUFFER_NAME_0 { "InputBuffer0" };
static constexpr string_view INDIRECT_IMAGE_NAME_0 { "IndirectImage0" };
static constexpr string_view INDIRECT_BUFFER_NAME_0 { "IndirectBuffer0" };
static constexpr uint32_t PUSH_CONSTANT { 1u };
// NOTE: created in render node graph
static constexpr string_view OUTPUT_BUFFER_NAME_0 { "OutputBuffer0" };

constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
constexpr const string_view RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY = "RenderDataStoreDefaultGpuResourceDataCopy";

struct TestResources {
    RenderHandleReference inputBufferHandle0;
    RenderHandleReference indirectImageHandle0;
    uint32_t outputBufferColor[TEST_ELEMENT_ELEMENT_COUNT] = {};

    RenderHandleReference renderNodeGraph;

    refcnt_ptr<IRenderDataStorePod> dataStorePod;
};

struct TestData {
    UTest::EngineResources engine;
    TestResources resources;
};

TestResources CreateTestResources(UTest::EngineResources& er, uint32_t constValue)
{
    TestResources res;
    // input buffer
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT | CORE_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                CORE_BUFFER_USAGE_TRANSFER_DST_BIT | CORE_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        bufferDesc.byteSize = BUFFER_BYTE_SIZE;
        const array_view<const uint8_t> dataView = { reinterpret_cast<const uint8_t*>(TEST_INPUT_BUFFER_COLOR),
            BUFFER_BYTE_SIZE };
        res.inputBufferHandle0 = er.device->GetGpuResourceManager().Create(INPUT_BUFFER_NAME_0, bufferDesc, dataView);
    }
    {
        GpuImageDesc desc;
        desc.width = 4;
        desc.height = 1;
        desc.depth = 1;
        desc.format = BASE_FORMAT_R16G16B16A16_UINT;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        desc.imageType = CORE_IMAGE_TYPE_2D;
        desc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        res.indirectImageHandle0 = er.device->GetGpuResourceManager().Create(INDIRECT_IMAGE_NAME_0, desc);
    }
    {
        res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphGfxComputeGenericRenderNodeTest.rng");
    }
    {
        res.dataStorePod = refcnt_ptr<IRenderDataStorePod>(
            er.context->GetRenderDataStoreManager().Create(IRenderDataStorePod::UID, "dataStore"));
        ShaderSpecializationRenderPod conf;
        conf.specializationConstantCount = 1;
        conf.specializationFlags[0].value = constValue;
        const array_view<const uint8_t> dataView = { reinterpret_cast<const uint8_t*>(&conf), sizeof(conf) };
        res.dataStorePod->CreatePod("ShaderSpecializationRenderPod", "computeConfig", dataView);
        res.dataStorePod->CreatePod(
            "pushConstants", "pushConstants", { reinterpret_cast<const uint8_t*>(&PUSH_CONSTANT), sizeof(uint32_t) });
    }

    return res;
}

void DestroyTestResources(IDevice& device, TestResources& tr)
{
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    tr.inputBufferHandle0 = {};
    tr.indirectImageHandle0 = {};
    tr.renderNodeGraph = {};
}

void TickTest(TestData& td, int32_t frameCountToTick)
{
    UTest::EngineResources& er = td.engine;
    TestResources& tr = td.resources;

    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        const bool copyThisFrame = (idx == (frameCountToTick - 1));

        if (copyThisFrame) {
            // Copy data from render node graph buffer to CPU at the end of the frame with helper
            IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
            const RenderHandleReference bufferHandle = gpuResourceMgr.GetBufferHandle(OUTPUT_BUFFER_NAME_0);
            IRenderFrameUtil& rfUtil = er.context->GetRenderUtil().GetRenderFrameUtil();
            rfUtil.CopyToCpu(bufferHandle, IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE);
        }

        er.engine->TickFrame();
        RenderHandleReference inputs[] = { tr.indirectImageHandle0, tr.inputBufferHandle0 };
        er.context->GetRenderNodeGraphManager().SetRenderNodeGraphResources(
            tr.renderNodeGraph, { inputs, countof(inputs) }, {});
        er.context->GetRenderer().RenderFrame({ &tr.renderNodeGraph, 1u });

        if (copyThisFrame) {
            // Copy data from the output buffer for evaluation
            IRenderFrameUtil& rfUtil = er.context->GetRenderUtil().GetRenderFrameUtil();
            IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
            const RenderHandleReference bufferHandle = gpuResourceMgr.GetBufferHandle(OUTPUT_BUFFER_NAME_0);
            const IRenderFrameUtil::FrameCopyData& data = rfUtil.GetFrameCopyData(bufferHandle);
            if (data.byteBuffer) {
                const array_view<const uint32_t> uintData = { reinterpret_cast<const uint32_t*>(
                                                                  data.byteBuffer->GetData().data()),
                    data.byteBuffer->GetData().size_bytes() / sizeof(uint32_t) };
                EXPECT_EQ(uintData.size_bytes(), sizeof(tr.outputBufferColor));
                if (uintData.size_bytes() == sizeof(tr.outputBufferColor)) {
                    for (size_t idx = 0; idx < TEST_ELEMENT_ELEMENT_COUNT; ++idx) {
                        tr.outputBufferColor[idx] = uintData[idx];
                    }
                }
            }
        }
    }
}

void ValidateDataTest(const TestData& td, uint32_t constValue)
{
    if (constValue == 0u) {
        for (size_t idx = 0; idx < TEST_ELEMENT_ELEMENT_COUNT; ++idx) {
            EXPECT_EQ(TEST_INPUT_BUFFER_COLOR[idx], td.resources.outputBufferColor[idx]);
        }
    } else {
        for (size_t idx = 0; idx < 4u; ++idx) {
            EXPECT_EQ(TEST_INPUT_BUFFER_COLOR[idx], td.resources.outputBufferColor[idx]);
        }
        for (size_t idx = 4u; idx < TEST_ELEMENT_ELEMENT_COUNT; ++idx) {
            EXPECT_EQ(TEST_INPUT_BUFFER_COLOR[idx] + PUSH_CONSTANT, td.resources.outputBufferColor[idx]);
        }
    }
}

void TestComputeGenericRenderNode(DeviceBackendType backend, uint32_t constValue)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    CreateEngineSetup(testData.engine);
    testData.resources = CreateTestResources(testData.engine, constValue);

    TickTest(testData, 3);
    ValidateDataTest(testData, constValue);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: GfxComputeGenericRenderNodeTestVulkan
 * @tc.desc: Tests RenderNodeComputeGeneric by issuing a compute shader with input data and push constants. The output
 * is expected to be (input data + push constant) or (input data) depending on the shader specialization. The output is
 * valitated in the end. Backend is Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxComputeGenericRenderNode, GfxComputeGenericRenderNodeTestVulkan, testing::ext::TestSize.Level1)
{
    TestComputeGenericRenderNode(DeviceBackendType::VULKAN, 0u);
    TestComputeGenericRenderNode(DeviceBackendType::VULKAN, 1u);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GfxComputeGenericRenderNodeTestOpenGL
 * @tc.desc: Tests RenderNodeComputeGeneric by issuing a compute shader with input data and push constants. The output
 * is expected to be (input data + push constant) or (input data) depending on the shader specialization. The output is
 * valitated in the end. Not Enabled on OpenGL due to lack of shader specialization ability.
 * @tc.type: FUNC
 */
UNIT_TEST(
    API_GfxComputeGenericRenderNode, DISABLED_GfxComputeGenericRenderNodeTestOpenGL, testing::ext::TestSize.Level1)
{
    // NOTE: Test fails on windows machines

    // Shader specialization doesn't work with OpenGL
    TestComputeGenericRenderNode(UTest::GetOpenGLBackend(), 0u);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
