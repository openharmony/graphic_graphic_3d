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

#include "gpu_resource_util.h"

#include <base/containers/byte_array.h>
#include <render/device/intf_device.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "device/gpu_buffer.h"
#include "device/gpu_resource_handle_util.h"
#include "device/gpu_resource_manager.h"
#include "util/log.h"
#if RENDER_HAS_VULKAN_BACKEND
#include "vulkan/gpu_resource_util_vk.h"
#endif
#if RENDER_HAS_GLES_BACKEND || RENDER_HAS_GL_BACKEND
#include "gles/gpu_resource_util_gles.h"
#endif

RENDER_BEGIN_NAMESPACE()
namespace GpuResourceUtil {
void CopyGpuResource(const IDevice& device, const GpuResourceManager& gpuResourceMgr, const RenderHandle handle,
    BASE_NS::ByteArray& byteArray)
{
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handle);
    PLUGIN_ASSERT_MSG(handleType == RenderHandleType::GPU_BUFFER, "only gpu buffers supported");
    if (handleType == RenderHandleType::GPU_BUFFER) {
        GpuBuffer* resource = gpuResourceMgr.GetBuffer(handle);
        PLUGIN_ASSERT(resource);
        if (resource == nullptr) {
            return;
        }

        const DeviceBackendType backendType = device.GetBackendType();
#if RENDER_HAS_VULKAN_BACKEND
        if (backendType == DeviceBackendType::VULKAN) {
            CopyGpuBufferVk(*resource, byteArray);
        }
#endif
#if RENDER_HAS_GLES_BACKEND || RENDER_HAS_GL_BACKEND
        if ((backendType == DeviceBackendType::OPENGL) || (backendType == DeviceBackendType::OPENGLES)) {
            CopyGpuBufferGLES(*resource, byteArray);
        }
#endif
    }
}

void DebugBufferName(const IDevice& device, const GpuBuffer& buffer, const BASE_NS::string_view name)
{
    const DeviceBackendType backendType = device.GetBackendType();
#if RENDER_HAS_VULKAN_BACKEND
    if (backendType == DeviceBackendType::VULKAN) {
        DebugBufferNameVk(device, buffer, name);
    }
#endif
#if RENDER_HAS_GLES_BACKEND || RENDER_HAS_GL_BACKEND
    if (backendType == DeviceBackendType::OPENGL) {
        DebugBufferNameGLES(device, buffer, name);
    }
#endif
}

void DebugImageName(const IDevice& device, const GpuImage& image, const BASE_NS::string_view name)
{
    const DeviceBackendType backendType = device.GetBackendType();
#if RENDER_HAS_VULKAN_BACKEND
    if (backendType == DeviceBackendType::VULKAN) {
        DebugImageNameVk(device, image, name);
    }
#endif
#if RENDER_HAS_GLES_BACKEND || RENDER_HAS_GL_BACKEND
    if (backendType == DeviceBackendType::OPENGL) {
        DebugImageNameGLES(device, image, name);
    }
#endif
}

void DebugSamplerName(const IDevice& device, const GpuSampler& sampler, const BASE_NS::string_view name)
{
    const DeviceBackendType backendType = device.GetBackendType();
#if RENDER_HAS_VULKAN_BACKEND
    if (backendType == DeviceBackendType::VULKAN) {
        DebugSamplerNameVk(device, sampler, name);
    }
#endif
#if RENDER_HAS_GLES_BACKEND || RENDER_HAS_GL_BACKEND
    if (backendType == DeviceBackendType::OPENGL) {
        DebugSamplerNameGLES(device, sampler, name);
    }
#endif
}

} // namespace GpuResourceUtil
RENDER_END_NAMESPACE()
