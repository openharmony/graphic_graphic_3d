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

#ifndef RENDER_DATA_STORE_RENDER_DATA_STORE_DEFAULT_GPU_RESOURCE_DATA_COPY_H
#define RENDER_DATA_STORE_RENDER_DATA_STORE_DEFAULT_GPU_RESOURCE_DATA_COPY_H

#include <cstdint>
#include <mutex>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
class IDevice;
class GpuResourceManager;
/**
RenderDataStoreDefaultGpuResourceDataCopy implementation.
*/
class RenderDataStoreDefaultGpuResourceDataCopy final : public IRenderDataStoreDefaultGpuResourceDataCopy {
public:
    RenderDataStoreDefaultGpuResourceDataCopy(IRenderContext& renderContext, const BASE_NS::string_view name);
    ~RenderDataStoreDefaultGpuResourceDataCopy() override = default;

    void CommitFrameData() override {};
    void PreRender() override {};
    void PostRender() override {};
    void PreRenderBackend() override {};
    // Do copy operation in end frame.
    void PostRenderBackend() override;
    void Clear() override;
    uint32_t GetFlags() const override
    {
        return 0;
    };

    void AddCopyOperation(const GpuResourceDataCopy& copyOp) override;

    // for plugin / factory interface
    static constexpr const char* const TYPE_NAME = "RenderDataStoreDefaultGpuResourceDataCopy";

    static IRenderDataStore* Create(IRenderContext& renderContext, char const* name);
    static void Destroy(IRenderDataStore* instance);

    BASE_NS::string_view GetTypeName() const override
    {
        return TYPE_NAME;
    }

    BASE_NS::string_view GetName() const override
    {
        return name_;
    }

    const BASE_NS::Uid& GetUid() const override
    {
        return UID;
    }

private:
    IDevice& device_;
    GpuResourceManager& gpuResourceMgr_;
    const BASE_NS::string name_;

    mutable std::mutex mutex_;

    bool waitForIdle_ { false };
    BASE_NS::vector<GpuResourceDataCopy> copyData_;
};
RENDER_END_NAMESPACE()

#endif // RENDER_DATA_STORE_RENDER_DATA_STORE_DEFAULT_GPU_RESOURCE_DATA_COPY_H
