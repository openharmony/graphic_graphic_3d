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

#include <device/device.h>
#include <device/gpu_resource_manager.h>
#include <device/render_frame_sync.h>
#include <device/shader_manager.h>
#include <device/swapchain.h>
#include <nodecontext/node_context_pso_manager.h>
#include <nodecontext/render_node_graph_manager.h>
#include <render_backend.h>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <core/implementation_uids.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_class_register.h>
#include <core/plugin/intf_interface.h>
#include <core/threading/intf_thread_pool.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/resource_handle.h>

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
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

static constexpr string_view CMD_LIST_DEBUG_NAME { "CMD_LIST_DEBUG_NAME" };
static constexpr string_view RENDER_NODE_DEBUG_NAME { "RENDER_NODE_DEBUG_NAME" };

static constexpr string_view VERTEX_BUFFER_NAME { "vertexBuffer" };
static constexpr string_view INDEX_BUFFER_NAME { "indexBuffer" };
static constexpr string_view INDIRECT_BUFFER_NAME { "indirectBuffer" };
static constexpr string_view COLOR_ATTACHMENT_NAME { "colorAttachment" };
static constexpr string_view DEPTH_ATTACHMENT_NAME { "depthAttachment" };
static constexpr string_view RESOLVE_ATTACHMENT_NAME { "resolveAttachment" };
static constexpr string_view DEPTH_RESOLVE_ATTACHMENT_NAME { "depthResolveAttachment" };

static constexpr uint32_t NUM_VERTICES = 4;
static constexpr uint32_t NUM_TRIANGLES = 2;
static constexpr float VERTEX_BUFFER_DATA[] = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };
static constexpr uint32_t INDEX_BUFFER_DATA[] = { 0, 1, 2, 2, 3, 0 };
static constexpr uint32_t INDIRECT_BUFFER_DATA[] = { 3, 1, 0, 0, 6, 1, 0, 0, 0 };
static constexpr uint32_t WIDTH = 128;
static constexpr uint32_t HEIGHT = 128;

struct TestResources {
    RenderHandleReference vertexBufferHandle;
    RenderHandleReference indexBufferHandle;
    RenderHandleReference indirectBufferHandle;

    RenderHandleReference colorHandle;
    RenderHandleReference depthHandle;
    RenderHandleReference resolveHandle;
    RenderHandleReference depthResolveHandle;

