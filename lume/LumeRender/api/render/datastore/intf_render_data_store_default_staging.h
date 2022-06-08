/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_RENDER_IRENDER_DATA_STORE_DEFAULT_STAGING_H
#define API_RENDER_IRENDER_DATA_STORE_DEFAULT_STAGING_H

#include <render/datastore/intf_render_data_store.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
/**
 * IRenderDataStoreDefaultStaging interface.
 * Interface to add custom staging data.
 *
 * Internally synchronized.
 */
class IRenderDataStoreDefaultStaging : public IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID { "6f3af7c1-5e6e-49c9-8364-c454a872d228" };

    /** Copy data to buffer on GPU through staging GPU buffer.
     * @param data Byte data.
     * @param dstHandle Dst resource.
     * @param bufferCopy Buffer copy info struct.
     */
    virtual void CopyDataToBuffer(const BASE_NS::array_view<const uint8_t>& data,
        const RenderHandleReference& dstHandle, const BufferCopy& bufferCopy) = 0;

    /** Copy data to coherent GPU buffer on CPU through staging render node.
     * GPU buffer must be created with CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT and CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT.
     * (Optionally: CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER)
     * @param data Byte data.
     * @param dstHandle Dst resource.
     * @param bufferCopy Buffer copy info struct.
     */
    virtual void CopyDataToBufferOnCpu(const BASE_NS::array_view<const uint8_t>& data,
        const RenderHandleReference& dstHandle, const BufferCopy& bufferCopy) = 0;

    /** Copy data to image on GPU through staging GPU buffer.
     * @param data Byte data.
     * @param dstHandle Dst resource.
     * @param BufferImageCopy Info for copying.
     */
    virtual void CopyDataToImage(const BASE_NS::array_view<const uint8_t>& data, const RenderHandleReference& dstHandle,
        const BufferImageCopy& bufferImageCopy) = 0;

    /** Copy buffer to buffer on GPU through staging.
     * @param srcHandle Src resource.
     * @param dstHandle Dst resource.
     * @param bufferCopy Buffer copy info struct.
     */
    virtual void CopyBufferToBuffer(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const BufferCopy& bufferCopy) = 0;

    /** Copy buffer to image on GPU through staging.
     * @param srcHandle Src resource.
     * @param dstHandle Dst resource.
     * @param bufferCopy Buffer image copy info struct.
     */
    virtual void CopyBufferToImage(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const BufferImageCopy& bufferImageCopy) = 0;

    /** Copy buffer to image on GPU through staging.
     * @param srcHandle Src resource.
     * @param dstHandle Dst resource.
     * @param bufferImageCopies Buffer image copy info struct for each image layer/level.
     */
    virtual void CopyBufferToImage(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        BASE_NS::array_view<const BufferImageCopy> bufferImageCopies) = 0;

    /** Copy image to buffer on GPU through staging.
     * @param srcHandle Src resource.
     * @param dstHandle Dst resource.
     * @param bufferImageCopy Buffer image copy info struct.
     */
    virtual void CopyImageToBuffer(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const BufferImageCopy& bufferImageCopy) = 0;

    /** Copy image to image on GPU through staging.
     * @param srcHandle Src resource.
     * @param dstHandle Dst resource.
     * @param imageCopy Image to image copy info struct.
     */
    virtual void CopyImageToImage(
        const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle, const ImageCopy& imageCopy) = 0;

protected:
    IRenderDataStoreDefaultStaging() = default;
    ~IRenderDataStoreDefaultStaging() override = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_DATA_STORE_DEFAULT_STAGING_H
