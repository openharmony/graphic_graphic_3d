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

#include "gpu_resource_manager.h"

#include <algorithm>
#include <cinttypes>
#if !defined(NDEBUG) || (defined(PLUGIN_LOG_DEBUG) && (PLUGIN_LOG_DEBUG == 1))
#include <sstream>
#include <thread>
#endif

#include <base/containers/fixed_string.h>
#include <base/math/mathf.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_buffer.h"
#include "device/gpu_image.h"
#include "device/gpu_resource_cache.h"
#include "device/gpu_resource_desc_flag_validation.h"
#include "device/gpu_resource_manager_base.h"
#include "device/gpu_sampler.h"
#include "resource_handle_impl.h"
#include "util/log.h"

#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
#include "device/gpu_resource_util.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
static constexpr uint32_t INVALID_PENDING_INDEX { ~0u };
static constexpr uint32_t MAX_IMAGE_EXTENT { 32768u }; // should be fetched from the device

static constexpr MemoryPropertyFlags NEEDED_DEVICE_MEMORY_PROPERTY_FLAGS_FOR_STAGING_MEM_OPT {
    CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT
};
static constexpr MemoryPropertyFlags ADD_STAGING_MEM_OPT_FLAGS { CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                 CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT };

// make sure that generation is valid
static constexpr uint64_t INVALIDATE_GENERATION_SHIFT { 32 };
EngineResourceHandle InvalidateWithGeneration(const EngineResourceHandle handle)
{
    return { handle.id | ~RenderHandleUtil::RES_HANDLE_GENERATION_MASK };
}

EngineResourceHandle UnpackNewHandle(
    const EngineResourceHandle& handle, const RenderHandleType type, const uint32_t arrayIndex)
{
    // increments generation counter
    if (RenderHandleUtil::IsValid(handle)) {
        const uint32_t gpuGenIndex = RenderHandleUtil::GetGenerationIndexPart(handle) + 1;
        return RenderHandleUtil::CreateEngineResourceHandle(type, arrayIndex, gpuGenIndex);
    } else {
        const uint32_t gpuGenIndex = uint32_t(handle.id >> INVALIDATE_GENERATION_SHIFT) + 1;
        return RenderHandleUtil::CreateEngineResourceHandle(type, arrayIndex, gpuGenIndex);
    }
}

// we need to know if image is a depth format when binding to descriptor set as read only
constexpr RenderHandleInfoFlags GetAdditionalImageFlagsFromFormat(const Format format)
{
    RenderHandleInfoFlags flags {};

    const bool isDepthFormat =
        ((format == Format::BASE_FORMAT_D16_UNORM) || (format == Format::BASE_FORMAT_X8_D24_UNORM_PACK32) ||
            (format == Format::BASE_FORMAT_D32_SFLOAT) || (format == Format::BASE_FORMAT_D24_UNORM_S8_UINT))
            ? true
            : false;
    if (isDepthFormat) {
        flags |= CORE_RESOURCE_HANDLE_DEPTH_IMAGE;
    }

    return flags;
}

#if (RENDER_VALIDATION_ENABLED == 1)
void ValidateGpuBufferDesc(const GpuBufferDesc& desc)
{
    if (desc.usageFlags == 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: BufferUsageFlags must not be 0");
    }
    if ((desc.usageFlags & (~GpuResourceDescFlagValidation::ALL_GPU_BUFFER_USAGE_FLAGS)) != 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: Invalid BufferUsageFlags (%u)", desc.usageFlags);
    }
    if (desc.memoryPropertyFlags == 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: MemoryPropertyFlags must not be 0");
    }
    if ((desc.memoryPropertyFlags & (~GpuResourceDescFlagValidation::ALL_MEMORY_PROPERTY_FLAGS)) != 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: Invalid MemoryPropertyFlags (%u)", desc.memoryPropertyFlags);
    }
    if ((desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER) &&
        ((desc.memoryPropertyFlags & MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)) {
        PLUGIN_LOG_E(
            "RENDER_VALIDATION: Invalid MemoryPropertyFlags for CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER (%u)",
            desc.memoryPropertyFlags);
    }
    if (desc.byteSize == 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: Buffer byteSize must larger than zero");
    }
}

void ValidateGpuImageDesc(const GpuImageDesc& desc, const string_view name)
{
    bool valid = true;
    if (desc.format == Format::BASE_FORMAT_UNDEFINED) {
        PLUGIN_LOG_E("RENDER_VALIDATION: Undefined image format");
        valid = false;
    }
    if (desc.imageType > ImageType::CORE_IMAGE_TYPE_3D) {
        PLUGIN_LOG_E("RENDER_VALIDATION: Unsupported image type");
        valid = false;
    }
    if ((desc.imageViewType == ImageViewType::CORE_IMAGE_VIEW_TYPE_2D) && (desc.layerCount > 1u)) {
        PLUGIN_LOG_E(
            "RENDER_VALIDATION: Default image view is done for sampling / shader resource access and needs to be "
            "CORE_IMAGE_VIEW_TYPE_2D_ARRAY with multiple layers");
        valid = false;
    }
    if (desc.imageTiling > ImageTiling::CORE_IMAGE_TILING_LINEAR) {
        PLUGIN_LOG_E("RENDER_VALIDATION: Unsupported image tiling mode (%u)", static_cast<uint32_t>(desc.imageTiling));
        valid = false;
    }
    if (desc.usageFlags == 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: ImageUsageFlags must not be 0");
        valid = false;
    }
    if ((desc.usageFlags & (~GpuResourceDescFlagValidation::ALL_GPU_IMAGE_USAGE_FLAGS)) != 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: Invalid ImageUsageFlags (%u)", desc.usageFlags);
        valid = false;
    }
    if (desc.memoryPropertyFlags == 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: MemoryPropertyFlags must not be 0");
        valid = false;
    }
    if ((desc.memoryPropertyFlags & (~GpuResourceDescFlagValidation::ALL_MEMORY_PROPERTY_FLAGS)) != 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: Invalid MemoryPropertyFlags (%u)", desc.memoryPropertyFlags);
        valid = false;
    }
    if (desc.width == 0 || desc.height == 0 || desc.depth == 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: Image extents must not be zero (x:%u, y:%u, z:%u)", desc.width, desc.height,
            desc.depth);
        valid = false;
    }
    if (desc.width > MAX_IMAGE_EXTENT || desc.height > MAX_IMAGE_EXTENT || desc.depth > MAX_IMAGE_EXTENT) {
        PLUGIN_LOG_E("RENDER_VALIDATION: Image extents must not be bigger than (%u) (x:%u, y:%u, z:%u)",
            MAX_IMAGE_EXTENT, desc.width, desc.height, desc.depth);
        valid = false;
    }
    if (desc.mipCount == 0 || desc.layerCount == 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: Image mip and layer count must be >=1 (mipCount:%u, layerCount:%u)",
            desc.mipCount, desc.layerCount);
        valid = false;
    }
    if ((desc.createFlags & (~GpuResourceDescFlagValidation::ALL_IMAGE_CREATE_FLAGS)) != 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: Invalid ImageCreateFlags (%u)", desc.createFlags);
        valid = false;
    }
    if ((desc.engineCreationFlags & CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS) &&
        ((desc.usageFlags & CORE_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0)) {
        PLUGIN_LOG_E(
            "RENDER_VALIDATION: Must use usage flags CORE_IMAGE_USAGE_TRANSFER_SRC_BIT when generating mip maps");
        valid = false;
    }
    if (desc.usageFlags & CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
        const ImageUsageFlags usageFlags =
            desc.usageFlags &
            ~(CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        if (usageFlags != 0) {
            PLUGIN_LOG_E(
                "RENDER_VALIDATION: If image usage flags contain CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, only "
                "DEPTH_STENCIL_ATTACHMENT_BIT, INPUT_ATTACHMENT_BIT, and COLOR_ATTACHMENT_BIT can be set.");
            valid = false;
        }
    }
    if ((desc.layerCount > 1u) && (desc.imageViewType <= CORE_IMAGE_VIEW_TYPE_3D)) {
        PLUGIN_LOG_E("RENDER_VALIDATION: If image layer count (%u) is larger than 1, then image view type must be "
                     "CORE_IMAGE_VIEW_TYPE_XX_ARRAY",
            desc.layerCount);
        valid = false;
    }

    if ((!valid) && (!name.empty())) {
        PLUGIN_LOG_E("RENDER_VALIDATION: validation issue(s) with image (name: %s)", name.data());
    }
}

void ValidateGpuAccelerationStructureDesc(const GpuAccelerationStructureDesc& desc)
{
    ValidateGpuBufferDesc(desc.bufferDesc);
    if ((desc.bufferDesc.usageFlags & CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT) == 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: usageFlags must include CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT "
                     "for acceleration structures");
    }
}

void ValidateGpuImageCopy(const GpuImageDesc& desc, const BufferImageCopy& copy, const string_view name)
{
    const uint32_t mip = copy.imageSubresource.mipLevel;
    const Size3D imageSize { desc.width >> mip, desc.height >> mip, desc.depth };
    if ((copy.imageOffset.width >= imageSize.width) || (copy.imageOffset.width >= imageSize.height) ||
        (copy.imageOffset.depth >= imageSize.depth)) {
        PLUGIN_LOG_E(
            "RENDER_VALIDATION: BufferImageCopy offset exceeds GpuImageDesc. Mipsize: %u, %u, %u, offset: %u %u %u. "
            "(name: %s)",
            imageSize.width, imageSize.height, imageSize.depth, copy.imageOffset.width, copy.imageOffset.height,
            copy.imageOffset.depth, name.data());
    }
}
#endif

GpuBufferDesc GetValidGpuBufferDesc(const GpuBufferDesc& desc)
{
    return GpuBufferDesc {
        desc.usageFlags & GpuResourceDescFlagValidation::ALL_GPU_BUFFER_USAGE_FLAGS,
        desc.memoryPropertyFlags & GpuResourceDescFlagValidation::ALL_MEMORY_PROPERTY_FLAGS,
        desc.engineCreationFlags,
        desc.byteSize,
    };
}

void CheckAndEnableMemoryOptimizations(const uint32_t gpuResourceMgrFlags, GpuBufferDesc& desc)
{
    if (gpuResourceMgrFlags & GpuResourceManager::GPU_RESOURCE_MANAGER_OPTIMIZE_STAGING_MEMORY) {
        if ((desc.memoryPropertyFlags == CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
            (desc.usageFlags & CORE_BUFFER_USAGE_TRANSFER_DST_BIT) &&
            (desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_ENABLE_MEMORY_OPTIMIZATIONS)) {
            desc.memoryPropertyFlags |= ADD_STAGING_MEM_OPT_FLAGS;
        }
    }
}

bool GetScalingImageNeed(const GpuImageDesc& desc, const array_view<const IImageContainer::SubImageDesc>& copies)
{
    bool scale = false;
    if (desc.engineCreationFlags & EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_SCALE) {
        // we only support single image (single buffer image copy) scaling
        if (copies.size() == 1) {
            scale = (copies[0].width != desc.width) || (copies[0].height != desc.height);
        }
    }
    return scale;
}

bool GetScalingImageNeed(const GpuImageDesc& desc, const array_view<const BufferImageCopy>& copies)
{
    bool scale = false;
    if (desc.engineCreationFlags & EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_SCALE) {
        // we only support single image (single buffer image copy) scaling
        if (copies.size() == 1) {
            scale = (copies[0].imageExtent.width != desc.width) || (copies[0].imageExtent.height != desc.height);
        }
    }
    return scale;
}

// staging needs to be locked when called with the input resources
void UpdateStagingScaling(
    const Format format, const array_view<const IImageContainer::SubImageDesc>& copies, ScalingImageDataStruct& siData)
{
    PLUGIN_ASSERT(copies.size() == 1);
    if (auto iter = siData.formatToScalingImages.find(format); iter != siData.formatToScalingImages.end()) {
        const size_t index = iter->second;
        PLUGIN_ASSERT(index < siData.scalingImages.size());
        auto& scaleImage = siData.scalingImages[index];
        scaleImage.maxWidth = Math::max(scaleImage.maxWidth, copies[0].width);
        scaleImage.maxHeight = Math::max(scaleImage.maxHeight, copies[0].height);
    } else {
        const size_t index = siData.scalingImages.size();
        siData.scalingImages.push_back({ {}, format, copies[0].width, copies[0].height });
        siData.formatToScalingImages[format] = index;
    }
}

void UpdateStagingScaling(
    const Format format, const array_view<const BufferImageCopy>& copies, ScalingImageDataStruct& siData)
{
    PLUGIN_ASSERT(copies.size() == 1);
    const auto& extent = copies[0].imageExtent;
    if (auto iter = siData.formatToScalingImages.find(format); iter != siData.formatToScalingImages.end()) {
        const size_t index = iter->second;
        PLUGIN_ASSERT(index < siData.scalingImages.size());
        auto& scaleImage = siData.scalingImages[index];
        scaleImage.maxWidth = Math::max(scaleImage.maxWidth, extent.width);
        scaleImage.maxHeight = Math::max(scaleImage.maxHeight, extent.height);
    } else {
        const size_t index = siData.scalingImages.size();
        siData.scalingImages.push_back({ {}, format, extent.width, extent.height });
        siData.formatToScalingImages[format] = index;
    }
}

GpuImageDesc GetStagingScalingImageDesc(const Format format, const uint32_t width, const uint32_t height)
{
    return GpuImageDesc {
        ImageType::CORE_IMAGE_TYPE_2D,
        ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
        format,
        ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
        // NOTE sampled is not needed, but image view should not be created
        ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
            ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT,
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        0,
        0, // No dynamic barrriers
        width,
        height,
        1u,
        1u,
        1u,
        1u,
        {},
    };
}

BufferImageCopy ConvertCoreBufferImageCopy(const IImageContainer::SubImageDesc& bufferImageCopy)
{
    return BufferImageCopy {
        /** Buffer offset */
        bufferImageCopy.bufferOffset,
        /** Buffer row length */
        bufferImageCopy.bufferRowLength,
        /** Buffer image height */
        bufferImageCopy.bufferImageHeight,
        /** Image subresource */
        { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, bufferImageCopy.mipLevel, 0, bufferImageCopy.layerCount },
        /** Image offset */
        { 0, 0, 0 },
        /** Image extent */
        { bufferImageCopy.width, bufferImageCopy.height, bufferImageCopy.depth },
    };
}

#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
void LogGpuResource(const RenderHandle& gpuHandle, const EngineResourceHandle engineHandle)
{
    constexpr string_view names[] = { "buffer", "image", "sampler" };
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(gpuHandle);
    uint32_t idx = 0;
    if (handleType == RenderHandleType::GPU_IMAGE) {
        idx = 1u;
    } else if (handleType == RenderHandleType::GPU_SAMPLER) {
        idx = 2u;
    }
    PLUGIN_LOG_E("gpu %s > %" PRIx64 "=%" PRIx64 " generation: %u=%u", names[idx].data(), gpuHandle.id, engineHandle.id,
        RenderHandleUtil::GetGenerationIndexPart(gpuHandle), RenderHandleUtil::GetGenerationIndexPart(engineHandle));
}
#endif
} // namespace