    uint32_t width;
    uint32_t height;
    Format colorFormat { BASE_FORMAT_R8G8B8A8_SRGB };
    Format depthFormat { BASE_FORMAT_D24_UNORM_S8_UINT };
};
void CreateTestResources(UTest::EngineResources& er, TestResources& res)
{
    // Vertex buffer
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = sizeof(VERTEX_BUFFER_DATA);
        bufferDesc.engineCreationFlags =
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        res.vertexBufferHandle = er.device->GetGpuResourceManager().Create(VERTEX_BUFFER_NAME, bufferDesc,
            { reinterpret_cast<const uint8_t*>(VERTEX_BUFFER_DATA), sizeof(VERTEX_BUFFER_DATA) });
    }
    // Index buffer
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = sizeof(INDEX_BUFFER_DATA);
        bufferDesc.engineCreationFlags =
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE |
            CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER | CORE_ENGINE_BUFFER_CREATION_DEFERRED_DESTROY;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_SRC_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT |
                                CORE_BUFFER_USAGE_INDEX_BUFFER_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.indexBufferHandle = er.device->GetGpuResourceManager().Create(INDEX_BUFFER_NAME, bufferDesc,
            { reinterpret_cast<const uint8_t*>(INDEX_BUFFER_DATA), sizeof(INDEX_BUFFER_DATA) });
    }
    // Indirect buffer
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = sizeof(INDIRECT_BUFFER_DATA);
        bufferDesc.engineCreationFlags =
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE |
            CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER | CORE_ENGINE_BUFFER_CREATION_DEFERRED_DESTROY;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_SRC_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT |
                                CORE_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.indirectBufferHandle = er.device->GetGpuResourceManager().Create(INDIRECT_BUFFER_NAME, bufferDesc,
            { reinterpret_cast<const uint8_t*>(INDIRECT_BUFFER_DATA), sizeof(INDIRECT_BUFFER_DATA) });
    }
    // Color attachment
    {
        GpuImageDesc imageDesc;
        imageDesc.width = res.width;
        imageDesc.height = res.height;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.sampleCountFlags = SampleCountFlagBits::CORE_SAMPLE_COUNT_4_BIT;
        imageDesc.format = res.colorFormat;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        res.colorHandle = er.device->GetGpuResourceManager().Create(COLOR_ATTACHMENT_NAME, imageDesc);
    }
    // Depth attachment
    {
        GpuImageDesc imageDesc;
        imageDesc.width = res.width;
        imageDesc.height = res.height;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.sampleCountFlags = SampleCountFlagBits::CORE_SAMPLE_COUNT_4_BIT;
        imageDesc.format = res.depthFormat;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        res.depthHandle = er.device->GetGpuResourceManager().Create(DEPTH_ATTACHMENT_NAME, imageDesc);
    }
    // Resolve attachment
    {
        GpuImageDesc imageDesc;
        imageDesc.width = res.width;
        imageDesc.height = res.height;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = res.colorFormat;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        res.resolveHandle = er.device->GetGpuResourceManager().Create(RESOLVE_ATTACHMENT_NAME, imageDesc);
    }
    // Depth resolve attachment
    {
        GpuImageDesc imageDesc;
        imageDesc.width = res.width;
        imageDesc.height = res.height;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = res.depthFormat;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        res.depthResolveHandle = er.device->GetGpuResourceManager().Create(DEPTH_RESOLVE_ATTACHMENT_NAME, imageDesc);
    }
    GpuResourceManager& gpuResourceMgr = (GpuResourceManager&)er.device->GetGpuResourceManager();
    ((Device*)er.device)->Activate();
    gpuResourceMgr.HandlePendingAllocations(true);
    ((Device*)er.device)->Deactivate();
}
void DestroyTestResources(TestResources& res)
{
    res.vertexBufferHandle = {};
    res.indexBufferHandle = {};
    res.indirectBufferHandle = {};

    res.colorHandle = {};
    res.depthHandle = {};
    res.resolveHandle = {};
}
void TestRenderCommandList(const UTest::EngineResources& engine, TestResources& res)
{
    Device& device = static_cast<Device&>(*engine.device);
    GpuQueue queue = device.GetValidGpuQueue({ GpuQueue::QueueType::GRAPHICS, 0 });
    bool enableMultiQueue = true;

    ShaderManager& shaderMgr = (ShaderManager&)device.GetShaderManager();
    GpuResourceManager& gpuResourceMgr = (GpuResourceManager&)device.GetGpuResourceManager();
    NodeContextPsoManager nodeContextPsoMgr { device, shaderMgr };
    unique_ptr<NodeContextDescriptorSetManager> nodeContextDescriptorSetMgr =
        move(device.CreateNodeContextDescriptorSetManager());

    auto shaderHandle = shaderMgr.GetShaderHandle("rendershaders://shader/RenderCommandListTest.shader");
    auto stateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderHandle.GetHandle());
    const auto& graphicsState = shaderMgr.GetGraphicsState(stateHandle);
    const auto& pipelineLayout = shaderMgr.GetReflectionPipelineLayout(shaderHandle);
    const auto& inputLayout = shaderMgr.GetReflectionVertexInputDeclaration(shaderHandle.GetHandle());
    ShaderSpecializationConstantDataView shaderSpec {};
    RenderHandle psoHandle = nodeContextPsoMgr.GetGraphicsPsoHandle(shaderHandle.GetHandle(), stateHandle.GetHandle(),
        pipelineLayout, inputLayout, shaderSpec, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        ASSERT_TRUE(cmdList.GetInterface(CORE_NS::IInterface::UID));
        ASSERT_TRUE(cmdList.GetInterface(CORE_NS::IInterface::UID)->GetInterface<IRenderCommandList>());
        ASSERT_TRUE(cmdList.GetInterface(IRenderCommandList::UID));
        ASSERT_FALSE(cmdList.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));

        const auto& constCmdList = cmdList;
        ASSERT_TRUE(constCmdList.GetInterface(CORE_NS::IInterface::UID));
        ASSERT_TRUE(constCmdList.GetInterface(CORE_NS::IInterface::UID)->GetInterface<IRenderCommandList>());
        ASSERT_TRUE(constCmdList.GetInterface(IRenderCommandList::UID));
        ASSERT_FALSE(constCmdList.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        cmdList.SetValidGpuQueueReleaseAcquireBarriers();
        cmdList.BeforeRenderNodeExecuteFrame();
        cmdList.AfterRenderNodeExecuteFrame();
        ASSERT_TRUE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        RenderPassDesc renderPassDesc {};
        renderPassDesc.subpassCount = 2;
        cmdList.BeginRenderPass(renderPassDesc, {});
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        RenderPassDesc renderPassDesc;
        renderPassDesc.attachmentCount = 4;
        renderPassDesc.attachmentHandles[0] = res.colorHandle.GetHandle();
        renderPassDesc.attachments[0].clearValue.color = { 0.f, 0.f, 0.f, 1.f };
        renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.attachmentHandles[1] = res.depthHandle.GetHandle();
        renderPassDesc.attachments[1].clearValue.depthStencil = { 1.f, 0u };
        renderPassDesc.attachments[1].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.attachmentHandles[2] = res.resolveHandle.GetHandle();
        renderPassDesc.attachments[2].clearValue.color = { 0.f, 0.f, 0.f, 1.f };
        renderPassDesc.attachments[2].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.attachmentHandles[3] = res.depthResolveHandle.GetHandle();
        renderPassDesc.attachments[3].clearValue.depthStencil = { 1.f, 0u };
        renderPassDesc.attachments[3].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.subpassCount = 2;
        renderPassDesc.subpassContents = SubpassContents::CORE_SUBPASS_CONTENTS_SECONDARY_COMMAND_LISTS;
        renderPassDesc.renderArea.extentWidth = res.width;
        renderPassDesc.renderArea.extentHeight = res.height;
        RenderPassSubpassDesc subpassDesc[2];
        subpassDesc[0].colorAttachmentCount = 1;
        subpassDesc[0].colorAttachmentIndices[0] = 0;
        subpassDesc[0].depthAttachmentCount = 1;
        subpassDesc[0].depthAttachmentIndex = 1;
        subpassDesc[0].resolveAttachmentCount = 1;
        subpassDesc[0].resolveAttachmentIndices[0] = 2;
        subpassDesc[0].depthResolveAttachmentCount = 1;
        subpassDesc[0].depthResolveAttachmentIndex = 3;
        subpassDesc[0].depthResolveModeFlags = ResolveModeFlagBits::CORE_RESOLVE_MODE_MAX_BIT;

        subpassDesc[1].colorAttachmentCount = 1;
        subpassDesc[1].colorAttachmentIndices[0] = 0;
        subpassDesc[1].depthAttachmentCount = 1;
        subpassDesc[1].depthAttachmentIndex = 1;
        subpassDesc[1].resolveAttachmentCount = 1;
        subpassDesc[1].resolveAttachmentIndices[0] = 2;
        subpassDesc[1].depthResolveAttachmentCount = 1;
        subpassDesc[1].depthResolveAttachmentIndex = 3;
        subpassDesc[1].depthResolveModeFlags = ResolveModeFlagBits::CORE_RESOLVE_MODE_MAX_BIT;
        cmdList.BeginRenderPass(renderPassDesc, { subpassDesc, 2 });
        cmdList.BeginRenderPass(renderPassDesc, { subpassDesc, 2 });
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        cmdList.EndRenderPass();
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        GeneralBarrier barrier1;
        barrier1.accessFlags = AccessFlagBits::CORE_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        barrier1.pipelineStageFlags = PipelineStageFlagBits::CORE_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        GeneralBarrier barrier2;
        barrier1.accessFlags = AccessFlagBits::CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier1.pipelineStageFlags = PipelineStageFlagBits::CORE_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        cmdList.CustomMemoryBarrier(barrier1, barrier2);
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        cmdList.BindPipeline(psoHandle);
        ImageResourceBarrier barrier;
        barrier.accessFlags = AccessFlagBits::CORE_ACCESS_SHADER_READ_BIT;
        barrier.imageLayout = ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED;
        barrier.pipelineStageFlags = PipelineStageFlagBits::CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        ImageSubresourceRange subresource;
        subresource.baseArrayLayer = 0;
        subresource.baseMipLevel = 0;
        subresource.imageAspectFlags = ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT;
        subresource.layerCount = 1;
        subresource.levelCount = 1;
        cmdList.CustomImageBarrier(res.colorHandle.GetHandle(), barrier, subresource);
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        Size2D fragmentSize;
        fragmentSize.width = 8;
        fragmentSize.height = 8;
        FragmentShadingRateCombinerOps ops;
        ops.op1 = FragmentShadingRateCombinerOp::CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP;
        ops.op2 = FragmentShadingRateCombinerOp::CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_MUL;
        cmdList.SetDynamicStateFragmentShadingRate(fragmentSize, ops);
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        float blendConstants[7] = { 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f };
        cmdList.SetDynamicStateBlendConstants({ blendConstants, countof(blendConstants) });
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        cmdList.SetExecuteBackendFramePosition();
        cmdList.SetExecuteBackendFramePosition();
        cmdList.ClearColorImage({}, {}, {});
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        cmdList.BeginRenderPass({}, {});
        cmdList.BlitImage({}, {}, {}, {});
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        cmdList.CopyImageToImage({}, {}, {});
        cmdList.CopyImageToBuffer({}, {}, {});
        cmdList.CopyBufferToBuffer({}, {}, {});
        cmdList.CopyBufferToImage({}, {}, {});
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        cmdList.BeginRenderPass({}, {});
        cmdList.CustomImageBarrier({}, {}, {});
        cmdList.CustomBufferBarrier({}, {}, {}, 0u, 0u);
        cmdList.CustomMemoryBarrier({}, {});
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
#if NDEBUG
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        cmdList.EndDisableAutomaticBarrierPoints();
        cmdList.BeginDisableAutomaticBarrierPoints();
        cmdList.BeginDisableAutomaticBarrierPoints();
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
#endif // NDEBUG
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        cmdList.BeginRenderPass({}, 0u, {});
        cmdList.BeginRenderPass({}, 0u, {});
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        cmdList.BindIndexBuffer({});
        VertexBuffer vbos[10];
        for (uint32_t i = 0; i < countof(vbos); ++i) {
            vbos[i].bufferHandle = {};
            vbos[i].bufferOffset = 0;
            vbos[i].byteSize = 0;
        }
        cmdList.BindVertexBuffers({ vbos, countof(vbos) });
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        PushConstant pc;
        pc.byteSize = 256u;
        cmdList.PushConstant(pc, nullptr);
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        cmdList.BindPipeline({});
        cmdList.Draw(0u, 0u, 0u, 0u);
        cmdList.DrawIndirect({}, 0u, 0u, 0u);
        cmdList.DrawIndexed(0u, 0u, 0u, 0u, 0u);
        cmdList.DrawIndexedIndirect({}, 0u, 0u, 0u);
        auto cmds = cmdList.GetRenderCommands();
        ASSERT_EQ(0u, cmds.size());
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        cmdList.BeginRenderPass({}, {});
        cmdList.BeginDisableAutomaticBarrierPoints();
        cmdList.AfterRenderNodeExecuteFrame();
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        ScissorDesc scissor;
        scissor.extentHeight = 0u;
        scissor.extentWidth = 0u;
        cmdList.SetDynamicStateScissor(scissor);
        ViewportDesc viewport;
        viewport.width = 0.5f;
        viewport.height = 0.5f;
        cmdList.SetDynamicStateViewport(viewport);
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        ImageSubresourceRange range;
        range.baseMipLevel = 6;
        range.levelCount = 2;
        range.baseArrayLayer = 7;
        range.layerCount = 1;
        cmdList.CustomImageBarrier({}, {}, {}, range);
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
#if NDEBUG
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        RenderPassDesc renderPass;
        renderPass.renderArea.extentHeight = 0u;
        renderPass.renderArea.extentWidth = 0u;
        renderPass.renderArea.offsetX = 16u;
        renderPass.renderArea.offsetY = 16u;
        renderPass.attachmentCount = 2;
        renderPass.subpassCount = 2;
        {
            GpuImageDesc imageDesc;
            imageDesc.width = 2u;
            imageDesc.height = 2u;
            imageDesc.depth = 1;
            imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
            imageDesc.format = res.colorFormat;
            imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
            imageDesc.imageType = CORE_IMAGE_TYPE_2D;
            imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
            imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            imageDesc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
            renderPass.attachmentHandles[0] = engine.device->GetGpuResourceManager().Create(imageDesc).GetHandle();
        }
        {
            GpuImageDesc imageDesc;
            imageDesc.width = 4u;
            imageDesc.height = 4u;
            imageDesc.depth = 1;
            imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
            imageDesc.format = res.colorFormat;
            imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
            imageDesc.imageType = CORE_IMAGE_TYPE_2D;
            imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
            imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            imageDesc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
            renderPass.attachmentHandles[1] = engine.device->GetGpuResourceManager().Create(imageDesc).GetHandle();
        }
        cmdList.BeginRenderPass(renderPass, {});
        cmdList.BeginRenderPass(renderPass, 0u, {});
        cmdList.BeginRenderPass(renderPass, 1u, {});
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
#endif // NDEBUG
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        DescriptorSetLayoutBindingResources bindingRes;
        bindingRes.bindingMask = 0u;
        bindingRes.descriptorSetBindingMask = 1u;
        SamplerDescriptor samplers[2];
        samplers[0].binding.descriptorType = CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE;
        samplers[1].binding.descriptorType = CORE_DESCRIPTOR_TYPE_SAMPLER;
        bindingRes.samplers = { samplers, countof(samplers) };
        ImageDescriptor images[2];
        images[0].binding.descriptorType = CORE_DESCRIPTOR_TYPE_SAMPLER;
        images[1].binding.descriptorType = CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindingRes.images = { images, countof(images) };
        BufferDescriptor buffers[6];
        buffers[0].binding.descriptorType = CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        buffers[1].binding.descriptorType = CORE_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        buffers[2].binding.descriptorType = CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        buffers[3].binding.descriptorType = CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        buffers[4].binding.descriptorType = CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE;
        buffers[5].binding.descriptorType = CORE_DESCRIPTOR_TYPE_SAMPLER;
        bindingRes.buffers = { buffers, countof(buffers) };
        cmdList.UpdateDescriptorSet({}, bindingRes);
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        RenderHandle handles[9] = { {}, {}, {}, {}, {}, {}, {}, {}, {} };
        cmdList.BindDescriptorSets(0u, { handles, countof(handles) });
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        uint32_t offsets[32];
        for (uint32_t i = 0; i < countof(offsets); ++i) {
            offsets[i] = 0u;
        }
        cmdList.BindDescriptorSet(0u, {}, { offsets, countof(offsets) });
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
    {
        RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr,
            nodeContextPsoMgr, queue, enableMultiQueue };
        cmdList.BeginFrame();
        RenderPassDesc renderPassDesc;
        renderPassDesc.attachmentCount = 4;
        renderPassDesc.attachmentHandles[0] = res.colorHandle.GetHandle();
        renderPassDesc.attachments[0].clearValue.color = { 0.f, 0.f, 0.f, 1.f };
        renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.attachmentHandles[1] = res.depthHandle.GetHandle();
        renderPassDesc.attachments[1].clearValue.depthStencil = { 1.f, 0u };
        renderPassDesc.attachments[1].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        renderPassDesc.attachments[1].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.attachments[3].stencilLoadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        renderPassDesc.attachments[3].stencilStoreOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.attachmentHandles[2] = {};
        renderPassDesc.attachments[2].clearValue.color = { 0.f, 0.f, 0.f, 1.f };
        renderPassDesc.attachments[2].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        renderPassDesc.attachments[2].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.attachmentHandles[3] = res.depthResolveHandle.GetHandle();
        renderPassDesc.attachments[3].clearValue.depthStencil = { 1.f, 0u };
        renderPassDesc.attachments[3].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        renderPassDesc.attachments[3].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.attachments[3].stencilLoadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        renderPassDesc.attachments[3].stencilStoreOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.subpassCount = 2;
        renderPassDesc.subpassContents = SubpassContents::CORE_SUBPASS_CONTENTS_INLINE;
        renderPassDesc.renderArea.extentWidth = res.width;
        renderPassDesc.renderArea.extentHeight = res.height;
        RenderPassSubpassDesc subpassDesc[2];
        subpassDesc[0].colorAttachmentCount = 1;
        subpassDesc[0].colorAttachmentIndices[0] = 0;
        subpassDesc[0].depthAttachmentCount = 1;
        subpassDesc[0].depthAttachmentIndex = 1;
        subpassDesc[0].resolveAttachmentCount = 1;
        subpassDesc[0].resolveAttachmentIndices[0] = 2;
        subpassDesc[0].depthResolveAttachmentCount = 1;
        subpassDesc[0].depthResolveAttachmentIndex = 3;
        subpassDesc[0].depthResolveModeFlags = ResolveModeFlagBits::CORE_RESOLVE_MODE_MAX_BIT;
        subpassDesc[0].stencilResolveModeFlags = ResolveModeFlagBits::CORE_RESOLVE_MODE_MIN_BIT;

        subpassDesc[1].colorAttachmentCount = 1;
        subpassDesc[1].colorAttachmentIndices[0] = 0;
        subpassDesc[1].depthAttachmentCount = 1;
        subpassDesc[1].depthAttachmentIndex = 1;
        subpassDesc[1].resolveAttachmentCount = 1;
        subpassDesc[1].resolveAttachmentIndices[0] = 2;
        subpassDesc[1].depthResolveAttachmentCount = 1;
        subpassDesc[1].depthResolveAttachmentIndex = 3;
        subpassDesc[1].depthResolveModeFlags = ResolveModeFlagBits::CORE_RESOLVE_MODE_MAX_BIT;
        subpassDesc[1].stencilResolveModeFlags = ResolveModeFlagBits::CORE_RESOLVE_MODE_MIN_BIT;
        cmdList.BeginRenderPass(renderPassDesc, { subpassDesc, 2 });
        ASSERT_FALSE(cmdList.HasValidRenderCommands());
    }
}

