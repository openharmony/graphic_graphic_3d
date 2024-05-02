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

#include "render_data_store_default_staging.h"

#include <cstdint>

#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "device/gpu_resource_manager.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
RenderDataStoreDefaultStaging::RenderDataStoreDefaultStaging(IRenderContext& renderContext, const string_view name)
    : gpuResourceMgr_(renderContext.GetDevice().GetGpuResourceManager()), name_(name)
{}

RenderDataStoreDefaultStaging::~RenderDataStoreDefaultStaging() {}

void RenderDataStoreDefaultStaging::PreRender()
{
    // get stuff ready for render nodes

    std::lock_guard<std::mutex> lock(mutex_);

    frameStagingConsumeData_ = move(stagingConsumeData_);
    frameStagingConsumeGpuBuffers_ = move(stagingGpuBuffers_);
}

void RenderDataStoreDefaultStaging::PostRender()
{
    std::lock_guard<std::mutex> lock(mutex_);

    // destroy already consumed staging buffers
    frameStagingConsumeGpuBuffers_.clear();
}

void RenderDataStoreDefaultStaging::Clear()
{
    // The data cannot be automatically cleared here
}

void RenderDataStoreDefaultStaging::CopyImageToBuffer(const RenderHandleReference& srcHandle,
    const RenderHandleReference& dstHandle, const BufferImageCopy& bufferImageCopy, const ResourceCopyInfo copyInfo)
{
    if (gpuResourceMgr_.IsGpuImage(srcHandle) && gpuResourceMgr_.IsGpuBuffer(dstHandle)) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto& staging =
            (copyInfo == ResourceCopyInfo::BEGIN_FRAME) ? stagingConsumeData_.beginFrame : stagingConsumeData_.endFrame;
        const uint32_t beginIndex = static_cast<uint32_t>(staging.bufferImageCopies.size());
        staging.bufferImageCopies.push_back(bufferImageCopy);

        staging.imageToBuffer.push_back(StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_SRC_TO_DST_COPY,
            srcHandle, dstHandle, beginIndex, 1u, {}, nullptr });
    }
}

void RenderDataStoreDefaultStaging::CopyImageToImage(const RenderHandleReference& srcHandle,
    const RenderHandleReference& dstHandle, const ImageCopy& imageCopy, const ResourceCopyInfo copyInfo)
{
    if (gpuResourceMgr_.IsGpuImage(srcHandle) && gpuResourceMgr_.IsGpuImage(dstHandle)) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto& staging =
            (copyInfo == ResourceCopyInfo::BEGIN_FRAME) ? stagingConsumeData_.beginFrame : stagingConsumeData_.endFrame;
        const uint32_t beginIndex = static_cast<uint32_t>(staging.imageCopies.size());
        staging.imageCopies.push_back(imageCopy);

        staging.imageToImage.push_back(StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_SRC_TO_DST_COPY,
            srcHandle, dstHandle, beginIndex, 1u, {}, nullptr });
    }
}

void RenderDataStoreDefaultStaging::CopyBufferToBuffer(const RenderHandleReference& srcHandle,
    const RenderHandleReference& dstHandle, const BufferCopy& bufferCopy, const ResourceCopyInfo copyInfo)
{
    if (gpuResourceMgr_.IsGpuBuffer(srcHandle) && gpuResourceMgr_.IsGpuBuffer(dstHandle)) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto& staging =
            (copyInfo == ResourceCopyInfo::BEGIN_FRAME) ? stagingConsumeData_.beginFrame : stagingConsumeData_.endFrame;
        const uint32_t beginIndex = static_cast<uint32_t>(staging.bufferCopies.size());
        staging.bufferCopies.push_back(bufferCopy);

        staging.bufferToBuffer.push_back(StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_DIRECT_SRC_COPY,
            srcHandle, dstHandle, beginIndex, 1u, {}, nullptr });
    }
}

void RenderDataStoreDefaultStaging::CopyBufferToImage(const RenderHandleReference& srcHandle,
    const RenderHandleReference& dstHandle, const BufferImageCopy& bufferImageCopy, const ResourceCopyInfo copyInfo)
{
    if (gpuResourceMgr_.IsGpuBuffer(srcHandle) && gpuResourceMgr_.IsGpuImage(dstHandle)) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto& staging =
            (copyInfo == ResourceCopyInfo::BEGIN_FRAME) ? stagingConsumeData_.beginFrame : stagingConsumeData_.endFrame;
        const uint32_t beginIndex = static_cast<uint32_t>(staging.bufferImageCopies.size());
        staging.bufferImageCopies.push_back(bufferImageCopy);

        staging.bufferToImage.push_back(StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_DIRECT_SRC_COPY,
            srcHandle, dstHandle, beginIndex, 1u, {}, nullptr });
    }
}

