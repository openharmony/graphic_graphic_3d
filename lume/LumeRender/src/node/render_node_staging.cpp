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
constexpr uint32_t MIP_MAP_TILING_FEATURES { CORE_FORMAT_FEATURE_BLIT_DST_BIT | CORE_FORMAT_FEATURE_BLIT_SRC_BIT };

void ExplicitBarrierUndefinedImageToTransferDst(IRenderCommandList& cmdList, const RenderHandle handle)
{
    const ImageResourceBarrier src { AccessFlagBits::CORE_ACCESS_TRANSFER_READ_BIT,
        PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT,
        ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED }; // NOTE: undefined, because we do not care the previous data
    const ImageResourceBarrier dst { AccessFlagBits::CORE_ACCESS_TRANSFER_WRITE_BIT,
        PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT, ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };
    const ImageSubresourceRange imageSubresourceRange { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,
        PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
    cmdList.CustomImageBarrier(handle, src, dst, imageSubresourceRange);
    cmdList.AddCustomBarrierPoint();
}

void ExplicitBarrierTransferDstImageToTransferSrc(IRenderCommandList& cmdList, const RenderHandle handle)
{
    const ImageResourceBarrier src { AccessFlagBits::CORE_ACCESS_TRANSFER_WRITE_BIT,
        PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT, ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };
    const ImageResourceBarrier dst { AccessFlagBits::CORE_ACCESS_TRANSFER_READ_BIT,
        PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT, ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };
    const ImageSubresourceRange imageSubresourceRange { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,
        PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
    cmdList.CustomImageBarrier(handle, src, dst, imageSubresourceRange);
    cmdList.AddCustomBarrierPoint();
}

void BlitScalingImage(IRenderCommandList& cmdList, const BufferImageCopy& copy, const RenderHandle srcHandle,
    const RenderHandle dstHandle, const GpuImageDesc& dstImageDesc)
{
    const ImageBlit imageBlit {
        {
            ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT,
            0,
            0,
            1,
        },
        {
            { 0, 0, 0 },
            { copy.imageExtent.width, copy.imageExtent.height, 1 },
        },
        {
            ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT,
            0,
            0,
            1,
        },
        {
            { 0, 0, 0 },
            { dstImageDesc.width, dstImageDesc.height, 1 },
        },
    };
    constexpr Filter filter = Filter::CORE_FILTER_LINEAR;
    cmdList.BlitImage(srcHandle, dstHandle, imageBlit, filter);
}

uint32_t GetMipLevelSize(uint32_t originalSize, uint32_t mipLevel)
{
    const uint32_t mipSize = originalSize >> mipLevel;
    return mipSize >= 1u ? mipSize : 1u;
}

void GenerateMipmaps(IRenderCommandList& cmdList, const GpuImageDesc& imageDesc, const RenderHandle mipImageHandle,
    const uint32_t currentMipLevel)
{
    constexpr Filter filter = Filter::CORE_FILTER_LINEAR;
    const uint32_t mipCount = imageDesc.mipCount;
    const uint32_t layerCount = imageDesc.layerCount;

    const uint32_t iw = imageDesc.width;
    const uint32_t ih = imageDesc.height;
    for (uint32_t mipIdx = currentMipLevel + 1; mipIdx < mipCount; ++mipIdx) {
        const uint32_t dstMipLevel = mipIdx;
        const uint32_t srcMipLevel = dstMipLevel - 1;
        // explicit transition for src mip
        {
            const ImageResourceBarrier src { AccessFlagBits::CORE_ACCESS_TRANSFER_WRITE_BIT,
                PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT,
                ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };
            const ImageResourceBarrier dst { AccessFlagBits::CORE_ACCESS_TRANSFER_READ_BIT,
                PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT,
                ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };
            const ImageSubresourceRange imageSubresourceRange { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT,
                srcMipLevel, 1, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
            cmdList.CustomImageBarrier(mipImageHandle, src, dst, imageSubresourceRange);
            cmdList.AddCustomBarrierPoint();
        }
        {
            const ImageBlit imageBlit {
                {
                    ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT,
                    srcMipLevel,
                    0,
                    layerCount,
                },
                {
                    { 0, 0, 0 },
                    { GetMipLevelSize(iw, srcMipLevel), GetMipLevelSize(ih, srcMipLevel), 1 },
                },
                {
                    ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT,
                    dstMipLevel,
                    0,
                    layerCount,
                },
                {
                    { 0, 0, 0 },
                    { GetMipLevelSize(iw, dstMipLevel), GetMipLevelSize(ih, dstMipLevel), 1 },
                },
            };
            cmdList.BlitImage(mipImageHandle, mipImageHandle, imageBlit, filter);

            // explicit "out" transition for src mip to dst to enable easy all mip transition in the end
            {
                const ImageResourceBarrier src { AccessFlagBits::CORE_ACCESS_TRANSFER_READ_BIT,
                    PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT,
                    ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };
                const ImageResourceBarrier dst { AccessFlagBits::CORE_ACCESS_TRANSFER_WRITE_BIT,
                    PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT,
                    ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };
                const ImageSubresourceRange imageSubresourceRange { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT,
                    srcMipLevel, 1, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
                cmdList.CustomImageBarrier(mipImageHandle, src, dst, imageSubresourceRange);
                cmdList.AddCustomBarrierPoint();
            }
        }
    }
}

void CopyBuffersToImages(const IRenderNodeGpuResourceManager& gpuResourceMgr, IRenderCommandList& cmdList,
    const vector<StagingCopyStruct>& bufferToImage, const vector<BufferImageCopy>& bufferImageCopies,
    const ScalingImageDataStruct& scalingImageData)
{
    for (const auto& ref : bufferToImage) {
        if (ref.invalidOperation) {
            continue;
        }
        const uint32_t beginIndex = ref.beginIndex;
        const uint32_t count = ref.count;
        for (uint32_t idx = 0; idx < count; ++idx) {
            const BufferImageCopy& copyRef = bufferImageCopies[beginIndex + idx];
            // barriers are only done for dynamic resources automatically (e.g. when copying to same image multiple
            // times)
            // NOTE: add desc to data (so we do not need to lock gpu resource manager)
            const GpuImageDesc imageDesc = gpuResourceMgr.GetImageDescriptor(ref.dstHandle.GetHandle());
            RenderHandle bufferToImageDst = ref.dstHandle.GetHandle();
            bool useScaling = false;
            if (ref.format != Format::BASE_FORMAT_UNDEFINED) {
                if (const auto iter = scalingImageData.formatToScalingImages.find(ref.format);
                    iter != scalingImageData.formatToScalingImages.cend()) {
                    PLUGIN_ASSERT(iter->second < scalingImageData.scalingImages.size());
                    bufferToImageDst = scalingImageData.scalingImages[iter->second].handle.GetHandle();
                    ExplicitBarrierUndefinedImageToTransferDst(cmdList, bufferToImageDst);
                    useScaling = true;
                }
            }
            cmdList.CopyBufferToImage(ref.srcHandle.GetHandle(), bufferToImageDst, copyRef);
            if (useScaling) {
                ExplicitBarrierTransferDstImageToTransferSrc(cmdList, bufferToImageDst);
                BlitScalingImage(cmdList, copyRef, bufferToImageDst, ref.dstHandle.GetHandle(), imageDesc);
            }

            // generate mips
            if (imageDesc.engineCreationFlags & EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS) {
                const FormatProperties formatProperties = gpuResourceMgr.GetFormatProperties(imageDesc.format);
                if ((formatProperties.optimalTilingFeatures & MIP_MAP_TILING_FEATURES) == MIP_MAP_TILING_FEATURES) {
                    const uint32_t mipCount = imageDesc.mipCount;
                    const uint32_t availableMipCount = ref.count;
                    const uint32_t currentMipLevel = copyRef.imageSubresource.mipLevel;

                    const bool isLastAvailableMipLevel = (currentMipLevel + 1) == availableMipCount;
                    const bool isMissingMipLevels = availableMipCount < mipCount;

                    if (isMissingMipLevels && isLastAvailableMipLevel) {
                        GenerateMipmaps(
                            cmdList, imageDesc, ref.dstHandle.GetHandle(), copyRef.imageSubresource.mipLevel);
                    }
                }
#if (RENDER_VALIDATION_ENABLED == 1)
                if ((formatProperties.optimalTilingFeatures & MIP_MAP_TILING_FEATURES) != MIP_MAP_TILING_FEATURES) {
                    PLUGIN_LOG_ONCE_W("core_validation_auto_mipmapping",
                        "RENDER_VALIDATION: requested automatic mip mapping not done for format: %u", imageDesc.format);
                }
#endif
            }
        }
    }
}

void CopyImagesToBuffersImpl(const IRenderNodeGpuResourceManager& gpuResourceMgr, IRenderCommandList& cmdList,
    const vector<StagingCopyStruct>& imageToBuffer, const vector<BufferImageCopy>& bufferImageCopies)
{
    for (const auto& ref : imageToBuffer) {
        if (ref.invalidOperation) {
            continue;
        }
        const uint32_t beginIndex = ref.beginIndex;
        const uint32_t count = ref.count;
        for (uint32_t idx = 0; idx < count; ++idx) {
            const BufferImageCopy& copyRef = bufferImageCopies[beginIndex + idx];
            cmdList.CopyImageToBuffer(ref.srcHandle.GetHandle(), ref.dstHandle.GetHandle(), copyRef);
        }
    }
}

void CopyImagesToImagesImpl(const IRenderNodeGpuResourceManager& gpuResourceMgr, IRenderCommandList& cmdList,
    const vector<StagingCopyStruct>& imageToImage, const vector<ImageCopy>& imageCopies)
{
    for (const auto& ref : imageToImage) {
        if (ref.invalidOperation) {
            continue;
        }
        const uint32_t beginIndex = ref.beginIndex;
        const uint32_t count = ref.count;
        for (uint32_t idx = 0; idx < count; ++idx) {
            const ImageCopy& copyRef = imageCopies[beginIndex + idx];
            cmdList.CopyImageToImage(ref.srcHandle.GetHandle(), ref.dstHandle.GetHandle(), copyRef);
        }
    }
}

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

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    dataStoreNameStaging_ = renderNodeGraphData.renderNodeGraphName + "RenderDataStoreDefaultStaging";
}

void RenderNodeStaging::PreExecuteFrame()
{
    // re-create needed gpu resources
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
    if (RenderDataStoreDefaultStaging* dataStoreDefaultStaging =
            static_cast<RenderDataStoreDefaultStaging*>(renderDataStoreMgr.GetRenderDataStore(dataStoreNameStaging_));
        dataStoreDefaultStaging) {
        hasData = hasData || dataStoreDefaultStaging->HasStagingData();
        renderDataStoreStaging = dataStoreDefaultStaging->ConsumeStagingData();
        renderDataStoreStagingDirectCopy = dataStoreDefaultStaging->ConsumeStagingDirectDataCopy();
    }

    // early out, no data
    if (!hasData) {
        return;
    }

    // memcpy to staging
    const StagingConsumeStruct staging = gpuResourceMgrImpl.ConsumeStagingData();
    CopyHostToStaging(gpuResourceMgr, staging);
    CopyHostToStaging(gpuResourceMgr, renderDataStoreStaging);
    // direct memcpy
    CopyHostDirectlyToBuffer(gpuResourceMgr, staging);
    CopyHostDirectlyToBuffer(gpuResourceMgr, renderDataStoreStagingDirectCopy);

    // images
    CopyStagingToImages(cmdList, gpuResourceMgr, staging, renderDataStoreStaging);
    CopyImagesToBuffers(cmdList, gpuResourceMgr, staging, renderDataStoreStaging);
    CopyImagesToImages(cmdList, gpuResourceMgr, staging, renderDataStoreStaging);

    // buffers
    CopyStagingToBuffers(cmdList, staging, renderDataStoreStaging);
}

void RenderNodeStaging::CopyHostToStaging(
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const StagingConsumeStruct& stagingData)
{
    auto const copyUserDataToStagingBuffer = [](auto& gpuResourceMgr, auto const& ref) {
        uint8_t* data = static_cast<uint8_t*>(gpuResourceMgr.MapBuffer(ref.srcHandle.GetHandle()));
        if (!data) {
            PLUGIN_LOG_E("staging: srcHandle %" PRIu64 " dstHandle %" PRIu64, ref.srcHandle.GetHandle().id,
                ref.dstHandle.GetHandle().id);
            return;
        }
        auto const& bufferDesc = gpuResourceMgr.GetBufferDescriptor(ref.srcHandle.GetHandle());
        const void* srcPtr = nullptr;
        size_t srcSize = 0;
        // should be removed already
        PLUGIN_ASSERT(ref.dataType != StagingCopyStruct::DataType::DATA_TYPE_DIRECT_SRC_COPY);
        if (ref.dataType == StagingCopyStruct::DataType::DATA_TYPE_VECTOR) {
            srcPtr = ref.stagingData.data();
            srcSize = ref.stagingData.size();
        } else if (ref.dataType == StagingCopyStruct::DataType::DATA_TYPE_IMAGE_CONTAINER) {
            PLUGIN_ASSERT(ref.imageContainerPtr);
            if (ref.imageContainerPtr) {
                srcPtr = ref.imageContainerPtr->GetData().data();
                srcSize = ref.imageContainerPtr->GetData().size_bytes();
            }
        }
        if ((srcPtr) && (srcSize > 0)) {
            PLUGIN_ASSERT(bufferDesc.byteSize >= srcSize);
            if (!CloneData(data, bufferDesc.byteSize, srcPtr, srcSize)) {
                PLUGIN_LOG_E("Copying of staging data failed");
            }
        }
        gpuResourceMgr.UnmapBuffer(ref.srcHandle.GetHandle());
    };

    for (const auto& ref : stagingData.bufferToImage) {
        if ((!ref.invalidOperation) && (ref.dataType != StagingCopyStruct::DataType::DATA_TYPE_DIRECT_SRC_COPY)) {
            copyUserDataToStagingBuffer(gpuResourceMgr, ref);
        }
    }
    for (const auto& ref : stagingData.bufferToBuffer) {
        if ((!ref.invalidOperation) && (ref.dataType != StagingCopyStruct::DataType::DATA_TYPE_DIRECT_SRC_COPY)) {
            copyUserDataToStagingBuffer(gpuResourceMgr, ref);
        }
    }
}

void RenderNodeStaging::CopyStagingToImages(IRenderCommandList& cmdList,
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const StagingConsumeStruct& stagingData,
    const StagingConsumeStruct& renderDataStoreStagingData)
{
    // explicit input barriers
    cmdList.BeginDisableAutomaticBarrierPoints();
    {
        ImageResourceBarrier src { 0, PipelineStageFlagBits::CORE_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
        const ImageResourceBarrier dst { AccessFlagBits::CORE_ACCESS_TRANSFER_WRITE_BIT,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT,
            ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };
        const ImageSubresourceRange imageSubresourceRange { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0,
            PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };

        for (const auto& ref : stagingData.bufferToImage) {
            if ((!ref.invalidOperation) && (!RenderHandleUtil::IsDynamicResource(ref.dstHandle.GetHandle()))) {
                cmdList.CustomImageBarrier(ref.dstHandle.GetHandle(), src, dst, imageSubresourceRange);
            }
        }
        for (const auto& ref : renderDataStoreStagingData.bufferToImage) {
            if ((!ref.invalidOperation) && (!RenderHandleUtil::IsDynamicResource(ref.dstHandle.GetHandle()))) {
                cmdList.CustomImageBarrier(ref.dstHandle.GetHandle(), src, dst, imageSubresourceRange);
            }
        }
    }
    cmdList.EndDisableAutomaticBarrierPoints();
    cmdList.AddCustomBarrierPoint();

    CopyBuffersToImages(gpuResourceMgr, cmdList, stagingData.bufferToImage, stagingData.bufferImageCopies,
        stagingData.scalingImageData);
    CopyBuffersToImages(gpuResourceMgr, cmdList, renderDataStoreStagingData.bufferToImage,
        renderDataStoreStagingData.bufferImageCopies, {}); // scaling from render data store not supported ATM

    // explicit output barriers
    cmdList.BeginDisableAutomaticBarrierPoints();
    {
        const ImageResourceBarrier src {
            AccessFlagBits::CORE_ACCESS_TRANSFER_WRITE_BIT, PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT,
            ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL // we expect all mips to be transferred to dst
        };
        const ImageResourceBarrier dst { AccessFlagBits::CORE_ACCESS_SHADER_READ_BIT,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_VERTEX_SHADER_BIT, // some shader stage
            ImageLayout::CORE_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        const ImageSubresourceRange imageSubresourceRange { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0,
            PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };

        for (const auto& ref : stagingData.bufferToImage) {
            if ((!ref.invalidOperation) && (!RenderHandleUtil::IsDynamicResource(ref.dstHandle.GetHandle()))) {
                cmdList.CustomImageBarrier(ref.dstHandle.GetHandle(), src, dst, imageSubresourceRange);
            }
        }
        for (const auto& ref : renderDataStoreStagingData.bufferToImage) {
            if ((!ref.invalidOperation) && (!RenderHandleUtil::IsDynamicResource(ref.dstHandle.GetHandle()))) {
                cmdList.CustomImageBarrier(ref.dstHandle.GetHandle(), src, dst, imageSubresourceRange);
            }
        }
    }
    cmdList.EndDisableAutomaticBarrierPoints();
    cmdList.AddCustomBarrierPoint();
}

void RenderNodeStaging::CopyImagesToBuffers(IRenderCommandList& cmdList,
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const StagingConsumeStruct& stagingData,
    const StagingConsumeStruct& renderDataStoreStagingData)
{
    // explicit input barriers
    cmdList.BeginDisableAutomaticBarrierPoints();
    {
        // we transfer all mip levels, but we only copy the one
        ImageResourceBarrier src { 0, PipelineStageFlagBits::CORE_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
        const ImageResourceBarrier dst { AccessFlagBits::CORE_ACCESS_TRANSFER_READ_BIT,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT,
            ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };
        const ImageSubresourceRange imageSubresourceRange { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0,
            PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };

        for (const auto& ref : renderDataStoreStagingData.imageToBuffer) {
            if ((!ref.invalidOperation) && (!RenderHandleUtil::IsDynamicResource(ref.srcHandle.GetHandle()))) {
                cmdList.CustomImageBarrier(ref.srcHandle.GetHandle(), src, dst, imageSubresourceRange);
            }
        }
    }
    cmdList.EndDisableAutomaticBarrierPoints();
    cmdList.AddCustomBarrierPoint();

    // Not supported through gpu resource manager staging
    PLUGIN_ASSERT(stagingData.imageToBuffer.empty());
    CopyImagesToBuffersImpl(gpuResourceMgr, cmdList, renderDataStoreStagingData.imageToBuffer,
        renderDataStoreStagingData.bufferImageCopies);

    // explicit output barriers
    cmdList.BeginDisableAutomaticBarrierPoints();
    {
        const ImageResourceBarrier src {
            AccessFlagBits::CORE_ACCESS_TRANSFER_READ_BIT, PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT,
            ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL // we expect all mips to be transferred to dst
        };
        // NOTE: should fetch usage flags
        const ImageResourceBarrier dst { AccessFlagBits::CORE_ACCESS_SHADER_READ_BIT,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_VERTEX_SHADER_BIT, // some shader stage
            ImageLayout::CORE_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        const ImageSubresourceRange imageSubresourceRange { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0,
            PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };

        for (const auto& ref : renderDataStoreStagingData.imageToBuffer) {
            if ((!ref.invalidOperation) && (!RenderHandleUtil::IsDynamicResource(ref.srcHandle.GetHandle()))) {
                cmdList.CustomImageBarrier(ref.srcHandle.GetHandle(), src, dst, imageSubresourceRange);
            }
        }
    }
    cmdList.EndDisableAutomaticBarrierPoints();
    cmdList.AddCustomBarrierPoint();
}

void RenderNodeStaging::CopyImagesToImages(IRenderCommandList& cmdList,
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const StagingConsumeStruct& stagingData,
    const StagingConsumeStruct& renderDataStoreStagingData)
{
    // explicit input barriers
    cmdList.BeginDisableAutomaticBarrierPoints();
    {
        // we transfer all mip levels, but we only copy the one
        constexpr ImageSubresourceRange imageSubresourceRange { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0,
            PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };

        constexpr ImageResourceBarrier src { 0, PipelineStageFlagBits::CORE_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
        constexpr ImageResourceBarrier srcDst { AccessFlagBits::CORE_ACCESS_TRANSFER_READ_BIT,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT,
            ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };

        constexpr ImageResourceBarrier dstDst { AccessFlagBits::CORE_ACCESS_TRANSFER_WRITE_BIT,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT,
            ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };

        for (const auto& ref : renderDataStoreStagingData.imageToImage) {
            if ((!ref.invalidOperation) && (!RenderHandleUtil::IsDynamicResource(ref.srcHandle.GetHandle()))) {
                cmdList.CustomImageBarrier(ref.srcHandle.GetHandle(), src, srcDst, imageSubresourceRange);
            }
            if ((!ref.invalidOperation) && (!RenderHandleUtil::IsDynamicResource(ref.dstHandle.GetHandle()))) {
                cmdList.CustomImageBarrier(ref.dstHandle.GetHandle(), src, dstDst, imageSubresourceRange);
            }
        }
    }
    cmdList.EndDisableAutomaticBarrierPoints();
    cmdList.AddCustomBarrierPoint();

    // Not supported through gpu resource manager staging
    PLUGIN_ASSERT(stagingData.imageToImage.empty());
    CopyImagesToImagesImpl(
        gpuResourceMgr, cmdList, renderDataStoreStagingData.imageToImage, renderDataStoreStagingData.imageCopies);

    // explicit output barriers
    cmdList.BeginDisableAutomaticBarrierPoints();
    {
        constexpr ImageResourceBarrier srcSrc { AccessFlagBits::CORE_ACCESS_TRANSFER_READ_BIT,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT,
            ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };
        constexpr ImageResourceBarrier dstSrc { AccessFlagBits::CORE_ACCESS_TRANSFER_WRITE_BIT,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT,
            ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };
        // NOTE: should fetch usage flags
        constexpr ImageResourceBarrier dst { AccessFlagBits::CORE_ACCESS_SHADER_READ_BIT,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_VERTEX_SHADER_BIT, // some shader stage
            ImageLayout::CORE_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        constexpr ImageSubresourceRange imageSubresourceRange { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0,
            PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };

        for (const auto& ref : renderDataStoreStagingData.imageToImage) {
            if ((!ref.invalidOperation) && (!RenderHandleUtil::IsDynamicResource(ref.srcHandle.GetHandle()))) {
                cmdList.CustomImageBarrier(ref.srcHandle.GetHandle(), srcSrc, dst, imageSubresourceRange);
            }
            if ((!ref.invalidOperation) && (!RenderHandleUtil::IsDynamicResource(ref.dstHandle.GetHandle()))) {
                cmdList.CustomImageBarrier(ref.dstHandle.GetHandle(), dstSrc, dst, imageSubresourceRange);
            }
        }
    }
    cmdList.EndDisableAutomaticBarrierPoints();
    cmdList.AddCustomBarrierPoint();
}

void RenderNodeStaging::CopyStagingToBuffers(IRenderCommandList& cmdList, const StagingConsumeStruct& stagingData,
    const StagingConsumeStruct& renderDataStoreStagingData)
{
    const auto copyBuffersToBuffers = [](IRenderCommandList& cmdList, const vector<StagingCopyStruct>& bufferToBuffer,
                                          const vector<BufferCopy>& bufferCopies) {
        for (const auto& ref : bufferToBuffer) {
            if (ref.invalidOperation) {
                continue;
            }
            const auto copies = array_view(bufferCopies.data() + ref.beginIndex, ref.count);
            for (const BufferCopy& copyRef : copies) {
                cmdList.CopyBufferToBuffer(ref.srcHandle.GetHandle(), ref.dstHandle.GetHandle(), copyRef);
            }
        }
    };

    // dynamic resources can create barriers if needed
    copyBuffersToBuffers(cmdList, stagingData.bufferToBuffer, stagingData.bufferCopies);
    copyBuffersToBuffers(cmdList, renderDataStoreStagingData.bufferToBuffer, renderDataStoreStagingData.bufferCopies);

    // explict output barriers
    cmdList.BeginDisableAutomaticBarrierPoints();
    {
        const BufferResourceBarrier src { AccessFlagBits::CORE_ACCESS_TRANSFER_WRITE_BIT,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT };
        const BufferResourceBarrier dst { 0, // access flags are not currently known
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT };

        for (const auto& ref : stagingData.bufferToBuffer) {
            if (!ref.invalidOperation) {
                cmdList.CustomBufferBarrier(
                    ref.dstHandle.GetHandle(), src, dst, 0, PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE);
            }
        }
        for (const auto& ref : renderDataStoreStagingData.bufferToBuffer) {
            if (!ref.invalidOperation) {
                cmdList.CustomBufferBarrier(
                    ref.dstHandle.GetHandle(), src, dst, 0, PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE);
            }
        }
    }
    cmdList.EndDisableAutomaticBarrierPoints();
    cmdList.AddCustomBarrierPoint();
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