void TestRenderCommandList(const UTest::EngineResources& engine, TestResources& res, bool parallel,
    const vector<RenderHandleReference>& swapchains = {})
{
    Device& device = static_cast<Device&>(*engine.device);
    GpuQueue queue = device.GetValidGpuQueue({ GpuQueue::QueueType::GRAPHICS, 0 });
    bool enableMultiQueue = true;

    ShaderManager& shaderMgr = (ShaderManager&)device.GetShaderManager();
    GpuResourceManager& gpuResourceMgr = (GpuResourceManager&)device.GetGpuResourceManager();
    NodeContextPsoManager nodeContextPsoMgr { device, shaderMgr };
    unique_ptr<NodeContextDescriptorSetManager> nodeContextDescriptorSetMgr =
        move(device.CreateNodeContextDescriptorSetManager());
    unique_ptr<NodeContextPoolManager> nodeContextPoolMgr = device.CreateNodeContextPoolManager(gpuResourceMgr, queue);
    unique_ptr<RenderBarrierList> renderBarrierList = make_unique<RenderBarrierList>(0U);

    RenderCommandList cmdList { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr, gpuResourceMgr, nodeContextPsoMgr,
        queue, enableMultiQueue };

    auto shaderHandle = shaderMgr.GetShaderHandle("rendershaders://shader/RenderCommandListTest.shader");
    auto stateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderHandle.GetHandle());
    const auto& graphicsState = shaderMgr.GetGraphicsState(stateHandle);
    const auto& pipelineLayout = shaderMgr.GetReflectionPipelineLayout(shaderHandle);
    const auto& inputLayout = shaderMgr.GetReflectionVertexInputDeclaration(shaderHandle.GetHandle());
    ShaderSpecializationConstantDataView shaderSpec {};
    RenderHandle psoHandle = nodeContextPsoMgr.GetGraphicsPsoHandle(shaderHandle.GetHandle(), stateHandle.GetHandle(),
        pipelineLayout, inputLayout, shaderSpec, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });

    nodeContextPoolMgr->BeginFrame();
    {
        cmdList.BeginFrame();
        for (uint32_t i = 0; i < 1000; i++) {
            cmdList.NextSubpass(SubpassContents::CORE_SUBPASS_CONTENTS_INLINE);
        }
        cmdList.BeginFrame();
        if (engine.backend == DeviceBackendType::VULKAN) {
            cmdList.SetExecuteBackendFramePosition();
        }

        AsBuildGeometryData geom;
        cmdList.BuildAccelerationStructures(geom, {}, {}, {});

        RenderPassDesc renderPassDesc;
        renderPassDesc.attachmentCount = 4;
        renderPassDesc.attachmentHandles[0] = res.colorHandle.GetHandle();
        renderPassDesc.attachments[0].clearValue.color = { 0.f, 0.f, 0.f, 1.f };
        renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.attachmentHandles[1] = res.depthHandle.GetHandle();
        renderPassDesc.attachments[1].clearValue.depthStencil = { 1.f, 0u };
        renderPassDesc.attachments[1].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        renderPassDesc.attachments[1].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.attachments[3].stencilLoadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        renderPassDesc.attachments[3].stencilStoreOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.attachmentHandles[2] = res.resolveHandle.GetHandle();
        renderPassDesc.attachments[2].clearValue.color = { 0.f, 0.f, 0.f, 1.f };
        renderPassDesc.attachments[2].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        renderPassDesc.attachments[2].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.attachmentHandles[3] = res.depthResolveHandle.GetHandle();
        renderPassDesc.attachments[3].clearValue.depthStencil = { 1.f, 0u };
        renderPassDesc.attachments[3].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        renderPassDesc.attachments[3].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.attachments[3].stencilLoadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        renderPassDesc.attachments[3].stencilStoreOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        renderPassDesc.subpassCount = 2;
        renderPassDesc.subpassContents = SubpassContents::CORE_SUBPASS_CONTENTS_INLINE;
        renderPassDesc.renderArea.extentWidth = res.width;
        renderPassDesc.renderArea.extentHeight = res.height;
        RenderPassSubpassDesc subpassDesc[2];
        subpassDesc[0].colorAttachmentCount = 1;
        subpassDesc[0].colorAttachmentIndices[0] = 0;
        subpassDesc[0].depthAttachmentCount = 1;
        subpassDesc[0].depthAttachmentIndex = 1;
        subpassDesc[0].resolveAttachmentCount = 1;
        subpassDesc[0].resolveAttachmentIndices[0] = 2;
        subpassDesc[0].depthResolveAttachmentCount = 1;
        subpassDesc[0].depthResolveAttachmentIndex = 3;
        subpassDesc[0].depthResolveModeFlags = ResolveModeFlagBits::CORE_RESOLVE_MODE_MAX_BIT;
        subpassDesc[0].stencilResolveModeFlags = ResolveModeFlagBits::CORE_RESOLVE_MODE_MIN_BIT;

        subpassDesc[1].colorAttachmentCount = 1;
        subpassDesc[1].colorAttachmentIndices[0] = 0;
        subpassDesc[1].depthAttachmentCount = 1;
        subpassDesc[1].depthAttachmentIndex = 1;
        subpassDesc[1].resolveAttachmentCount = 1;
        subpassDesc[1].resolveAttachmentIndices[0] = 2;
        subpassDesc[1].depthResolveAttachmentCount = 1;
        subpassDesc[1].depthResolveAttachmentIndex = 3;
        subpassDesc[1].depthResolveModeFlags = ResolveModeFlagBits::CORE_RESOLVE_MODE_MAX_BIT;
        subpassDesc[1].stencilResolveModeFlags = ResolveModeFlagBits::CORE_RESOLVE_MODE_MIN_BIT;
        cmdList.BeginRenderPass(renderPassDesc, { subpassDesc, 2 });

        cmdList.BindPipeline(psoHandle);

        ViewportDesc viewportDesc;
        viewportDesc.width = res.width;
        viewportDesc.height = res.height;
        viewportDesc.x = 0;
        viewportDesc.y = 0;
        viewportDesc.minDepth = 0;
        viewportDesc.maxDepth = 1;
        cmdList.SetDynamicStateViewport(viewportDesc);

        ScissorDesc scissorDesc;
        scissorDesc.extentWidth = res.width;
        scissorDesc.extentHeight = res.height;
        scissorDesc.offsetX = 0;
        scissorDesc.offsetY = 0;
        cmdList.SetDynamicStateScissor(scissorDesc);

        VertexBuffer vbo;
        vbo.bufferHandle = res.vertexBufferHandle.GetHandle();
        cmdList.BindVertexBuffers({ &vbo, 1 });
        cmdList.Draw(3, 1, 0, 0);

        IndexBuffer ibo;
        ibo.bufferHandle = res.indexBufferHandle.GetHandle();
        ibo.indexType = IndexType::CORE_INDEX_TYPE_UINT32;
        cmdList.BindIndexBuffer(ibo);
        cmdList.DrawIndexed(6, 1, 0, 0, 0);

        cmdList.DrawIndirect(res.indirectBufferHandle.GetHandle(), 0, 1, 4 * sizeof(uint32_t));
        cmdList.DrawIndexedIndirect(
            res.indirectBufferHandle.GetHandle(), 4 * sizeof(uint32_t), 1, 5 * sizeof(uint32_t));

        cmdList.NextSubpass(SubpassContents::CORE_SUBPASS_CONTENTS_INLINE);
        if (engine.backend == DeviceBackendType::VULKAN) {
            cmdList.SetDynamicStateDepthBias(0.001f, 0.f, 1.f);
            cmdList.SetDynamicStateDepthBounds(0.0001f, 0.999f);
        }
        cmdList.SetDynamicStateLineWidth(1.2f);
        if (engine.backend == DeviceBackendType::VULKAN) {
            float blendConstants[] = { 1.f, 1.f, 1.f, 1.f };
            cmdList.SetDynamicStateBlendConstants({ blendConstants, countof(blendConstants) });
        }
        cmdList.SetDynamicStateStencilCompareMask(StencilFaceFlagBits::CORE_STENCIL_FACE_FRONT_BIT, 1u);
        cmdList.SetDynamicStateStencilCompareMask(StencilFaceFlagBits::CORE_STENCIL_FACE_BACK_BIT, 1u);

        cmdList.SetDynamicStateStencilWriteMask(StencilFaceFlagBits::CORE_STENCIL_FACE_FRONT_BIT, 1u);
        cmdList.SetDynamicStateStencilWriteMask(StencilFaceFlagBits::CORE_STENCIL_FACE_BACK_BIT, 1u);

        cmdList.SetDynamicStateStencilReference(StencilFaceFlagBits::CORE_STENCIL_FACE_FRONT_BIT, 1u);
        cmdList.SetDynamicStateStencilReference(StencilFaceFlagBits::CORE_STENCIL_FACE_BACK_BIT, 1u);

        cmdList.EndRenderPass();
        ASSERT_TRUE(cmdList.HasValidRenderCommands());
    }

    auto cmds = cmdList.GetRenderCommands();
    for (auto& cmd : cmds) {
        if (cmd.type == RenderCommandType::BEGIN_RENDER_PASS) {
            RenderCommandBeginRenderPass& rcbrp = *static_cast<RenderCommandBeginRenderPass*>(cmd.rc);
            rcbrp.imageLayouts.attachmentFinalLayouts[0] = CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            rcbrp.imageLayouts.attachmentFinalLayouts[1] = CORE_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            rcbrp.imageLayouts.attachmentFinalLayouts[2] = CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            rcbrp.imageLayouts.attachmentFinalLayouts[3] = CORE_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
    }

    const auto factory = CORE_NS::GetInstance<CORE_NS::ITaskQueueFactory>(CORE_NS::UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(factory->GetNumberOfCores());
    auto parallelQueue = factory->CreateParallelTaskQueue(threadPool);

    device.Activate();
    if (!parallel) {
        parallelQueue = {};
    }
    auto renderBackend =
        device.CreateRenderBackend(gpuResourceMgr, static_cast<CORE_NS::ITaskQueue*>(parallelQueue.get()));
    RenderCommandContext rcc;
    rcc.debugName = CMD_LIST_DEBUG_NAME;
    rcc.submitDepencies.waitSemaphoreCount = 0;
    rcc.submitDepencies.signalSemaphore = false;
    rcc.submitDepencies.waitForSwapchainAcquireSignal = true;
    rcc.renderCommandList = &cmdList;
    rcc.renderBarrierList = renderBarrierList.get();
    rcc.nodeContextDescriptorSetMgr = &*nodeContextDescriptorSetMgr;
    rcc.nodeContextPsoMgr = &nodeContextPsoMgr;
    rcc.renderGraphRenderNodeIndex = 0;
    rcc.nodeContextPoolMgr = nodeContextPoolMgr.get();
    RenderCommandFrameData rcfd;
    rcfd.renderCommandContexts.emplace_back(rcc);
    rcfd.renderFrameSync = nullptr;
    rcfd.firstSwapchainNodeIdx = 0;

    RenderBackendBackBufferConfiguration rbbbc;
    if (device.HasSwapchain()) {
        if (swapchains.empty()) {
            rbbbc.swapchainData.push_back({});
            auto& ref = rbbbc.swapchainData[0U];
            ref.config.backBufferType = NodeGraphBackBufferConfiguration::BackBufferType::SWAPCHAIN;
            // NOTE: old way removed
            // memcpy(ref.config.backBufferName, RESOLVE_ATTACHMENT_NAME.data(), RESOLVE_ATTACHMENT_NAME.size());
            // ref.config.backBufferName[RESOLVE_ATTACHMENT_NAME.size()] = 0;
            ref.config.present = true;
            ref.backBufferState.gpuQueue = queue;
        } else {
            for (auto& swapchain : swapchains) {
                auto& ref = rbbbc.swapchainData.emplace_back();
                ref.handle = swapchain.GetHandle();
                ref.config.backBufferType = NodeGraphBackBufferConfiguration::BackBufferType::SWAPCHAIN;
                // NOTE: old way removed
                // memcpy(ref.config.backBufferName, RESOLVE_ATTACHMENT_NAME.data(), RESOLVE_ATTACHMENT_NAME.size());
                // ref.config.backBufferName[RESOLVE_ATTACHMENT_NAME.size()] = 0;
                ref.config.present = true;
                ref.backBufferState.gpuQueue = queue;
            }
        }
    }

    device.SetRenderBackendRunning(true);
    renderBackend->Render(rcfd, rbbbc);
    renderBackend->Present(rbbbc);
    device.SetRenderBackendRunning(false);

    gpuResourceMgr.EndFrame();

    device.WaitForIdle();
}
void TestRenderCommandList(DeviceBackendType backend, bool parallel)
{
    UTest::EngineResources engine;
    engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        engine.createWindow = true;
    }
    TestResources res;
    res.width = WIDTH;
    res.height = HEIGHT;
    {
        UTest::CreateEngineSetup(engine);
        CreateTestResources(engine, res);
    }
    TestRenderCommandList(engine, res, parallel);
    ((Device*)engine.device)->Deactivate();
    {
        DestroyTestResources(res);
        UTest::DestroyEngine(engine);
    }
}