GpuResourceManager::GpuResourceManager(Device& device, const CreateInfo& createInfo)
    : device_(device), gpuResourceMgrFlags_(createInfo.flags),
      gpuBufferMgr_(make_unique<GpuResourceManagerTyped<GpuBuffer, GpuBufferDesc>>(device)),
      gpuImageMgr_(make_unique<GpuResourceManagerTyped<GpuImage, GpuImageDesc>>(device)),
      gpuSamplerMgr_(make_unique<GpuResourceManagerTyped<GpuSampler, GpuSamplerDesc>>(device))
{
    gpuResourceCache_ = make_unique<GpuResourceCache>(*this);

    bufferStore_.mgr = gpuBufferMgr_.get();
    imageStore_.mgr = gpuImageMgr_.get();
    samplerStore_.mgr = gpuSamplerMgr_.get();

    const MemoryPropertyFlags deviceSharedMemoryPropertyFlags = device_.GetSharedMemoryPropertyFlags();
    // remove create info flag if not really available
    if (((gpuResourceMgrFlags_ & GPU_RESOURCE_MANAGER_OPTIMIZE_STAGING_MEMORY) == 0) ||
        !((deviceSharedMemoryPropertyFlags & NEEDED_DEVICE_MEMORY_PROPERTY_FLAGS_FOR_STAGING_MEM_OPT) ==
            NEEDED_DEVICE_MEMORY_PROPERTY_FLAGS_FOR_STAGING_MEM_OPT)) {
        gpuResourceMgrFlags_ = gpuResourceMgrFlags_ & ~GPU_RESOURCE_MANAGER_OPTIMIZE_STAGING_MEMORY;
    }
}

GpuResourceManager::~GpuResourceManager()
{
    // reset cache before logging
    // cache logs it's own un-released resources
    gpuResourceCache_.reset();

#if (RENDER_VALIDATION_ENABLED == 1)
    auto checkAndPrintValidation = [](const PerManagerStore& store, const string_view name) {
        uint32_t aliveCounter = 0;
        const auto clientLock = std::lock_guard(store.clientMutex);
        for (const auto& ref : store.clientHandles) {
            if (ref && (ref.GetRefCount() > 1)) {
                aliveCounter++;
            }
        }
        if (aliveCounter > 0) {
            PLUGIN_LOG_W(
                "RENDER_VALIDATION: Not all %s handle references released (count: %u)", name.data(), aliveCounter);
        }
    };
    checkAndPrintValidation(bufferStore_, "GPU buffer");
    checkAndPrintValidation(imageStore_, "GPU image");
    checkAndPrintValidation(samplerStore_, "GPU sampler");
#endif
}

RenderHandleReference GpuResourceManager::Get(const RenderHandle& handle) const
{
    if (RenderHandleUtil::IsValid(handle)) {
        const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handle);
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        if (handleType == RenderHandleType::GPU_BUFFER) {
            auto& store = bufferStore_;
            auto const clientLock = std::shared_lock(store.clientMutex);
            if (arrayIndex < static_cast<uint32_t>(store.clientHandles.size())) {
                return store.clientHandles[arrayIndex];
            }
        } else if (handleType == RenderHandleType::GPU_IMAGE) {
            auto& store = imageStore_;
            auto const clientLock = std::shared_lock(store.clientMutex);
            if (arrayIndex < static_cast<uint32_t>(store.clientHandles.size())) {
                return store.clientHandles[arrayIndex];
            }
        } else if (handleType == RenderHandleType::GPU_SAMPLER) {
            auto& store = samplerStore_;
            auto const clientLock = std::shared_lock(store.clientMutex);
            if (arrayIndex < static_cast<uint32_t>(store.clientHandles.size())) {
                return store.clientHandles[arrayIndex];
            }
        }
        PLUGIN_LOG_I(
            "invalid gpu resource handle (id: %" PRIu64 ", type: %u)", handle.id, static_cast<uint32_t>(handleType));
    }
    return RenderHandleReference {};
}

GpuBufferDesc GpuResourceManager::GetStagingBufferDesc(const uint32_t byteSize)
{
    return {
        BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT |
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_SINGLE_SHOT_STAGING,
        byteSize,
    };
}

// call to evaluate if there's already pending resources which we will replace
// store.clientMutex needs to be locked
uint32_t GpuResourceManager::GetPendingOptionalResourceIndex(
    const PerManagerStore& store, const RenderHandle& handle, const string_view name)
{
    uint32_t optionalResourceIndex = ~0u;
    uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const bool hasReplaceHandle = (arrayIndex < static_cast<uint32_t>(store.clientHandles.size()));
    if ((!hasReplaceHandle) && (!name.empty())) {
        if (auto const iter = store.nameToClientIndex.find(name); iter != store.nameToClientIndex.cend()) {
            arrayIndex = RenderHandleUtil::GetIndexPart(iter->second);
        }
    }
    if (arrayIndex < static_cast<uint32_t>(store.clientHandles.size())) {
        if (const uint32_t pendingArrIndex = store.additionalData[arrayIndex].indexToPendingData;
            pendingArrIndex != INVALID_PENDING_INDEX) {
            PLUGIN_ASSERT(pendingArrIndex < store.pendingData.allocations.size());
            if (pendingArrIndex < static_cast<uint32_t>(store.pendingData.allocations.size())) {
                const auto& allocOp = store.pendingData.allocations[pendingArrIndex];
                optionalResourceIndex = allocOp.optionalResourceIndex;
            }
        }
    }
    return optionalResourceIndex;
}

// needs to be locked when called
RenderHandleReference GpuResourceManager::CreateStagingBuffer(const GpuBufferDesc& desc)
{
    PerManagerStore& store = bufferStore_;
    return StoreAllocation(store, { ResourceDescriptor { desc }, {}, {}, RenderHandleType::GPU_BUFFER, ~0u, 0u })
        .handle;
}

// needs to be locked when called
GpuResourceManager::StoreAllocationData GpuResourceManager::CreateBuffer(
    const string_view name, const RenderHandle& replacedHandle, const GpuBufferDesc& desc)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    ValidateGpuBufferDesc(desc);
#endif
    MemoryPropertyFlags additionalMemPropFlags = 0U;
    if (device_.GetBackendType() == DeviceBackendType::VULKAN) {
        additionalMemPropFlags = (desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER)
                                    ? (MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                                    : 0U;
    } else {
        additionalMemPropFlags = (desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER)
                                    ? (MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                                    : 0U;
    }
    const GpuBufferDesc validatedDesc {
        desc.usageFlags | defaultBufferUsageFlags_,
        desc.memoryPropertyFlags | additionalMemPropFlags,
        desc.engineCreationFlags,
        Math::max(desc.byteSize, 1u),
        desc.format,
    };
    PerManagerStore& store = bufferStore_;
    if (validatedDesc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE) {
        // replace immediate created if still pending (i.e. not usable on the GPU)
        // memory pointers on client become invalid
        const uint32_t emplaceResourceIndex = static_cast<uint32_t>(store.pendingData.buffers.size());
        const uint32_t optionalResourceIndex =
            Math::min(emplaceResourceIndex, GetPendingOptionalResourceIndex(store, replacedHandle, name));

        if (unique_ptr<GpuBuffer> gpuBuffer = [this](const GpuBufferDesc validatedDesc) {
            // protect GPU memory allocations
            auto lock = std::lock_guard(allocationMutex_);
            return device_.CreateGpuBuffer(validatedDesc);
        }(validatedDesc)) {
            // safety checks
            if ((optionalResourceIndex < emplaceResourceIndex) &&
                (optionalResourceIndex < store.pendingData.buffers.size())) {
                store.pendingData.buffers[optionalResourceIndex] = move(gpuBuffer);
            } else {
                store.pendingData.buffers.push_back(move(gpuBuffer));
            }
        }

        StoreAllocationData sad = StoreAllocation(store, { ResourceDescriptor { validatedDesc }, name, replacedHandle,
                                                             RenderHandleType::GPU_BUFFER, optionalResourceIndex, 0u });
        // additional data is increased in StoreAllocation
        // there are as many additional data elements as clientHandle elements
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(sad.handle.GetHandle());
        PLUGIN_ASSERT(arrayIndex < store.additionalData.size());
        if (GpuBuffer* buffer = store.pendingData.buffers[optionalResourceIndex].get(); buffer) {
            store.additionalData[arrayIndex].resourcePtr = reinterpret_cast<uintptr_t>(reinterpret_cast<void*>(buffer));
        }
        return sad;
    } else {
        return StoreAllocation(store,
            { ResourceDescriptor { validatedDesc }, name, replacedHandle, RenderHandleType::GPU_BUFFER, ~0u, 0u });
    }
}

RenderHandleReference GpuResourceManager::Create(const string_view name, const GpuBufferDesc& desc)
{
    RenderHandleReference handle;

    GpuBufferDesc validDesc = GetValidGpuBufferDesc(desc);
    CheckAndEnableMemoryOptimizations(gpuResourceMgrFlags_, validDesc);

    if (desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE) {
        device_.Activate();
    }
    PerManagerStore& store = bufferStore_;
    {
        const auto lock = std::lock_guard(store.clientMutex);

        handle = CreateBuffer(name, {}, validDesc).handle;
    }
    if (desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE) {
        device_.Deactivate();
    }
    return handle;
}

RenderHandleReference GpuResourceManager::Create(const RenderHandleReference& replacedHandle, const GpuBufferDesc& desc)
{
    RenderHandleReference handle;

    const RenderHandle rawHandle = replacedHandle.GetHandle();
#if (RENDER_VALIDATION_ENABLED == 1)
    const bool valid = RenderHandleUtil::IsValid(rawHandle);
    const RenderHandleType type = RenderHandleUtil::GetHandleType(rawHandle);
    if (valid && (type != RenderHandleType::GPU_BUFFER)) {
        PLUGIN_LOG_E("RENDER_VALIDATION: trying to replace a non GPU buffer handle (type: %u) with GpuBufferDesc",
            (uint32_t)type);
    }
#endif
    GpuBufferDesc validDesc = GetValidGpuBufferDesc(desc);
    CheckAndEnableMemoryOptimizations(gpuResourceMgrFlags_, validDesc);

    if (desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE) {
        device_.Activate();
    }
    {
        PerManagerStore& store = bufferStore_;
        const auto lock = std::lock_guard(store.clientMutex);

        handle = CreateBuffer({}, rawHandle, validDesc).handle;
    }
    if (desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE) {
        device_.Deactivate();
    }
    return handle;
}

RenderHandleReference GpuResourceManager::Create(
    const string_view name, const GpuBufferDesc& desc, const array_view<const uint8_t> data)
{
    RenderHandleReference handle;

#if (RENDER_VALIDATION_ENABLED == 1)
    ValidateGpuBufferDesc(desc);
#endif

    GpuBufferDesc validDesc = GetValidGpuBufferDesc(desc);
    CheckAndEnableMemoryOptimizations(gpuResourceMgrFlags_, validDesc);
    const bool useStagingBuffer =
        (validDesc.memoryPropertyFlags & CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? false : true;

    auto& store = bufferStore_;
    if (desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE) {
        device_.Activate();
    }
    {
        StoreAllocationData sad;
        const auto lock = std::lock_guard(store.clientMutex);

        sad = CreateBuffer(name, {}, validDesc);
        const uint32_t minByteSize = std::min(validDesc.byteSize, (uint32_t)data.size_bytes());

        auto const stagingLock = std::lock_guard(stagingMutex_);

        stagingOperations_.bufferCopies.push_back(BufferCopy { 0, 0, minByteSize });
        const uint32_t beginIndex = (uint32_t)stagingOperations_.bufferCopies.size() - 1;
        vector<uint8_t> copiedData(data.cbegin().ptr(), data.cbegin().ptr() + minByteSize);

        // add staging vector index handle to resource handle in pending allocations
        PLUGIN_ASSERT(sad.allocationIndex < store.pendingData.allocations.size());
        auto& allocRef = store.pendingData.allocations[sad.allocationIndex];
        allocRef.optionalStagingVectorIndex = static_cast<uint32_t>(stagingOperations_.bufferToBuffer.size());
        allocRef.optionalStagingCopyType = useStagingBuffer ? StagingCopyStruct::CopyType::BUFFER_TO_BUFFER
                                                            : StagingCopyStruct::CopyType::CPU_TO_BUFFER;

        if (useStagingBuffer) {
            const uint32_t stagingBufferByteSize =
                useStagingBuffer ? static_cast<uint32_t>(copiedData.size_in_bytes()) : 0u;
            stagingOperations_.bufferToBuffer.push_back(
                StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_VECTOR, {}, sad.handle, beginIndex, 1,
                    move(copiedData), nullptr, Format::BASE_FORMAT_UNDEFINED, stagingBufferByteSize, false });
        } else {
            stagingOperations_.cpuToBuffer.push_back(StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_VECTOR,
                {}, sad.handle, beginIndex, 1, move(copiedData), nullptr, Format::BASE_FORMAT_UNDEFINED, 0u, false });
        }
        handle = move(sad.handle);
    }
    if (desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE) {
        device_.Deactivate();
    }
    return handle;
}

RenderHandleReference GpuResourceManager::Create(const GpuBufferDesc& desc)
{
    RenderHandleReference handle;
    GpuBufferDesc validDesc = GetValidGpuBufferDesc(desc);
    CheckAndEnableMemoryOptimizations(gpuResourceMgrFlags_, validDesc);
    if (desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE) {
        device_.Activate();
    }
    {
        auto& store = bufferStore_;
        const auto lock = std::lock_guard(store.clientMutex);

        handle = CreateBuffer({}, {}, validDesc).handle;
    }
    if (desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE) {
        device_.Deactivate();
    }
    return handle;
}

RenderHandleReference GpuResourceManager::Create(const GpuBufferDesc& desc, const array_view<const uint8_t> data)
{
    // this is a fwd-method, desc is validated inside the called method
    return Create({}, desc, data);
}

