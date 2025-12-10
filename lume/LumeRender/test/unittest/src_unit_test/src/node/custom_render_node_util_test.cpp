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

#include <nodecontext/render_node_graph_manager.h>
#include <nodecontext/render_node_manager.h>

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

#include "node/nodes/render_node_test_util.h"
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

struct TestData {
    UTest::EngineResources engine;
    RenderHandleReference renderNodeGraph;
};

void CreateTestResources(TestData& td)
{
    // Add render node factories
    {
        RenderNodeGraphManager& renderNodeGraphMgr =
            static_cast<RenderNodeGraphManager&>(td.engine.context->GetRenderNodeGraphManager());
        RenderNodeManager& renderNodeMgr = renderNodeGraphMgr.GetRenderNodeManager();
        {
            RenderNodeTypeInfo info { { RenderNodeTestUtil::UID }, RenderNodeTestUtil::UID,
                RenderNodeTestUtil::TYPE_NAME, RenderNodeTestUtil::Create, RenderNodeTestUtil::Destroy,
                RenderNodeTestUtil::BACKEND_FLAGS, RenderNodeTestUtil::CLASS_TYPE };
            renderNodeMgr.AddRenderNodeFactory(info);
        }
    }
    // Create render node graph
    {
        td.renderNodeGraph = td.engine.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphRenderNodeTestUtilTest.rng");
    }
}

void DestroyTestResources(IDevice& device, TestData& td)
{
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    td.renderNodeGraph = {};
}

void TickTest(const TestData& td, int32_t frameCountToTick)
{
    const UTest::EngineResources& er = td.engine;

    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        er.engine->TickFrame();
        er.context->GetRenderer().RenderFrame({ &td.renderNodeGraph, 1 });
    }
}

void TestRenderNodeUtil(DeviceBackendType backend)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        UTest::CreateEngineSetup(testData.engine);
        CreateTestResources(testData);
    }
    TickTest(testData, 1);
    {
        DestroyTestResources(*testData.engine.device, testData);
        UTest::DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: RenderNodeTestUtilTestVulkan
 * @tc.desc: Tests the render node util in a custom rendernode which helps to create renderpasses, create resources,
 * parse json inputs, etc. Backend is Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_CustomRenderNode, RenderNodeTestUtilTestVulkan, testing::ext::TestSize.Level1)
{
    TestRenderNodeUtil(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: RenderNodeTestUtilTestOpenGL
 * @tc.desc: Tests the render node util in a custom rendernode which helps to create renderpasses, create resources,
 * parse json inputs, etc. Backend is OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_CustomRenderNode, RenderNodeTestUtilTestOpenGL, testing::ext::TestSize.Level1)
{
    TestRenderNodeUtil(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
