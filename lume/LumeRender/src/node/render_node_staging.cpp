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

#include "render_node_staging.h"

#include <cinttypes>

#include <base/containers/vector.h>
#include <base/math/mathf.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>

#include "datastore/render_data_store_default_staging.h"
#include "default_engine_constants.h"
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

        const auto* baseSrcDataBegin = static_cast<const uint8_t*>(ref.stagingData.data());
        const size_t bufferBeginIndex = Math::min(stagingConsumeStruct.bufferCopies.size(), size_t(ref.beginIndex));
        const size_t bufferEndIndex = Math::min(stagingConsumeStruct.bufferCopies.size(), bufferBeginIndex + ref.count);
        for (size_t bufferIdx = bufferBeginIndex; bufferIdx < bufferEndIndex; ++bufferIdx) {
            const BufferCopy& currBufferCopy = stagingConsumeStruct.bufferCopies[bufferIdx];
            const size_t dstOffset = Math::min(bufferDesc.byteSize, currBufferCopy.dstOffset);
            uint8_t* dstData = baseDstDataBegin + dstOffset;
            const uint8_t* srcPtr = nullptr;
            size_t srcSize = 0;
            if (ref.dataType == StagingCopyStruct::DataType::DATA_TYPE_VECTOR) {
                const size_t srcOffset = Math::min(ref.stagingData.size(), size_t(currBufferCopy.srcOffset));
                srcPtr = baseSrcDataBegin + srcOffset;
                srcSize = Math::min(ref.stagingData.size() - srcOffset, size_t(currBufferCopy.size));
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
        uint8_t* dstPtr = static_cast<uint8_t*>(gpuResourceMgr.MapBuffer(ref.dstHandle.GetHandle()));
        if (!dstPtr) {
            PLUGIN_LOG_E("staging: dstHandle %" PRIu64, ref.dstHandle.GetHandle().id);
            return;
        }
        const auto& bufferDesc = gpuResourceMgr.GetBufferDescriptor(ref.dstHandle.GetHandle());

        const size_t dstOffset = Math::min(bufferDesc.byteSize, ref.bufferCopy.dstOffset);
        dstPtr += dstOffset;

        const size_t srcOffset = Math::min(ref.data.size(), size_t(ref.bufferCopy.srcOffset));
        const uint8_t* srcPtr = ref.data.data() + srcOffset;

        if (!CloneData(dstPtr, bufferDesc.byteSize - dstOffset, srcPtr, ref.data.size() - srcOffset)) {
            PLUGIN_LOG_E("Copying of staging data failed");
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
        if (auto* dataStoreDefaultStaging = static_cast<RenderDataStoreDefaultStaging*>(
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
    if (auto* dataStoreDefaultStaging =
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

    RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "RenderStaging", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);

    // memcpy to staging
    const StagingConsumeStruct staging = gpuResourceMgrImpl.ConsumeStagingData();
    vector<RenderStaging::StagingMappedBuffer> mappedBuffers(staging.stagingBuffers.size());

    for (uint32_t i = 0; i < staging.stagingBuffers.size(); i++) {
        RenderHandle buffer = staging.stagingBuffers[i];
        uint32_t size = staging.stagingByteSizes[i];

        if (RenderHandleUtil::IsValid(buffer) && (size)) {
            if (auto* dataPtr = static_cast<uint8_t*>(gpuResourceMgr.MapBufferMemory(buffer)); dataPtr) {
                mappedBuffers[i] = { buffer, dataPtr, size };
            }
        }
    }
    renderStaging.CopyHostToStaging(staging, mappedBuffers);

    for (auto& buffer : mappedBuffers) {
        gpuResourceMgr.UnmapBuffer(buffer.handle);
    }

    renderStaging.CopyHostToStaging(gpuResourceMgr, renderDataStoreStaging);
    // direct memcpy
    CopyHostDirectlyToBuffer(gpuResourceMgr, staging);
    CopyHostDirectlyToBuffer(gpuResourceMgr, renderDataStoreStagingDirectCopy);

    // images
    renderStaging.ClearImages(cmdList, gpuResourceMgr, renderDataStoreImageClear);
    renderStaging.CopyStagingToImages(cmdList, gpuResourceMgr, staging, renderDataStoreStaging);
    renderStaging.CopyImagesToBuffers(cmdList, staging, renderDataStoreStaging);
    renderStaging.CopyImagesToImages(cmdList, staging, renderDataStoreStaging);

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