// needs to be locked when called
GpuResourceManager::StoreAllocationData GpuResourceManager::CreateImage(
    const string_view name, const RenderHandle& replacedHandle, const GpuImageDesc& desc)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    ValidateGpuImageDesc(desc, name);
#endif

    PerManagerStore& store = imageStore_;
    const StoreAllocationInfo info {
        ResourceDescriptor { GpuImageDesc {
            desc.imageType,
            desc.imageViewType,
            device_.GetFormatOrFallback(desc.format),
            (desc.imageTiling > ImageTiling::CORE_IMAGE_TILING_LINEAR) ? ImageTiling::CORE_IMAGE_TILING_OPTIMAL
                                                                       : desc.imageTiling,
            (desc.usageFlags & GpuResourceDescFlagValidation::ALL_GPU_IMAGE_USAGE_FLAGS) | defaultImageUsageFlags_,
            ((desc.memoryPropertyFlags != 0) ? desc.memoryPropertyFlags
                                             : MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &
                GpuResourceDescFlagValidation::ALL_MEMORY_PROPERTY_FLAGS,
            desc.createFlags & GpuResourceDescFlagValidation::ALL_IMAGE_CREATE_FLAGS,
            desc.engineCreationFlags,
            Math::min(MAX_IMAGE_EXTENT, Math::max(1u, desc.width)),
            Math::min(MAX_IMAGE_EXTENT, Math::max(1u, desc.height)),
            Math::min(MAX_IMAGE_EXTENT, Math::max(1u, desc.depth)),
            Math::max(1u,
                Math::min(desc.mipCount,
                    static_cast<uint32_t>(std::log2f(static_cast<float>(Math::max(desc.width, desc.height)))) + 1u)),
            Math::max(1u, desc.layerCount),
            Math::max(1u, desc.sampleCountFlags),
            desc.componentMapping,
        } },
        name, replacedHandle, RenderHandleType::GPU_IMAGE, ~0u, 0u
    };
    if (info.descriptor.imageDescriptor.format == Format::BASE_FORMAT_UNDEFINED) {
        PLUGIN_LOG_E("Undefined image BASE_FORMAT_UNDEFINED (input format %u)", static_cast<uint32_t>(desc.format));
        return {};
    }

    return StoreAllocation(store, info);
}

RenderHandleReference GpuResourceManager::Create(const RenderHandleReference& replacedHandle, const GpuImageDesc& desc)
{
    const RenderHandle rawHandle = replacedHandle.GetHandle();
#if (RENDER_VALIDATION_ENABLED == 1)
    const bool valid = RenderHandleUtil::IsValid(rawHandle);
    const RenderHandleType type = RenderHandleUtil::GetHandleType(rawHandle);
    if (valid && (type != RenderHandleType::GPU_IMAGE)) {
        PLUGIN_LOG_E(
            "RENDER_VALIDATION: trying to replace a non GPU image handle (type: %u) with GpuImageDesc", (uint32_t)type);
    }
#endif
    PerManagerStore& store = imageStore_;
    const auto lock = std::lock_guard(store.clientMutex);

    return CreateImage({}, rawHandle, desc).handle;
}

RenderHandleReference GpuResourceManager::Create(const string_view name, const GpuImageDesc& desc)
{
    PerManagerStore& store = imageStore_;
    const auto lock = std::lock_guard(store.clientMutex);

    return CreateImage(name, {}, desc).handle;
}

RenderHandleReference GpuResourceManager::Create(const GpuImageDesc& desc)
{
    PerManagerStore& store = imageStore_;
    const auto lock = std::lock_guard(store.clientMutex);

    return CreateImage({}, {}, desc).handle;
}

void GpuResourceManager::RemapGpuImageHandle(
    const RenderHandle& clientHandle, const RenderHandle& clientHandleGpuResource)
{
    bool validClientHandles = (RenderHandleUtil::GetHandleType(clientHandle) == RenderHandleType::GPU_IMAGE) ||
                              (RenderHandleUtil::GetHandleType(clientHandleGpuResource) == RenderHandleType::GPU_IMAGE);
    if (validClientHandles) {
        PerManagerStore& store = imageStore_;
        auto const lock = std::lock_guard(store.clientMutex);

        const uint32_t clientArrayIndex = RenderHandleUtil::GetIndexPart(clientHandle);
        const uint32_t clientResourceArrayIndex = RenderHandleUtil::GetIndexPart(clientHandleGpuResource);
        validClientHandles =
            validClientHandles && ((clientArrayIndex < (uint32_t)store.clientHandles.size()) &&
                                      (clientResourceArrayIndex < (uint32_t)store.clientHandles.size()));
        if (validClientHandles) {
            store.descriptions[clientArrayIndex] = store.descriptions[clientResourceArrayIndex];
            store.pendingData.remaps.push_back(RemapDescription { clientHandle, clientHandleGpuResource });
        }
    }

    if (!validClientHandles) {
        PLUGIN_LOG_E("invalid client handles given to RemapGpuImageHandle()");
    }
}

RenderHandleReference GpuResourceManager::Create(
    const string_view name, const GpuImageDesc& desc, IImageContainer::Ptr image)
{
    StoreAllocationData sad;
    if (image) {
        PerManagerStore& store = imageStore_;
        auto const lockImg = std::lock_guard(store.clientMutex);

        sad = CreateImage(name, {}, desc);
        if (IsGpuImage(sad.handle)) {
            auto const lockStag = std::lock_guard(stagingMutex_);

            const auto& copies = image->GetBufferImageCopies();
            const bool scaleImage = GetScalingImageNeed(desc, copies);

            Format format = Format::BASE_FORMAT_UNDEFINED;
            if (scaleImage) { // needs to be locked
                UpdateStagingScaling(desc.format, copies, stagingOperations_.scalingImageData);
                format = desc.format;
            }
            for (const auto& copyRef : copies) {
                stagingOperations_.bufferImageCopies.push_back(ConvertCoreBufferImageCopy(copyRef));
            }

            // add staging handle to resource handle in pending allocations
            PLUGIN_ASSERT(sad.allocationIndex < store.pendingData.allocations.size());
            auto& allocRef = store.pendingData.allocations[sad.allocationIndex];
            allocRef.optionalStagingVectorIndex = static_cast<uint32_t>(stagingOperations_.bufferToImage.size());
            allocRef.optionalStagingCopyType = StagingCopyStruct::CopyType::BUFFER_TO_IMAGE;

            const uint32_t stagingBufferByteSize = static_cast<uint32_t>(image->GetData().size_bytes());
            const uint32_t count = static_cast<uint32_t>(copies.size());
            const uint32_t beginIndex = static_cast<uint32_t>(stagingOperations_.bufferImageCopies.size()) - count;
            stagingOperations_.bufferToImage.push_back(
                StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_IMAGE_CONTAINER, {}, sad.handle, beginIndex,
                    count, {}, move(image), format, stagingBufferByteSize, false });
        }
    } else {
        PLUGIN_LOG_E("invalid image pointer to Create GPU image");
    }
    return sad.handle;
}

RenderHandleReference GpuResourceManager::Create(const string_view name, const GpuImageDesc& desc,
    const array_view<const uint8_t> data, const array_view<const BufferImageCopy> bufferImageCopies)
{
    StoreAllocationData sad;
    {
        PerManagerStore& store = imageStore_;
        auto const lockImg = std::lock_guard(store.clientMutex);

        sad = CreateImage(name, {}, desc);
        if (IsGpuImage(sad.handle)) {
            auto const lockStag = std::lock_guard(stagingMutex_);

            Format format = Format::BASE_FORMAT_UNDEFINED;
            if (GetScalingImageNeed(desc, bufferImageCopies)) { // needs to be locked
                UpdateStagingScaling(desc.format, bufferImageCopies, stagingOperations_.scalingImageData);
                format = desc.format;
            }
            for (const auto& copyRef : bufferImageCopies) {
#if (RENDER_VALIDATION_ENABLED == 1)
                ValidateGpuImageCopy(desc, copyRef, name);
#endif
                stagingOperations_.bufferImageCopies.push_back(copyRef);
            }
            // add staging vector index to resource alloc in pending allocations
            PLUGIN_ASSERT(sad.allocationIndex < store.pendingData.allocations.size());
            auto& allocRef = store.pendingData.allocations[sad.allocationIndex];
            allocRef.optionalStagingVectorIndex = static_cast<uint32_t>(stagingOperations_.bufferToImage.size());
            allocRef.optionalStagingCopyType = StagingCopyStruct::CopyType::BUFFER_TO_IMAGE;

            const uint32_t stagingBufferByteSize = static_cast<uint32_t>(data.size_bytes());
            const uint32_t count = (uint32_t)bufferImageCopies.size();
            const uint32_t beginIndex = (uint32_t)stagingOperations_.bufferImageCopies.size() - count;

            vector<uint8_t> copiedData(data.cbegin().ptr(), data.cend().ptr());
            stagingOperations_.bufferToImage.push_back(
                StagingCopyStruct { StagingCopyStruct::DataType::DATA_TYPE_VECTOR, {}, sad.handle, beginIndex, count,
                    move(copiedData), nullptr, format, stagingBufferByteSize, false });
        }
    }
    return sad.handle;
}

RenderHandleReference GpuResourceManager::Create(
    const string_view name, const GpuImageDesc& desc, const array_view<const uint8_t> data)
{
    BufferImageCopy bufferImageCopy {
        0,
        desc.width,
        desc.height,
        { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0, desc.layerCount },
        { 0, 0, 0 },
        { desc.width, desc.height, desc.depth },
    };

    const array_view<const BufferImageCopy> av(&bufferImageCopy, 1);
    return Create(name, desc, data, av);
}

RenderHandleReference GpuResourceManager::CreateView(
    const string_view name, const GpuImageDesc& desc, const GpuImagePlatformData& gpuImagePlatformData)
{
    device_.Activate();
    PerManagerStore& store = imageStore_;
    const auto lock = std::lock_guard(store.clientMutex);

    // replace immediate created if still pending (i.e. not usable on the GPU)
    const uint32_t emplaceResourceIndex = static_cast<uint32_t>(store.pendingData.images.size());
    const uint32_t optionalResourceIndex =
        Math::min(emplaceResourceIndex, GetPendingOptionalResourceIndex(store, {}, name));

    if (unique_ptr<GpuImage> gpuImage = [this](const GpuImageDesc& desc,
        const GpuImagePlatformData& gpuImagePlatformData) {
            // protect GPU memory allocations
            auto lock = std::lock_guard(allocationMutex_);
            return device_.CreateGpuImageView(desc, gpuImagePlatformData);
        } (desc, gpuImagePlatformData)) {
        // safety checks
        if ((optionalResourceIndex < emplaceResourceIndex) &&
            (optionalResourceIndex < store.pendingData.images.size())) {
            store.pendingData.images[optionalResourceIndex] = move(gpuImage);
        } else {
            store.pendingData.images.push_back(move(gpuImage));
        }
    }
    device_.Deactivate();

    return StoreAllocation(
        store, { ResourceDescriptor { desc }, name, {}, RenderHandleType::GPU_IMAGE, optionalResourceIndex, 0u })
        .handle;
}

RenderHandleReference GpuResourceManager::CreateView(
    const string_view name, const GpuImageDesc& desc, const BackendSpecificImageDesc& backendSpecificData)
{
    device_.Activate();
    PerManagerStore& store = imageStore_;
    const auto lock = std::lock_guard(store.clientMutex);

    // replace immediate created if still pending (i.e. not usable on the GPU)
    const uint32_t emplaceResourceIndex = static_cast<uint32_t>(store.pendingData.images.size());
    const uint32_t optionalResourceIndex =
        Math::min(emplaceResourceIndex, GetPendingOptionalResourceIndex(store, {}, name));

    // additional handle flags provide information if platform conversion is needed
    uint32_t additionalHandleFlags = 0u;

    if (unique_ptr<GpuImage> gpuImage = 
        [this](const GpuImageDesc& desc, const BackendSpecificImageDesc& backendSpecificData) {
            // protect GPU memory allocations
            auto lock = std::lock_guard(allocationMutex_);
            return device_.CreateGpuImageView(desc, backendSpecificData);
        } (desc, backendSpecificData)) {
        const auto additionalImageFlags = gpuImage->GetAdditionalFlags();
        additionalHandleFlags =
            (additionalImageFlags & GpuImage::AdditionalFlagBits::ADDITIONAL_PLATFORM_CONVERSION_BIT)
                ? CORE_RESOURCE_HANDLE_PLATFORM_CONVERSION
                : 0u;
        // safety checks
        if ((optionalResourceIndex < emplaceResourceIndex) &&
            (optionalResourceIndex < store.pendingData.images.size())) {
            store.pendingData.images[optionalResourceIndex] = move(gpuImage);
        } else {
            store.pendingData.images.push_back(move(gpuImage));
        }
    }
    device_.Deactivate();

    const auto& images = store.pendingData.images;
    const auto& finalDesc = (optionalResourceIndex < images.size() && images[optionalResourceIndex])
                                ? images[optionalResourceIndex]->GetDesc()
                                : desc;
    auto handle = StoreAllocation(store, { ResourceDescriptor { finalDesc }, name, {}, RenderHandleType::GPU_IMAGE,
                                             optionalResourceIndex, additionalHandleFlags })
                      .handle;
#if (RENDER_VALIDATION_ENABLED == 1)
    if ((additionalHandleFlags & CORE_RESOURCE_HANDLE_PLATFORM_CONVERSION) &&
        !RenderHandleUtil::IsPlatformConversionResource(handle.GetHandle())) {
        PLUGIN_LOG_ONCE_W("core_validation_create_view_plat_conversion",
            "RENDER_VALIDATION: platform conversion needing resource cannot replace existing resource handle (name: "
            "%s)",
            name.data());
    }
#endif
    return handle;
}

RenderHandleReference GpuResourceManager::Create(const GpuImageDesc& desc, const array_view<const uint8_t> data,
    const array_view<const BufferImageCopy> bufferImageCopies)
{
    return Create({}, desc, data, bufferImageCopies);
}

RenderHandleReference GpuResourceManager::Create(const GpuImageDesc& desc, const array_view<const uint8_t> data)
{
    return Create({}, desc, data);
}

RenderHandleReference GpuResourceManager::Create(const GpuImageDesc& desc, IImageContainer::Ptr image)
{
    return Create({}, desc, move(image));
}