void RenderDataStoreDefaultStaging::ClearColorImage(const RenderHandleReference& handle, const ClearColorValue color)
{
    if (gpuResourceMgr_.IsGpuImage(handle) && (!RenderHandleUtil::IsDepthImage(handle.GetHandle()))) {
        std::lock_guard<std::mutex> lock(mutex_);

        stagingConsumeData_.beginFrameClear.clears.push_back({ handle, color });
    }
}

void RenderDataStoreDefaultStaging::CopyDataToBuffer(
    const array_view<const uint8_t>& dat, const RenderHandleReference& dstHandle, const BufferCopy& bufferCopy)
{
    if ((dat.size_bytes() > 0) && gpuResourceMgr_.IsGpuBuffer(dstHandle)) {
        // NOTE: validation
        // create staging buffer
        const GpuBufferDesc stagingBufferDesc =
            GpuResourceManager::GetStagingBufferDesc(static_cast<uint32_t>(dat.size_bytes()));
        const RenderHandleReference stagingBufferHandle = gpuResourceMgr_.Create(stagingBufferDesc);

        std::lock_guard<std::mutex> lock(mutex_);

        stagingGpuBuffers_.push_back(stagingBufferHandle);

        auto& staging = stagingConsumeData_.beginFrame;
        const uint32_t beginIndex = static_cast<uint32_t>(staging.bufferCopies.size());
        staging.bufferCopies.push_back(BufferCopy { 0, 0, static_cast<uint32_t>(dat.size_bytes()) });

        vector<uint8_t> copiedData(dat.cbegin().ptr(), dat.cend().ptr());
        staging.bufferToBuffer.push_back(StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_VECTOR,
            stagingBufferHandle, dstHandle, beginIndex, 1, move(copiedData), nullptr });
    }
}

void RenderDataStoreDefaultStaging::CopyDataToBufferOnCpu(
    const array_view<const uint8_t>& dat, const RenderHandleReference& dstHandle, const BufferCopy& bufferCopy)
{
    if ((dat.size_bytes() > 0) && gpuResourceMgr_.IsGpuBuffer(dstHandle)) {
        const GpuBufferDesc bufDesc = gpuResourceMgr_.GetBufferDescriptor(dstHandle);
        if ((bufDesc.memoryPropertyFlags & CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            (bufDesc.memoryPropertyFlags & CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            PLUGIN_ASSERT(bufferCopy.size <= static_cast<uint32_t>(dat.size()));

            std::lock_guard<std::mutex> lock(mutex_);

            vector<uint8_t> copiedData(dat.cbegin().ptr(), dat.cend().ptr());
            stagingConsumeData_.beginFrameDirect.dataCopies.push_back(
                DirectDataCopyOnCpu { dstHandle, bufferCopy, move(copiedData) });
        } else {
            PLUGIN_LOG_E("CopyDataToBufferOnCpu invalid buffer given (needs host_visibile and host_coherent).");
        }
    }
}

void RenderDataStoreDefaultStaging::CopyDataToImage(const array_view<const uint8_t>& dat,
    const RenderHandleReference& dstHandle, const BufferImageCopy& bufferImageCopy)
{
    if ((dat.size_bytes() > 0) && gpuResourceMgr_.IsGpuImage(dstHandle)) {
        // NOTE: validation
        // create staging buffer
        const GpuBufferDesc stagingBufferDesc =
            GpuResourceManager::GetStagingBufferDesc(static_cast<uint32_t>(dat.size_bytes()));
        const RenderHandleReference& stagingBufferHandle = gpuResourceMgr_.Create(stagingBufferDesc);

        std::lock_guard<std::mutex> lock(mutex_);

        stagingGpuBuffers_.push_back(stagingBufferHandle);

        auto& staging = stagingConsumeData_.beginFrame;
        const uint32_t beginIndex = static_cast<uint32_t>(staging.bufferImageCopies.size());
        staging.bufferImageCopies.push_back(bufferImageCopy);

        vector<uint8_t> copiedData(dat.cbegin().ptr(), dat.cend().ptr());
        staging.bufferToImage.push_back(StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_VECTOR,
            stagingBufferHandle, dstHandle, beginIndex, 1, move(copiedData), nullptr });
    }
}

void RenderDataStoreDefaultStaging::CopyBufferToImage(const RenderHandleReference& srcHandle,
    const RenderHandleReference& dstHandle, BASE_NS::array_view<const BufferImageCopy> bufferImageCopies)
{
    if (gpuResourceMgr_.IsGpuBuffer(srcHandle) && gpuResourceMgr_.IsGpuImage(dstHandle)) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto& staging = stagingConsumeData_.beginFrame;
        const uint32_t beginIndex = static_cast<uint32_t>(staging.bufferImageCopies.size());
        staging.bufferImageCopies.insert(
            staging.bufferImageCopies.end(), bufferImageCopies.begin(), bufferImageCopies.end());

        staging.bufferToImage.push_back(StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_DIRECT_SRC_COPY,
            srcHandle, dstHandle, beginIndex, static_cast<uint32_t>(bufferImageCopies.size()), {}, nullptr });
    }
}

