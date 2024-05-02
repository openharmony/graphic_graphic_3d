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

#ifndef RENDER_RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_STAGING_H
#define RENDER_RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_STAGING_H

#include <cstdint>
#include <mutex>

#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "device/gpu_resource_manager.h"

RENDER_BEGIN_NAMESPACE()
class IRenderContext;

struct DirectDataCopyOnCpu {
    RenderHandleReference dstHandle;
    BufferCopy bufferCopy;
    BASE_NS::vector<uint8_t> data;
};
struct StagingDirectDataCopyConsumeStruct {
    BASE_NS::vector<DirectDataCopyOnCpu> dataCopies;
};

struct ImageClearCommand {
    RenderHandleReference handle;
    ClearColorValue color;
};
struct StagingImageClearConsumeStruct {
    BASE_NS::vector<ImageClearCommand> clears;
};

/**
RenderDataStoreDefaultStaging implementation.
*/
class RenderDataStoreDefaultStaging final : public IRenderDataStoreDefaultStaging {
public:
    RenderDataStoreDefaultStaging(IRenderContext& renderContext, const BASE_NS::string_view name);
    ~RenderDataStoreDefaultStaging() override;

    void CommitFrameData() override {};
    void PreRender() override;
    void PostRender() override;
    void PreRenderBackend() override {};
    void PostRenderBackend() override {};
    void Clear() override;
    uint32_t GetFlags() const override
    {
        return 0;
    };

    void CopyDataToBuffer(const BASE_NS::array_view<const uint8_t>& data, const RenderHandleReference& dstHandle,
        const BufferCopy& bufferCopy) override;
    void CopyDataToBufferOnCpu(const BASE_NS::array_view<const uint8_t>& data, const RenderHandleReference& dstHandle,
        const BufferCopy& bufferCopy) override;

    void CopyDataToImage(const BASE_NS::array_view<const uint8_t>& data, const RenderHandleReference& dstHandle,
        const BufferImageCopy& bufferImageCopy) override;

    void CopyBufferToBuffer(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const BufferCopy& bufferCopy) override;

    void CopyBufferToImage(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const BufferImageCopy& bufferImageCopy) override;

    void CopyImageToBuffer(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const BufferImageCopy& bufferImageCopy) override;

    void CopyBufferToImage(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        BASE_NS::array_view<const BufferImageCopy> bufferImageCopies) override;

    void CopyImageToImage(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const ImageCopy& imageCopy) override;

    void ClearColorImage(const RenderHandleReference& handle, const ClearColorValue color) override;

    void CopyImageToBuffer(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const BufferImageCopy& bufferImageCopy, const ResourceCopyInfo copyInfo) override;
    void CopyImageToImage(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const ImageCopy& imageCopy, const ResourceCopyInfo copyInfo) override;
    void CopyBufferToBuffer(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const BufferCopy& bufferCopy, const ResourceCopyInfo copyInfo) override;
    void CopyBufferToImage(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const BufferImageCopy& bufferImageCopy, const ResourceCopyInfo copyInfo) override;

    bool HasBeginStagingData() const;
    bool HasEndStagingData() const;
    StagingConsumeStruct ConsumeBeginStagingData();
    StagingDirectDataCopyConsumeStruct ConsumeBeginStagingDirectDataCopy();
    StagingImageClearConsumeStruct ConsumeBeginStagingImageClears();
    StagingConsumeStruct ConsumeEndStagingData();

    uint32_t GetImageClearByteSize() const;

    // for plugin / factory interface
    static constexpr const char* const TYPE_NAME = "RenderDataStoreDefaultStaging";

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
    IGpuResourceManager& gpuResourceMgr_;
    const BASE_NS::string name_;

    mutable std::mutex mutex_;

    struct StagingConsumeData {
        StagingConsumeStruct beginFrame;
        StagingConsumeStruct endFrame;

        StagingDirectDataCopyConsumeStruct beginFrameDirect;

        StagingImageClearConsumeStruct beginFrameClear;
    };
    StagingConsumeData stagingConsumeData_;
    // in pre render data moved here
    StagingConsumeData frameStagingConsumeData_;

    BASE_NS::vector<RenderHandleReference> stagingGpuBuffers_;
    BASE_NS::vector<RenderHandleReference> frameStagingConsumeGpuBuffers_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_STAGING_H
