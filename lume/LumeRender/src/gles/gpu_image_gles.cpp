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

#include "gpu_image_gles.h"

#if (RENDER_PERF_ENABLED == 1)
#include <core/implementation_uids.h>
#include <core/perf/intf_performance_data_manager.h>
#endif

#include <base/math/vector.h>
#include <render/namespace.h>

#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "gles/swapchain_gles.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
#if (RENDER_PERF_ENABLED == 1)
void RecordAllocation(const int64_t alignedByteSize)
{
    if (auto* inst = CORE_NS::GetInstance<CORE_NS::IPerformanceDataManagerFactory>(CORE_NS::UID_PERFORMANCE_FACTORY);
        inst) {
        CORE_NS::IPerformanceDataManager* pdm = inst->Get("Memory");
        pdm->UpdateData("AllGpuImages", "GPU_IMAGE", alignedByteSize);
    }
}
#endif

GpuImagePlatformDataGL ConvertFormat(const DeviceGLES::ImageFormat& fmt)
{
    GpuImagePlatformDataGL result {};
    if (fmt.coreFormat == BASE_FORMAT_UNDEFINED) {
        PLUGIN_ASSERT_MSG(false, "Unsupported texture format in GpuImageGLES::convertFormat!");
    }
    result.bytesperpixel = fmt.bytesperpixel;
    result.dataType = fmt.dataType;
    result.format = fmt.format;
    result.internalFormat = fmt.internalFormat;
    result.compression = { fmt.compression.compressed, fmt.compression.blockW, fmt.compression.blockH,
        fmt.compression.bytesperblock };
    result.swizzle = fmt.swizzle;
    return result;
}

constexpr uint32_t ConvertSampleCountFlags(SampleCountFlags flags)
{
    uint32_t sampleCount = 1;
    if ((uint32_t)flags != 1) {
        // MSAA
        sampleCount = (uint32_t)flags;
    }
    return sampleCount;
}

GLenum MapSwizzle(ComponentSwizzle swizzle, uint32_t input)
{
    switch (swizzle) {
        case CORE_COMPONENT_SWIZZLE_IDENTITY:
            return input;
        case CORE_COMPONENT_SWIZZLE_ZERO:
            return GL_ZERO;
        case CORE_COMPONENT_SWIZZLE_ONE:
            return GL_ONE;
        case CORE_COMPONENT_SWIZZLE_R:
            return GL_RED;
        case CORE_COMPONENT_SWIZZLE_G:
            return GL_GREEN;
        case CORE_COMPONENT_SWIZZLE_B:
            return GL_BLUE;
        case CORE_COMPONENT_SWIZZLE_A:
            return GL_ALPHA;
        default:
            break;
    }
    return input;
}

void DoSwizzle(const GpuImageDesc& desc, GpuImagePlatformDataGL& plat)
{
    plat.swizzle.x = MapSwizzle(desc.componentMapping.r, plat.swizzle.x);
    plat.swizzle.y = MapSwizzle(desc.componentMapping.g, plat.swizzle.y);
    plat.swizzle.z = MapSwizzle(desc.componentMapping.b, plat.swizzle.z);
    plat.swizzle.w = MapSwizzle(desc.componentMapping.a, plat.swizzle.w);
}

