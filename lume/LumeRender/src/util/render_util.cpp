/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "util/render_util.h"

#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>

#include "device/gpu_resource_manager.h"

RENDER_BEGIN_NAMESPACE()
using namespace BASE_NS;

RenderUtil::RenderUtil(const IRenderContext& renderContext) : renderContext_(renderContext)
{
    renderFrameUtil_ = make_unique<RenderFrameUtil>(renderContext);
}

void RenderUtil::BeginFrame()
{
    renderFrameUtil_->BeginFrame();
}

void RenderUtil::EndFrame()
{
    renderFrameUtil_->EndFrame();
}

RenderHandleDesc RenderUtil::GetRenderHandleDesc(const RenderHandleReference& handle) const
{
    RenderHandleDesc desc;
    if (handle) {
        const RenderHandleType handleType = handle.GetHandleType();
        desc.type = handleType;
        desc.refCount = handle.GetRefCount();
        if ((handleType >= RenderHandleType::GPU_BUFFER) && (handleType <= RenderHandleType::GPU_SAMPLER)) {
            const IGpuResourceManager& mgr = renderContext_.GetDevice().GetGpuResourceManager();
            desc.name = mgr.GetName(handle);
            desc.refCount -= 1; // one count in mgr
        } else if ((handleType >= RenderHandleType::SHADER_STATE_OBJECT) &&
                   (handleType <= RenderHandleType::GRAPHICS_STATE)) {
            const IShaderManager& mgr = renderContext_.GetDevice().GetShaderManager();
            IShaderManager::IdDesc info = mgr.GetIdDesc(handle);
            desc.name = info.path;
            desc.additionalName = info.variant;
        } else if (handleType == RenderHandleType::RENDER_NODE_GRAPH) {
            const IRenderNodeGraphManager& mgr = renderContext_.GetRenderNodeGraphManager();
            const RenderNodeGraphDescInfo info = mgr.GetInfo(handle);
            desc.name = info.renderNodeGraphUri;
            desc.additionalName = info.renderNodeGraphName;
            desc.refCount -= 1; // one count in mgr
        }
    }
    return desc;
}

RenderHandleReference RenderUtil::GetRenderHandle(const RenderHandleDesc& desc) const
{
    RenderHandleReference rhr;
    if (desc.type != RenderHandleType::UNDEFINED) {
        if ((desc.type >= RenderHandleType::GPU_BUFFER) && (desc.type <= RenderHandleType::GPU_SAMPLER)) {
            const IGpuResourceManager& mgr = renderContext_.GetDevice().GetGpuResourceManager();
            if (desc.type == RenderHandleType::GPU_BUFFER) {
                rhr = mgr.GetBufferHandle(desc.name);
            } else if (desc.type == RenderHandleType::GPU_IMAGE) {
                rhr = mgr.GetImageHandle(desc.name);
            } else if (desc.type == RenderHandleType::GPU_SAMPLER) {
                rhr = mgr.GetSamplerHandle(desc.name);
            }
        } else if ((desc.type >= RenderHandleType::SHADER_STATE_OBJECT) &&
                   (desc.type <= RenderHandleType::GRAPHICS_STATE)) {
            const IShaderManager& mgr = renderContext_.GetDevice().GetShaderManager();
            if ((desc.type == RenderHandleType::SHADER_STATE_OBJECT) ||
                (desc.type == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT)) {
                rhr = mgr.GetShaderHandle(desc.name, desc.additionalName);
            } else if (desc.type == RenderHandleType::GRAPHICS_STATE) {
                rhr = mgr.GetGraphicsStateHandle(desc.name, desc.additionalName);
            } else if (desc.type == RenderHandleType::PIPELINE_LAYOUT) {
                rhr = mgr.GetPipelineLayoutHandle(desc.name);
            } else if (desc.type == RenderHandleType::VERTEX_INPUT_DECLARATION) {
                rhr = mgr.GetVertexInputDeclarationHandle(desc.name);
            }
        } else if (desc.type == RenderHandleType::RENDER_NODE_GRAPH) {
        }
    }
    return rhr;
}

void RenderUtil::SetRenderTimings(const RenderTimings::Times& times)
{
    renderTimings_.prevFrame = renderTimings_.frame;
    renderTimings_.frame = times;
}

RenderTimings RenderUtil::GetRenderTimings() const
{
    return renderTimings_;
}

IRenderFrameUtil& RenderUtil::GetRenderFrameUtil() const
{
    return *renderFrameUtil_;
}

RENDER_END_NAMESPACE()
