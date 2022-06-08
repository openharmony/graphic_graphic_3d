/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

    frameStagingConsumeStruct_ = std::move(stagingConsumeStruct_);
    frameStagingDirectCopyConsumeStruct_ = std::move(stagingDirectCopyConsumeStruct_);

    frameStagingConsumeGpuBuffers_ = std::move(stagingGpuBuffers_);
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

        stagingGpuBuffers_.emplace_back(stagingBufferHandle);

        const uint32_t beginIndex = static_cast<uint32_t>(stagingConsumeStruct_.bufferCopies.size());
        stagingConsumeStruct_.bufferCopies.emplace_back(BufferCopy { 0, 0, static_cast<uint32_t>(dat.size_bytes()) });

        vector<uint8_t> copiedData(dat.cbegin().ptr(), dat.cend().ptr());
        stagingConsumeStruct_.bufferToBuffer.emplace_back(
            StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_VECTOR, stagingBufferHandle, dstHandle,
                beginIndex, 1, move(copiedData), nullptr });
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
            stagingDirectCopyConsumeStruct_.dataCopies.emplace_back(
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

        stagingGpuBuffers_.emplace_back(stagingBufferHandle);

        const uint32_t beginIndex = static_cast<uint32_t>(stagingConsumeStruct_.bufferImageCopies.size());
        stagingConsumeStruct_.bufferImageCopies.emplace_back(bufferImageCopy);

        vector<uint8_t> copiedData(dat.cbegin().ptr(), dat.cend().ptr());
        stagingConsumeStruct_.bufferToImage.emplace_back(
            StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_VECTOR, stagingBufferHandle, dstHandle,
                beginIndex, 1, move(copiedData), nullptr });
    }
}

void RenderDataStoreDefaultStaging::CopyBufferToBuffer(
    const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle, const BufferCopy& bufferCopy)
{
    if (gpuResourceMgr_.IsGpuBuffer(srcHandle) && gpuResourceMgr_.IsGpuBuffer(dstHandle)) {
        std::lock_guard<std::mutex> lock(mutex_);

        const uint32_t beginIndex = static_cast<uint32_t>(stagingConsumeStruct_.bufferCopies.size());
        stagingConsumeStruct_.bufferCopies.emplace_back(bufferCopy);

        stagingConsumeStruct_.bufferToBuffer.emplace_back(
            StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_DIRECT_SRC_COPY, srcHandle, dstHandle,
                beginIndex, 1u, {}, nullptr });
    }
}

void RenderDataStoreDefaultStaging::CopyBufferToImage(const RenderHandleReference& srcHandle,
    const RenderHandleReference& dstHandle, const BufferImageCopy& bufferImageCopy)
{
    if (gpuResourceMgr_.IsGpuBuffer(srcHandle) && gpuResourceMgr_.IsGpuImage(dstHandle)) {
        std::lock_guard<std::mutex> lock(mutex_);

        const uint32_t beginIndex = static_cast<uint32_t>(stagingConsumeStruct_.bufferImageCopies.size());
        stagingConsumeStruct_.bufferImageCopies.emplace_back(bufferImageCopy);

        stagingConsumeStruct_.bufferToImage.emplace_back(
            StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_DIRECT_SRC_COPY, srcHandle, dstHandle,
                beginIndex, 1u, {}, nullptr });
    }
}

void RenderDataStoreDefaultStaging::CopyBufferToImage(const RenderHandleReference& srcHandle,
    const RenderHandleReference& dstHandle, BASE_NS::array_view<const BufferImageCopy> bufferImageCopies)
{
    if (gpuResourceMgr_.IsGpuBuffer(srcHandle) && gpuResourceMgr_.IsGpuImage(dstHandle)) {
        std::lock_guard<std::mutex> lock(mutex_);

        const uint32_t beginIndex = static_cast<uint32_t>(stagingConsumeStruct_.bufferImageCopies.size());
        stagingConsumeStruct_.bufferImageCopies.insert(
            stagingConsumeStruct_.bufferImageCopies.end(), bufferImageCopies.begin(), bufferImageCopies.end());

        stagingConsumeStruct_.bufferToImage.emplace_back(
            StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_DIRECT_SRC_COPY, srcHandle, dstHandle,
                beginIndex, static_cast<uint32_t>(bufferImageCopies.size()), {}, nullptr });
    }
}

void RenderDataStoreDefaultStaging::CopyImageToBuffer(const RenderHandleReference& srcHandle,
    const RenderHandleReference& dstHandle, const BufferImageCopy& bufferImageCopy)
{
    if (gpuResourceMgr_.IsGpuImage(srcHandle) && gpuResourceMgr_.IsGpuBuffer(dstHandle)) {
        std::lock_guard<std::mutex> lock(mutex_);

        const uint32_t beginIndex = static_cast<uint32_t>(stagingConsumeStruct_.bufferImageCopies.size());
        stagingConsumeStruct_.bufferImageCopies.emplace_back(bufferImageCopy);

        stagingConsumeStruct_.imageToBuffer.emplace_back(
            StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_SRC_TO_DST_COPY, srcHandle, dstHandle,
                beginIndex, 1u, {}, nullptr });
    }
}

void RenderDataStoreDefaultStaging::CopyImageToImage(
    const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle, const ImageCopy& imageCopy)
{
    if (gpuResourceMgr_.IsGpuImage(srcHandle) && gpuResourceMgr_.IsGpuImage(dstHandle)) {
        std::lock_guard<std::mutex> lock(mutex_);

        const uint32_t beginIndex = static_cast<uint32_t>(stagingConsumeStruct_.imageCopies.size());
        stagingConsumeStruct_.imageCopies.emplace_back(imageCopy);

        stagingConsumeStruct_.imageToImage.emplace_back(
            StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_SRC_TO_DST_COPY, srcHandle, dstHandle,
                beginIndex, 1u, {}, nullptr });
    }
}

bool RenderDataStoreDefaultStaging::HasStagingData() const
{
    if (frameStagingConsumeStruct_.bufferToBuffer.empty() && frameStagingConsumeStruct_.bufferToImage.empty() &&
        frameStagingConsumeStruct_.imageToBuffer.empty() && frameStagingConsumeStruct_.imageToImage.empty() &&
        frameStagingConsumeStruct_.cpuToBuffer.empty() && frameStagingConsumeStruct_.bufferCopies.empty() &&
        frameStagingConsumeStruct_.bufferImageCopies.empty() && frameStagingConsumeStruct_.imageCopies.empty() &&
        frameStagingDirectCopyConsumeStruct_.dataCopies.empty()) {
        return false;
    } else {
        return true;
    }
}

StagingConsumeStruct RenderDataStoreDefaultStaging::ConsumeStagingData()
{
    StagingConsumeStruct scs = move(frameStagingConsumeStruct_);
    return scs;
}

StagingDirectDataCopyConsumeStruct RenderDataStoreDefaultStaging::ConsumeStagingDirectDataCopy()
{
    StagingDirectDataCopyConsumeStruct scs = move(frameStagingDirectCopyConsumeStruct_);
    return scs;
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