void GenerateImageStorage(DeviceGLES& device, const GpuImageDesc& desc, GpuImagePlatformDataGL& plat)
{
    const uint32_t sampleCount = ConvertSampleCountFlags(desc.sampleCountFlags);
    glGenTextures(1, &plat.image);
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
    if (plat.image) {
        PLUGIN_LOG_D("gpu image id >: %u", plat.image);
    }
#endif

    const Math::UVec2 size2D { desc.width, desc.height };
    switch (desc.imageViewType) {
        case CORE_IMAGE_VIEW_TYPE_2D: {
            PLUGIN_ASSERT_MSG(desc.layerCount == 1, "layerCount != 1 for normal texture!");
            if (sampleCount > 1) {
                plat.type = GL_TEXTURE_2D_MULTISAMPLE;
                // must use fixed sample locations so that renderbuffer/texture fbos work.
                device.TexStorage2DMultisample(plat.image, plat.type, sampleCount, plat.internalFormat, size2D, true);
            } else {
                plat.type = GL_TEXTURE_2D;
                device.TexStorage2D(plat.image, plat.type, desc.mipCount, plat.internalFormat, size2D);
            }
            break;
        }
        case CORE_IMAGE_VIEW_TYPE_CUBE: {
            PLUGIN_ASSERT_MSG(desc.layerCount == 6, "layerCount != 6 for cubemap!");
            plat.type = GL_TEXTURE_CUBE_MAP;
            device.TexStorage2D(plat.image, plat.type, desc.mipCount, plat.internalFormat, size2D);
            break;
        }
        case CORE_IMAGE_VIEW_TYPE_2D_ARRAY: {
            PLUGIN_ASSERT_MSG(desc.layerCount <= 256, "layerCount > 256 for 2D Array!");
            plat.type = GL_TEXTURE_2D_ARRAY;
            device.TexStorage3D(plat.image, plat.type, desc.mipCount, plat.internalFormat,
                { desc.width, desc.height, desc.layerCount });
            break;
        }
        case CORE_IMAGE_VIEW_TYPE_3D: {
            plat.type = GL_TEXTURE_3D;
            device.TexStorage3D(
                plat.image, plat.type, desc.mipCount, plat.internalFormat, { desc.width, desc.height, desc.depth });
            break;
        }
        case CORE_IMAGE_VIEW_TYPE_1D:
        case CORE_IMAGE_VIEW_TYPE_1D_ARRAY:
        case CORE_IMAGE_VIEW_TYPE_CUBE_ARRAY:
        default:
            PLUGIN_ASSERT_MSG(false, "UNSUPPORTED IMAGEVIEW TYPE");
            plat.type = GL_NONE;
            break;
    }
    // set swizzle.
    DoSwizzle(desc, plat);
    device.TexSwizzle(plat.image, plat.type, plat.swizzle);
}
} // namespace

GpuImagePlatformDataGL GpuImageGLES::GetPlatformData(const DeviceGLES& device, Format format)
{
    const auto& fmt = device.GetGlImageFormat(format);
    return ConvertFormat(fmt);
}

GpuImageGLES::GpuImageGLES(Device& device, const GpuImageDesc& desc)
    : GpuImage(), device_((DeviceGLES&)device), desc_(desc)
{
    PLUGIN_ASSERT(device_.IsActive());
    PLUGIN_ASSERT_MSG(desc_.memoryPropertyFlags & MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Device local memory is only memory property supported for GLES GpuImage (flags: %u)",
        desc_.memoryPropertyFlags);
    plat_ = GetPlatformData(device_, desc_.format);
    // Check for formats that require special attention.
    if ((desc.usageFlags & CORE_IMAGE_USAGE_STORAGE_BIT)) {
        // Force the format to "other" (there might be others that require "special handling".
        if (desc_.format == BASE_FORMAT_B10G11R11_UFLOAT_PACK32) {
            plat_ = GetPlatformData(device_, BASE_FORMAT_R16G16B16A16_SFLOAT);
        }
    }
    const bool isSrc = (desc_.usageFlags & CORE_IMAGE_USAGE_TRANSFER_SRC_BIT);
    const bool isDst = (desc_.usageFlags & CORE_IMAGE_USAGE_TRANSFER_DST_BIT);
    const bool isSample = (desc_.usageFlags & CORE_IMAGE_USAGE_SAMPLED_BIT);
    const bool isStorage = (desc_.usageFlags & CORE_IMAGE_USAGE_STORAGE_BIT);
    // could check for bool isColor = (desc_.usageFlags & CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    // could check for bool isDepth = (desc_.usageFlags & CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    // could check for bool isTrans = (desc_.usageFlags & CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
    const bool isInput = (desc_.usageFlags & CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
    if ((!isSrc) && (!isDst) && (!isSample) && (!isStorage) && (!isInput)) {
        // Use render buffers for "non-input,non-compute,non-texture,no src/dst transfer" images
        // ie. temporary render targets
        glGenRenderbuffers(1, &plat_.renderBuffer);
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
        if (plat_.renderBuffer) {
            PLUGIN_LOG_D("gpu renderbuffer id >: %u", plat_.renderBuffer);
        }
#endif
        glBindRenderbuffer(GL_RENDERBUFFER, plat_.renderBuffer);
        const uint32_t sampleCount = ConvertSampleCountFlags(desc_.sampleCountFlags);
        if (sampleCount > 1) {
#if RENDER_HAS_GLES_BACKEND
            // Sadly Mali driver doesn't understand a multisampled render buffer created with the core function attached
            // to the same FBO with FramebufferTexture2DMultisampleEXT attachments, so in order to use that on Mali
            // render buffer must also use the EXT function.
            if (device_.HasExtension("GL_EXT_multisampled_render_to_texture2")) {
                glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, (GLsizei)sampleCount, plat_.internalFormat,
                    (GLsizei)desc_.width, (GLsizei)desc_.height);
            } else {
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, (GLsizei)sampleCount, plat_.internalFormat,
                    (GLsizei)desc_.width, (GLsizei)desc_.height);
            }
#else
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, (GLsizei)sampleCount, plat_.internalFormat,
                (GLsizei)desc_.width, (GLsizei)desc_.height);
#endif
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER, plat_.internalFormat, (GLsizei)desc_.width, (GLsizei)desc_.height);
        }
    } else {
        GenerateImageStorage(device_, desc_, plat_);
    }