RenderHandleReference GpuResourceManager::Create(const string_view name, const GpuSamplerDesc& desc)
{
    PerManagerStore& store = samplerStore_;
    const auto lock = std::lock_guard(store.clientMutex);

    return StoreAllocation(store, { ResourceDescriptor { desc }, name, {}, RenderHandleType::GPU_SAMPLER, ~0u, 0u })
        .handle;
}

RenderHandleReference GpuResourceManager::Create(
    const RenderHandleReference& replacedHandle, const GpuSamplerDesc& desc)
{
    const RenderHandle rawHandle = replacedHandle.GetHandle();
#if (RENDER_VALIDATION_ENABLED == 1)
    const bool valid = RenderHandleUtil::IsValid(rawHandle);
    const RenderHandleType type = RenderHandleUtil::GetHandleType(rawHandle);
    if (valid && (type != RenderHandleType::GPU_SAMPLER)) {
        PLUGIN_LOG_E("RENDER_VALIDATION: trying to replace a non GPU sampler handle (type: %u) with GpuSamplerDesc",
            (uint32_t)type);
    }
#endif
    PerManagerStore& store = samplerStore_;
    const auto lock = std::lock_guard(store.clientMutex);

    return StoreAllocation(
        store, { ResourceDescriptor { desc }, {}, rawHandle, RenderHandleType::GPU_SAMPLER, ~0u, 0u })
        .handle;
}

RenderHandleReference GpuResourceManager::Create(const GpuSamplerDesc& desc)
{
    return Create("", desc);
}

GpuResourceManager::StoreAllocationData GpuResourceManager::CreateAccelerationStructure(
    const BASE_NS::string_view name, const RenderHandle& replacedHandle, const GpuAccelerationStructureDesc& desc)
{
    PerManagerStore& store = bufferStore_;
#if (RENDER_VALIDATION_ENABLED == 1)
    ValidateGpuAccelerationStructureDesc(desc);
#endif
    GpuAccelerationStructureDesc validatedDesc = desc;
    validatedDesc.bufferDesc.usageFlags |= defaultBufferUsageFlags_;
    validatedDesc.bufferDesc.byteSize = Math::max(validatedDesc.bufferDesc.byteSize, 1u),
    validatedDesc.bufferDesc.usageFlags |= CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT;

    constexpr auto additionalBufferFlags = CORE_RESOURCE_HANDLE_ACCELERATION_STRUCTURE;
    return StoreAllocation(store, { ResourceDescriptor { validatedDesc }, name, replacedHandle,
                                      RenderHandleType::GPU_BUFFER, ~0u, additionalBufferFlags });
}

RenderHandleReference GpuResourceManager::Create(const GpuAccelerationStructureDesc& desc)
{
    PerManagerStore& store = bufferStore_;
    const auto lock = std::lock_guard(store.clientMutex);

    return CreateAccelerationStructure("", {}, desc).handle;
}

RenderHandleReference GpuResourceManager::Create(const string_view name, const GpuAccelerationStructureDesc& desc)
{
    PerManagerStore& store = bufferStore_;
    const auto lock = std::lock_guard(store.clientMutex);

    return CreateAccelerationStructure(name, {}, desc).handle;
}

RenderHandleReference GpuResourceManager::Create(
    const RenderHandleReference& replacedHandle, const GpuAccelerationStructureDesc& desc)
{
    const RenderHandle rawHandle = replacedHandle.GetHandle();
#if (RENDER_VALIDATION_ENABLED == 1)
    const bool valid = RenderHandleUtil::IsValid(rawHandle);
    const RenderHandleType type = RenderHandleUtil::GetHandleType(rawHandle);
    if (valid &&
        ((type != RenderHandleType::GPU_BUFFER) || (!RenderHandleUtil::IsGpuAccelerationStructure(rawHandle)))) {
        PLUGIN_LOG_E("RENDER_VALIDATION: trying to replace a non GPU acceleration structure handle (type: %u) with "
                     "GpuAccelerationStructureDesc",
            (uint32_t)type);
    }
#endif
    PerManagerStore& store = bufferStore_;
    const auto lock = std::lock_guard(store.clientMutex);

    return CreateAccelerationStructure("", rawHandle, desc).handle;
}

RenderHandleReference GpuResourceManager::CreateSwapchainImage(
    const RenderHandleReference& replacedHandle, const BASE_NS::string_view name, const GpuImageDesc& desc)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    ValidateGpuImageDesc(desc, "");
#endif

    PerManagerStore& store = imageStore_;
    const auto lock = std::lock_guard(store.clientMutex);

    const uint32_t addFlags = RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_SWAPCHAIN_RESOURCE;
    // NOTE: no mips for swapchains
    // TODO: NOTE: allocation type is undefined
    const StoreAllocationInfo info {
        ResourceDescriptor { GpuImageDesc {
            desc.imageType,
            desc.imageViewType,
            device_.GetFormatOrFallback(desc.format),
            (desc.imageTiling > ImageTiling::CORE_IMAGE_TILING_LINEAR) ? ImageTiling::CORE_IMAGE_TILING_OPTIMAL
                                                                       : desc.imageTiling,
            (desc.usageFlags & GpuResourceDescFlagValidation::ALL_GPU_IMAGE_USAGE_FLAGS) | defaultImageUsageFlags_,
            ((desc.memoryPropertyFlags != 0) ? desc.memoryPropertyFlags
                                             : MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &
                GpuResourceDescFlagValidation::ALL_MEMORY_PROPERTY_FLAGS,
            desc.createFlags & GpuResourceDescFlagValidation::ALL_IMAGE_CREATE_FLAGS,
            desc.engineCreationFlags,
            Math::min(MAX_IMAGE_EXTENT, Math::max(1u, desc.width)),
            Math::min(MAX_IMAGE_EXTENT, Math::max(1u, desc.height)),
            Math::min(MAX_IMAGE_EXTENT, Math::max(1u, desc.depth)),
            1u, // hard-coded mip count
            Math::max(1u, desc.layerCount),
            Math::max(1u, desc.sampleCountFlags),
            desc.componentMapping,
        } },
        name, replacedHandle.GetHandle(), RenderHandleType::GPU_IMAGE, ~0u, addFlags, AllocType::UNDEFINED
    };
    if (info.descriptor.imageDescriptor.format == Format::BASE_FORMAT_UNDEFINED) {
        PLUGIN_LOG_E("Undefined image BASE_FORMAT_UNDEFINED (input format %u)", static_cast<uint32_t>(desc.format));
        return {};
    }

    return StoreAllocation(store, info).handle;
}

RenderHandleReference GpuResourceManager::CreateShallowHandle(const GpuImageDesc& desc)
{
    PerManagerStore& store = imageStore_;
    const auto lock = std::lock_guard(store.clientMutex);

#if (RENDER_VALIDATION_ENABLED == 1)
    ValidateGpuImageDesc(desc, "");
#endif

    const uint32_t addFlags = RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_SHALLOW_RESOURCE;
    const StoreAllocationInfo info {
        ResourceDescriptor { GpuImageDesc {
            desc.imageType,
            desc.imageViewType,
            device_.GetFormatOrFallback(desc.format),
            (desc.imageTiling > ImageTiling::CORE_IMAGE_TILING_LINEAR) ? ImageTiling::CORE_IMAGE_TILING_OPTIMAL
                                                                       : desc.imageTiling,
            (desc.usageFlags & GpuResourceDescFlagValidation::ALL_GPU_IMAGE_USAGE_FLAGS) | defaultImageUsageFlags_,
            ((desc.memoryPropertyFlags != 0) ? desc.memoryPropertyFlags
                                             : MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &
                GpuResourceDescFlagValidation::ALL_MEMORY_PROPERTY_FLAGS,
            desc.createFlags & GpuResourceDescFlagValidation::ALL_IMAGE_CREATE_FLAGS,
            desc.engineCreationFlags,
            Math::min(MAX_IMAGE_EXTENT, Math::max(1u, desc.width)),
            Math::min(MAX_IMAGE_EXTENT, Math::max(1u, desc.height)),
            Math::min(MAX_IMAGE_EXTENT, Math::max(1u, desc.depth)),
            Math::max(1u,
                Math::min(desc.mipCount,
                    static_cast<uint32_t>(std::log2f(static_cast<float>(Math::max(desc.width, desc.height)))) + 1u)),
            Math::max(1u, desc.layerCount),
            Math::max(1u, desc.sampleCountFlags),
            desc.componentMapping,
        } },
        "", {}, RenderHandleType::GPU_IMAGE, ~0u, addFlags, AllocType::UNDEFINED
    };
    if (info.descriptor.imageDescriptor.format == Format::BASE_FORMAT_UNDEFINED) {
        PLUGIN_LOG_E("Undefined image BASE_FORMAT_UNDEFINED (input format %u)", static_cast<uint32_t>(desc.format));
        return {};
    }

    return StoreAllocation(store, info).handle;
}

// has staging lock and possible gpu buffer lock inside
void GpuResourceManager::RemoveStagingOperations(const OperationDescription& destroyAlloc)
{
    // remove possible stagings
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(destroyAlloc.handle);
    if (((handleType == RenderHandleType::GPU_BUFFER) || (handleType == RenderHandleType::GPU_IMAGE)) &&
        (destroyAlloc.optionalStagingCopyType != StagingCopyStruct::CopyType::UNDEFINED)) {
        auto const lockStaging = std::lock_guard(stagingMutex_);

        auto invalidateStagingCopy = [](const OperationDescription& alloc, vector<StagingCopyStruct>& vecRef) {
            PLUGIN_ASSERT(alloc.optionalStagingVectorIndex < vecRef.size());
            vecRef[alloc.optionalStagingVectorIndex].srcHandle = {};
            vecRef[alloc.optionalStagingVectorIndex].dstHandle = {};
            vecRef[alloc.optionalStagingVectorIndex].invalidOperation = true;
        };

        if (destroyAlloc.optionalStagingCopyType == StagingCopyStruct::CopyType::BUFFER_TO_BUFFER) {
            invalidateStagingCopy(destroyAlloc, stagingOperations_.bufferToBuffer);
        } else if (destroyAlloc.optionalStagingCopyType == StagingCopyStruct::CopyType::BUFFER_TO_IMAGE) {
            invalidateStagingCopy(destroyAlloc, stagingOperations_.bufferToImage);
        } else if (destroyAlloc.optionalStagingCopyType == StagingCopyStruct::CopyType::IMAGE_TO_BUFFER) {
            invalidateStagingCopy(destroyAlloc, stagingOperations_.imageToBuffer);
        } else if (destroyAlloc.optionalStagingCopyType == StagingCopyStruct::CopyType::IMAGE_TO_IMAGE) {
            invalidateStagingCopy(destroyAlloc, stagingOperations_.imageToImage);
        } else if (destroyAlloc.optionalStagingCopyType == StagingCopyStruct::CopyType::CPU_TO_BUFFER) {
            invalidateStagingCopy(destroyAlloc, stagingOperations_.cpuToBuffer);
        } else {
            PLUGIN_ASSERT(false);
        }
    }

    // NOTE: we do not clean-up/invalidate copy operations stagingOperations_.cpuToBuffer ATM
    // it is user's responsibility do not use handle that you've destroyed
}

// needs to be locked from outside
// staging cannot be locked when called
void GpuResourceManager::Destroy(PerManagerStore& store, const RenderHandle& handle)
{
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
    PLUGIN_LOG_E("gpu resource deallocation %" PRIx64, handle.id);
#endif

    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    if (arrayIndex < store.clientHandles.size()) {
        if (!(store.clientHandles[arrayIndex])) {
            return; // early out if re-destroying the same handle
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        const uint32_t currGeneration =
            RenderHandleUtil::GetGenerationIndexPart(store.clientHandles[arrayIndex].GetHandle());
        const uint32_t destroyHandleGeneration = RenderHandleUtil::GetGenerationIndexPart(handle);
        if (currGeneration != destroyHandleGeneration) {
            PLUGIN_LOG_W("RENDER_VALIDATION: destroy handle is not the current generation (destroy:%u != current:%u)",
                currGeneration, destroyHandleGeneration);
        }
#endif
        const uint32_t hasNameId = RenderHandleUtil::GetHasNamePart(handle);
        if (hasNameId != 0) {
            // remove name if present
            if (auto const pos = std::find_if(store.nameToClientIndex.begin(), store.nameToClientIndex.end(),
                    [arrayIndex](auto const& nameToHandle) { return nameToHandle.second == arrayIndex; });
                pos != store.nameToClientIndex.end()) {
                store.nameToClientIndex.erase(pos);
            }
        }

        // we do not set default values to GpuXDesc (we leave the old data, it won't be used)
        // invalidate handle, noticed when trying to re-destroy (early-out in the beginning of the if)
        store.clientHandles[arrayIndex] = {};

        // if the handle is already found and it's an alloc we do not want to allocate and then deallocate
        if (const uint32_t pendingArrIndex = store.additionalData[arrayIndex].indexToPendingData;
            pendingArrIndex != INVALID_PENDING_INDEX) {
            // NOTE: check valid assert here (if called before resources are created)
            if (pendingArrIndex < store.pendingData.allocations.size()) {
                auto& ref = store.pendingData.allocations[pendingArrIndex];
                if (ref.allocType == AllocType::ALLOC) {
                    ref.allocType = AllocType::REMOVED;
                    RemoveStagingOperations(ref);
                }
            }
        } else {
            PLUGIN_ASSERT(store.additionalData[arrayIndex].indexToPendingData == INVALID_PENDING_INDEX);
            store.additionalData[arrayIndex] = { 0, static_cast<uint32_t>(store.pendingData.allocations.size()) };
            store.pendingData.allocations.emplace_back(
                handle, ResourceDescriptor { GpuBufferDesc {} }, AllocType::DEALLOC, static_cast<uint32_t>(~0u));
            // there cannot be staging operations because pendingData was not found
            // all other operations to destroyable handle are user's responsibility
        }
    }
}

// needs to be locked when called
void GpuResourceManager::DestroyImmediate(PerManagerStore& store, const RenderHandle& handle)
{
    if (RenderHandleUtil::IsValid(handle)) { // found, Destroy immediate
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        if (arrayIndex < (uint32_t)store.gpuHandles.size()) {
            store.mgr->DestroyImmediate(arrayIndex);
            store.clientHandles[arrayIndex] = {};
            store.additionalData[arrayIndex] = {};
            store.gpuHandles[arrayIndex] = InvalidateWithGeneration(store.gpuHandles[arrayIndex]);
        }
    }
}

void GpuResourceManager::Destroy(const RenderHandle& handle)
{
    if (RenderHandleUtil::IsValid(handle)) {
        const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handle);
        if (handleType == RenderHandleType::GPU_BUFFER) {
            const auto clientLock = std::lock_guard(bufferStore_.clientMutex);
            Destroy(bufferStore_, handle);
        } else if (handleType == RenderHandleType::GPU_IMAGE) {
            const auto clientLock = std::lock_guard(imageStore_.clientMutex);
            Destroy(imageStore_, handle);
        } else if (handleType == RenderHandleType::GPU_SAMPLER) {
            const auto clientLock = std::lock_guard(samplerStore_.clientMutex);
            Destroy(samplerStore_, handle);
        } else {
            PLUGIN_LOG_I("invalid gpu resource handle : %" PRIu64, handle.id);
        }
    }
}

RenderHandleReference GpuResourceManager::GetHandle(const PerManagerStore& store, const string_view name) const
{
    if (name.empty()) { // early out before lock
        return RenderHandleReference {};
    }

    auto const clientLock = std::shared_lock(store.clientMutex);
    if (auto const pos = store.nameToClientIndex.find(name); pos != store.nameToClientIndex.end()) {
        PLUGIN_ASSERT(pos->second < static_cast<uint32_t>(store.clientHandles.size()));
        return store.clientHandles[pos->second];
    }
    // NOTE: This is used in some places to check if the GPU resource is found
    // therefore no error prints here
    return RenderHandleReference {};
}

RenderHandle GpuResourceManager::GetRawHandle(const PerManagerStore& store, const string_view name) const
{
    if (name.empty()) { // early out before lock
        return RenderHandle {};
    }

    auto const clientLock = std::shared_lock(store.clientMutex);
    if (auto const pos = store.nameToClientIndex.find(name); pos != store.nameToClientIndex.end()) {
        PLUGIN_ASSERT(pos->second < static_cast<uint32_t>(store.clientHandles.size()));
        return store.clientHandles[pos->second].GetHandle();
    }
    // NOTE: This is used in some places to check if the GPU resource is found
    // therefore no error prints here
    return RenderHandle {};
}

RenderHandleReference GpuResourceManager::GetBufferHandle(const string_view name) const
{
    return GetHandle(bufferStore_, name);
}

RenderHandleReference GpuResourceManager::GetImageHandle(const string_view name) const
{
    return GetHandle(imageStore_, name);
}

RenderHandleReference GpuResourceManager::GetSamplerHandle(const string_view name) const
{
    return GetHandle(samplerStore_, name);
}

RenderHandle GpuResourceManager::GetBufferRawHandle(const string_view name) const
{
    return GetRawHandle(bufferStore_, name);
}

RenderHandle GpuResourceManager::GetImageRawHandle(const string_view name) const
{
    return GetRawHandle(imageStore_, name);
}

RenderHandle GpuResourceManager::GetSamplerRawHandle(const string_view name) const
{
    return GetRawHandle(samplerStore_, name);
}

GpuBufferDesc GpuResourceManager::GetBufferDescriptor(const RenderHandle& handle) const
{
    if (!IsGpuBuffer(handle)) {
        return GpuBufferDesc {};
    }
    {
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        const PerManagerStore& store = bufferStore_;
        auto const clientLock = std::shared_lock(store.clientMutex);
        if (arrayIndex < (uint32_t)store.descriptions.size()) {
#if (RENDER_VALIDATION_ENABLED == 1)
            if (RenderHandleUtil::GetGenerationIndexPart(store.clientHandles[arrayIndex].GetHandle()) !=
                RenderHandleUtil::GetGenerationIndexPart(handle)) {
                const auto name = GetName(handle);
                PLUGIN_LOG_E(
                    "RENDER_VALIDATION: buffer client handle is not current generation (name:%s)", name.c_str());
            }
#endif
            return store.descriptions[arrayIndex].combinedBufDescriptor.bufferDesc;
        }
    }
    return GpuBufferDesc {};
}

GpuBufferDesc GpuResourceManager::GetBufferDescriptor(const RenderHandleReference& handle) const
{
    return GetBufferDescriptor(handle.GetHandle());
}

GpuImageDesc GpuResourceManager::GetImageDescriptor(const RenderHandle& handle) const
{
    if (!IsGpuImage(handle)) {
        return GpuImageDesc {};
    }
    {
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        const PerManagerStore& store = imageStore_;
        auto const clientLock = std::shared_lock(store.clientMutex);
        if (arrayIndex < (uint32_t)store.descriptions.size()) {
#if (RENDER_VALIDATION_ENABLED == 1)
            if (RenderHandleUtil::GetGenerationIndexPart(store.clientHandles[arrayIndex].GetHandle()) !=
                RenderHandleUtil::GetGenerationIndexPart(handle)) {
                const auto name = GetName(handle);
                PLUGIN_LOG_E(
                    "RENDER_VALIDATION: image client handle is not current generation (name:%s)", name.c_str());
            }
#endif
            return store.descriptions[arrayIndex].imageDescriptor;
        }
    }
    return GpuImageDesc {};
}

GpuImageDesc GpuResourceManager::GetImageDescriptor(const RenderHandleReference& handle) const
{
    return GetImageDescriptor(handle.GetHandle());
}

GpuSamplerDesc GpuResourceManager::GetSamplerDescriptor(const RenderHandle& handle) const
{
    if (!IsGpuSampler(handle)) {
        return GpuSamplerDesc {};
    }
    {
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        const PerManagerStore& store = samplerStore_;
        auto const clientLock = std::shared_lock(store.clientMutex);
        if (arrayIndex < (uint32_t)store.descriptions.size()) {
#if (RENDER_VALIDATION_ENABLED == 1)
            if (RenderHandleUtil::GetGenerationIndexPart(store.clientHandles[arrayIndex].GetHandle()) !=
                RenderHandleUtil::GetGenerationIndexPart(handle)) {
                const auto name = GetName(handle);
                PLUGIN_LOG_E(
                    "RENDER_VALIDATION: sampler client handle is not current generation (name:%s)", name.c_str());
            }
#endif
            return store.descriptions[arrayIndex].samplerDescriptor;
        }
    }
    return GpuSamplerDesc {};
}

GpuSamplerDesc GpuResourceManager::GetSamplerDescriptor(const RenderHandleReference& handle) const
{
    return GetSamplerDescriptor(handle.GetHandle());
}

GpuAccelerationStructureDesc GpuResourceManager::GetAccelerationStructureDescriptor(const RenderHandle& handle) const
{
    if (!IsGpuAccelerationStructure(handle)) {
        return GpuAccelerationStructureDesc {};
    }
    {
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        const PerManagerStore& store = bufferStore_;
        auto const clientLock = std::shared_lock(store.clientMutex);
        if (arrayIndex < (uint32_t)store.descriptions.size()) {
#if (RENDER_VALIDATION_ENABLED == 1)
            if (RenderHandleUtil::GetGenerationIndexPart(store.clientHandles[arrayIndex].GetHandle()) !=
                RenderHandleUtil::GetGenerationIndexPart(handle)) {
                const auto name = GetName(handle);
                PLUGIN_LOG_E(
                    "RENDER_VALIDATION: acceleration structure client handle is not current generation (name:%s)",
                    name.c_str());
            }
#endif
            return store.descriptions[arrayIndex].combinedBufDescriptor;
        }
    }
    return GpuAccelerationStructureDesc {};
}

GpuAccelerationStructureDesc GpuResourceManager::GetAccelerationStructureDescriptor(
    const RenderHandleReference& handle) const
{
    return GetAccelerationStructureDescriptor(handle.GetHandle());
}

string GpuResourceManager::GetName(const RenderHandle& handle) const
{
    if (RenderHandleUtil::GetHasNamePart(handle) != 0) {
        const PerManagerStore* store = nullptr;
        const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handle);
        if (handleType == RenderHandleType::GPU_BUFFER) {
            store = &bufferStore_;
        } else if (handleType == RenderHandleType::GPU_IMAGE) {
            store = &imageStore_;
        } else if (handleType == RenderHandleType::GPU_SAMPLER) {
            store = &samplerStore_;
        }
        if (store) {
            const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
            for (const auto& iter : store->nameToClientIndex) {
                if (arrayIndex == iter.second) {
                    return iter.first;
                }
            }
        }
    }
    return {};
}

