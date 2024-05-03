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

#include "gpu_buffer_gles.h"

#if (RENDER_PERF_ENABLED == 1)
#include <core/implementation_uids.h>
#include <core/perf/intf_performance_data_manager.h>
#endif
#include <render/namespace.h>

#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "util/log.h"

#define IS_BIT(value, bit) ((((value) & (bit)) == (bit)) ? (GLboolean)GL_TRUE : (GLboolean)GL_FALSE)

RENDER_BEGIN_NAMESPACE()
namespace {
#if (RENDER_PERF_ENABLED == 1)
void RecordAllocation(const int64_t alignedByteSize)
{
    if (auto* inst = CORE_NS::GetInstance<CORE_NS::IPerformanceDataManagerFactory>(CORE_NS::UID_PERFORMANCE_FACTORY);
        inst) {
        CORE_NS::IPerformanceDataManager* pdm = inst->Get("Memory");
        pdm->UpdateData("AllGpuBuffers", "GPU_BUFFER", alignedByteSize);
    }
}
#endif

constexpr GLenum INIT_TARGET = GL_COPY_WRITE_BUFFER;

constexpr uint32_t MakeFlags(uint32_t requiredFlags)
{
    uint32_t flags = 0;
    if ((requiredFlags & CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == 0) {
        // allow non device local (non gpu) memory (since device_local not set)
        flags |= GL_CLIENT_STORAGE_BIT_EXT;
    }
    if (requiredFlags & CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        // can be mapped (reads?)
        flags |= GL_MAP_WRITE_BIT;
    }
    if (requiredFlags & CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        // no need to flush
        flags |= GL_MAP_COHERENT_BIT_EXT;
    }
    if (flags & GL_MAP_COHERENT_BIT_EXT) {
        // It is an error to specify MAP_COHERENT_BIT_EXT without also specifying MAP_PERSISTENT_BIT_EXT.
        flags |= GL_MAP_PERSISTENT_BIT_EXT;
    }
    if (flags & GL_MAP_PERSISTENT_BIT_EXT) {
        // If <flags> contains MAP_PERSISTENT_BIT_EXT, it must also contain at least one of
        // MAP_READ_BIT or MAP_WRITE_BIT.
        flags |= GL_MAP_WRITE_BIT;
    }
    return flags;
}
} // namespace

GpuBufferGLES::GpuBufferGLES(Device& device, const GpuBufferDesc& desc)
    : device_((DeviceGLES&)device), plat_({ {}, 0u, 0u, desc.byteSize, 0u, desc.byteSize }), desc_(desc),
      isPersistantlyMapped_((desc.memoryPropertyFlags & CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
                            (desc.memoryPropertyFlags & CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT)),
      // At some point see if other memory property flags should be used.
      isMappable_(IS_BIT(desc.memoryPropertyFlags, CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
{
    PLUGIN_ASSERT(device_.IsActive());
    glGenBuffers(1, &plat_.buffer);

    // we need to handle the alignment only for mappable uniform buffers due to binding offset
    GLint minAlignment = sizeof(float) * 4u; // NOTE: un-educated guess here.
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minAlignment);

    minAlignment = minAlignment > 0 ? minAlignment : 1;
    plat_.alignedBindByteSize = ((plat_.bindMemoryByteSize + (minAlignment - 1)) / minAlignment) * minAlignment;
    plat_.alignedByteSize = plat_.alignedBindByteSize;

    if (desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER) {
        isRingBuffer_ = true;
        plat_.alignedByteSize *= device_.GetCommandBufferingCount();
    }

    const auto oldBind = device_.BoundBuffer(INIT_TARGET);
    device_.BindBuffer(INIT_TARGET, plat_.buffer);

    // check for the extension
    if (const bool hasBufferStorageEXT = device_.HasExtension("GL_EXT_buffer_storage"); hasBufferStorageEXT) {
        uint32_t flags = MakeFlags(desc.memoryPropertyFlags);
        glBufferStorageEXT(INIT_TARGET, static_cast<GLsizeiptr>(plat_.alignedByteSize), nullptr, flags);
        if (isPersistantlyMapped_) {
            // make the persistant mapping..
            flags = flags & (~GL_CLIENT_STORAGE_BIT_EXT); // not valid for map buffer..
            data_ = reinterpret_cast<uint8_t*>(
                glMapBufferRange(INIT_TARGET, 0, static_cast<GLsizeiptr>(plat_.alignedByteSize), flags));
        }
    } else {
        // glBufferStorageEXT not found, so persistant mapping is not possible.
        isPersistantlyMapped_ = false;
        // legacy path, no glbufferStorage extension.
        if (desc_.engineCreationFlags & EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_SINGLE_SHOT_STAGING) {
            // single shot staging buffer so give the driver more hints. (cpu write once, gpu read few times)
            glBufferData(INIT_TARGET, static_cast<GLsizeiptr>(plat_.alignedByteSize), nullptr, GL_STREAM_DRAW);
        } else {
            if (isMappable_) {
                // modified repeatedly , used many times.
                glBufferData(INIT_TARGET, static_cast<GLsizeiptr>(plat_.alignedByteSize), nullptr, GL_DYNAMIC_DRAW);
            } else {
                // modified once, used many times.
                glBufferData(INIT_TARGET, static_cast<GLsizeiptr>(plat_.alignedByteSize), nullptr, GL_STATIC_DRAW);
            }
        }
    }
    device_.BindBuffer(INIT_TARGET, oldBind);

#if (RENDER_PERF_ENABLED == 1)
    RecordAllocation(static_cast<int64_t>(plat_.alignedByteSize));
#endif

#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
    PLUGIN_LOG_E("gpu buffer >: %u", plat_.buffer);
#endif
}

GpuBufferGLES::~GpuBufferGLES()
{
    if (plat_.buffer) {
        PLUGIN_ASSERT(device_.IsActive());
        if ((isPersistantlyMapped_) || (isMapped_)) {
            isMapped_ = false;
            // Unmap the buffer.
            device_.BindBuffer(GL_COPY_WRITE_BUFFER, plat_.buffer);
            glUnmapBuffer(GL_COPY_WRITE_BUFFER);
            device_.BindBuffer(GL_COPY_WRITE_BUFFER, 0);
        }
        device_.DeleteBuffer(plat_.buffer);
    }

#if (RENDER_PERF_ENABLED == 1)
    RecordAllocation(-static_cast<int64_t>(plat_.alignedByteSize));
#endif
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
    PLUGIN_LOG_E("gpu buffer <: %u", plat_.buffer);
#endif
}

const GpuBufferDesc& GpuBufferGLES::GetDesc() const
{
    return desc_;
}

const GpuBufferPlatformDataGL& GpuBufferGLES::GetPlatformData() const
{
    return plat_;
}

void* GpuBufferGLES::Map()
{
    if (!isMappable_) {
        PLUGIN_LOG_E("trying to map non-mappable gpu buffer");
        return nullptr;
    }
    if (isMapped_) {
        PLUGIN_LOG_E("gpu buffer already mapped");
        Unmap();
    }
    isMapped_ = true;

    if (isRingBuffer_) {
        plat_.currentByteOffset += plat_.alignedBindByteSize;
        if (plat_.currentByteOffset >= plat_.alignedByteSize) {
            plat_.currentByteOffset = 0;
        }
    }

    void* ret = nullptr;
    if (isPersistantlyMapped_) {
        if (data_) {
            ret = data_ + plat_.currentByteOffset;
        }
    } else {
        PLUGIN_ASSERT(device_.IsActive());
        const auto oldBind = device_.BoundBuffer(GL_COPY_WRITE_BUFFER);
        device_.BindBuffer(GL_COPY_WRITE_BUFFER, plat_.buffer);
        if (!isRingBuffer_) {
            ret = glMapBufferRange(GL_COPY_WRITE_BUFFER, 0, static_cast<GLsizeiptr>(plat_.alignedByteSize),
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        } else {
            ret = glMapBufferRange(GL_COPY_WRITE_BUFFER, static_cast<GLintptr>(plat_.currentByteOffset),
                static_cast<GLsizeiptr>(plat_.bindMemoryByteSize), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
        }
        device_.BindBuffer(GL_COPY_WRITE_BUFFER, oldBind);
    }
    return ret;
}

void GpuBufferGLES::Unmap() const
{
    if (!isMappable_) {
        PLUGIN_LOG_E("trying to unmap non-mappable gpu buffer");
    }
    if (!isMapped_) {
        PLUGIN_LOG_E("gpu buffer not mapped");
    }
    isMapped_ = false;

    if (!isPersistantlyMapped_) {
        PLUGIN_ASSERT(device_.IsActive());
        const auto oldBind = device_.BoundBuffer(GL_COPY_WRITE_BUFFER);
        device_.BindBuffer(GL_COPY_WRITE_BUFFER, plat_.buffer);
        glUnmapBuffer(GL_COPY_WRITE_BUFFER);
        device_.BindBuffer(GL_COPY_WRITE_BUFFER, oldBind);
    }
}

void* GpuBufferGLES::MapMemory()
{
    if (!isMappable_) {
        PLUGIN_LOG_E("trying to map non-mappable gpu buffer");
        return nullptr;
    }
    if (isMapped_) {
        PLUGIN_LOG_E("gpu buffer already mapped");
        Unmap();
    }
    isMapped_ = true;

    void* ret = nullptr;
    if (isPersistantlyMapped_) {
        ret = data_;
    } else {
        PLUGIN_ASSERT(device_.IsActive());
        const auto oldBind = device_.BoundBuffer(GL_COPY_WRITE_BUFFER);
        device_.BindBuffer(GL_COPY_WRITE_BUFFER, plat_.buffer);
        if (!isRingBuffer_) {
            ret = glMapBufferRange(GL_COPY_WRITE_BUFFER, 0, static_cast<GLsizeiptr>(plat_.alignedByteSize),
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        } else {
            ret = glMapBufferRange(GL_COPY_WRITE_BUFFER, 0, static_cast<GLsizeiptr>(plat_.alignedByteSize),
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
        }
        device_.BindBuffer(GL_COPY_WRITE_BUFFER, oldBind);
    }
    return ret;
}
RENDER_END_NAMESPACE()
