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

#ifndef DEVICE_GPU_RESOURCE_MANAGER_BASE_H
#define DEVICE_GPU_RESOURCE_MANAGER_BASE_H

#include <cstdint>

#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class Device;

/* GpuResourceManagerBase.
 * Not internally synchronized. */
class GpuResourceManagerBase {
public:
    GpuResourceManagerBase() = default;
    virtual ~GpuResourceManagerBase() = default;

    virtual void HandlePendingDeallocations() = 0;
    virtual void HandlePendingDeallocationsImmediate() = 0;

protected:
    friend class GpuResourceManager;
    virtual void Resize(const size_t maxSize) = 0;
    virtual void Destroy(const uint32_t index) = 0;
    virtual void DestroyImmediate(const uint32_t index) = 0;
};

template<typename ResourceType, typename CreateInfoType>
class GpuResourceManagerTyped final : GpuResourceManagerBase {
public:
    explicit GpuResourceManagerTyped(Device& device);
    virtual ~GpuResourceManagerTyped() = default;

    GpuResourceManagerTyped(const GpuResourceManagerTyped&) = delete;
    GpuResourceManagerTyped& operator=(const GpuResourceManagerTyped&) = delete;

    ResourceType* Get(const uint32_t index) const;

protected:
    friend class GpuResourceManager;

    // handle pending deallocations (waits safe frames to deallocate).
    void HandlePendingDeallocations() override;
    // handle pending deallocations immediately.
    void HandlePendingDeallocationsImmediate() override;

    // resize the resource vector
    void Resize(const size_t maxSize) override;
    // deferred gpu resource destruction.
    void Destroy(const uint32_t index) override;
    // forced immediate Destroy, prefer not to use this
    void DestroyImmediate(const uint32_t index) override;

    // create new, send old to pending deallocations if replacing
    template<typename AdditionalInfoType>
    void Create(const uint32_t index, const CreateInfoType& desc, BASE_NS::unique_ptr<ResourceType> optionalResource,
        const bool useAdditionalDesc, const AdditionalInfoType& additonalDesc);

    Device& device_;

#if (RENDER_VALIDATION_ENABLED == 1)
    size_t GetValidResourceCount() const;
#endif

private:
    BASE_NS::vector<BASE_NS::unique_ptr<ResourceType>> resources_;
    // resource, frame index
    struct PendingDeallocData {
        BASE_NS::unique_ptr<ResourceType> resource;
        uint64_t frameIndex;
    };
    BASE_NS::vector<PendingDeallocData> pendingDeallocations_;
};
RENDER_END_NAMESPACE()

#include "gpu_resource_manager_base.inl"

#endif // DEVICE_GPU_RESOURCE_MANAGER_BASE_H