string GpuResourceManager::GetName(const RenderHandleReference& handle) const
{
    return GetName(handle.GetHandle());
}

vector<RenderHandleReference> GpuResourceManager::GetHandles(const PerManagerStore& store) const
{
    const auto lock = std::shared_lock(store.clientMutex);

    vector<RenderHandleReference> res;
    res.reserve(store.clientHandles.size());
    for (const auto& ref : store.clientHandles) {
        if (ref) {
            res.push_back(ref);
        }
    }
    return res;
}

vector<RenderHandleReference> GpuResourceManager::GetGpuBufferHandles() const
{
    return GetHandles(bufferStore_);
}

vector<RenderHandleReference> GpuResourceManager::GetGpuImageHandles() const
{
    return GetHandles(imageStore_);
}

vector<RenderHandleReference> GpuResourceManager::GetGpuSamplerHandles() const
{
    return GetHandles(samplerStore_);
}

void GpuResourceManager::SetDefaultGpuBufferCreationFlags(const BufferUsageFlags usageFlags)
{
    defaultBufferUsageFlags_ = usageFlags;
}

void GpuResourceManager::SetDefaultGpuImageCreationFlags(const ImageUsageFlags usageFlags)
{
    defaultImageUsageFlags_ = usageFlags;
}

void GpuResourceManager::CreateGpuResource(const OperationDescription& op, const uint32_t arrayIndex,
    const RenderHandleType resourceType, const uintptr_t preCreatedResVec)
{
    if (resourceType == RenderHandleType::GPU_BUFFER) {
        PLUGIN_ASSERT(preCreatedResVec);
        if (RenderHandleUtil::IsGpuAccelerationStructure(op.handle)) {
            PLUGIN_ASSERT(op.optionalResourceIndex == ~0u);
            gpuBufferMgr_->Create<GpuAccelerationStructureDesc>(arrayIndex,
                op.descriptor.combinedBufDescriptor.bufferDesc, {}, true, op.descriptor.combinedBufDescriptor);
        } else {
            if (op.optionalResourceIndex != ~0u) {
                BufferVector& res = *(reinterpret_cast<BufferVector*>(reinterpret_cast<void*>(preCreatedResVec)));
                gpuBufferMgr_->Create<GpuAccelerationStructureDesc>(arrayIndex,
                    op.descriptor.combinedBufDescriptor.bufferDesc, move(res[op.optionalResourceIndex]), false,
                    op.descriptor.combinedBufDescriptor);
            } else {
                gpuBufferMgr_->Create<GpuAccelerationStructureDesc>(arrayIndex,
                    op.descriptor.combinedBufDescriptor.bufferDesc, {}, false, op.descriptor.combinedBufDescriptor);
            }
        }
    } else if (resourceType == RenderHandleType::GPU_IMAGE) {
        PLUGIN_ASSERT(preCreatedResVec);
        if (op.optionalResourceIndex != ~0u) {
            ImageVector& images = *(reinterpret_cast<ImageVector*>(reinterpret_cast<void*>(preCreatedResVec)));
            gpuImageMgr_->Create<uint32_t>(
                arrayIndex, op.descriptor.imageDescriptor, move(images[op.optionalResourceIndex]), false, 0);
        } else {
            gpuImageMgr_->Create<uint32_t>(arrayIndex, op.descriptor.imageDescriptor, {}, false, 0);
        }
    } else if (resourceType == RenderHandleType::GPU_SAMPLER) {
        PLUGIN_ASSERT(preCreatedResVec == 0);
        PLUGIN_ASSERT(op.optionalResourceIndex == ~0u);
        gpuSamplerMgr_->Create<uint32_t>(arrayIndex, op.descriptor.samplerDescriptor, {}, false, 0);
    }
}

// needs to be locked when called, and call only for valid gpu handles
void GpuResourceManager::DestroyGpuResource(const OperationDescription& operation, const uint32_t arrayIndex,
    const RenderHandleType resourceType, PerManagerStore& store)
{
    store.mgr->Destroy(arrayIndex);
    PLUGIN_ASSERT(arrayIndex < static_cast<uint32_t>(store.gpuHandles.size()));
    store.clientHandles[arrayIndex] = {};
    store.additionalData[arrayIndex] = {};
    store.gpuHandles[arrayIndex] = InvalidateWithGeneration(store.gpuHandles[arrayIndex]);
}

void GpuResourceManager::HandlePendingRemappings(
    const vector<RemapDescription>& pendingShallowRemappings, vector<EngineResourceHandle>& gpuHandles)
{
    for (auto const& shallowRemap : pendingShallowRemappings) {
        // find the gpu handle where the client handle wants to point to
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(shallowRemap.resourceClientHandle);
        if (arrayIndex < gpuHandles.size()) {
            const EngineResourceHandle gpuHandle = gpuHandles[arrayIndex];

            const bool validGpuHandle = RenderHandleUtil::IsValid(gpuHandle);
            const uint32_t remapArrayIndex = RenderHandleUtil::GetIndexPart(shallowRemap.shallowClientHandle);
            if (validGpuHandle && (remapArrayIndex < (uint32_t)gpuHandles.size())) {
                gpuHandles[remapArrayIndex] = gpuHandle;
            } else {
                PLUGIN_LOG_E("gpuimage handle remapping failed; client handle not found");
            }
        }
    }
}

