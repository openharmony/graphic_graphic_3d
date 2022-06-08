/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
