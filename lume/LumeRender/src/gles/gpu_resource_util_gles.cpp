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

void DebugBufferNameGLES(const IDevice& device, const GpuBuffer& buffer, const string_view name)
{
    const GpuBufferPlatformDataGL& cplat = (static_cast<const GpuBufferGLES&>(buffer)).GetPlatformData();
    GpuBufferPlatformDataGL& plat = const_cast<GpuBufferPlatformDataGL&>(cplat);
    if (plat.buffer) {
        glObjectLabel(GL_BUFFER, plat.buffer, (GLsizei)name.length(), name.data());
    }
}

void DebugImageNameGLES(const IDevice& device, const GpuImage& image, const string_view name)
{
    const GpuImagePlatformDataGL& plat = (static_cast<const GpuImageGLES&>(image)).GetPlatformData();
    if (plat.image) {
        glObjectLabel(GL_TEXTURE, plat.image, (GLsizei)name.length(), name.data());
    }
}

void DebugSamplerNameGLES(const IDevice& device, const GpuSampler& sampler, const string_view name)
{
    const GpuSamplerPlatformDataGL& plat = (static_cast<const GpuSamplerGLES&>(sampler)).GetPlatformData();
    if (plat.sampler) {
        glObjectLabel(GL_SAMPLER, plat.sampler, (GLsizei)name.length(), name.data());
    }
}
} // namespace GpuResourceUtil
RENDER_END_NAMESPACE()