void GpuResourceManager::HandlePendingAllocationsImpl(const bool isFrameEnd)
{
    auto handleStorePendingAllocations = [this](PerManagerStore& store, bool isFrameEnd) {
        store.clientMutex.lock();
        // protect GPU memory allocations
        allocationMutex_.lock();
        // check for handle destructions
        for (const auto& handleRef : store.clientHandles) {
            if (handleRef && (handleRef.GetRefCount() <= 1)) {
                Destroy(store, handleRef.GetHandle());
            }
        }

        const auto pendingAllocations = move(store.pendingData.allocations);
        auto pendingBuffers = move(store.pendingData.buffers);
        auto pendingImages = move(store.pendingData.images);
        auto pendingRemaps = move(store.pendingData.remaps);
        uintptr_t pendingRes = 0; // ptr to pending resource vector
        if (store.handleType == RenderHandleType::GPU_BUFFER) {
            pendingRes = reinterpret_cast<uintptr_t>(static_cast<void*>(&pendingBuffers));
        } else if (store.handleType == RenderHandleType::GPU_IMAGE) {
            pendingRes = reinterpret_cast<uintptr_t>(static_cast<void*>(&pendingImages));
        }

        // increase the gpu handle vector sizes if needed
        if (store.clientHandles.size() > store.gpuHandles.size()) {
            store.gpuHandles.resize(store.clientHandles.size());
            store.mgr->Resize(store.clientHandles.size());
        }

        for (auto const& allocation : pendingAllocations) {
            const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(allocation.handle);
            // NOTE: needs to be cleared here
            store.additionalData[arrayIndex].indexToPendingData = ~0u;
            // NOTE: immediately created resource replacing should be prevented (ptr might be present still)
            if (allocation.allocType == AllocType::REMOVED) {
                PLUGIN_ASSERT(arrayIndex < static_cast<uint32_t>(store.clientHandles.size()));
                store.availableHandleIds.push_back(allocation.handle.id);
                store.clientHandles[arrayIndex] = {};
                store.additionalData[arrayIndex] = {};
                continue;
            }

            PLUGIN_ASSERT(arrayIndex < (uint32_t)store.gpuHandles.size());
            // first allocation, then dealloc, with dealloc we need to check for deferred destruction
            if (allocation.allocType == AllocType::ALLOC) {
                // NOTE: this is essential to get correct, this maps render pass etc. hashing
                // if the generation counter is old there might vulkan image layout issues etc.
                const EngineResourceHandle gpuHandle =
                    UnpackNewHandle(store.gpuHandles[arrayIndex], store.handleType, arrayIndex);
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
                LogGpuResource(allocation.handle, gpuHandle);
#endif
                CreateGpuResource(allocation, arrayIndex, store.handleType, pendingRes);
                store.gpuHandles[arrayIndex] = gpuHandle;
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
                store.debugTagAllocations.push_back(allocation.handle);
#endif
            } else if (allocation.allocType == AllocType::DEALLOC) {
                if (RenderHandleUtil::IsDeferredDestroy(allocation.handle) && (!isFrameEnd)) {
                    // push deferred destroys back to wait for the end of the frame destruction
                    store.pendingData.allocations.push_back(allocation);
                } else {
                    store.availableHandleIds.push_back(allocation.handle.id);
                    DestroyGpuResource(allocation, arrayIndex, store.handleType, store);
                }
            }
            // there might be undefined type, e.g. for shallow handles

            // render graph frame state reset for trackable and not auto reset frame states
            // with alloc there might be a replace which needs this as well as with dealloc
            if (RenderHandleUtil::IsDynamicResource(allocation.handle) &&
                (!RenderHandleUtil::IsResetOnFrameBorders(allocation.handle))) {
                PLUGIN_ASSERT((store.handleType == RenderHandleType::GPU_BUFFER) ||
                              (store.handleType == RenderHandleType::GPU_IMAGE));
                clientHandleStateDestroy_.resources.push_back(allocation.handle);
            }
        }

        // inside mutex (calls device)
        store.mgr->HandlePendingDeallocations();

        allocationMutex_.unlock();
        // although the pendingData was moved and doesn't need locks anymore, other parts of the store are modified
        // as well so we need to hold the lock until here.
        store.clientMutex.unlock();

        if (store.handleType == RenderHandleType::GPU_IMAGE) {
            // no lock needed for gpuHandles access
            HandlePendingRemappings(pendingRemaps, store.gpuHandles);
        }
    };

    handleStorePendingAllocations(bufferStore_, isFrameEnd);
    handleStorePendingAllocations(imageStore_, isFrameEnd);
    handleStorePendingAllocations(samplerStore_, isFrameEnd);

#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
    ProcessDebugTags();
#endif
}

void GpuResourceManager::HandlePendingAllocations()
{
    HandlePendingAllocationsImpl(false);
}

void GpuResourceManager::EndFrame()
{
    DestroyFrameStaging();
    HandlePendingAllocationsImpl(true);
}

void GpuResourceManager::RenderBackendImmediateRemapGpuImageHandle(
    const RenderHandle& clientHandle, const RenderHandle& clientHandleGpuResource)
{
    const uint32_t clientArrayIndex = RenderHandleUtil::GetIndexPart(clientHandle);
    const uint32_t clientResourceArrayIndex = RenderHandleUtil::GetIndexPart(clientHandleGpuResource);
    const bool areGpuImages = (RenderHandleUtil::GetHandleType(clientHandle) == RenderHandleType::GPU_IMAGE) &&
                              (RenderHandleUtil::GetHandleType(clientHandleGpuResource) == RenderHandleType::GPU_IMAGE);
    if (areGpuImages) {
        PerManagerStore& store = imageStore_;
        auto const clientLock = std::lock_guard(store.clientMutex);

        if ((clientArrayIndex < store.gpuHandles.size()) && (clientResourceArrayIndex < store.gpuHandles.size())) {
            // NOTE: old owned gpu resource should be destroyed
            PLUGIN_ASSERT(RenderHandleUtil::IsValid(store.gpuHandles[clientResourceArrayIndex]));

            store.gpuHandles[clientArrayIndex] = store.gpuHandles[clientResourceArrayIndex];
            store.descriptions[clientArrayIndex] = store.descriptions[clientResourceArrayIndex];
        } else {
            PLUGIN_LOG_E("invalid backend gpu image remapping indices");
        }
    } else {
        PLUGIN_LOG_E("invalid backend gpu image remapping handles");
    }
}

void GpuResourceManager::LockFrameStagingData()
{
    {
        std::lock_guard lock(stagingMutex_);
        perFrameStagingData_ = move(stagingOperations_);
    }

    // create frame staging buffers and set handles for staging
    {
        std::lock_guard lock(bufferStore_.clientMutex);

        perFrameStagingBuffers_.reserve(
            perFrameStagingData_.bufferToBuffer.size() + perFrameStagingData_.bufferToImage.size());
        for (auto& ref : perFrameStagingData_.bufferToBuffer) {
            if ((!ref.invalidOperation) && (ref.stagingBufferByteSize > 0)) {
                perFrameStagingBuffers_.push_back(CreateStagingBuffer(GetStagingBufferDesc(ref.stagingBufferByteSize)));
                ref.srcHandle = perFrameStagingBuffers_.back();
            }
        }
        for (auto& ref : perFrameStagingData_.bufferToImage) {
            if ((!ref.invalidOperation) && (ref.stagingBufferByteSize > 0)) {
                perFrameStagingBuffers_.push_back(CreateStagingBuffer(GetStagingBufferDesc(ref.stagingBufferByteSize)));
                ref.srcHandle = perFrameStagingBuffers_.back();
            }
        }
    }

    // create image scaling targets and set handles
    perFrameStagingScalingImages_.resize(perFrameStagingData_.scalingImageData.scalingImages.size());
    for (size_t idx = 0; idx < perFrameStagingData_.scalingImageData.scalingImages.size(); ++idx) {
        auto& scalingImageRef = perFrameStagingData_.scalingImageData.scalingImages[idx];
        perFrameStagingScalingImages_[idx] = Create(
            GetStagingScalingImageDesc(scalingImageRef.format, scalingImageRef.maxWidth, scalingImageRef.maxHeight));
        scalingImageRef.handle = perFrameStagingScalingImages_[idx];
    }
}

void GpuResourceManager::DestroyFrameStaging()
{
    // explicit destruction of staging resources
    {
        PerManagerStore& store = bufferStore_;
        auto const clientLock = std::lock_guard(store.clientMutex);
        for (const auto& handleRef : perFrameStagingBuffers_) {
            Destroy(store, handleRef.GetHandle());
        }
        perFrameStagingBuffers_.clear();
    }
    {
        PerManagerStore& store = imageStore_;
        auto const clientLock = std::lock_guard(store.clientMutex);
        for (const auto& handleRef : perFrameStagingScalingImages_) {
            Destroy(store, handleRef.GetHandle());
        }
        perFrameStagingScalingImages_.clear();
    }
}

bool GpuResourceManager::HasStagingData() const
{
    if (perFrameStagingData_.bufferToBuffer.empty() && perFrameStagingData_.bufferToImage.empty() &&
        perFrameStagingData_.imageToBuffer.empty() && perFrameStagingData_.imageToImage.empty() &&
        perFrameStagingData_.cpuToBuffer.empty() && perFrameStagingData_.bufferCopies.empty() &&
        perFrameStagingData_.bufferImageCopies.empty() && perFrameStagingData_.imageCopies.empty()) {
        return false;
    } else {
        return true;
    }
}

StagingConsumeStruct GpuResourceManager::ConsumeStagingData()
{
    StagingConsumeStruct staging = move(perFrameStagingData_);
    return staging;
}

GpuResourceManager::StateDestroyConsumeStruct GpuResourceManager::ConsumeStateDestroyData()
{
    StateDestroyConsumeStruct srcs = move(clientHandleStateDestroy_);
    return srcs;
}

void* GpuResourceManager::MapBuffer(const RenderHandle& handle) const
{
    if (GpuBuffer* buffer = GetBuffer(handle); buffer) {
        return buffer->Map();
    } else {
        return nullptr;
    }
}

void* GpuResourceManager::MapBuffer(const RenderHandleReference& handle) const
{
    return MapBuffer(handle.GetHandle());
}

void* GpuResourceManager::MapBufferMemory(const RenderHandle& handle) const
{
    const bool isOutsideRendererMappable = RenderHandleUtil::IsMappableOutsideRenderer(handle);
    void* data = nullptr;
    if (isOutsideRendererMappable) {
        const bool isCreatedImmediate = RenderHandleUtil::IsImmediatelyCreated(handle);
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        const auto clientLock = std::lock_guard(bufferStore_.clientMutex);
        if (isCreatedImmediate && (arrayIndex < static_cast<uint32_t>(bufferStore_.clientHandles.size()))) {
#if (RENDER_VALIDATION_ENABLED == 1)
            if (RenderHandleUtil::GetGenerationIndexPart(bufferStore_.clientHandles[arrayIndex].GetHandle()) !=
                RenderHandleUtil::GetGenerationIndexPart(handle)) {
                const string name = GetName(bufferStore_.clientHandles[arrayIndex].GetHandle());
                PLUGIN_LOG_E("RENDER_VALIDATION: client handle is not current generation (name%s)", name.c_str());
            }
            if (!bufferStore_.additionalData[arrayIndex].resourcePtr) {
                PLUGIN_LOG_E("RENDER_VALIDATION: invalid pointer with mappable GPU buffer MapBufferMemory");
            }
#endif
            if (bufferStore_.additionalData[arrayIndex].resourcePtr) {
                data = (reinterpret_cast<GpuBuffer*>(bufferStore_.additionalData[arrayIndex].resourcePtr))->MapMemory();
            }
        }
    } else if (GpuBuffer* buffer = GetBuffer(handle); buffer) {
        data = buffer->MapMemory();
    }
    return data;
}

void* GpuResourceManager::MapBufferMemory(const RenderHandleReference& handle) const
{
    return MapBufferMemory(handle.GetHandle());
}

void GpuResourceManager::UnmapBuffer(const RenderHandle& handle) const
{
    const bool isOutsideRendererMappable = RenderHandleUtil::IsMappableOutsideRenderer(handle);
    if (isOutsideRendererMappable) {
        const bool isCreatedImmediate = RenderHandleUtil::IsImmediatelyCreated(handle);
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        const auto clientLock = std::lock_guard(bufferStore_.clientMutex);
        if (isCreatedImmediate && (arrayIndex < static_cast<uint32_t>(bufferStore_.clientHandles.size()))) {
#if (RENDER_VALIDATION_ENABLED == 1)
            if (RenderHandleUtil::GetGenerationIndexPart(bufferStore_.clientHandles[arrayIndex].GetHandle()) !=
                RenderHandleUtil::GetGenerationIndexPart(handle)) {
                const string name = GetName(bufferStore_.clientHandles[arrayIndex].GetHandle());
                PLUGIN_LOG_E("RENDER_VALIDATION: client handle is not current generation (name%s)", name.c_str());
            }
            if (bufferStore_.additionalData[arrayIndex].resourcePtr) {
                (reinterpret_cast<GpuBuffer*>(bufferStore_.additionalData[arrayIndex].resourcePtr))->Unmap();
            } else {
                PLUGIN_LOG_E("RENDER_VALIDATION: invalid pointer with mappable GPU buffer UnmapBuffer");
            }
#endif
        }
    } else if (const GpuBuffer* buffer = GetBuffer(handle); buffer) {
        buffer->Unmap();
    }
}

void GpuResourceManager::UnmapBuffer(const RenderHandleReference& handle) const
{
    return UnmapBuffer(handle.GetHandle());
}

// must be locked when called
GpuResourceManager::PendingData GpuResourceManager::CommitPendingData(PerManagerStore& store)
{
    return { move(store.pendingData.allocations), move(store.pendingData.buffers), move(store.pendingData.images),
        move(store.pendingData.remaps) };
}

void GpuResourceManager::DebugPrintValidCounts()
{
#if (RENDER_VALIDATION_ENABLED == 1)
    PLUGIN_LOG_D("GPU buffer count: %u", static_cast<uint32_t>(gpuBufferMgr_->GetValidResourceCount()));
    PLUGIN_LOG_D("GPU image count: %u", static_cast<uint32_t>(gpuImageMgr_->GetValidResourceCount()));
    PLUGIN_LOG_D("GPU sampler count: %u", static_cast<uint32_t>(gpuSamplerMgr_->GetValidResourceCount()));
#endif
}

