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

#ifndef RENDER_DEVICE_GPU_RESOURCE_CACHE_H
#define RENDER_DEVICE_GPU_RESOURCE_CACHE_H

#include <mutex>

#include <base/containers/array_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <render/device/intf_gpu_resource_cache.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class GpuResourceManager;

/* Gpu resource cache.
Caches gpu resource for use and can be obtained for re-use.
*/
class GpuResourceCache final : public IGpuResourceCache {
public:
    explicit GpuResourceCache(RENDER_NS::GpuResourceManager& gpuResourceMgr);
    ~GpuResourceCache();

    GpuResourceCache(const GpuResourceCache&) = delete;
    GpuResourceCache& operator=(const GpuResourceCache&) = delete;

    RenderHandleReference ReserveGpuImage(const CacheGpuImageDesc& desc) override;
    BASE_NS::vector<RenderHandleReference> ReserveGpuImages(
        const BASE_NS::array_view<const CacheGpuImageDesc> descs) override;
    CacheGpuImageDesc GetCacheGpuImageDesc(const RenderHandleReference& gpuImageHandle) const override;

    CacheGpuImagePair ReserveGpuImagePair(
        const CacheGpuImageDesc& desc, const SampleCountFlags sampleCountFlags) override;

    uint64_t HashCacheGpuImageDesc(const CacheGpuImageDesc& desc) const;

    void BeginFrame(const uint64_t frameCount);

    struct ImageData {
        RenderHandleReference handle;
        uint64_t hash { 0 };
    };
    // access read handles when remapping in render graph
    BASE_NS::array_view<const ImageData> GetImageData() const;

private:
    RENDER_NS::GpuResourceManager& gpuResourceMgr_;

    RenderHandleReference ReserveGpuImageImpl(const CacheGpuImageDesc& desc);
    void AllocateAndRemapImages();
    void DestroyOldImages();

    // needs to be locked when touching image containers
    mutable std::mutex mutex_;
    struct FrameData {
        BASE_NS::vector<ImageData> images;
    };
    FrameData frameData_[2u];

    struct GpuBackedData {
        uint64_t hash { 0 };
        uint64_t frameUseIndex { 0 };
        RenderHandleReference handle;
    };
    BASE_NS::vector<GpuBackedData> gpuBackedImages_;

    uint32_t writeIdx_ { 0 };
    uint64_t frameCounter_ { 0 };
};
RENDER_END_NAMESPACE()
#endif // RENDER_DEVICE_GPU_RESOURCE_CACHE_H