void RenderDataStoreDefaultStaging::CopyBufferToBuffer(
    const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle, const BufferCopy& bufferCopy)
{
    CopyBufferToBuffer(srcHandle, dstHandle, bufferCopy, ResourceCopyInfo::BEGIN_FRAME);
}

void RenderDataStoreDefaultStaging::CopyBufferToImage(const RenderHandleReference& srcHandle,
    const RenderHandleReference& dstHandle, const BufferImageCopy& bufferImageCopy)
{
    CopyBufferToImage(srcHandle, dstHandle, bufferImageCopy, ResourceCopyInfo::BEGIN_FRAME);
}

void RenderDataStoreDefaultStaging::CopyImageToBuffer(const RenderHandleReference& srcHandle,
    const RenderHandleReference& dstHandle, const BufferImageCopy& bufferImageCopy)
{
    CopyImageToBuffer(srcHandle, dstHandle, bufferImageCopy, ResourceCopyInfo::BEGIN_FRAME);
}

void RenderDataStoreDefaultStaging::CopyImageToImage(
    const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle, const ImageCopy& imageCopy)
{
    CopyImageToImage(srcHandle, dstHandle, imageCopy, ResourceCopyInfo::BEGIN_FRAME);
}

bool RenderDataStoreDefaultStaging::HasBeginStagingData() const
{
    const auto& staging = frameStagingConsumeData_;
    const auto& begStaging = staging.beginFrame;
    const bool noBeginStaging = begStaging.bufferToBuffer.empty() && begStaging.bufferToImage.empty() &&
                                begStaging.imageToBuffer.empty() && begStaging.imageToImage.empty() &&
                                begStaging.cpuToBuffer.empty() && begStaging.bufferCopies.empty() &&
                                begStaging.bufferImageCopies.empty() && begStaging.imageCopies.empty();
    const bool noBeginDirectCopy = staging.beginFrameDirect.dataCopies.empty();
    const bool noBeginClear = staging.beginFrameClear.clears.empty();
    if (noBeginStaging && noBeginDirectCopy && noBeginClear) {
        return false;
    } else {
        return true;
    }
}

bool RenderDataStoreDefaultStaging::HasEndStagingData() const
{
    const auto& endStaging = frameStagingConsumeData_.endFrame;
    const bool noEndStaging = endStaging.bufferToBuffer.empty() && endStaging.bufferToImage.empty() &&
                              endStaging.imageToBuffer.empty() && endStaging.imageToImage.empty() &&
                              endStaging.cpuToBuffer.empty() && endStaging.bufferCopies.empty() &&
                              endStaging.bufferImageCopies.empty() && endStaging.imageCopies.empty();
    if (noEndStaging) {
        return false;
    } else {
        return true;
    }
}

StagingConsumeStruct RenderDataStoreDefaultStaging::ConsumeBeginStagingData()
{
    StagingConsumeStruct scs = move(frameStagingConsumeData_.beginFrame);
    return scs;
}

StagingDirectDataCopyConsumeStruct RenderDataStoreDefaultStaging::ConsumeBeginStagingDirectDataCopy()
{
    StagingDirectDataCopyConsumeStruct scs = move(frameStagingConsumeData_.beginFrameDirect);
    return scs;
}

StagingImageClearConsumeStruct RenderDataStoreDefaultStaging::ConsumeBeginStagingImageClears()
{
    StagingImageClearConsumeStruct scs = move(frameStagingConsumeData_.beginFrameClear);
    return scs;
}

StagingConsumeStruct RenderDataStoreDefaultStaging::ConsumeEndStagingData()
{
    StagingConsumeStruct scs = move(frameStagingConsumeData_.endFrame);
    return scs;
}

uint32_t RenderDataStoreDefaultStaging::GetImageClearByteSize() const
{
    uint32_t byteSize = 0;
    for (const auto& ref : frameStagingConsumeData_.beginFrameClear.clears) {
        const GpuImageDesc desc = gpuResourceMgr_.GetImageDescriptor(ref.handle);
        const uint32_t pixelBytes = gpuResourceMgr_.GetFormatProperties(ref.handle).bytesPerPixel;
        byteSize += (pixelBytes * desc.width * desc.height); // no 3D supported
    }
    return byteSize;
}

// for plugin / factory interface
IRenderDataStore* RenderDataStoreDefaultStaging::Create(IRenderContext& renderContext, char const* name)
{
    return new RenderDataStoreDefaultStaging(renderContext, name);
}

void RenderDataStoreDefaultStaging::Destroy(IRenderDataStore* instance)
{
    delete static_cast<RenderDataStoreDefaultStaging*>(instance);
}
RENDER_END_NAMESPACE()