#if (RENDER_PERF_ENABLED == 1)
    RecordAllocation(static_cast<int64_t>(plat_.bytesperpixel * desc_.width * desc_.height * desc_.depth));
#endif
}

GpuImageGLES::GpuImageGLES(Device& device, const GpuImageDesc& desc, const GpuImagePlatformData& platformData)
    : GpuImage(), device_((DeviceGLES&)device), plat_((const GpuImagePlatformDataGL&)platformData), desc_(desc),
      ownsResources_(false)
{
    PLUGIN_ASSERT(device_.IsActive());
#if (RENDER_HAS_GLES_BACKEND == 1)
    if (reinterpret_cast<GLeglImageOES>(plat_.eglImage) != 0) {
        glGenTextures(1, &plat_.image);
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
        PLUGIN_LOG_D("gpu image id >: %u", plat_.image);
#endif

        constexpr const uint32_t TEMP_BIND_UNIT = 15u;
        // store the previous tex id for restoring.
        const auto oldTex = device_.BoundTexture(TEMP_BIND_UNIT, GL_TEXTURE_EXTERNAL_OES);
        device_.BindTexture(TEMP_BIND_UNIT, GL_TEXTURE_EXTERNAL_OES, plat_.image);

        // need to make sure the correct texUnit is active when calling GL directly.
        device_.SetActiveTextureUnit(TEMP_BIND_UNIT);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, reinterpret_cast<GLeglImageOES>(plat_.eglImage));

        // set swizzle.
        DoSwizzle(desc, plat_);
        device_.TexSwizzle(plat_.image, GL_TEXTURE_EXTERNAL_OES, plat_.swizzle);

        // restore previous tex id.
        device_.BindTexture(TEMP_BIND_UNIT, GL_TEXTURE_EXTERNAL_OES, oldTex);
    }
#endif // RENDER_HAS_GLES_BACKEND
}

GpuImageGLES::~GpuImageGLES()
{
    PLUGIN_ASSERT(device_.IsActive());
    if (ownsResources_) {
        if (plat_.image) {
            device_.DeleteTexture(plat_.image);
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
            PLUGIN_LOG_D("gpu image id <: %u", plat_.image);
#endif
        }
        if (plat_.renderBuffer) {
            glDeleteRenderbuffers(1, &plat_.renderBuffer);
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
            PLUGIN_LOG_D("gpu render buffer id <: %u", plat_.renderBuffer);
#endif
        }

#if (RENDER_PERF_ENABLED == 1)
        RecordAllocation(-static_cast<int64_t>(plat_.bytesperpixel * desc_.width * desc_.height * desc_.depth));
#endif
    }
#if (RENDER_HAS_GLES_BACKEND == 1)
    if (reinterpret_cast<GLeglImageOES>(plat_.eglImage) != 0) {
        if (plat_.image) {
            device_.DeleteTexture(plat_.image);
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
            PLUGIN_LOG_D("gpu image id <: %u", plat_.image);
#endif
        }
    }
#endif
}

const GpuImageDesc& GpuImageGLES::GetDesc() const
{
    return desc_;
}

const GpuImagePlatformDataGL& GpuImageGLES::GetPlatformData() const
{
    return plat_;
}

const GpuImagePlatformData& GpuImageGLES::GetBasePlatformData() const
{
    return plat_;
}

GpuImage::AdditionalFlags GpuImageGLES::GetAdditionalFlags() const
{
    // expecting all egl images to need special platform conversion (e.g. OES binding)
    return (plat_.eglImage) ? ADDITIONAL_PLATFORM_CONVERSION_BIT : 0u;
}
RENDER_END_NAMESPACE()
