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

#ifndef DEVICE_GPU_RESOURCE_MANAGER_BASE_INL
#define DEVICE_GPU_RESOURCE_MANAGER_BASE_INL

#include <algorithm>
#include <cinttypes>

#include <render/namespace.h>

#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
template<typename ResourceType, typename CreateInfoType>
inline GpuResourceManagerTyped<ResourceType, CreateInfoType>::GpuResourceManagerTyped(Device& device) : device_(device)
{}

template<typename ResourceType, typename CreateInfoType>
inline ResourceType* GpuResourceManagerTyped<ResourceType, CreateInfoType>::Get(const uint32_t index) const
{
    if (index < resources_.size()) {
        return resources_[index].get();
    } else {
        return nullptr;
    }
}

template<typename ResourceType, typename CreateInfoType>
template<typename AdditionalInfoType>
inline void GpuResourceManagerTyped<ResourceType, CreateInfoType>::Create(const uint32_t index,
    const CreateInfoType& desc, BASE_NS::unique_ptr<ResourceType> optionalResource, const bool useAdditionalDesc,
    const AdditionalInfoType& additionalDesc)
{
    if (index < static_cast<uint32_t>(resources_.size())) { // use existing location
        // add old for deallocation if found
        if (resources_[index]) {
            pendingDeallocations_.push_back({ move(resources_[index]), device_.GetFrameCount() });
        }

        if (optionalResource) {
            resources_[index] = move(optionalResource);
        } else {
            if constexpr (BASE_NS::is_same_v<ResourceType, GpuBuffer>) {
                if (useAdditionalDesc) {
                    resources_[index] = device_.CreateGpuBuffer(additionalDesc);
                } else {
                    resources_[index] = device_.CreateGpuBuffer(desc);
                }
            } else if constexpr (BASE_NS::is_same_v<ResourceType, GpuImage>) {
                resources_[index] = device_.CreateGpuImage(desc);
            } else if constexpr (BASE_NS::is_same_v<ResourceType, GpuSampler>) {
                resources_[index] = device_.CreateGpuSampler(desc);
            }
        }
    } else {
        if (optionalResource) {
            resources_.push_back(move(optionalResource));
        } else {
            if constexpr (BASE_NS::is_same_v<ResourceType, GpuBuffer>) {
                resources_.push_back(device_.CreateGpuBuffer(desc));
            } else if constexpr (BASE_NS::is_same_v<ResourceType, GpuImage>) {
                resources_.push_back(device_.CreateGpuImage(desc));
            } else if constexpr (BASE_NS::is_same_v<ResourceType, GpuSampler>) {
                resources_.push_back(device_.CreateGpuSampler(desc));
            }
        }
        PLUGIN_ASSERT(index == static_cast<uint32_t>(resources_.size() - 1u));
    }
}

template<typename ResourceType, typename CreateInfoType>
inline void GpuResourceManagerTyped<ResourceType, CreateInfoType>::HandlePendingDeallocations()
{
    if (!pendingDeallocations_.empty()) {
        auto const minAge = device_.GetCommandBufferingCount() + 1;
        auto const ageLimit = (device_.GetFrameCount() < minAge) ? 0 : (device_.GetFrameCount() - minAge);

        auto const oldResources = std::partition(pendingDeallocations_.begin(), pendingDeallocations_.end(),
            [ageLimit](auto const& handleTime) { return handleTime.frameIndex >= ageLimit; });

        pendingDeallocations_.erase(oldResources, pendingDeallocations_.end());
    }
}

template<typename ResourceType, typename CreateInfoType>
inline void GpuResourceManagerTyped<ResourceType, CreateInfoType>::HandlePendingDeallocationsImmediate()
{
    pendingDeallocations_.clear();
}

template<typename ResourceType, typename CreateInfoType>
inline void GpuResourceManagerTyped<ResourceType, CreateInfoType>::Destroy(const uint32_t index)
{
    PLUGIN_ASSERT(index < static_cast<uint32_t>(resources_.size()));
    if (index < static_cast<uint32_t>(resources_.size())) {
        pendingDeallocations_.push_back({ move(resources_[index]), device_.GetFrameCount() });
    }
}

template<typename ResourceType, typename CreateInfoType>
inline void GpuResourceManagerTyped<ResourceType, CreateInfoType>::DestroyImmediate(const uint32_t index)
{
    PLUGIN_ASSERT(index < static_cast<uint32_t>(resources_.size()));
    if (index < static_cast<uint32_t>(resources_.size())) {
        resources_[index].reset();
    }
}

template<typename ResourceType, typename CreateInfoType>
inline void GpuResourceManagerTyped<ResourceType, CreateInfoType>::Resize(const size_t maxSize)
{
    // ATM our code does not allow reducing the stored space
    // this assert is here to note the current design
    PLUGIN_ASSERT(maxSize >= resources_.size());
    resources_.resize(maxSize);
}

#if (RENDER_VALIDATION_ENABLED == 1)
template<typename ResourceType, typename CreateInfoType>
size_t GpuResourceManagerTyped<ResourceType, CreateInfoType>::GetValidResourceCount() const
{
    size_t count = 0;
    for (const auto& res : resources_) {
        if (res) {
            count++;
        }
    }
    return count;
}
#endif
RENDER_END_NAMESPACE()

#endif // DEVICE_GPU_RESOURCE_MANAGER_BASE_INL