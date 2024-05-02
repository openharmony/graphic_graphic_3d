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

#ifndef API_RENDER_DEVICE_IGPU_RESOURCE_CACHE_H
#define API_RENDER_DEVICE_IGPU_RESOURCE_CACHE_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string_view.h>
#include <base/util/formats.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
/** \addtogroup group_igpuresourcecache
 *  @{
 */
/** Cache GPU image desc
 */
struct CacheGpuImageDesc {
    /** format */
    BASE_NS::Format format { BASE_NS::BASE_FORMAT_R8G8B8A8_SRGB };

    /** width */
    uint32_t width { 0u };
    /** height */
    uint32_t height { 0u };

    /** mip count */
    uint32_t mipCount { 1u };
    /** layer count */
    uint32_t layerCount { 1u };

    /** MSAA sample count */
    SampleCountFlags sampleCountFlags { SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT };

    /** component mapping */
    ComponentMapping componentMapping {};
};

/** Cache GPU image pair
 */
struct CacheGpuImagePair {
    /** First image */
    RenderHandleReference firstImage {};
    /** Second image */
    RenderHandleReference secondImage {};
};

/** Gpu resource cache.
 *  Internally synchronized.
 *
 *  Caches gpu resource for use and can be obtained for re-use.
 */
class IGpuResourceCache {
public:
    IGpuResourceCache(const IGpuResourceCache&) = delete;
    IGpuResourceCache& operator=(const IGpuResourceCache&) = delete;

    /** Reserve GPU image.
     * @param desc image description.
     * @return handle.
     */
    virtual RenderHandleReference ReserveGpuImage(const CacheGpuImageDesc& desc) = 0;

    /** Reserve GPU images.
     * @param descs image descriptions.
     * @return handles.
     */
    virtual BASE_NS::vector<RenderHandleReference> ReserveGpuImages(
        const BASE_NS::array_view<const CacheGpuImageDesc> descs) = 0;

    /** Get image description.
     * @param handle Handle of the image.
     * @return Image description.
     */
    virtual CacheGpuImageDesc GetCacheGpuImageDesc(const RenderHandleReference& handle) const = 0;

    /** Reserve GPU image pair usually for msaa -> resolve.
     * @param desc image description.
     * @param sampleCountFlags Sample count flags for the second image.
     * @return Image pair.
     */
    virtual CacheGpuImagePair ReserveGpuImagePair(
        const CacheGpuImageDesc& desc, const SampleCountFlags sampleCountFlags) = 0;

protected:
    IGpuResourceCache() = default;
    virtual ~IGpuResourceCache() = default;
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_DEVICE_IGPU_RESOURCE_MANAGER_H
