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

#include "render_node_staging.h"

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
namespace {
void CopyHostDirectlyToBuffer(
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const StagingConsumeStruct& stagingConsumeStruct)
{
    for (const auto& ref : stagingConsumeStruct.cpuToBuffer) {
        if (ref.invalidOperation) {
            continue;
        }
        uint8_t* baseDstDataBegin = static_cast<uint8_t*>(gpuResourceMgr.MapBuffer(ref.dstHandle.GetHandle()));
        if (!baseDstDataBegin) {
            PLUGIN_LOG_E("staging: direct dstHandle %" PRIu64, ref.dstHandle.GetHandle().id);
            return;
        }
        auto const& bufferDesc = gpuResourceMgr.GetBufferDescriptor(ref.dstHandle.GetHandle());
        const uint8_t* baseDstDataEnd = baseDstDataBegin + bufferDesc.byteSize;

        const uint8_t* baseSrcDataBegin = static_cast<const uint8_t*>(ref.stagingData.data());
        const uint32_t bufferBeginIndex = ref.beginIndex;
        const uint32_t bufferEndIndex = bufferBeginIndex + ref.count;
        for (uint32_t bufferIdx = bufferBeginIndex; bufferIdx < bufferEndIndex; ++bufferIdx) {
            PLUGIN_ASSERT(bufferIdx < stagingConsumeStruct.bufferCopies.size());
            const BufferCopy& currBufferCopy = stagingConsumeStruct.bufferCopies[bufferIdx];
            uint8_t* dstData = baseDstDataBegin + currBufferCopy.dstOffset;
            const uint8_t* srcPtr = nullptr;
            size_t srcSize = 0;
            if (ref.dataType == StagingCopyStruct::DataType::DATA_TYPE_VECTOR) {
                srcPtr = baseSrcDataBegin + currBufferCopy.srcOffset;
                srcSize = currBufferCopy.size;
            }
            if ((srcPtr) && (srcSize > 0)) {
                PLUGIN_ASSERT(bufferDesc.byteSize >= srcSize);
                if (!CloneData(dstData, size_t(baseDstDataEnd - dstData), srcPtr, srcSize)) {
                    PLUGIN_LOG_E("Copying of CPU data directly failed");
                }
            }
        }
        gpuResourceMgr.UnmapBuffer(ref.dstHandle.GetHandle());
    }
}

void CopyHostDirectlyToBuffer(
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const StagingDirectDataCopyConsumeStruct& stagingData)
{
    for (const auto& ref : stagingData.dataCopies) {
        uint8_t* data = static_cast<uint8_t*>(gpuResourceMgr.MapBuffer(ref.dstHandle.GetHandle()));
        if (!data) {
            PLUGIN_LOG_E("staging: dstHandle %" PRIu64, ref.dstHandle.GetHandle().id);
            return;
        }
        const size_t dstOffset = ref.bufferCopy.dstOffset;
        data += dstOffset;
        auto const& bufferDesc = gpuResourceMgr.GetBufferDescriptor(ref.dstHandle.GetHandle());
        const uint8_t* srcPtr = ref.data.data() + ref.bufferCopy.srcOffset;
        const size_t copySize =
            std::min(ref.data.size() - size_t(ref.bufferCopy.srcOffset), size_t(ref.bufferCopy.size));
        if ((srcPtr) && (copySize > 0)) {
            PLUGIN_ASSERT((size_t(bufferDesc.byteSize) - dstOffset) >= copySize);
            if (!CloneData(data, bufferDesc.byteSize - dstOffset, srcPtr, copySize)) {
                PLUGIN_LOG_E("Copying of staging data failed");
            }
        }
        gpuResourceMgr.UnmapBuffer(ref.dstHandle.GetHandle());
    }
}
} // namespace

void RenderNodeStaging::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    renderStaging.Init(*renderNodeContextMgr_);

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    dataStoreNameStaging_ = renderNodeGraphData.renderNodeGraphName + "RenderDataStoreDefaultStaging";
}

void RenderNodeStaging::PreExecuteFrame()
{
    // re-create needed gpu resources
    if (renderNodeContextMgr_->GetRenderNodeGraphData().renderingConfiguration.renderBackend ==
        DeviceBackendType::OPENGLES) {
        const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        if (RenderDataStoreDefaultStaging* dataStoreDefaultStaging = static_cast<RenderDataStoreDefaultStaging*>(
                renderDataStoreMgr.GetRenderDataStore(dataStoreNameStaging_));
            dataStoreDefaultStaging) {
            renderStaging.PreExecuteFrame(dataStoreDefaultStaging->GetImageClearByteSize());
        }
    }
}

void RenderNodeStaging::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    auto& gpuResourceMgrImpl =
        static_cast<RenderNodeGpuResourceManager&>(renderNodeContextMgr_->GetGpuResourceManager());
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    bool hasData = gpuResourceMgrImpl.HasStagingData();

    StagingConsumeStruct renderDataStoreStaging;
    StagingDirectDataCopyConsumeStruct renderDataStoreStagingDirectCopy;
    StagingImageClearConsumeStruct renderDataStoreImageClear;
    if (RenderDataStoreDefaultStaging* dataStoreDefaultStaging =
            static_cast<RenderDataStoreDefaultStaging*>(renderDataStoreMgr.GetRenderDataStore(dataStoreNameStaging_));
        dataStoreDefaultStaging) {
        hasData = hasData || dataStoreDefaultStaging->HasBeginStagingData();
        renderDataStoreStaging = dataStoreDefaultStaging->ConsumeBeginStagingData();
        renderDataStoreStagingDirectCopy = dataStoreDefaultStaging->ConsumeBeginStagingDirectDataCopy();
        renderDataStoreImageClear = dataStoreDefaultStaging->ConsumeBeginStagingImageClears();
    }

    // early out, no data
    if (!hasData) {
        return;
    }

    // memcpy to staging
    const StagingConsumeStruct staging = gpuResourceMgrImpl.ConsumeStagingData();
    renderStaging.CopyHostToStaging(gpuResourceMgr, staging);
    renderStaging.CopyHostToStaging(gpuResourceMgr, renderDataStoreStaging);
    // direct memcpy
    CopyHostDirectlyToBuffer(gpuResourceMgr, staging);
    CopyHostDirectlyToBuffer(gpuResourceMgr, renderDataStoreStagingDirectCopy);

    // images
    renderStaging.ClearImages(cmdList, gpuResourceMgr, renderDataStoreImageClear);
    renderStaging.CopyStagingToImages(cmdList, gpuResourceMgr, staging, renderDataStoreStaging);
    renderStaging.CopyImagesToBuffers(cmdList, gpuResourceMgr, staging, renderDataStoreStaging);
    renderStaging.CopyImagesToImages(cmdList, gpuResourceMgr, staging, renderDataStoreStaging);

    // buffers
    renderStaging.CopyBuffersToBuffers(cmdList, staging, renderDataStoreStaging);
}

// for plugin / factory interface
IRenderNode* RenderNodeStaging::Create()
{
    return new RenderNodeStaging();
}

void RenderNodeStaging::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeStaging*>(instance);
}
RENDER_END_NAMESPACE()
