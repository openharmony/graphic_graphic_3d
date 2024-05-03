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

#include "render_node_end_frame_staging.h"

#include <algorithm>
#include <cinttypes>
#include <cstring>

#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>

#include "datastore/render_data_store_default_staging.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
void RenderNodeEndFrameStaging::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    dataStoreNameStaging_ = renderNodeGraphData.renderNodeGraphName + "RenderDataStoreDefaultStaging";
}

void RenderNodeEndFrameStaging::PreExecuteFrame()
{
    // re-create needed gpu resources
}

void RenderNodeEndFrameStaging::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    auto& gpuResourceMgrImpl =
        static_cast<RenderNodeGpuResourceManager&>(renderNodeContextMgr_->GetGpuResourceManager());
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    bool hasData = gpuResourceMgrImpl.HasStagingData();

    StagingConsumeStruct renderDataStoreStaging;
    if (RenderDataStoreDefaultStaging* dataStoreDefaultStaging =
            static_cast<RenderDataStoreDefaultStaging*>(renderDataStoreMgr.GetRenderDataStore(dataStoreNameStaging_));
        dataStoreDefaultStaging) {
        hasData = hasData || dataStoreDefaultStaging->HasEndStagingData();
        renderDataStoreStaging = dataStoreDefaultStaging->ConsumeEndStagingData();
    }

    // early out, no data
    if (!hasData) {
        return;
    }

    const StagingConsumeStruct staging = {};
    // images
    renderStaging.CopyStagingToImages(cmdList, gpuResourceMgr, staging, renderDataStoreStaging);
    renderStaging.CopyImagesToBuffers(cmdList, gpuResourceMgr, staging, renderDataStoreStaging);
    renderStaging.CopyImagesToImages(cmdList, gpuResourceMgr, staging, renderDataStoreStaging);
    // buffers
    renderStaging.CopyBuffersToBuffers(cmdList, staging, renderDataStoreStaging);
}

// for plugin / factory interface
IRenderNode* RenderNodeEndFrameStaging::Create()
{
    return new RenderNodeEndFrameStaging();
}

void RenderNodeEndFrameStaging::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeEndFrameStaging*>(instance);
}
RENDER_END_NAMESPACE()
