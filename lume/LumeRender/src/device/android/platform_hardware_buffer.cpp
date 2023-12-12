/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <android/hardware_buffer.h>
#include <android/hardware_buffer_jni.h>

#include <base/util/formats.h>

#include "device/device.h"

RENDER_BEGIN_NAMESPACE()
namespace {
BASE_NS::Format GetCoreFormatFromHwBufferFormat(uint32_t hwBufferFormat)
{
    BASE_NS::Format format = BASE_NS::Format::BASE_FORMAT_UNDEFINED;
    if (hwBufferFormat == AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM) {
        format = BASE_NS::Format::BASE_FORMAT_R8G8B8A8_UNORM;
    } else if (hwBufferFormat == AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM) {
        format = BASE_NS::Format::BASE_FORMAT_R8G8B8A8_UNORM;
    } else if (hwBufferFormat == AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM) {
        format = BASE_NS::Format::BASE_FORMAT_R8G8B8_UNORM;
    } else if (hwBufferFormat == AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM) {
        format = BASE_NS::Format::BASE_FORMAT_R5G6B5_UNORM_PACK16;
    } else if (hwBufferFormat == AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT) {
        format = BASE_NS::Format::BASE_FORMAT_R16G16B16A16_SFLOAT;
    } else if (hwBufferFormat == AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM) {
        format = BASE_NS::Format::BASE_FORMAT_A2B10G10R10_UNORM_PACK32;
    } else if (hwBufferFormat == AHARDWAREBUFFER_FORMAT_BLOB) {
        // technically we would need to create a buffer
        format = BASE_NS::Format::BASE_FORMAT_UNDEFINED;
    } else if (hwBufferFormat == AHARDWAREBUFFER_FORMAT_D16_UNORM) {
        format = BASE_NS::Format::BASE_FORMAT_D16_UNORM;
    } else if (hwBufferFormat == AHARDWAREBUFFER_FORMAT_D24_UNORM) {
        format = BASE_NS::Format::BASE_FORMAT_X8_D24_UNORM_PACK32;
    } else if (hwBufferFormat == AHARDWAREBUFFER_FORMAT_D24_UNORM_S8_UINT) {
        format = BASE_NS::Format::BASE_FORMAT_D24_UNORM_S8_UINT;
    } else if (hwBufferFormat == AHARDWAREBUFFER_FORMAT_D32_FLOAT) {
        format = BASE_NS::Format::BASE_FORMAT_D32_SFLOAT;
    } else if (hwBufferFormat == AHARDWAREBUFFER_FORMAT_D32_FLOAT_S8_UINT) {
        format = BASE_NS::Format::BASE_FORMAT_D32_SFLOAT_S8_UINT;
    } else if (hwBufferFormat == AHARDWAREBUFFER_FORMAT_S8_UINT) {
        format = BASE_NS::Format::BASE_FORMAT_S8_UINT;
    }
#if (__ANDROID_API__ >= 28)
    else if (hwBufferFormat == AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420) {
        format = BASE_NS::Format::BASE_FORMAT_G8_B8R8_2PLANE_420_UNORM;
    }
#endif

    // the format can be undefined external format as well
    return format;
}
} // namespace

GpuImageDesc GetImageDescFromHwBufferDesc(uintptr_t platformHwBuffer)
{
    auto* aHwBuffer = reinterpret_cast<AHardwareBuffer*>(platformHwBuffer);
    AHardwareBuffer_Desc desc;
    AHardwareBuffer_describe(aHwBuffer, &desc);

    ImageViewType imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
    ImageCreateFlags createFlags = 0;
    if (desc.layers > 1) {
        if ((desc.usage & AHARDWAREBUFFER_USAGE_GPU_CUBE_MAP) == 0) {
            imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D_ARRAY;
            createFlags |= CORE_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
        } else if (desc.layers == 6U) {
            imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_CUBE;
            createFlags |= CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        } else if ((desc.layers % 6U) == 0U) {
            imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            createFlags |= CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }
    }

    // NOTE: some formats seems to be in unknown state here (e.g. Android Image hw buffer)
    const BASE_NS::Format format = GetCoreFormatFromHwBufferFormat(desc.format);

    ImageUsageFlags usage = 0U;
    if (desc.usage & AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE) {
        usage |= CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    }
    if (desc.usage & AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER) {
        if ((desc.format >= AHARDWAREBUFFER_FORMAT_D16_UNORM) && (desc.format <= AHARDWAREBUFFER_FORMAT_S8_UINT)) {
            usage |= CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        } else {
            usage |= CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
    }
    if (desc.usage & AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER) {
        usage |= CORE_IMAGE_USAGE_STORAGE_BIT;
    }

    // spec says "images with external formats must only be used as sampled images"
    if (format == BASE_NS::BASE_FORMAT_UNDEFINED) {
        usage &= CORE_IMAGE_USAGE_SAMPLED_BIT;
    }

    auto ret = GpuImageDesc {
        ImageType::CORE_IMAGE_TYPE_2D,
        imageViewType,
        format,
        CORE_IMAGE_TILING_OPTIMAL,
        usage,
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        createFlags,
        CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS,
        desc.width,
        desc.height,
        1,
        1,
        desc.layers,
        SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,
        // Use identity mapping, older version needed to swap B and G channels to get colors correct when usage is not
        // defined for ImageReader (API level < 29) but that causes incorrect colors with new Adrenos, Mali seems to
        // ignore the swizzle totally
        { CORE_COMPONENT_SWIZZLE_R, CORE_COMPONENT_SWIZZLE_G, CORE_COMPONENT_SWIZZLE_B, CORE_COMPONENT_SWIZZLE_A },
    };
    return ret;
}
RENDER_END_NAMESPACE()
