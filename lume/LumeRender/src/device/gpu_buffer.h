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

#ifndef DEVICE_GPU_BUFFER_H
#define DEVICE_GPU_BUFFER_H

#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
struct GpuBufferDesc;

/** Gpu buffer.
 * Immutable platform gpu buffer wrapper.
 */
class GpuBuffer {
public:
    GpuBuffer() = default;
    virtual ~GpuBuffer() = default;

    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;

    virtual const GpuBufferDesc& GetDesc() const = 0;

    // Map will return a pointer to the data.
    // If mapBufferingCount is bigger than 1, the pointer advances to next block with every call (ring buffer)
    virtual void* Map() = 0;
    // Maps always from the beginning of the buffer. Does not advance dynamic ring buffer.
    virtual void* MapMemory() = 0;

    virtual void Unmap() const = 0;
};
RENDER_END_NAMESPACE()

#endif // DEVICE_GPU_BUFFER_H