void GpuResourceManager::WaitForIdleAndDestroyGpuResources()
{
    PLUGIN_LOG_D("WFIADGR thread id: %" PRIu64, (uint64_t)std::hash<std::thread::id> {}(std::this_thread::get_id()));
    device_.Activate();
    device_.WaitForIdle();

    // 1. immediate Destroy all the handles that are to be destroyed
    // 2. throw away everything else that's pending
    auto DestroyPendingData = [this](PerManagerStore& store) {
        const auto lock = std::lock_guard(store.clientMutex);
        const auto allocLock = std::lock_guard(allocationMutex_);

        auto pd = CommitPendingData(store);
        if (store.clientHandles.size() > store.gpuHandles.size()) {
            store.gpuHandles.resize(store.clientHandles.size());
            store.mgr->Resize(store.clientHandles.size());
        }

#if (RENDER_VALIDATION_ENABLED == 1)
        uint32_t dc = 0;
#endif
        for (const auto& ref : pd.allocations) {
            if (ref.allocType == AllocType::DEALLOC) {
                store.availableHandleIds.push_back(ref.handle.id);
                DestroyImmediate(store, ref.handle);
                // render graph frame state reset for trackable and not auto reset frame states
                if (RenderHandleUtil::IsDynamicResource(ref.handle) &&
                    (!RenderHandleUtil::IsResetOnFrameBorders(ref.handle))) {
                    PLUGIN_ASSERT((store.handleType == RenderHandleType::GPU_BUFFER) ||
                                  (store.handleType == RenderHandleType::GPU_IMAGE));
                    clientHandleStateDestroy_.resources.push_back(ref.handle);
                }
#if (RENDER_VALIDATION_ENABLED == 1)
                dc++;
            } else if (ref.allocType == AllocType::REMOVED) {
                dc++;
#endif
            }
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_D("WFIADGR: d: %u r (type:%u)", dc, uint32_t(store.handleType));
        PLUGIN_LOG_D("WFIADGR: pa cl: %u (t:%u)", (uint32_t)pd.allocations.size() - dc, uint32_t(store.handleType));
#endif

        // inside mutex (calls device)
        store.mgr->HandlePendingDeallocationsImmediate();
    };

    DestroyPendingData(bufferStore_);
    DestroyPendingData(imageStore_);
    DestroyPendingData(samplerStore_);

    // make sure that all staging resources are forcefully destroyed
    LockFrameStagingData();
    ConsumeStagingData(); // consume cpu data
    DestroyFrameStaging();

    {
        // additional possible staging buffer clean-up
        auto& store = bufferStore_;
        const auto lock = std::lock_guard(store.clientMutex);
        const auto allocLock = std::lock_guard(allocationMutex_);
        store.mgr->HandlePendingDeallocationsImmediate();
    }

    DebugPrintValidCounts();

    device_.Deactivate();
}

EngineResourceHandle GpuResourceManager::GetGpuHandle(
    const PerManagerStore& store, const RenderHandle& clientHandle) const
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(clientHandle);
    if (arrayIndex < (uint32_t)store.gpuHandles.size()) {
#if (RENDER_VALIDATION_ENABLED == 1)
        if (RenderHandleUtil::GetGenerationIndexPart(store.clientHandles[arrayIndex].GetHandle()) !=
            RenderHandleUtil::GetGenerationIndexPart(clientHandle)) {
            const string name = GetName(store.clientHandles[arrayIndex].GetHandle());
            PLUGIN_LOG_E("RENDER_VALIDATION: client handle is not current generation (name%s)", name.c_str());
        }
        if (!RenderHandleUtil::IsValid(store.gpuHandles[arrayIndex])) {
            PLUGIN_LOG_E("RENDER_VALIDATION : invalid gpu handle %" PRIx64, clientHandle.id);
        }
#endif
        return store.gpuHandles[arrayIndex];
    } else {
        PLUGIN_LOG_E("No gpu resource handle for client handle : %" PRIx64, clientHandle.id);
        return EngineResourceHandle {};
    }
}

EngineResourceHandle GpuResourceManager::GetGpuHandle(const RenderHandle& clientHandle) const
{
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(clientHandle);
    if (handleType == RenderHandleType::GPU_BUFFER) {
        return GetGpuHandle(bufferStore_, clientHandle);
    } else if (handleType == RenderHandleType::GPU_IMAGE) {
        return GetGpuHandle(imageStore_, clientHandle);
    } else if (handleType == RenderHandleType::GPU_SAMPLER) {
        return GetGpuHandle(samplerStore_, clientHandle);
    } else {
        PLUGIN_LOG_E("invalid gpu resource handle : %" PRIx64, clientHandle.id);
        return {};
    }
}

GpuBuffer* GpuResourceManager::GetBuffer(const RenderHandle& handle) const
{
    const EngineResourceHandle resHandle = GetGpuHandle(bufferStore_, handle);
    return gpuBufferMgr_->Get(RenderHandleUtil::GetIndexPart(resHandle));
}

GpuImage* GpuResourceManager::GetImage(const RenderHandle& handle) const
{
    const EngineResourceHandle resHandle = GetGpuHandle(imageStore_, handle);
    return gpuImageMgr_->Get(RenderHandleUtil::GetIndexPart(resHandle));
}

GpuSampler* GpuResourceManager::GetSampler(const RenderHandle& handle) const
{
    const EngineResourceHandle resHandle = GetGpuHandle(samplerStore_, handle);
    return gpuSamplerMgr_->Get(RenderHandleUtil::GetIndexPart(resHandle));
}

uint32_t GpuResourceManager::GetBufferHandleCount() const
{
    return static_cast<uint32_t>(bufferStore_.gpuHandles.size());
}

uint32_t GpuResourceManager::GetImageHandleCount() const
{
    return static_cast<uint32_t>(imageStore_.gpuHandles.size());
}

RenderHandle GpuResourceManager::CreateClientHandle(const RenderHandleType type,
    const ResourceDescriptor& resourceDescriptor, const uint64_t handleId, const uint32_t hasNameId,
    const uint32_t additionalInfoFlags)
{
    RenderHandle handle;

    const uint32_t index = RenderHandleUtil::GetIndexPart(handleId);
    const uint32_t generationIndex = RenderHandleUtil::GetGenerationIndexPart(handleId) + 1; // next gen
    if (type == RenderHandleType::GPU_BUFFER) {
        // NOTE: additional flags for GPU acceleration structure might be needed
        const auto& rd = resourceDescriptor.combinedBufDescriptor.bufferDesc;
        RenderHandleInfoFlags infoFlags = (rd.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS)
                                              ? CORE_RESOURCE_HANDLE_DYNAMIC_TRACK
                                              : 0u;
        infoFlags |= (rd.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE)
                         ? CORE_RESOURCE_HANDLE_IMMEDIATELY_CREATED
                         : 0;
        infoFlags |= (rd.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_DEFERRED_DESTROY)
                         ? CORE_RESOURCE_HANDLE_DEFERRED_DESTROY
                         : 0;
        infoFlags |= (rd.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER)
                         ? CORE_RESOURCE_HANDLE_MAP_OUTSIDE_RENDERER
                         : 0;
        infoFlags |= additionalInfoFlags;
        handle = RenderHandleUtil::CreateGpuResourceHandle(type, infoFlags, index, generationIndex, hasNameId);
    } else if (type == RenderHandleType::GPU_IMAGE) {
        const auto& rd = resourceDescriptor.imageDescriptor;
        RenderHandleInfoFlags infoFlags {};
        infoFlags |= (rd.engineCreationFlags & CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS)
                         ? CORE_RESOURCE_HANDLE_DYNAMIC_TRACK
                         : 0u;
        infoFlags |= (rd.engineCreationFlags & CORE_ENGINE_IMAGE_CREATION_DEFERRED_DESTROY)
                         ? CORE_RESOURCE_HANDLE_DEFERRED_DESTROY
                         : 0;
        infoFlags |= ((rd.engineCreationFlags & CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS) &&
                         (resourceDescriptor.imageDescriptor.mipCount > 1U))
                         ? CORE_RESOURCE_HANDLE_DYNAMIC_ADDITIONAL_STATE
                         : 0u;
        // force transient attachments to be state reset on frame borders
        infoFlags |= ((rd.engineCreationFlags & CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS) ||
                         (rd.usageFlags & CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT))
                         ? CORE_RESOURCE_HANDLE_RESET_ON_FRAME_BORDERS
                         : 0u;
        infoFlags |= GetAdditionalImageFlagsFromFormat(rd.format);
        infoFlags |= additionalInfoFlags;
        handle = RenderHandleUtil::CreateGpuResourceHandle(type, infoFlags, index, generationIndex, hasNameId);
    } else if (type == RenderHandleType::GPU_SAMPLER) {
        handle = RenderHandleUtil::CreateGpuResourceHandle(type, 0, index, generationIndex, hasNameId);
    }
    return handle;
}

// needs to be locked when called
uint64_t GpuResourceManager::GetNextAvailableHandleId(PerManagerStore& store)
{
    uint64_t handleId = INVALID_RESOURCE_HANDLE;
    auto& availableHandleIds = store.availableHandleIds;
    if (!availableHandleIds.empty()) {
        handleId = availableHandleIds.back();
        availableHandleIds.pop_back();
    } else {
        handleId = static_cast<uint32_t>(store.clientHandles.size())
                   << RenderHandleUtil::RES_HANDLE_ID_SHIFT; // next idx
    }
    return handleId;
}

// needs to be locked when called
// staging cannot be locked when called
GpuResourceManager::StoreAllocationData GpuResourceManager::StoreAllocation(
    PerManagerStore& store, const StoreAllocationInfo& info)
{
    // NOTE: PerManagerStore gpu handles cannot be touched here
    StoreAllocationData data;

    // there cannot be both (valid name and a valid replaced handle)
    const uint32_t replaceArrayIndex = RenderHandleUtil::GetIndexPart(info.replacedHandle);
    bool hasReplaceHandle = (replaceArrayIndex < (uint32_t)store.clientHandles.size());
    const uint32_t hasNameId = (!info.name.empty()) ? 1u : 0u;
    if (hasReplaceHandle) {
        data.handle = store.clientHandles[replaceArrayIndex];
        // NOTE: should be documented better, and prevented
        PLUGIN_ASSERT_MSG(!RenderHandleUtil::IsDeferredDestroy(data.handle.GetHandle()),
            "deferred desctruction resources cannot be replaced");
        if (!RenderHandleUtil::IsValid(data.handle.GetHandle())) {
#if (RENDER_VALIDATION_ENABLED == 1)
            PLUGIN_LOG_E("RENDER_VALIDATION: invalid replaced handle given to GPU resource manager, creating new");
#endif
            const uint64_t handleId = GetNextAvailableHandleId(store);
            data.handle = RenderHandleReference(
                CreateClientHandle(info.type, info.descriptor, handleId, hasNameId, info.addHandleFlags),
                IRenderReferenceCounter::Ptr(new RenderReferenceCounter()));
            hasReplaceHandle = false;
            if (hasNameId) {
                // NOTE: should remove old name if it was in use
                store.nameToClientIndex[info.name] = RenderHandleUtil::GetIndexPart(data.handle.GetHandle());
            }
        }
    } else if (hasNameId != 0u) {
        if (auto const iter = store.nameToClientIndex.find(info.name); iter != store.nameToClientIndex.cend()) {
            PLUGIN_ASSERT(iter->second < static_cast<uint32_t>(store.clientHandles.size()));
            data.handle = store.clientHandles[iter->second];
            PLUGIN_ASSERT_MSG(!RenderHandleUtil::IsDeferredDestroy(data.handle.GetHandle()),
                "deferred desctruction resources cannot be replaced");
        } else {
            const uint64_t handleId = GetNextAvailableHandleId(store);
            data.handle = RenderHandleReference(
                CreateClientHandle(info.type, info.descriptor, handleId, hasNameId, info.addHandleFlags),
                IRenderReferenceCounter::Ptr(new RenderReferenceCounter()));
            store.nameToClientIndex[info.name] = RenderHandleUtil::GetIndexPart(data.handle.GetHandle());
        }
    } else {
        const uint64_t handleId = GetNextAvailableHandleId(store);
        data.handle = RenderHandleReference(
            CreateClientHandle(info.type, info.descriptor, handleId, hasNameId, info.addHandleFlags),
            IRenderReferenceCounter::Ptr(new RenderReferenceCounter()));
    }

    PLUGIN_ASSERT(data.handle);
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(data.handle.GetHandle());
    PLUGIN_ASSERT(store.clientHandles.size() == store.descriptions.size());
    PLUGIN_ASSERT(store.clientHandles.size() == store.additionalData.size());
    if (arrayIndex >= (uint32_t)store.clientHandles.size()) {
        store.clientHandles.push_back(data.handle);
        store.additionalData.push_back({});
        store.descriptions.push_back(info.descriptor);
    } else {
        store.clientHandles[arrayIndex] = data.handle;
        // store.additionalData[arrayIndex] cannot be cleared here (staging cleaned based on this)
        store.descriptions[arrayIndex] = info.descriptor;
    }

#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
    PLUGIN_LOG_E("gpu resource allocation %" PRIx64 " (name: %s)", data.handle.GetHandle().id, info.name.data());
#endif

    // store allocation for GPU allocation

    // needs to find the allocation and replace
    if (hasReplaceHandle || (hasNameId != 0)) {
        if (const uint32_t pendingArrIndex = store.additionalData[arrayIndex].indexToPendingData;
            (pendingArrIndex != INVALID_PENDING_INDEX) && (pendingArrIndex < store.pendingData.allocations.size())) {
            data.allocationIndex = pendingArrIndex;
        }
    }
    if (data.allocationIndex == ~0u) {
        data.allocationIndex = store.pendingData.allocations.size();
        store.additionalData[arrayIndex].indexToPendingData = static_cast<uint32_t>(data.allocationIndex);
        store.pendingData.allocations.emplace_back(
            data.handle.GetHandle(), info.descriptor, info.allocType, info.optResourceIndex);
    } else { // quite rare case and slow path
        // replace this frame's allocation
        auto& allocOp = store.pendingData.allocations[data.allocationIndex];
        // invalid flag should be only needed and all the allocations would be done later
        // i.e. create staging buffers and fetch based on index in render node staging
        RemoveStagingOperations(allocOp);
        store.pendingData.allocations[data.allocationIndex] = { data.handle.GetHandle(), info.descriptor,
            info.allocType, info.optResourceIndex };
    }

    return data;
}

bool GpuResourceManager::IsGpuBuffer(const RenderHandleReference& handle) const
{
    return IsGpuBuffer(handle.GetHandle());
}

bool GpuResourceManager::IsGpuImage(const RenderHandleReference& handle) const
{
    return IsGpuImage(handle.GetHandle());
}

bool GpuResourceManager::IsGpuSampler(const RenderHandleReference& handle) const
{
    return IsGpuSampler(handle.GetHandle());
}

bool GpuResourceManager::IsGpuAccelerationStructure(const RenderHandleReference& handle) const
{
    return IsGpuAccelerationStructure(handle.GetHandle());
}

bool GpuResourceManager::IsSwapchain(const RenderHandleReference& handle) const
{
    return IsSwapchain(handle.GetHandle());
}

bool GpuResourceManager::IsMappableOutsideRender(const RenderHandleReference& handle) const
{
    return RenderHandleUtil::IsMappableOutsideRenderer(handle.GetHandle());
}

bool GpuResourceManager::IsGpuBuffer(const RenderHandle& handle) const
{
    return RenderHandleUtil::IsGpuBuffer(handle);
}

bool GpuResourceManager::IsGpuImage(const RenderHandle& handle) const
{
    return RenderHandleUtil::IsGpuImage(handle);
}

bool GpuResourceManager::IsGpuSampler(const RenderHandle& handle) const
{
    return RenderHandleUtil::IsGpuSampler(handle);
}

