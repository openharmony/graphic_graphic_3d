/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

/**
RenderDataStoreDefaultStaging implementation.
*/
class RenderDataStoreDefaultStaging final : public IRenderDataStoreDefaultStaging {
public:
    RenderDataStoreDefaultStaging(IRenderContext& renderContext, const BASE_NS::string_view name);
    ~RenderDataStoreDefaultStaging() override;

    void PreRender() override;
    void PreRenderBackend() override {};
    void PostRender() override;
    void Clear() override;

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

    bool HasStagingData() const;
    StagingConsumeStruct ConsumeStagingData();
    StagingDirectDataCopyConsumeStruct ConsumeStagingDirectDataCopy();

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

    StagingConsumeStruct stagingConsumeStruct_;
    StagingDirectDataCopyConsumeStruct stagingDirectCopyConsumeStruct_;

    // in pre render data moved here
    StagingConsumeStruct frameStagingConsumeStruct_;
    StagingDirectDataCopyConsumeStruct frameStagingDirectCopyConsumeStruct_;

    BASE_NS::vector<RenderHandleReference> stagingGpuBuffers_;
    BASE_NS::vector<RenderHandleReference> frameStagingConsumeGpuBuffers_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_STAGING_H
