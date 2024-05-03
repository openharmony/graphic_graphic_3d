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

#ifndef API_RENDER_IRENDER_DATA_STORE_DEFAULT_GPU_RESOURCE_DATA_COPY_H
#define API_RENDER_IRENDER_DATA_STORE_DEFAULT_GPU_RESOURCE_DATA_COPY_H

#include <base/containers/byte_array.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
/**
 * IRenderDataStoreDefaultGpuResourceDataCopy interface.
 * Interface to add copy operations.
 *
 * Copy will happen after the frame has been rendered.
 *
 * Internally synchronized.
 */
class IRenderDataStoreDefaultGpuResourceDataCopy : public IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID { "f3ce24fd-a624-4b8f-9013-006fc5fd8760" };

    /* The type of the copy */
    enum class CopyType : uint8_t {
        /** Undefined */
        UNDEFINED,
        /** Wait for idle (waits for the GPU) */
        WAIT_FOR_IDLE,
    };

    struct GpuResourceDataCopy {
        /* The type of the copy */
        CopyType copyType { CopyType::UNDEFINED };
        /* GPU resource handle that will be copied. */
        RenderHandleReference gpuHandle;
        /* Byte array where to copy. */
        BASE_NS::ByteArray* byteArray { nullptr };
    };

    /** Copy data to buffer on GPU through staging GPU buffer.
     * @param data Byte data.
     * @param dstHandle Dst resource.
     * @param bufferCopy Buffer copy info struct.
     */
    virtual void AddCopyOperation(const GpuResourceDataCopy& copyOp) = 0;

protected:
    IRenderDataStoreDefaultGpuResourceDataCopy() = default;
    ~IRenderDataStoreDefaultGpuResourceDataCopy() override = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_DATA_STORE_DEFAULT_GPU_RESOURCE_DATA_COPY_H