void TestRenderCommandListWithSwapchain(DeviceBackendType backend, uint32_t swapchainCount = 1)
{
    UTest::EngineResources engine;
    engine.backend = backend;
    engine.createWindow = true;
    engine.width = WIDTH;
    engine.height = HEIGHT;
    UTest::CreateEngineSetup(engine);

#if defined(__OHOS__) || defined(__ANDROID__) // Can't create multiple windows per app on mobile
    if (swapchainCount > 1) {
        return;
    }
#endif

    vector<string> windowNames = {};
    windowNames.push_back(UTest::TEST_NATIVE_WINDOW_NAME);

    const uint32_t secondaryWindowCount = swapchainCount - 1;
    for (uint32_t i = 0; i < secondaryWindowCount; i++) {
        const string name = string("secondary") + BASE_NS::to_string(i);
        UTest::CreateNativeWindow(WIDTH, HEIGHT, name);
        windowNames.push_back(name);
    }

    vector<uint64_t> surfaces = {};
    for (const string& name : windowNames) {
        uint64_t& surface = surfaces.emplace_back(UTest::CreateSurface(engine, name));
#if !defined(__OHOS__) // no surfaces in OHOS, only windows
        ASSERT_NE(surface, 0);
#endif
    }

    SwapchainCreateInfo swapchainInfo {};
#if defined(__OHOS__)
    swapchainInfo.window.window = reinterpret_cast<uintptr_t>(::Test::g_ohosApp->GetWindowHandle());
    ASSERT_NE(::Test::g_ohosApp->GetWindowHandle(), nullptr);
#endif

    ASSERT_EQ(surfaces.size(), swapchainCount);
    ASSERT_EQ(windowNames.size(), swapchainCount);
    {
        swapchainInfo.swapchainFlags = RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_COLOR_BUFFER_BIT |
                                       RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_DEPTH_BUFFER_BIT |
                                       RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_VSYNC_BIT |
                                       RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_SRGB_BIT;

        vector<RenderHandleReference> swapchains = {};
        for (uint32_t i = 0; i < swapchainCount; i++) {
            swapchainInfo.surfaceHandle = surfaces[i];
            auto& swapchain = swapchains.emplace_back(
                engine.device->CreateSwapchainHandle(swapchainInfo, RenderHandleReference {}, BASE_NS::to_string(i)));
            ASSERT_NE(nullptr, ((Device*)engine.device)->GetSwapchain(swapchain.GetHandle()));
        }
        ASSERT_EQ(swapchains.size(), swapchainCount);

        ((Device*)engine.device)->Activate();
        ((GpuResourceManager&)engine.device->GetGpuResourceManager()).HandlePendingAllocations(true);
        ((Device*)engine.device)->Deactivate();

        TestResources res;
        res.colorFormat = ((Device*)engine.device)->GetSwapchain(swapchains[0].GetHandle())->GetDesc().format;
        res.depthFormat =
            ((Device*)engine.device)->GetSwapchain(swapchains[0].GetHandle())->GetDescDepthBuffer().format;
        res.width = engine.width;
        res.height = engine.height;
        CreateTestResources(engine, res);

        TestRenderCommandList(engine, res, true, swapchains);
        ((Device*)engine.device)->Deactivate();

        for (auto& swapchain : swapchains) {
            engine.device->DestroySwapchain(swapchain);
        }

        for (uint64_t surface : surfaces) {
            UTest::DestroySurface(engine, surface);
        }

        for (const string& name : windowNames) {
            UTest::DestroyNativeWindow(name);
        }
        swapchains.clear();
        windowNames.clear();
        surfaces.clear();

        DestroyTestResources(res);
        UTest::DestroyEngine(engine);
    }
}