bool GpuResourceManager::IsGpuAccelerationStructure(const RenderHandle& handle) const
{
    return RenderHandleUtil::IsGpuAccelerationStructure(handle);
}

bool GpuResourceManager::IsSwapchain(const RenderHandle& handle) const
{
    return RenderHandleUtil::IsSwapchain(handle);
}

bool GpuResourceManager::IsValid(const RenderHandle& handle) const
{
    return RenderHandleUtil::IsValid(handle);
}

FormatProperties GpuResourceManager::GetFormatProperties(const RenderHandle& handle) const
{
    const RenderHandleType type = RenderHandleUtil::GetHandleType(handle);
    Format format = Format::BASE_FORMAT_UNDEFINED;
    if (type == RenderHandleType::GPU_BUFFER) {
        const GpuBufferDesc desc = GetBufferDescriptor(handle);
        format = desc.format;
    } else if (type == RenderHandleType::GPU_IMAGE) {
        const GpuImageDesc desc = GetImageDescriptor(handle);
        format = desc.format;
    }
    return device_.GetFormatProperties(format);
}

FormatProperties GpuResourceManager::GetFormatProperties(const RenderHandleReference& handle) const
{
    return GetFormatProperties(handle.GetHandle());
}

FormatProperties GpuResourceManager::GetFormatProperties(const Format format) const
{
    return device_.GetFormatProperties(format);
}

ImageAspectFlags GpuResourceManager::GetImageAspectFlags(const RenderHandleReference& handle) const
{
    return GetImageAspectFlags(handle.GetHandle());
}

ImageAspectFlags GpuResourceManager::GetImageAspectFlags(const RenderHandle& handle) const
{
    const RenderHandleType type = RenderHandleUtil::GetHandleType(handle);
    Format format = Format::BASE_FORMAT_UNDEFINED;
    if (type == RenderHandleType::GPU_BUFFER) {
        const GpuBufferDesc desc = GetBufferDescriptor(handle);
        format = desc.format;
    } else if (type == RenderHandleType::GPU_IMAGE) {
        const GpuImageDesc desc = GetImageDescriptor(handle);
        format = desc.format;
    }
    return GetImageAspectFlags(format);
}

ImageAspectFlags GpuResourceManager::GetImageAspectFlags(const BASE_NS::Format format) const
{
    ImageAspectFlags flags {};
    const bool isDepthFormat = ((format == BASE_FORMAT_D16_UNORM) || (format == BASE_FORMAT_X8_D24_UNORM_PACK32) ||
                                   (format == BASE_FORMAT_D32_SFLOAT) || (format == BASE_FORMAT_D16_UNORM_S8_UINT) ||
                                   (format == BASE_FORMAT_D24_UNORM_S8_UINT))
                                   ? true
                                   : false;
    if (isDepthFormat) {
        flags |= ImageAspectFlagBits::CORE_IMAGE_ASPECT_DEPTH_BIT;

        const bool isStencilFormat =
            ((format == BASE_FORMAT_S8_UINT) || (format == BASE_FORMAT_D16_UNORM_S8_UINT) ||
                (format == BASE_FORMAT_D24_UNORM_S8_UINT) || (format == BASE_FORMAT_D32_SFLOAT_S8_UINT))
                ? true
                : false;
        if (isStencilFormat) {
            flags |= ImageAspectFlagBits::CORE_IMAGE_ASPECT_STENCIL_BIT;
        }

    } else if (format == BASE_FORMAT_S8_UINT) {
        flags |= ImageAspectFlagBits::CORE_IMAGE_ASPECT_STENCIL_BIT;
    } else if (format != BASE_FORMAT_UNDEFINED) {
        flags |= ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT;
    }

    return flags;
}

GpuImageDesc GpuResourceManager::CreateGpuImageDesc(const CORE_NS::IImageContainer::ImageDesc& desc) const
{
    GpuImageDesc gpuImageDesc;
    // default values for loaded images
    gpuImageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
    gpuImageDesc.usageFlags |= CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
    gpuImageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (desc.imageFlags & IImageContainer::ImageFlags::FLAGS_CUBEMAP_BIT) {
        gpuImageDesc.createFlags |= ImageCreateFlagBits::CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }
    if ((desc.imageFlags & IImageContainer::ImageFlags::FLAGS_REQUESTING_MIPMAPS_BIT) && (desc.mipCount > 1)) {
        gpuImageDesc.engineCreationFlags |= EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS;
        gpuImageDesc.usageFlags |= CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    gpuImageDesc.imageType = static_cast<RENDER_NS::ImageType>(desc.imageType);
    gpuImageDesc.imageViewType = static_cast<RENDER_NS::ImageViewType>(desc.imageViewType);
    gpuImageDesc.format = desc.format;
    gpuImageDesc.width = desc.width;
    gpuImageDesc.height = desc.height;
    gpuImageDesc.depth = desc.depth;
    gpuImageDesc.mipCount = desc.mipCount;
    gpuImageDesc.layerCount = desc.layerCount;
    return gpuImageDesc;
}

IGpuResourceCache& GpuResourceManager::GetGpuResourceCache() const
{
    return *gpuResourceCache_;
}

#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
void GpuResourceManager::ProcessDebugTags()
{
    // NOTE: there's a minor possibility that client data has changed before these are locked
    // but the GPU resource is the correct one
    // define CORE_EXTENT_DEBUG_GPU_RESOURCE_MGR_NAMES for more precise debugging
#if defined(CORE_EXTENT_DEBUG_GPU_RESOURCE_MGR_NAMES)
    const auto frameIdx = to_string(device_.GetFrameCount());
#endif
    auto AddDebugTags = [&](PerManagerStore& store, const RenderHandleType handleType) {
        const auto lock = std::lock_guard(store.clientMutex);
        vector<RenderHandle> allocs = move(store.debugTagAllocations);
        for (const auto& handle : allocs) {
            if (RenderHandleUtil::GetHasNamePart(handle) == 0) {
                continue;
            }
            const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
            string name = GetName(store.clientHandles[arrayIndex].GetHandle());
#if defined(CORE_EXTENT_DEBUG_GPU_RESOURCE_MGR_NAMES)
            const auto extentName =
                "_chandle:" + to_string(handle.id) + "_idx:" + to_string(arrayIndex) + "_fr:" + frameIdx;
            name += extentName;
#endif
            if (handleType == RenderHandleType::GPU_BUFFER) {
                GpuResourceUtil::DebugBufferName(device_, *gpuBufferMgr_->Get(arrayIndex), name);
            } else if (handleType == RenderHandleType::GPU_IMAGE) {
                GpuResourceUtil::DebugImageName(device_, *gpuImageMgr_->Get(arrayIndex), name);
            } else if (handleType == RenderHandleType::GPU_SAMPLER) {
                GpuResourceUtil::DebugSamplerName(device_, *gpuSamplerMgr_->Get(arrayIndex), name);
            }
        }
    };
    AddDebugTags(bufferStore_, RenderHandleType::GPU_BUFFER);
    AddDebugTags(imageStore_, RenderHandleType::GPU_IMAGE);
    AddDebugTags(samplerStore_, RenderHandleType::GPU_SAMPLER);
}
#endif

RenderNodeGpuResourceManager::RenderNodeGpuResourceManager(GpuResourceManager& gpuResourceManager)
    : gpuResourceMgr_(gpuResourceManager)
{}

RenderNodeGpuResourceManager::~RenderNodeGpuResourceManager() {}

RenderHandleReference RenderNodeGpuResourceManager::Get(const RenderHandle& handle) const
{
    return gpuResourceMgr_.Get(handle);
}

RenderHandleReference RenderNodeGpuResourceManager::Create(const GpuBufferDesc& desc)
{
    return gpuResourceMgr_.Create(desc);
}

RenderHandleReference RenderNodeGpuResourceManager::Create(const string_view name, const GpuBufferDesc& desc)
{
    return gpuResourceMgr_.Create(name, desc);
}

RenderHandleReference RenderNodeGpuResourceManager::Create(
    const RenderHandleReference& handle, const GpuBufferDesc& desc)
{
    return gpuResourceMgr_.Create(handle, desc);
}

RenderHandleReference RenderNodeGpuResourceManager::Create(
    const string_view name, const GpuBufferDesc& desc, const array_view<const uint8_t> data)
{
    return gpuResourceMgr_.Create(name, desc, data);
}

RenderHandleReference RenderNodeGpuResourceManager::Create(const GpuImageDesc& desc)
{
    return gpuResourceMgr_.Create(desc);
}

RenderHandleReference RenderNodeGpuResourceManager::Create(const string_view name, const GpuImageDesc& desc)
{
    return gpuResourceMgr_.Create(name, desc);
}

RenderHandleReference RenderNodeGpuResourceManager::Create(
    const RenderHandleReference& handle, const GpuImageDesc& desc)
{
    return gpuResourceMgr_.Create(handle, desc);
}

RenderHandleReference RenderNodeGpuResourceManager::Create(
    const string_view name, const GpuImageDesc& desc, const array_view<const uint8_t> data)
{
    return gpuResourceMgr_.Create(name, desc, data);
}

RenderHandleReference RenderNodeGpuResourceManager::Create(const string_view name, const GpuSamplerDesc& desc)
{
    return gpuResourceMgr_.Create(name, desc);
}

RenderHandleReference RenderNodeGpuResourceManager::Create(
    const RenderHandleReference& handle, const GpuSamplerDesc& desc)
{
    return gpuResourceMgr_.Create(handle, desc);
}

RenderHandleReference RenderNodeGpuResourceManager::Create(const GpuSamplerDesc& desc)
{
    return gpuResourceMgr_.Create(desc);
}

RenderHandleReference RenderNodeGpuResourceManager::Create(
    const string_view name, const GpuAccelerationStructureDesc& desc)
{
    return gpuResourceMgr_.Create(name, desc);
}

RenderHandleReference RenderNodeGpuResourceManager::Create(
    const RenderHandleReference& handle, const GpuAccelerationStructureDesc& desc)
{
    return gpuResourceMgr_.Create(handle, desc);
}

RenderHandleReference RenderNodeGpuResourceManager::Create(const GpuAccelerationStructureDesc& desc)
{
    return gpuResourceMgr_.Create(desc);
}

RenderHandle RenderNodeGpuResourceManager::GetBufferHandle(const string_view name) const
{
    return gpuResourceMgr_.GetBufferRawHandle(name);
}

RenderHandle RenderNodeGpuResourceManager::GetImageHandle(const string_view name) const
{
    return gpuResourceMgr_.GetImageRawHandle(name);
}

RenderHandle RenderNodeGpuResourceManager::GetSamplerHandle(const string_view name) const
{
    return gpuResourceMgr_.GetSamplerRawHandle(name);
}

GpuBufferDesc RenderNodeGpuResourceManager::GetBufferDescriptor(const RenderHandle& handle) const
{
    return gpuResourceMgr_.GetBufferDescriptor(handle);
}

GpuImageDesc RenderNodeGpuResourceManager::GetImageDescriptor(const RenderHandle& handle) const
{
    return gpuResourceMgr_.GetImageDescriptor(handle);
}

GpuSamplerDesc RenderNodeGpuResourceManager::GetSamplerDescriptor(const RenderHandle& handle) const
{
    return gpuResourceMgr_.GetSamplerDescriptor(handle);
}

GpuAccelerationStructureDesc RenderNodeGpuResourceManager::GetAccelerationStructureDescriptor(
    const RenderHandle& handle) const
{
    return gpuResourceMgr_.GetAccelerationStructureDescriptor(handle);
}

bool RenderNodeGpuResourceManager::HasStagingData() const
{
    return gpuResourceMgr_.HasStagingData();
}

StagingConsumeStruct RenderNodeGpuResourceManager::ConsumeStagingData()
{
    return gpuResourceMgr_.ConsumeStagingData();
}

void* RenderNodeGpuResourceManager::MapBuffer(const RenderHandle& handle) const
{
    return gpuResourceMgr_.MapBuffer(handle);
}

void* RenderNodeGpuResourceManager::MapBufferMemory(const RenderHandle& handle) const
{
    return gpuResourceMgr_.MapBufferMemory(handle);
}

void RenderNodeGpuResourceManager::UnmapBuffer(const RenderHandle& handle) const
{
    gpuResourceMgr_.UnmapBuffer(handle);
}

GpuResourceManager& RenderNodeGpuResourceManager::GetGpuResourceManager()
{
    return gpuResourceMgr_;
}

bool RenderNodeGpuResourceManager::IsValid(const RenderHandle& handle) const
{
    return RenderHandleUtil::IsValid(handle);
}

bool RenderNodeGpuResourceManager::IsGpuBuffer(const RenderHandle& handle) const
{
    return RenderHandleUtil::IsGpuBuffer(handle);
}

bool RenderNodeGpuResourceManager::IsGpuImage(const RenderHandle& handle) const
{
    return RenderHandleUtil::IsGpuImage(handle);
}

bool RenderNodeGpuResourceManager::IsGpuSampler(const RenderHandle& handle) const
{
    return RenderHandleUtil::IsGpuSampler(handle);
}

bool RenderNodeGpuResourceManager::IsGpuAccelerationStructure(const RenderHandle& handle) const
{
    return RenderHandleUtil::IsGpuAccelerationStructure(handle);
}

bool RenderNodeGpuResourceManager::IsSwapchain(const RenderHandle& handle) const
{
    return RenderHandleUtil::IsSwapchain(handle);
}

FormatProperties RenderNodeGpuResourceManager::GetFormatProperties(const RenderHandle& handle) const
{
    return gpuResourceMgr_.GetFormatProperties(handle);
}

FormatProperties RenderNodeGpuResourceManager::GetFormatProperties(const Format format) const
{
    return gpuResourceMgr_.GetFormatProperties(format);
}

ImageAspectFlags RenderNodeGpuResourceManager::GetImageAspectFlags(const RenderHandle& handle) const
{
    return gpuResourceMgr_.GetImageAspectFlags(handle);
}

ImageAspectFlags RenderNodeGpuResourceManager::GetImageAspectFlags(const Format format) const
{
    return gpuResourceMgr_.GetImageAspectFlags(format);
}

string RenderNodeGpuResourceManager::GetName(const RenderHandle& handle) const
{
    return gpuResourceMgr_.GetName(handle);
}

IGpuResourceCache& RenderNodeGpuResourceManager::GetGpuResourceCache() const
{
    return gpuResourceMgr_.GetGpuResourceCache();
}

RENDER_END_NAMESPACE()
