/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef API_RENDER_IRENDER_DATA_STORE_DEFAULT_STAGING_H
#define API_RENDER_IRENDER_DATA_STORE_DEFAULT_STAGING_H

#include <base/util/uid.h>
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

    /** Copy info to copy begin of frame or end of frame
     */
    enum class ResourceCopyInfo : uint32_t {
        /** Copy data in the beginning of the next frame (default) */
        BEGIN_FRAME = 0,
        /** Copy data in the end of the next frame */
        END_FRAME = 1,
    };

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

    /** Copy image to buffer on GPU through staging.
     * @param srcHandle Src resource.
     * @param dstHandle Dst resource.
     * @param bufferImageCopy Buffer image copy info struct.
     * @param copyInfo Copy info.
     */
    virtual void CopyImageToBuffer(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const BufferImageCopy& bufferImageCopy, const ResourceCopyInfo copyInfo) = 0;

    /** Copy image to image on GPU through staging.
     * @param srcHandle Src resource.
     * @param dstHandle Dst resource.
     * @param imageCopy Image to image copy info struct.
     * @param copyInfo Copy info.
     */
    virtual void CopyImageToImage(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const ImageCopy& imageCopy, const ResourceCopyInfo copyInfo) = 0;

    /** Copy buffer to buffer on GPU through staging.
     * @param srcHandle Src resource.
     * @param dstHandle Dst resource.
     * @param bufferCopy Buffer copy info struct.
     * @param copyInfo Copy info.
     */
    virtual void CopyBufferToBuffer(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const BufferCopy& bufferCopy, const ResourceCopyInfo copyInfo) = 0;

    /** Copy buffer to image on GPU through staging.
     * @param srcHandle Src resource.
     * @param dstHandle Dst resource.
     * @param bufferCopy Buffer image copy info struct.
     * @param copyInfo Copy info.
     */
    virtual void CopyBufferToImage(const RenderHandleReference& srcHandle, const RenderHandleReference& dstHandle,
        const BufferImageCopy& bufferImageCopy, const ResourceCopyInfo copyInfo) = 0;

    /** Clear image. Might be usable in the first frame if image is partially updated after that
     * Often should not be used every frame.
     * NOTE: some backends like OpenGLES might not support fully converted texture format clears.
     * Prefer using shader clears if typical zero clears etc. are not desired
     * @param handle Color image handle
     * @param color Clear color value
     */
    virtual void ClearColorImage(const RenderHandleReference& handle, const ClearColorValue color) = 0;

protected:
    IRenderDataStoreDefaultStaging() = default;
    ~IRenderDataStoreDefaultStaging() override = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_DATA_STORE_DEFAULT_STAGING_H
