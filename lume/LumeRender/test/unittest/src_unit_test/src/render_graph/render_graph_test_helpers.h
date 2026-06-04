/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef RENDER_GRAPH_TEST_HELPERS_H
#define RENDER_GRAPH_TEST_HELPERS_H

#include <nodecontext/render_command_list.h>
#include <nodecontext/render_node_graph_manager.h>
#include <nodecontext/render_node_graph_node_store.h>
#include <nodecontext/render_node_manager.h>

#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>

#include "node/nodes/render_node_draw.h"
#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

namespace RenderGraphTestUtil {

using namespace BASE_NS;
using namespace RENDER_NS;

inline constexpr float TEST_VERTEX_DATA[] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f};

struct TestData {
    UTest::EngineResources engine;
    RenderHandleReference renderNodeGraph;
    RenderHandleReference outputImageHandle;
    RenderHandleReference depthImageHandle;
    RenderHandleReference vertexBufferHandle;
};

template <typename Node>
inline void RegisterNodeFactory(TestData& td)
{
    RenderNodeGraphManager& rngMgr =
        static_cast<RenderNodeGraphManager&>(td.engine.context->GetRenderNodeGraphManager());
    RenderNodeManager& rnMgr = rngMgr.GetRenderNodeManager();
    RenderNodeTypeInfo info{
        {Node::UID}, Node::UID, Node::TYPE_NAME, Node::Create, Node::Destroy, Node::BACKEND_FLAGS, Node::CLASS_TYPE};
    rnMgr.AddRenderNodeFactory(info);
}

inline void LoadRenderNodeGraph(TestData& td, string_view rngUri)
{
    td.renderNodeGraph = td.engine.context->GetRenderNodeGraphManager().LoadAndCreate(
        IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, rngUri);

    RenderNodeGraphManager& rngMgr =
        static_cast<RenderNodeGraphManager&>(td.engine.context->GetRenderNodeGraphManager());
    rngMgr.HandlePendingAllocations();
}

inline RenderHandleReference CreateColorImage(
    TestData& td, string_view name, Format format = BASE_FORMAT_R8G8B8A8_UNORM)
{
    GpuImageDesc imageDesc;
    imageDesc.width = 16;
    imageDesc.height = 16;
    imageDesc.depth = 1;
    imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
    imageDesc.format = format;
    imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
    imageDesc.imageType = CORE_IMAGE_TYPE_2D;
    imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
    imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    imageDesc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT;
    return td.engine.device->GetGpuResourceManager().Create(name, imageDesc);
}

inline RenderHandleReference CreateDepthImage(TestData& td, string_view name)
{
    GpuImageDesc imageDesc;
    imageDesc.width = 16;
    imageDesc.height = 16;
    imageDesc.depth = 1;
    imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
    imageDesc.format = BASE_FORMAT_D24_UNORM_S8_UINT;
    imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
    imageDesc.imageType = CORE_IMAGE_TYPE_2D;
    imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
    imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    imageDesc.usageFlags = CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT;
    return td.engine.device->GetGpuResourceManager().Create(name, imageDesc);
}

inline void CreateVertexBuffer(TestData& td)
{
    GpuBufferDesc bufferDesc;
    bufferDesc.byteSize = sizeof(TEST_VERTEX_DATA);
    bufferDesc.engineCreationFlags =
        CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE;
    bufferDesc.usageFlags = CORE_BUFFER_USAGE_VERTEX_BUFFER_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    td.vertexBufferHandle = td.engine.device->GetGpuResourceManager().Create(
        "VertexBuffer", bufferDesc, {reinterpret_cast<const uint8_t*>(TEST_VERTEX_DATA), sizeof(TEST_VERTEX_DATA)});
}

inline void DestroyTestResources(TestData& td)
{
    td.renderNodeGraph = {};
    td.outputImageHandle = {};
    td.depthImageHandle = {};
    td.vertexBufferHandle = {};
}

inline void TickFrames(const TestData& td, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        td.engine.engine->TickFrame();
        td.engine.context->GetRenderer().RenderFrame({&td.renderNodeGraph, 1});
    }
}

inline RenderNodeGraphNodeStore* GetNodeStore(const TestData& td)
{
    RenderNodeGraphManager& mgr = static_cast<RenderNodeGraphManager&>(td.engine.context->GetRenderNodeGraphManager());
    return mgr.Get(td.renderNodeGraph.GetHandle());
}

inline const RenderCommandBeginRenderPass* FindBeginRenderPass(const RenderCommandList& cmdList)
{
    auto cmds = cmdList.GetRenderCommands();
    for (const auto& cmd : cmds) {
        if (cmd.type == RenderCommandType::BEGIN_RENDER_PASS) {
            return static_cast<const RenderCommandBeginRenderPass*>(cmd.rc);
        }
    }
    return nullptr;
}

inline const RenderCommandBarrierPoint* FindBarrierPoint(const RenderCommandList& cmdList)
{
    auto cmds = cmdList.GetRenderCommands();
    for (const auto& cmd : cmds) {
        if (cmd.type == RenderCommandType::BARRIER_POINT) {
            return static_cast<const RenderCommandBarrierPoint*>(cmd.rc);
        }
    }
    return nullptr;
}

}  // namespace RenderGraphTestUtil

#endif  // RENDER_GRAPH_TEST_HELPERS_H
