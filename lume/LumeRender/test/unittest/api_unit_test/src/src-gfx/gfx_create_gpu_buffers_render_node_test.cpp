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
static constexpr string_view BUFFER_NAME_0 { "Buffer0" };
// NOTE: created in render node graph
static constexpr string_view BUFFER_NAME_1 { "Buffer1" };

struct TestResources {
    RenderHandleReference bufferHandle0;
    RenderHandleReference renderNodeGraph;
};

struct TestData {
    UTest::EngineResources engine;
    TestResources resources;
};

TestResources CreateTestResources(UTest::EngineResources& er)
{
    TestResources res;
    res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
        IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
        "test://renderNodeGraphGfxCreateGpuBuffersRenderNodeTest.rng");
    return res;
}

void DestroyTestResources(IDevice& device, TestResources& tr)
{
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    tr.bufferHandle0 = {};
    tr.renderNodeGraph = {};
}

void TickTest(TestData& td, int32_t frameCountToTick)
{
    UTest::EngineResources& er = td.engine;
    TestResources& tr = td.resources;

    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = idx + 5u;
        if (idx == 0) {
            bufferDesc.format = BASE_FORMAT_R8G8B8A8_UINT;
            bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
            bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        } else {
            bufferDesc.format = BASE_FORMAT_R32G32B32A32_SFLOAT;
            bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE;
            bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
            bufferDesc.memoryPropertyFlags =
                CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        }
        tr.bufferHandle0 = er.device->GetGpuResourceManager().Create(BUFFER_NAME_0, bufferDesc);

        er.engine->TickFrame();
        er.context->GetRenderer().RenderFrame({ &tr.renderNodeGraph, 1u });

        auto bufferHandle1 = er.device->GetGpuResourceManager().GetBufferHandle(BUFFER_NAME_1);
        auto bufferDesc1 = er.device->GetGpuResourceManager().GetBufferDescriptor(bufferHandle1);
        ASSERT_EQ(bufferDesc.byteSize, bufferDesc1.byteSize);
        ASSERT_EQ(bufferDesc.engineCreationFlags, bufferDesc1.engineCreationFlags);
        ASSERT_EQ(bufferDesc.memoryPropertyFlags, bufferDesc1.memoryPropertyFlags);
        ASSERT_EQ(bufferDesc.usageFlags, bufferDesc1.usageFlags);
    }
}

void TestCreateGpuBuffersRenderNode(DeviceBackendType backend)
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
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: GfxCreateGpuBuffersRenderNodeTestVulkan
 * @tc.desc: Tests the creation of GPU buffers with render nodes using RenderNodeCreateGpuBuffers in the Vulkan Backend.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxCreateGpuBuffersRenderNode, GfxCreateGpuBuffersRenderNodeTestVulkan, testing::ext::TestSize.Level1)
{
    TestCreateGpuBuffersRenderNode(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GfxCreateGpuBuffersRenderNodeTestOpenGL
 * @tc.desc: Tests the creation of GPU buffers with render nodes using RenderNodeCreateGpuBuffers in the OpenGL Backend.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxCreateGpuBuffersRenderNode, GfxCreateGpuBuffersRenderNodeTestOpenGL, testing::ext::TestSize.Level1)
{
    TestCreateGpuBuffersRenderNode(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
