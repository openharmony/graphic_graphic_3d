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

#include "render_node_create_gpu_buffers.h"

#include <render/device/intf_gpu_resource_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/render_data_structures.h>

#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
bool CheckForDescUpdates(const GpuBufferDesc& dependencyDesc, GpuBufferDesc& desc)
{
    bool needsUpdate = (dependencyDesc.byteSize != desc.byteSize);
    needsUpdate = needsUpdate || (dependencyDesc.usageFlags != desc.usageFlags);
    needsUpdate = needsUpdate || (dependencyDesc.memoryPropertyFlags != desc.memoryPropertyFlags);
    needsUpdate = needsUpdate || (dependencyDesc.engineCreationFlags != desc.engineCreationFlags);
    needsUpdate = needsUpdate || (dependencyDesc.format != desc.format);
    if (needsUpdate) {
        desc = dependencyDesc;
    }
    return needsUpdate;
}
} // namespace

void RenderNodeCreateGpuBuffers::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    if (jsonInputs_.gpuBufferDescs.empty()) {
        PLUGIN_LOG_W("RenderNodeCreateGpuBuffers: No gpu buffer descs given");
    }

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    descs_.reserve(jsonInputs_.gpuBufferDescs.size());
    for (const auto& ref : jsonInputs_.gpuBufferDescs) {
        GpuBufferDesc desc = ref.desc;

        RenderHandle dependencyHandle;
        if (!ref.dependencyBufferName.empty()) {
            dependencyHandle = gpuResourceMgr.GetBufferHandle(ref.dependencyBufferName);
            if (RenderHandleUtil::IsValid(dependencyHandle)) {
                const GpuBufferDesc dependencyDesc = gpuResourceMgr.GetBufferDescriptor(dependencyHandle);

                // update desc
                CheckForDescUpdates(dependencyDesc, desc);
            } else {
                PLUGIN_LOG_E("GpuBuffer dependency name not found: %s", ref.dependencyBufferName.c_str());
            }
        }
        dependencyHandles_.push_back(dependencyHandle);

        names_.push_back({ string(ref.name), string(ref.shareName) });
        descs_.push_back(desc);
        resourceHandles_.push_back(gpuResourceMgr.Create(ref.name, desc));
    }

    // broadcast the resources
    for (size_t idx = 0; idx < resourceHandles_.size(); ++idx) {
        IRenderNodeGraphShareManager& rngShareMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        rngShareMgr.RegisterRenderNodeOutput(names_[idx].shareName, resourceHandles_[idx].GetHandle());
    }
}

void RenderNodeCreateGpuBuffers::PreExecuteFrame()
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    for (size_t idx = 0; idx < resourceHandles_.size(); ++idx) {
        const RenderHandle dependencyHandle = dependencyHandles_[idx];
        if (gpuResourceMgr.IsValid(dependencyHandle)) {
            const GpuBufferDesc& dependencyDesc = gpuResourceMgr.GetBufferDescriptor(dependencyHandle);
            GpuBufferDesc& descRef = descs_[idx];

            const bool recreateBuffer = CheckForDescUpdates(dependencyDesc, descRef);
            if (recreateBuffer) {
                // replace the handle
                resourceHandles_[idx] = gpuResourceMgr.Create(resourceHandles_[idx], descRef);
            }
        }
    }

    // broadcast the resources
    for (size_t idx = 0; idx < resourceHandles_.size(); ++idx) {
        IRenderNodeGraphShareManager& rngShareMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        rngShareMgr.RegisterRenderNodeOutput(names_[idx].shareName, resourceHandles_[idx].GetHandle());
    }
}

void RenderNodeCreateGpuBuffers::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.gpuBufferDescs = parserUtil.GetGpuBufferDescs(jsonVal, "gpuBufferDescs");
}

// for plugin / factory interface
IRenderNode* RenderNodeCreateGpuBuffers::Create()
{
    return new RenderNodeCreateGpuBuffers();
}

void RenderNodeCreateGpuBuffers::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeCreateGpuBuffers*>(instance);
}
RENDER_END_NAMESPACE()
