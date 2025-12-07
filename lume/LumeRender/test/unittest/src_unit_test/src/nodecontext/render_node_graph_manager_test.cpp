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
void TestRenderGraphManager(const UTest::EngineResources& engine)
{
    RenderNodeGraphManager& renderNodeGraphMgr =
        static_cast<RenderNodeGraphManager&>(engine.context->GetRenderNodeGraphManager());
    RenderNodeGraphDesc desc;
    desc.renderNodeGraphName = "renderNodeGraph0";
    auto handle0 = renderNodeGraphMgr.Create(
        IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_DYNAMIC, desc, "renderNodeGraph0");
    renderNodeGraphMgr.HandlePendingAllocations();
    ASSERT_NE(RenderHandle {}, handle0.GetHandle());
    ASSERT_EQ(
        ((const RenderNodeGraphManager&)renderNodeGraphMgr).Get(handle0.GetHandle()).GetHandle(), handle0.GetHandle());
    ASSERT_EQ(((const RenderNodeGraphManager&)renderNodeGraphMgr).Get(RenderHandle {}).GetHandle(), RenderHandle {});
    {
        auto info = renderNodeGraphMgr.GetInfo(handle0);
        ASSERT_EQ(desc.renderNodeGraphName, info.renderNodeGraphName);
    }
    handle0 = {};
    renderNodeGraphMgr.HandlePendingAllocations();
    {
        auto info = renderNodeGraphMgr.GetInfo(handle0);
        ASSERT_TRUE(info.renderNodeGraphName.empty());
    }
    desc.renderNodeGraphName = "renderNodeGraph1";
    auto handle1 =
        renderNodeGraphMgr.Create(IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_DYNAMIC, desc);
    renderNodeGraphMgr.HandlePendingAllocations();
    ASSERT_NE(RenderHandle {}, handle1.GetHandle());
    auto result = renderNodeGraphMgr.LoadAndCreate(
        IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, "files://nonExistingFile.rng");
    ASSERT_EQ(RenderHandle {}, result.GetHandle());
    {
        RenderNodeDesc nodeDesc;
        nodeDesc.typeName = "RenderNodeBloom";
        nodeDesc.nodeName = "Bloom";
        renderNodeGraphMgr.PushBackRenderNode(handle1, nodeDesc);
    }
    {
        RenderNodeDesc nodeDesc;
        nodeDesc.typeName = "RenderNodeCreateGpuImages";
        nodeDesc.nodeName = "GpuCreate";
        renderNodeGraphMgr.InsertBeforeRenderNode(handle1, nodeDesc, "Bloom");
    }
    {
        RenderNodeDesc nodeDesc;
        nodeDesc.typeName = "RenderNodeBackBuffer";
        nodeDesc.nodeName = "BackBuffer";
        renderNodeGraphMgr.InsertAfterRenderNode(handle1, nodeDesc, "GpuCreate");
    }
    renderNodeGraphMgr.HandlePendingAllocations();
    {
        auto info = renderNodeGraphMgr.GetInfo(handle1);
        ASSERT_EQ(3, info.nodes.size());
    }
    renderNodeGraphMgr.EraseRenderNode(handle1, "Bloom");
    renderNodeGraphMgr.HandlePendingAllocations();
    {
        auto info = renderNodeGraphMgr.GetInfo(handle1);
        ASSERT_EQ(2, info.nodes.size());
    }
    RenderHandleReference inputs[] = { handle1, handle1, handle1, handle1, handle1 };
    RenderHandleReference outputs[] = { handle1, handle1, handle1, handle1, handle1 };
    renderNodeGraphMgr.SetRenderNodeGraphResources(handle1, { inputs, countof(inputs) }, { outputs, countof(outputs) });
    renderNodeGraphMgr.HandlePendingAllocations();
    {
        auto resources = renderNodeGraphMgr.GetRenderNodeGraphResources(handle1);
        ASSERT_FALSE(resources.inputResources.empty());
        ASSERT_FALSE(resources.outputResources.empty());
    }
    {
        auto resources = renderNodeGraphMgr.GetRenderNodeGraphResources(RenderHandleReference {});
        ASSERT_TRUE(resources.inputResources.empty());
        ASSERT_TRUE(resources.outputResources.empty());
    }
    renderNodeGraphMgr.Destroy("RenderNodeBackBuffer");
    {
        auto info = renderNodeGraphMgr.GetInfo(handle1);
        ASSERT_EQ(1, info.nodes.size());
    }
    {
        auto invalidHandle = RenderHandleUtil::CreateHandle(RenderHandleType::RENDER_NODE_GRAPH, 16);
        auto handle = static_cast<const RenderNodeGraphManager&>(renderNodeGraphMgr).Get(invalidHandle);
        ASSERT_EQ(RenderHandle {}, handle.GetHandle());
        RenderHandleReference refHandle { invalidHandle, {} };
        auto info = renderNodeGraphMgr.GetInfo(refHandle);
        ASSERT_EQ(0, info.nodes.size());
    }
    {
        auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::RENDER_NODE_GRAPH, 0, 3);
        ASSERT_EQ(nullptr, renderNodeGraphMgr.Get(handle));
        RenderHandleReference refHandle { handle, {} };
        auto info = renderNodeGraphMgr.GetInfo(refHandle);
        ASSERT_EQ(0, info.nodes.size());
        renderNodeGraphMgr.SetRenderNodeGraphResources(refHandle, {}, {});
        auto resources = renderNodeGraphMgr.GetRenderNodeGraphResources(refHandle);
        ASSERT_EQ(0, resources.inputResources.size());
    }
    {
        auto handle = renderNodeGraphMgr.LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://RenderNodeGraphManagerTest.rng");
        renderNodeGraphMgr.HandlePendingAllocations();
        renderNodeGraphMgr.EraseRenderNode(handle, "NodeName");
        RenderNodeDesc desc;
        desc.typeName = "NonExistingType";
        desc.nodeName = "Name1";
        renderNodeGraphMgr.InsertAfterRenderNode(handle, desc, "Name1");
        renderNodeGraphMgr.InsertAfterRenderNode({}, desc, "Name1");
        renderNodeGraphMgr.HandlePendingAllocations();
    }
    {
        auto handle = renderNodeGraphMgr.LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_DYNAMIC,
            "test://RenderNodeGraphManagerTest.rng");
        renderNodeGraphMgr.HandlePendingAllocations();
        renderNodeGraphMgr.UpdateRenderNodeGraph({}, {});
        renderNodeGraphMgr.EraseRenderNode(handle, "NonExistingNode");
        renderNodeGraphMgr.HandlePendingAllocations();
        RenderHandleReference invalidHandle;
        renderNodeGraphMgr.SetRenderNodeGraphResources(handle, { &invalidHandle, 1 }, { &invalidHandle, 1 });
    }
    ASSERT_EQ(nullptr, renderNodeGraphMgr.Get(RenderHandle {}));
}
} // namespace

/**
 * @tc.name: RenderNodeGraphManagerTest
 * @tc.desc: Tests the functionality of RenderNodeGraphManager by creating render node graphs via files or manually, and
 * inserting/pushing/erasing render nodes from code.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeGraphManager, RenderNodeGraphManagerTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    if (engine.backend == DeviceBackendType::OPENGL) {
        engine.createWindow = true;
    }
    UTest::CreateEngineSetup(engine);
    TestRenderGraphManager(engine);
    UTest::DestroyEngine(engine);
}
