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

#ifndef GLES_GPU_BUFFER_GLES_H
#define GLES_GPU_BUFFER_GLES_H

#include <cstdint>

#include <render/device/gpu_resource_desc.h>
#include <render/gles/intf_device_gles.h>
#include <render/namespace.h>

#include "device/gpu_buffer.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class DeviceGLES;
struct GpuBufferPlatformDataGL final : public GpuBufferPlatformData {
    uint32_t buffer { 0 };
    uint32_t alignedBindByteSize { 0 };
    uint32_t alignedByteSize { 0 };
    // map changes offset if buffered
    uint32_t currentByteOffset { 0 };
    // alignedByteSize / mapBufferingCount (if no buffering alignedByteSize == bindMemoryByteSize)
    uint32_t bindMemoryByteSize { 0 };
};

class GpuBufferGLES final : public GpuBuffer {
public:
    GpuBufferGLES(Device& device, const GpuBufferDesc& desc);
    ~GpuBufferGLES();

    const GpuBufferDesc& GetDesc() const override;
    const GpuBufferPlatformDataGL& GetPlatformData() const;

    void* Map() override;
    void* MapMemory() override;
    void Unmap() const override;

private:
    DeviceGLES& device_;

    GpuBufferPlatformDataGL plat_;
    GpuBufferDesc desc_;

    // debug assert usage only
    mutable bool isMapped_ { false };

    bool isPersistantlyMapped_ { false };
    bool isMappable_ { false };
    bool isRingBuffer_ { false };

    uint8_t* data_ { nullptr };
};
RENDER_END_NAMESPACE()

#endif // GLES_GPU_BUFFER_GLES_H
