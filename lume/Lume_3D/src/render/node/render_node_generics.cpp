#include "render_node_generics.h"

#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>

using namespace RENDER_NS;
using namespace CORE_NS;

void RenderNodeMixin::Load(const BASE_NS::string_view shader, BASE_NS::vector<DescriptorCounts>& counts,
    ComputeShaderData& debugShaderData, size_t reserved)
{
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ = *this;
    auto& utils = renderNodeContextMgr_->GetRenderNodeUtil();

    auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    const IShaderManager::ShaderData sd = shaderMgr.GetShaderDataByShaderName(shader);
    debugShaderData.shader = sd.shader;
    debugShaderData.pipelineLayout = sd.pipelineLayout;
    debugShaderData.pipelineLayoutData = sd.pipelineLayoutData;
    debugShaderData.threadGroupSize = shaderMgr.GetReflectionThreadGroupSize(debugShaderData.shader);

    debugShaderData.pso = renderNodeContextMgr_->GetPsoManager().GetComputePsoHandle(
        debugShaderData.shader, debugShaderData.pipelineLayout, {});

    for (size_t i = 0; i < reserved; ++i)
        counts.push_back(utils.GetDescriptorCounts(debugShaderData.pipelineLayoutData));
}

void RenderNodeMixin::Load(const BASE_NS::string_view shader, BASE_NS::vector<DescriptorCounts>& counts,
    GraphicsShaderData& debugShaderData, Base::array_view<const DynamicStateEnum> dynstates, size_t reserved)
{
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ = *this;
    auto& utils = renderNodeContextMgr_->GetRenderNodeUtil();

    auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();

    const IShaderManager::GraphicsShaderData sd =
        shaderMgr.GetGraphicsShaderDataByShaderHandle(shaderMgr.GetShaderHandle(shader));

    debugShaderData.shader = sd.shader;
    debugShaderData.pipelineLayout = sd.pipelineLayout;
    debugShaderData.graphicsState = sd.graphicsState;
    debugShaderData.pipelineLayoutData = sd.pipelineLayoutData;

    debugShaderData.pso = renderNodeContextMgr_->GetPsoManager().GetGraphicsPsoHandle(
        debugShaderData.shader, debugShaderData.graphicsState, debugShaderData.pipelineLayout, {}, {}, dynstates);

    for (size_t i = 0; i < reserved; ++i) {
        counts.push_back(utils.GetDescriptorCounts(debugShaderData.pipelineLayoutData));
    }
}

RenderPass RenderNodeMixin::CreateRenderPass(const RenderHandle input)
{
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ = *this;
    const GpuImageDesc desc = renderNodeContextMgr_->GetGpuResourceManager().GetImageDescriptor(input);
    RenderPass rp;
    rp.renderPassDesc.attachmentCount = 1u;
    rp.renderPassDesc.attachmentHandles[0u] = input;
    rp.renderPassDesc.attachments[0u].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
    rp.renderPassDesc.attachments[0u].clearValue.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    rp.renderPassDesc.attachments[0u].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
    rp.renderPassDesc.renderArea = { 0, 0, desc.width, desc.height };

    rp.renderPassDesc.subpassCount = 1u;
    rp.subpassDesc.colorAttachmentCount = 1u;
    rp.subpassDesc.colorAttachmentIndices[0u] = 0u;
    rp.subpassStartIndex = 0u;

    return rp;
}

void RenderNodeMixin::RecreateImages(RenderSize size, RENDER_NS::RenderHandleReference& handle)
{
    if (size.w == 0 || size.h == 0) {
        return;
    }

    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ = *this;
    auto& gpuResourceManager = renderNodeContextMgr_->GetGpuResourceManager();
    auto desc = gpuResourceManager.GetImageDescriptor(handle.GetHandle());
    desc.width = uint32_t(size.w);
    desc.height = uint32_t(size.h);
    handle = gpuResourceManager.Create(handle, desc);
    CORE_ASSERT(gpuResourceManager.IsValid(handle.GetHandle()));
}

void RenderNodeMixin::RecreateImages(
    RenderSize size, RENDER_NS::RenderHandleReference& handle, RENDER_NS::GpuImageDesc desc)
{
    if (size.w == 0 || size.h == 0) {
        return;
    }

    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ = *this;
    auto& gpuResourceManager = renderNodeContextMgr_->GetGpuResourceManager();
    auto descCurr = gpuResourceManager.GetImageDescriptor(handle.GetHandle());

    if (int(descCurr.width) != size.w || int(descCurr.height) != size.h) {
        desc.width = uint32_t(size.w);
        desc.height = uint32_t(size.h);
        handle = gpuResourceManager.Create(handle, desc);
        CORE_ASSERT(gpuResourceManager.IsValid(handle.GetHandle()));
    }
}