void TestSecondaryCommandList(DeviceBackendType backend)
{
    UTest::EngineResources er;
    TestResources res;
    res.width = 128;
    res.height = 128;
    er.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        er.createWindow = true;
    }
    {
        UTest::CreateEngineSetup(er);
        CreateTestResources(er, res);
    }

    {
        Device& device = static_cast<Device&>(*er.device);
        GpuQueue queue = device.GetValidGpuQueue({ GpuQueue::QueueType::GRAPHICS, 0 });
        bool enableMultiQueue = true;

        ShaderManager& shaderMgr = (ShaderManager&)device.GetShaderManager();
        GpuResourceManager& gpuResourceMgr = (GpuResourceManager&)device.GetGpuResourceManager();
        NodeContextPsoManager nodeContextPsoMgr1 { device, shaderMgr };
        unique_ptr<NodeContextDescriptorSetManager> nodeContextDescriptorSetMgr1 =
            move(device.CreateNodeContextDescriptorSetManager());

        RenderCommandList cmdList1 { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr1, gpuResourceMgr,
            nodeContextPsoMgr1, queue, enableMultiQueue };
        const auto& nodeContextPoolMgr1 = device.CreateNodeContextPoolManager(gpuResourceMgr, queue);

        auto shaderHandle = shaderMgr.GetShaderHandle("rendershaders://shader/RenderCommandListTest.shader");
        auto stateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderHandle.GetHandle());
        const auto& graphicsState = shaderMgr.GetGraphicsState(stateHandle);
        const auto& pipelineLayout = shaderMgr.GetReflectionPipelineLayout(shaderHandle);
        const auto& inputLayout = shaderMgr.GetReflectionVertexInputDeclaration(shaderHandle.GetHandle());
        ShaderSpecializationConstantDataView shaderSpec {};
        RenderHandle psoHandle1 =
            nodeContextPsoMgr1.GetGraphicsPsoHandle(shaderHandle.GetHandle(), stateHandle.GetHandle(), pipelineLayout,
                inputLayout, shaderSpec, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });

        nodeContextPoolMgr1->BeginFrame();
        {
            cmdList1.BeginFrame();

            RenderPassDesc renderPassDesc;
            renderPassDesc.attachmentCount = 4;
            renderPassDesc.attachmentHandles[0] = res.colorHandle.GetHandle();
            renderPassDesc.attachments[0].clearValue.color = { 0.f, 0.f, 0.f, 1.f };
            renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
            renderPassDesc.attachmentHandles[1] = res.depthHandle.GetHandle();
            renderPassDesc.attachments[1].clearValue.depthStencil = { 1.f, 0u };
            renderPassDesc.attachments[1].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            renderPassDesc.attachments[1].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
            renderPassDesc.attachments[3].stencilLoadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            renderPassDesc.attachments[3].stencilStoreOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
            renderPassDesc.attachmentHandles[2] = res.resolveHandle.GetHandle();
            renderPassDesc.attachments[2].clearValue.color = { 0.f, 0.f, 0.f, 1.f };
            renderPassDesc.attachments[2].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            renderPassDesc.attachments[2].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
            renderPassDesc.attachmentHandles[3] = res.depthResolveHandle.GetHandle();
            renderPassDesc.attachments[3].clearValue.depthStencil = { 1.f, 0u };
            renderPassDesc.attachments[3].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            renderPassDesc.attachments[3].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
            renderPassDesc.attachments[3].stencilLoadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            renderPassDesc.attachments[3].stencilStoreOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
            renderPassDesc.subpassCount = 2;
            renderPassDesc.subpassContents = SubpassContents::CORE_SUBPASS_CONTENTS_SECONDARY_COMMAND_LISTS;
            renderPassDesc.renderArea.extentWidth = res.width;
            renderPassDesc.renderArea.extentHeight = res.height;

            RenderPassSubpassDesc subpassDesc;
            subpassDesc.colorAttachmentCount = 1;
            subpassDesc.colorAttachmentIndices[0] = 0;
            subpassDesc.depthAttachmentCount = 1;
            subpassDesc.depthAttachmentIndex = 1;
            subpassDesc.resolveAttachmentCount = 1;
            subpassDesc.resolveAttachmentIndices[0] = 2;
            subpassDesc.depthResolveAttachmentCount = 1;
            subpassDesc.depthResolveAttachmentIndex = 3;
            subpassDesc.depthResolveModeFlags = ResolveModeFlagBits::CORE_RESOLVE_MODE_MAX_BIT;
            subpassDesc.stencilResolveModeFlags = ResolveModeFlagBits::CORE_RESOLVE_MODE_MIN_BIT;

            cmdList1.BeginRenderPass(renderPassDesc, 0, subpassDesc);

            cmdList1.BindPipeline(psoHandle1);

            ViewportDesc viewportDesc;
            viewportDesc.width = res.width;
            viewportDesc.height = res.height;
            viewportDesc.x = 0;
            viewportDesc.y = 0;
            viewportDesc.minDepth = 0;
            viewportDesc.maxDepth = 1;
            cmdList1.SetDynamicStateViewport(viewportDesc);

            ScissorDesc scissorDesc;
            scissorDesc.extentWidth = res.width;
            scissorDesc.extentHeight = res.height;
            scissorDesc.offsetX = 0;
            scissorDesc.offsetY = 0;
            cmdList1.SetDynamicStateScissor(scissorDesc);

            VertexBuffer vbo;
            vbo.bufferHandle = res.vertexBufferHandle.GetHandle();
            cmdList1.BindVertexBuffers({ &vbo, 1 });
            cmdList1.Draw(3, 1, 0, 0);

            cmdList1.EndRenderPass();
            ASSERT_TRUE(cmdList1.HasValidRenderCommands());
        }

        auto cmds = cmdList1.GetRenderCommands();
        for (auto& cmd : cmds) {
            if (cmd.type == RenderCommandType::BEGIN_RENDER_PASS) {
                RenderCommandBeginRenderPass& rcbrp = *static_cast<RenderCommandBeginRenderPass*>(cmd.rc);
                rcbrp.imageLayouts.attachmentFinalLayouts[0] = CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                rcbrp.imageLayouts.attachmentFinalLayouts[1] = CORE_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                rcbrp.imageLayouts.attachmentFinalLayouts[2] = CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                rcbrp.imageLayouts.attachmentFinalLayouts[3] = CORE_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
        }

        const auto factory = CORE_NS::GetInstance<CORE_NS::ITaskQueueFactory>(CORE_NS::UID_TASK_QUEUE_FACTORY);
        auto threadPool = factory->CreateThreadPool(factory->GetNumberOfCores());
        auto parallelQueue = factory->CreateParallelTaskQueue(threadPool);

        device.Activate();
        auto renderBackend =
            device.CreateRenderBackend(gpuResourceMgr, static_cast<CORE_NS::ITaskQueue*>(parallelQueue.get()));

        NodeContextPsoManager nodeContextPsoMgr2 { device, shaderMgr };
        unique_ptr<NodeContextDescriptorSetManager> nodeContextDescriptorSetMgr2 =
            move(device.CreateNodeContextDescriptorSetManager());
        const auto& nodeContextPoolMgr2 = device.CreateNodeContextPoolManager(gpuResourceMgr, queue);

        RenderCommandList cmdList2 { RENDER_NODE_DEBUG_NAME, *nodeContextDescriptorSetMgr2, gpuResourceMgr,
            nodeContextPsoMgr2, queue, enableMultiQueue };
        RenderHandle psoHandle2 =
            nodeContextPsoMgr2.GetGraphicsPsoHandle(shaderHandle.GetHandle(), stateHandle.GetHandle(), pipelineLayout,
                inputLayout, shaderSpec, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });

        nodeContextPoolMgr2->BeginFrame();
        {
            cmdList2.BeginFrame();

            RenderPassDesc renderPassDesc;
            renderPassDesc.attachmentCount = 4;
            renderPassDesc.attachmentHandles[0] = res.colorHandle.GetHandle();
            renderPassDesc.attachments[0].clearValue.color = { 0.f, 0.f, 0.f, 1.f };
            renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
            renderPassDesc.attachmentHandles[1] = res.depthHandle.GetHandle();
            renderPassDesc.attachments[1].clearValue.depthStencil = { 1.f, 0u };
            renderPassDesc.attachments[1].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            renderPassDesc.attachments[1].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
            renderPassDesc.attachments[3].stencilLoadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            renderPassDesc.attachments[3].stencilStoreOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
            renderPassDesc.attachmentHandles[2] = res.resolveHandle.GetHandle();
            renderPassDesc.attachments[2].clearValue.color = { 0.f, 0.f, 0.f, 1.f };
            renderPassDesc.attachments[2].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            renderPassDesc.attachments[2].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
            renderPassDesc.attachmentHandles[3] = res.depthResolveHandle.GetHandle();
            renderPassDesc.attachments[3].clearValue.depthStencil = { 1.f, 0u };
            renderPassDesc.attachments[3].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            renderPassDesc.attachments[3].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
            renderPassDesc.attachments[3].stencilLoadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            renderPassDesc.attachments[3].stencilStoreOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
            renderPassDesc.subpassCount = 2;
            renderPassDesc.subpassContents = SubpassContents::CORE_SUBPASS_CONTENTS_SECONDARY_COMMAND_LISTS;
            renderPassDesc.renderArea.extentWidth = res.width;
            renderPassDesc.renderArea.extentHeight = res.height;

            RenderPassSubpassDesc subpassDesc;
            subpassDesc.colorAttachmentCount = 1;
            subpassDesc.colorAttachmentIndices[0] = 0;
            subpassDesc.depthAttachmentCount = 1;
            subpassDesc.depthAttachmentIndex = 1;
            subpassDesc.resolveAttachmentCount = 1;
            subpassDesc.resolveAttachmentIndices[0] = 2;
            subpassDesc.depthResolveAttachmentCount = 1;
            subpassDesc.depthResolveAttachmentIndex = 3;
            subpassDesc.depthResolveModeFlags = ResolveModeFlagBits::CORE_RESOLVE_MODE_MAX_BIT;
            subpassDesc.stencilResolveModeFlags = ResolveModeFlagBits::CORE_RESOLVE_MODE_MIN_BIT;

            cmdList2.BeginRenderPass(renderPassDesc, 1, subpassDesc);

            cmdList2.BindPipeline(psoHandle2);

            ViewportDesc viewportDesc;
            viewportDesc.width = res.width;
            viewportDesc.height = res.height;
            viewportDesc.x = 0;
            viewportDesc.y = 0;
            viewportDesc.minDepth = 0;
            viewportDesc.maxDepth = 1;
            cmdList2.SetDynamicStateViewport(viewportDesc);

            ScissorDesc scissorDesc;
            scissorDesc.extentWidth = res.width;
            scissorDesc.extentHeight = res.height;
            scissorDesc.offsetX = 0;
            scissorDesc.offsetY = 0;
            cmdList2.SetDynamicStateScissor(scissorDesc);

            VertexBuffer vbo;
            vbo.bufferHandle = res.vertexBufferHandle.GetHandle();
            cmdList2.BindVertexBuffers({ &vbo, 1 });
            cmdList2.Draw(3, 1, 0, 0);

            cmdList2.EndRenderPass();
            ASSERT_TRUE(cmdList2.HasValidRenderCommands());
        }
        auto cmds2 = cmdList2.GetRenderCommands();
        for (auto& cmd : cmds2) {
            if (cmd.type == RenderCommandType::BEGIN_RENDER_PASS) {
                RenderCommandBeginRenderPass& rcbrp = *static_cast<RenderCommandBeginRenderPass*>(cmd.rc);
                rcbrp.imageLayouts.attachmentFinalLayouts[0] = CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                rcbrp.imageLayouts.attachmentFinalLayouts[1] = CORE_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                rcbrp.imageLayouts.attachmentFinalLayouts[2] = CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                rcbrp.imageLayouts.attachmentFinalLayouts[3] = CORE_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
        }

        RenderCommandFrameData rcfd;

        RenderCommandContext rcc;
        rcc.debugName = CMD_LIST_DEBUG_NAME;
        rcc.submitDepencies.waitSemaphoreCount = 0;
        rcc.submitDepencies.signalSemaphore = true;
        rcc.submitDepencies.waitForSwapchainAcquireSignal = false;
        rcc.renderCommandList = &cmdList1;
        rcc.renderBarrierList = new RenderBarrierList(0);
        rcc.nodeContextDescriptorSetMgr = &*nodeContextDescriptorSetMgr1;
        rcc.nodeContextPsoMgr = &nodeContextPsoMgr1;
        rcc.renderGraphRenderNodeIndex = 0;
        rcc.nodeContextPoolMgr = nodeContextPoolMgr1.get();
        rcfd.renderCommandContexts.emplace_back(rcc);

        rcc.renderCommandList = &cmdList2;
        rcc.nodeContextDescriptorSetMgr = &*nodeContextDescriptorSetMgr2;
        rcc.nodeContextPsoMgr = &nodeContextPsoMgr2;
        rcc.renderGraphRenderNodeIndex = 0;
        rcc.nodeContextPoolMgr = nodeContextPoolMgr2.get();
        rcfd.renderCommandContexts.emplace_back(rcc);

        rcfd.renderFrameSync = device.CreateRenderFrameSync().release();
        rcfd.firstSwapchainNodeIdx = 0;

        RenderBackendBackBufferConfiguration rbbbc;
        if (device.HasSwapchain()) {
            rbbbc.swapchainData.push_back({});
            auto& ref = rbbbc.swapchainData[0U];
            ref.config.backBufferType = NodeGraphBackBufferConfiguration::BackBufferType::SWAPCHAIN;
            // NOTE: old way removed
            // memcpy(ref.config.backBufferName, RESOLVE_ATTACHMENT_NAME.data(), RESOLVE_ATTACHMENT_NAME.size());
            // ref.config.backBufferName[RESOLVE_ATTACHMENT_NAME.size()] = 0;
            ref.config.present = true;
            ref.backBufferState.gpuQueue = queue;
        }
        device.SetRenderBackendRunning(true);
        renderBackend->Render(rcfd, rbbbc);
        renderBackend->Present(rbbbc);
        device.SetRenderBackendRunning(false);

        gpuResourceMgr.EndFrame();

        device.WaitForIdle();
        delete rcc.renderBarrierList;
        delete rcfd.renderFrameSync;
    }
    {
        ((Device*)er.device)->Deactivate();
        DestroyTestResources(res);
        UTest::DestroyEngine(er);
    }
}
} // namespace

