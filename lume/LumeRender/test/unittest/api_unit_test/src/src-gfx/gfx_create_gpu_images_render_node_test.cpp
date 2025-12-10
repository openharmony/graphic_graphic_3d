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
static constexpr string_view IMAGE_NAME_0 { "Image0" };
// NOTE: created in render node graph
static constexpr string_view IMAGE_NAME_1 { "Image1" };

struct TestResources {
    RenderHandleReference imageHandle0;
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
        "test://renderNodeGraphGfxCreateGpuImagesRenderNodeTest.rng");
    return res;
}

void DestroyTestResources(IDevice& device, TestResources& tr)
{
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    tr.imageHandle0 = {};
    tr.renderNodeGraph = {};
}

void TickTest(TestData& td, int32_t frameCountToTick)
{
    UTest::EngineResources& er = td.engine;
    TestResources& tr = td.resources;

    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        GpuImageDesc imageDesc;
        if (idx == 0) {
            imageDesc.width = 4u;
            imageDesc.height = 4u;
            imageDesc.depth = 1u;
            imageDesc.layerCount = 2u;
            imageDesc.sampleCountFlags = SampleCountFlagBits::CORE_SAMPLE_COUNT_4_BIT;
            imageDesc.format = BASE_FORMAT_R32G32B32A32_SFLOAT;
        } else {
            imageDesc.width = 5u;
            imageDesc.height = 7u;
            imageDesc.depth = 1u;
            imageDesc.layerCount = 1u;
            imageDesc.sampleCountFlags = SampleCountFlagBits::CORE_SAMPLE_COUNT_2_BIT;
            imageDesc.format = BASE_FORMAT_R8G8B8A8_SINT;
        }
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D_ARRAY;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                               CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        tr.imageHandle0 = er.device->GetGpuResourceManager().Create(IMAGE_NAME_0, imageDesc);

        er.engine->TickFrame();
        er.context->GetRenderer().RenderFrame({ &tr.renderNodeGraph, 1u });

        auto imageHandle1 = er.device->GetGpuResourceManager().GetImageHandle(IMAGE_NAME_1);
        auto imageDesc1 = er.device->GetGpuResourceManager().GetImageDescriptor(imageHandle1);
        ASSERT_EQ(imageDesc.width, imageDesc1.width);
        ASSERT_EQ(imageDesc.height, imageDesc1.height);
        ASSERT_EQ(imageDesc.depth, imageDesc1.depth);
        ASSERT_EQ(imageDesc.mipCount, imageDesc1.mipCount);
        ASSERT_EQ(imageDesc.layerCount, imageDesc1.layerCount);
        ASSERT_EQ(imageDesc.format, imageDesc1.format);
        ASSERT_EQ(imageDesc.sampleCountFlags, imageDesc1.sampleCountFlags);
    }
}

void TestCreateGpuImagesRenderNode(DeviceBackendType backend)
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
 * @tc.name: GfxCreateGpuImagesRenderNodeTestVulkan
 * @tc.desc: Tests the creation of GPU Images with render nodes using RenderNodeCreateGpuImages in the Vulkan Backend.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxCreateGpuImagesRenderNode, GfxCreateGpuImagesRenderNodeTestVulkan, testing::ext::TestSize.Level1)
{
    TestCreateGpuImagesRenderNode(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GfxCreateGpuImagesRenderNodeTestOpenGL
 * @tc.desc: Tests the creation of GPU Images with render nodes using RenderNodeCreateGpuImages in the OpenGL Backend.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxCreateGpuImagesRenderNode, GfxCreateGpuImagesRenderNodeTestOpenGL, testing::ext::TestSize.Level1)
{
    TestCreateGpuImagesRenderNode(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
