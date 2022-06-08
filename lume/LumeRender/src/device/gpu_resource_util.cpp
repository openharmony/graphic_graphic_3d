/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "gpu_resource_util.h"

#include <base/containers/byte_array.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "device/device.h"
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
void CopyGpuResource(const Device& device, const GpuResourceManager& gpuResourceMgr, const RenderHandle handle,
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

void DebugBufferName(const Device& device, const GpuBuffer& buffer, const BASE_NS::string_view name)
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

void DebugImageName(const Device& device, const GpuImage& image, const BASE_NS::string_view name)
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

void DebugSamplerName(const Device& device, const GpuSampler& sampler, const BASE_NS::string_view name)
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
