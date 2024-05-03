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

#include "render_staging.h"

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
} // namespace

void RenderStaging::Init(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
}

void RenderStaging::PreExecuteFrame(const uint32_t clearByteSize)
{
    if (renderNodeContextMgr_->GetRenderNodeGraphData().renderingConfiguration.renderBackend ==
        DeviceBackendType::OPENGLES) {
        if (clearByteSize == 0) {
            additionalCopyBuffer_ = {};
        } else {
            additionalCopyBuffer_.byteOffset = 0;
            additionalCopyBuffer_.byteSize = clearByteSize;
            const GpuBufferDesc desc {
                BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_SRC_BIT, // usageFlags
                MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                    MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // memoryPropertyFlags
                0,                                                                 // engineCreationFlags
                additionalCopyBuffer_.byteSize,                                    // byteSize
                BASE_NS::Format::BASE_FORMAT_UNDEFINED,                            // format
            };

            additionalCopyBuffer_.handle =
                renderNodeContextMgr_->GetGpuResourceManager().Create(additionalCopyBuffer_.handle, desc);
        }
    }
}

void RenderStaging::CopyHostToStaging(
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

void RenderStaging::CopyStagingToImages(IRenderCommandList& cmdList,
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

void RenderStaging::CopyImagesToBuffers(IRenderCommandList& cmdList,
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

void RenderStaging::CopyImagesToImages(IRenderCommandList& cmdList, const IRenderNodeGpuResourceManager& gpuResourceMgr,
    const StagingConsumeStruct& stagingData, const StagingConsumeStruct& renderDataStoreStagingData)
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

void RenderStaging::CopyBuffersToBuffers(IRenderCommandList& cmdList, const StagingConsumeStruct& stagingData,
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

void RenderStaging::ClearImages(IRenderCommandList& cmdList, const IRenderNodeGpuResourceManager& gpuResourceMgr,
    const StagingImageClearConsumeStruct& imageClearData)
{
    // explicit input barriers for resources that are not dynamic trackable
    // NOTE: one probably only needs to clear dynamic trackable resources anyhow
    cmdList.BeginDisableAutomaticBarrierPoints();
    {
        // we transfer all mip levels
        constexpr ImageSubresourceRange imageSubresourceRange { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0,
            PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };

        constexpr ImageResourceBarrier src { 0, PipelineStageFlagBits::CORE_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED };
        constexpr ImageResourceBarrier dst { AccessFlagBits::CORE_ACCESS_TRANSFER_READ_BIT,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_TRANSFER_BIT,
            ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };

        for (const auto& ref : imageClearData.clears) {
            if (!RenderHandleUtil::IsDynamicResource(ref.handle.GetHandle())) {
                cmdList.CustomImageBarrier(ref.handle.GetHandle(), src, dst, imageSubresourceRange);
            }
        }
    }
    cmdList.EndDisableAutomaticBarrierPoints();
    cmdList.AddCustomBarrierPoint();

    // alternative path for GLES, ClearColorImage
    if (renderNodeContextMgr_->GetRenderNodeGraphData().renderingConfiguration.renderBackend ==
        DeviceBackendType::OPENGLES) {
        if (additionalCopyBuffer_.handle && (!imageClearData.clears.empty())) {
            if (auto dataPtr =
                    reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(additionalCopyBuffer_.handle.GetHandle()));
                dataPtr) {
                for (const auto& ref : imageClearData.clears) {
                    const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(ref.handle.GetHandle());
                    const uint32_t bytesPerPixel =
                        gpuResourceMgr.GetFormatProperties(ref.handle.GetHandle()).bytesPerPixel;
                    const uint32_t imgByteSize = desc.width * desc.height * bytesPerPixel;
                    ImageSubresourceLayers imageSubresourceLayers { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0,
                        0, 1u };
                    const BufferImageCopy bic {
                        additionalCopyBuffer_.byteOffset, // bufferOffset
                        0,                                // bufferRowLength
                        0,                                // bufferImageHeight
                        imageSubresourceLayers,           // imageSubresource
                        { 0, 0, 0 },                      // imageOffset
                        { desc.width, desc.height, 1u },  // imageExtent
                    };
#if (RENDER_VALIDATION_ENABLED == 1)
                    const bool hasDiffColorVals =
                        ((ref.color.uint32[0] != ref.color.uint32[1]) || (ref.color.uint32[0] != ref.color.uint32[2]) ||
                            (ref.color.uint32[0] != ref.color.uint32[3]));
                    if ((bytesPerPixel > 4u) || hasDiffColorVals || (desc.depth > 1u)) {
                        PLUGIN_LOG_ONCE_W("RenderStaging::ClearImages_gles",
                            "RENDER_VALIDATION: only byte clears support with OpenGLES ClearColorImage");
                    }
#endif
                    if ((additionalCopyBuffer_.byteOffset + imgByteSize) <= additionalCopyBuffer_.byteSize) {
                        ClearToValue(
                            dataPtr, additionalCopyBuffer_.byteSize, uint8_t(0xff & ref.color.uint32[0]), imgByteSize);
                        cmdList.CopyBufferToImage(
                            additionalCopyBuffer_.handle.GetHandle(), ref.handle.GetHandle(), bic);
                    }

                    // advance
                    additionalCopyBuffer_.byteOffset += imgByteSize;
                    dataPtr = dataPtr + additionalCopyBuffer_.byteOffset;
                }
            }
            gpuResourceMgr.UnmapBuffer(additionalCopyBuffer_.handle.GetHandle());
        }
    } else {
        constexpr ImageSubresourceRange imgSubresourceRange { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0,
            PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
        for (const auto& ref : imageClearData.clears) {
            cmdList.ClearColorImage(ref.handle.GetHandle(), ref.color, { &imgSubresourceRange, 1 });
        }
        // NOTE: there's no way to guess the desired layout for non dynamic trackable clear resources
    }
}
RENDER_END_NAMESPACE()
