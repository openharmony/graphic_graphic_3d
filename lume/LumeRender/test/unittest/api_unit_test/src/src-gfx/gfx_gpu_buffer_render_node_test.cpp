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
static constexpr uint32_t TEST_ELEMENT_ELEMENT_COUNT { 4u };
static constexpr uint32_t TEST_INPUT_BUFFER_COLOR[] = { 0xFF, 0x1, 0x1, 0xFFFFffff };
static constexpr uint32_t BUFFER_BYTE_SIZE { sizeof(uint32_t) * TEST_ELEMENT_ELEMENT_COUNT };
static constexpr string_view INPUT_BUFFER_NAME_0 { "InputBuffer0" };
static constexpr string_view INDIRECT_BUFFER_NAME_0 { "IndirectBuffer0" };
// NOTE: created in render node graph
static constexpr string_view OUTPUT_BUFFER_NAME_0 { "OutputBuffer0" };

struct TestResources {
    RenderHandleReference inputBufferHandle0;
    RenderHandleReference indirectBufferHandle0;

    uint32_t outputBufferColor[4u] = { 0, 0, 0, 0 };

    RenderHandleReference renderNodeGraph;
};

struct TestData {
    UTest::EngineResources engine;
    TestResources resources;
};

TestResources CreateTestResources(UTest::EngineResources& er)
{
    TestResources res;
    // Input buffer
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT | CORE_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferDesc.byteSize = BUFFER_BYTE_SIZE;
        const array_view<const uint8_t> dataView = { reinterpret_cast<const uint8_t*>(TEST_INPUT_BUFFER_COLOR),
            BUFFER_BYTE_SIZE };
        res.inputBufferHandle0 = er.device->GetGpuResourceManager().Create(INPUT_BUFFER_NAME_0, bufferDesc, dataView);
    }
    // Indirect args buffer
    {
        struct DispatchIndirectArgs {
            uint32_t x { 0u };
            uint32_t y { 0u };
            uint32_t z { 0u };
            uint32_t w { 0u };
        };

        GpuBufferDesc bufferDesc;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT | CORE_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                CORE_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        bufferDesc.byteSize = sizeof(DispatchIndirectArgs);
        const DispatchIndirectArgs dia { 4u, 1u, 1u, 0u };
        const array_view<const uint8_t> dataView = { reinterpret_cast<const uint8_t*>(&dia),
            sizeof(DispatchIndirectArgs) };
        res.indirectBufferHandle0 =
            er.device->GetGpuResourceManager().Create(INDIRECT_BUFFER_NAME_0, bufferDesc, dataView);
    }
    {
        res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphGfxGpuBufferRenderNodeTest.rng");
    }
    return res;
}

void DestroyTestResources(IDevice& device, TestResources& tr)
{
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    tr.inputBufferHandle0 = {};
    tr.indirectBufferHandle0 = {};
    tr.renderNodeGraph = {};
}

void TickTest(TestData& td, int32_t frameCountToTick)
{
    UTest::EngineResources& er = td.engine;
    TestResources& tr = td.resources;

    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        const bool copyThisFrame = (idx == (frameCountToTick - 1));

        RenderHandleReference bufferHandle;
        if (copyThisFrame) {
            // Copy data from render node graph buffer to CPU at the end of the frame with helper
            IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
            bufferHandle = gpuResourceMgr.GetBufferHandle(OUTPUT_BUFFER_NAME_0);
            IRenderFrameUtil& rfUtil = er.context->GetRenderUtil().GetRenderFrameUtil();
            rfUtil.CopyToCpu(bufferHandle, IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE);
        }

        er.engine->TickFrame();
        er.context->GetRenderer().RenderFrame({ &tr.renderNodeGraph, 1u });

        if (copyThisFrame) {
            // Copy data from the output buffer for evaluation
            IRenderFrameUtil& rfUtil = er.context->GetRenderUtil().GetRenderFrameUtil();
            const IRenderFrameUtil::FrameCopyData& data = rfUtil.GetFrameCopyData(bufferHandle);
            if (data.byteBuffer) {
                const array_view<const uint32_t> uintData = { reinterpret_cast<const uint32_t*>(
                                                                  data.byteBuffer->GetData().data()),
                    data.byteBuffer->GetData().size_bytes() / sizeof(uint32_t) };
                ASSERT_TRUE(uintData.size_bytes() == sizeof(tr.outputBufferColor));
                if (uintData.size_bytes() == sizeof(tr.outputBufferColor)) {
                    for (size_t idx = 0; idx < TEST_ELEMENT_ELEMENT_COUNT; ++idx) {
                        tr.outputBufferColor[idx] = uintData[idx];
                    }
                }
            }
        }
    }
}

void ValidateDataTest(const TestData& td)
{
    for (size_t idx = 0; idx < TEST_ELEMENT_ELEMENT_COUNT; ++idx) {
        ASSERT_TRUE(td.resources.outputBufferColor[idx] == TEST_INPUT_BUFFER_COLOR[idx]);
    }
}

void TestBufferRenderNode(DeviceBackendType backend)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResources(testData.engine);
    }
    TickTest(testData, 3);
    ValidateDataTest(testData);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: GpuBufferRenderNodeTestVulkan
 * @tc.desc: Tests for RenderNodeComputeGeneric by dispatching compute via an indirect buffer. The compute shader copies
 * input data to the output data, and cpu validates that all the data is copied and indirect dispatch works. Backend is
 * Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuBufferRenderNodeTest, GpuBufferRenderNodeTestVulkan, testing::ext::TestSize.Level1)
{
    TestBufferRenderNode(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GpuBufferRenderNodeTestOpenGL
 * @tc.desc: Tests for RenderNodeComputeGeneric by dispatching compute via an indirect buffer. The compute shader copies
 * input data to the output data, and cpu validates that all the data is copied and indirect dispatch works. Backend is
 * OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuBufferRenderNodeTest, GpuBufferRenderNodeTestOpenGL, testing::ext::TestSize.Level1)
{
    TestBufferRenderNode(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
