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

#include "gpu_resource_cache.h"

#include <algorithm>
#include <cinttypes>

#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/math/mathf.h>
#include <render/resource_handle.h>

#include "device/gpu_resource_manager.h"
#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
using namespace BASE_NS;

namespace {
RenderHandleReference CreateImage(GpuResourceManager& gpuResourceMgr, const CacheGpuImageDesc& desc)
{
    constexpr ImageUsageFlags USAGE_FLAGS { ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                            ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT };
    constexpr ImageUsageFlags MSAA_USAGE_FLAGS { ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                 ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT };
    constexpr MemoryPropertyFlags MEMORY_FLAGS { MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    constexpr MemoryPropertyFlags MSAA_MEMORY_FLAGS {
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT
    };
    const ImageUsageFlags usageFlags =
        (desc.sampleCountFlags > SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT) ? MSAA_USAGE_FLAGS : USAGE_FLAGS;
    const MemoryPropertyFlags memoryFlags =
        (desc.sampleCountFlags > SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT) ? MSAA_MEMORY_FLAGS : MEMORY_FLAGS;

    const GpuImageDesc newDesc {
        ImageType::CORE_IMAGE_TYPE_2D,          // imageType
        ImageViewType::CORE_IMAGE_VIEW_TYPE_2D, // imageViewType
        desc.format,                            // format
        ImageTiling::CORE_IMAGE_TILING_OPTIMAL, // imageTiling
        usageFlags,                             // usageFlags
        memoryFlags,                            // memoryPropertyFlags
        0,                                      // createFlags
        EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS |
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS, // engineCreationFlags
        desc.width,                                                                               // width
        desc.height,                                                                              // height
        1,                                                                                        // depth
        desc.mipCount,                                                                            // mipCount
        desc.layerCount,                                                                          // layerCount
        desc.sampleCountFlags,                                                                    // sampleCountFlags
        desc.componentMapping,                                                                    // componentMapping
    };

    return gpuResourceMgr.CreateShallowHandle(newDesc);
}
} // namespace

GpuResourceCache::GpuResourceCache(GpuResourceManager& gpuResourceMgr) : gpuResourceMgr_(gpuResourceMgr) {}

GpuResourceCache::~GpuResourceCache()
{
#if (RENDER_VALIDATION_ENABLED == 1)
    {
        const auto clientLock = std::lock_guard(mutex_);

        uint32_t aliveCounter = 0;
        const uint32_t readIdx = 1u - writeIdx_;
        for (const auto& imagesRef : frameData_[writeIdx_].images) {
            if (imagesRef.handle && (imagesRef.handle.GetRefCount() > 2)) { // 2:count number
                aliveCounter++;
            }
        }
        for (const auto& imagesRef : frameData_[readIdx].images) {
            if (imagesRef.handle && (imagesRef.handle.GetRefCount() > 2)) { // 2: count number
                aliveCounter++;
            }
        }
        if (aliveCounter > 0) {
            PLUGIN_LOG_W("RENDER_VALIDATION: Not all GPU resource cache references released (count: %u)", aliveCounter);
        }
    };
#endif
}

void GpuResourceCache::BeginFrame(const uint64_t frameCount)
{
    const auto lock = std::lock_guard(mutex_);

    // NOTE: does not try to re-use handles
    if (frameCount == frameCounter_) {
        return;
    }
    frameCounter_ = frameCount;
    writeIdx_ = 1u - writeIdx_;

    AllocateAndRemapImages();
    DestroyOldImages();
}

array_view<const GpuResourceCache::ImageData> GpuResourceCache::GetImageData() const
{
    const uint32_t readIdx = 1u - writeIdx_;
    return frameData_[readIdx].images;
}

RenderHandleReference GpuResourceCache::ReserveGpuImageImpl(const CacheGpuImageDesc& desc)
{
    const auto lock = std::lock_guard(mutex_);

    auto& fd = frameData_[writeIdx_];
    fd.images.push_back({ CreateImage(gpuResourceMgr_, desc), HashCacheGpuImageDesc(desc) });
    return fd.images.back().handle;
}

RenderHandleReference GpuResourceCache::ReserveGpuImage(const CacheGpuImageDesc& desc)
{
    return ReserveGpuImageImpl(desc);
}

vector<RenderHandleReference> GpuResourceCache::ReserveGpuImages(const array_view<const CacheGpuImageDesc> descs)
{
    vector<RenderHandleReference> handles(descs.size());
    for (size_t idx = 0; idx < handles.size(); ++idx) {
        handles[idx] = ReserveGpuImageImpl(descs[idx]);
    }
    return handles;
}

CacheGpuImageDesc GpuResourceCache::GetCacheGpuImageDesc(const RenderHandleReference& gpuImageHandle) const
{
    const GpuImageDesc desc = gpuResourceMgr_.GetImageDescriptor(gpuImageHandle);
    return { desc.format, desc.width, desc.height, desc.mipCount, desc.layerCount, desc.sampleCountFlags,
        desc.componentMapping };
}

CacheGpuImagePair GpuResourceCache::ReserveGpuImagePair(
    const CacheGpuImageDesc& desc, const SampleCountFlags sampleCountFlags)
{
    // allocate both handles
    CacheGpuImageDesc secondDesc = desc;
    secondDesc.sampleCountFlags = sampleCountFlags;
    return CacheGpuImagePair { ReserveGpuImageImpl(desc), ReserveGpuImageImpl(secondDesc) };
}

void GpuResourceCache::AllocateAndRemapImages()
{
    const uint32_t readIdx = 1u - writeIdx_;
    const auto& images = frameData_[readIdx].images;
    for (const auto& ref : images) {
        RenderHandle remapHandle;
        for (auto& gpuRef : gpuBackedImages_) {
            if ((gpuRef.hash == ref.hash) && (gpuRef.frameUseIndex != frameCounter_)) {
                remapHandle = gpuRef.handle.GetHandle();
                gpuRef.frameUseIndex = frameCounter_;
                break;
            }
        }
        if (!RenderHandleUtil::IsValid(remapHandle)) {
            GpuImageDesc desc = gpuResourceMgr_.GetImageDescriptor(ref.handle);
            RenderHandleReference handle = gpuResourceMgr_.Create(desc);
            remapHandle = handle.GetHandle();
            gpuBackedImages_.push_back({ ref.hash, frameCounter_, move(handle) });
        }
        gpuResourceMgr_.RemapGpuImageHandle(ref.handle.GetHandle(), remapHandle);
    }
}

void GpuResourceCache::DestroyOldImages()
{
    // shallow handles
    {
        const uint32_t readIdx = 1u - writeIdx_;
        auto& imagesRef = frameData_[readIdx].images;
        const auto invalidHandles = std::partition(imagesRef.begin(), imagesRef.end(), [&](const ImageData& imgRef) {
            if (imgRef.handle.GetRefCount() > 2) {
                return true;
            } else {
                PLUGIN_ASSERT(imgRef.handle.GetRefCount() == 2);
                return false;
            }
        });
        if (invalidHandles != imagesRef.end()) {
            imagesRef.erase(invalidHandles, imagesRef.end());
        }
    }
    // gpu backed resources
    {
        constexpr uint64_t minAge = 2;
        const auto ageLimit = (frameCounter_ < minAge) ? 0 : (frameCounter_ - minAge);
        const auto invalidHandles =
            std::partition(gpuBackedImages_.begin(), gpuBackedImages_.end(), [&](const GpuBackedData& imgRef) {
                if (imgRef.frameUseIndex < ageLimit) {
                    return false;
                } else {
                    return true;
                }
            });
        if (invalidHandles != gpuBackedImages_.end()) {
            gpuBackedImages_.erase(invalidHandles, gpuBackedImages_.end());
        }
    }
}

uint64_t GpuResourceCache::HashCacheGpuImageDesc(const CacheGpuImageDesc& desc) const
{
    // pack: width, height, mipCount, layerCount to a single 64 bit "hash"
    const uint32_t sizeHash = (desc.width << 16u) | desc.height;
    const uint32_t mipLayerSwizzleHash = (desc.componentMapping.r << 28) | (desc.componentMapping.g << 24) |
                                         (desc.componentMapping.b << 20) | (desc.componentMapping.a << 16) |
                                         (desc.mipCount << 8u) | desc.layerCount;
    const uint64_t formatSamplesHash =
        (((uint64_t)desc.format) << 32) | (((uint64_t)desc.sampleCountFlags) & 0xFFFFffff);
    uint64_t hash = (((uint64_t)mipLayerSwizzleHash) << 32u) | sizeHash;
    HashCombine(hash, formatSamplesHash);
    return hash;
}
RENDER_END_NAMESPACE()