/**
 * @tc.name: RenderCommandListTest
 * @tc.desc:  Tests for RenderCommandList validation by issueing different commands and checking if the command list has
 * valid or invalid commands.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderCommandList, RenderCommandListTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    if (engine.backend == DeviceBackendType::OPENGL) {
        engine.createWindow = true;
    }
    TestResources res;
    res.width = WIDTH;
    res.height = HEIGHT;
    {
        UTest::CreateEngineSetup(engine);
        CreateTestResources(engine, res);
    }
    TestRenderCommandList(engine, res);
    {
        DestroyTestResources(res);
        UTest::DestroyEngine(engine);
    }
}

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: RenderCommandListTestVulkan
 * @tc.desc: Tests RenderCommandList by issuing various commands and rendering it in the Vulkan backend to verify
 * that command lists are translated to the correct backend commands. Tests command list execution in paralell and
 * non-paralell.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderCommandList, RenderCommandListTestVulkan, testing::ext::TestSize.Level1)
{
    TestRenderCommandList(DeviceBackendType::VULKAN, true);
    TestRenderCommandList(DeviceBackendType::VULKAN, false);
}
/**
 * @tc.name: RenderCommandListWithSwapchainTestVulkan
 * @tc.desc: Tests RenderCommandList by issuing various commands and rendering it in the Vulkan backend to verify
 * that command lists are translated to the correct backend commands and swapchain synchronization and present works in
 * conjunction with the RenderCommandList.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderCommandList, RenderCommandListWithSwapchainTestVulkan, testing::ext::TestSize.Level1)
{
    TestRenderCommandListWithSwapchain(DeviceBackendType::VULKAN);
}
/**
 * @tc.name: RenderCommandListWithMultiSwapchainTestVulkan
 * @tc.desc: Works similarly to RenderCommandListWithSwapchainTestVulkan, but tests that the renderer can function with
 * multiple swapchains and windows.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderCommandList, RenderCommandListWithMultiSwapchainTestVulkan, testing::ext::TestSize.Level1)
{
    TestRenderCommandListWithSwapchain(DeviceBackendType::VULKAN, 16);
}
/**
 * @tc.name: SecondaryCommandListTestVulkan
 * @tc.desc: Tests for Secondary Command List Test Vulkan. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderCommandList, SecondaryCommandListTestVulkan, testing::ext::TestSize.Level1)
{
    TestSecondaryCommandList(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: RenderCommandListTestOpenGL
 * @tc.desc: Tests RenderCommandList by issuing various commands and rendering it in the OpenGL backend to verify
 * that command lists are translated to the correct backend commands. Tests command list execution in paralell and
 * non-paralell.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderCommandList, RenderCommandListTestOpenGL, testing::ext::TestSize.Level1)
{
    TestRenderCommandList(UTest::GetOpenGLBackend(), true);
    TestRenderCommandList(UTest::GetOpenGLBackend(), false);
}
/**
 * @tc.name: RenderCommandListWithSwapchainTestOpenGL
 * @tc.desc: Tests RenderCommandList by issuing various commands and rendering it in the OpenGL backend to verify
 * that command lists are translated to the correct backend commands and swapchain synchronization and present works in
 * conjunction with the RenderCommandList.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderCommandList, RenderCommandListWithSwapchainTestOpenGL, testing::ext::TestSize.Level1)
{
    TestRenderCommandListWithSwapchain(UTest::GetOpenGLBackend());
}
/**
 * @tc.name: RenderCommandListWithMultiSwapchainTestOpenGL
 * @tc.desc: Works similarly to RenderCommandListWithSwapchainTestOpenGL, but tests that the renderer can function with
 * multiple swapchains and windows.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderCommandList, RenderCommandListWithMultiSwapchainTestOpenGL, testing::ext::TestSize.Level1)
{
    TestRenderCommandListWithSwapchain(UTest::GetOpenGLBackend(), 16);
}
/**
 * @tc.name: SecondaryCommandListTestOpenGL
 * @tc.desc: Tests for Secondary Command List Test Open Gl. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderCommandList, SecondaryCommandListTestOpenGL, testing::ext::TestSize.Level1)
{
    TestSecondaryCommandList(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
