/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "gles/gpu_resource_util_gles.h"

#include <base/containers/byte_array.h>
#include <base/containers/string_view.h>
#include <render/gles/intf_device_gles.h>
#include <render/namespace.h>

#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "gles/gpu_buffer_gles.h"
#include "gles/gpu_image_gles.h"
#include "gles/gpu_sampler_gles.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace GpuResourceUtil {

void CopyGpuBufferGLES(GpuBuffer& buffer, ByteArray& byteArray)
{
    GpuBufferGLES& glesBuffer = (GpuBufferGLES&)buffer;
    if (const void* resData = glesBuffer.MapMemory(); resData) {
        const GpuBufferDesc& desc = glesBuffer.GetDesc();
        CloneData(byteArray.GetData().data(), byteArray.GetData().size_bytes(), (const uint8_t*)resData, desc.byteSize);
        glesBuffer.Unmap();
    }
}

void DebugBufferNameGLES(const Device& device, const GpuBuffer& buffer, const string_view name)
{
    const GpuBufferPlatformDataGL& cplat = (static_cast<const GpuBufferGLES&>(buffer)).GetPlatformData();
    GpuBufferPlatformDataGL& plat = const_cast<GpuBufferPlatformDataGL&>(cplat);
    if (plat.buffer) {
        glObjectLabel(GL_BUFFER, plat.buffer, (GLsizei)name.length(), name.data());
    }
}

void DebugImageNameGLES(const Device& device, const GpuImage& image, const string_view name)
{
    const GpuImagePlatformDataGL& plat = (static_cast<const GpuImageGLES&>(image)).GetPlatformData();
    if (plat.image) {
        glObjectLabel(GL_TEXTURE, plat.image, (GLsizei)name.length(), name.data());
    }
}

void DebugSamplerNameGLES(const Device& device, const GpuSampler& sampler, const string_view name)
{
    const GpuSamplerPlatformDataGL& plat = (static_cast<const GpuSamplerGLES&>(sampler)).GetPlatformData();
    if (plat.sampler) {
        glObjectLabel(GL_SAMPLER, plat.sampler, (GLsizei)name.length(), name.data());
    }
}
} // namespace GpuResourceUtil
RENDER_END_NAMESPACE()
