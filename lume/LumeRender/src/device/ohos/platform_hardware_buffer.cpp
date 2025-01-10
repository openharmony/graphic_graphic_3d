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

#include <surface/native_buffer.h>

#include <base/util/formats.h>

#include "device/device.h"
#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
namespace {
using BASE_NS::Format;
constexpr BASE_NS::pair<OH_NativeBuffer_Format, Format> FORMATS[] = {
    { NATIVEBUFFER_PIXEL_FMT_CLUT8, Format::BASE_FORMAT_UNDEFINED },
    { NATIVEBUFFER_PIXEL_FMT_CLUT1, Format::BASE_FORMAT_UNDEFINED },
    { NATIVEBUFFER_PIXEL_FMT_CLUT4, Format::BASE_FORMAT_UNDEFINED },
    { NATIVEBUFFER_PIXEL_FMT_RGB_565, Format::BASE_FORMAT_R5G6B5_UNORM_PACK16 },           // RGB565
    { NATIVEBUFFER_PIXEL_FMT_RGBA_5658, Format::BASE_FORMAT_UNDEFINED },                   // RGBA5658
    { NATIVEBUFFER_PIXEL_FMT_RGBX_4444, Format::BASE_FORMAT_UNDEFINED },                   // RGBX4444
    { NATIVEBUFFER_PIXEL_FMT_RGBA_4444, Format::BASE_FORMAT_R4G4B4A4_UNORM_PACK16 },       // RGBA4444
    { NATIVEBUFFER_PIXEL_FMT_RGB_444, Format::BASE_FORMAT_UNDEFINED },                     // RGB444
    { NATIVEBUFFER_PIXEL_FMT_RGBX_5551, Format::BASE_FORMAT_UNDEFINED },                   // RGBX5551
    { NATIVEBUFFER_PIXEL_FMT_RGBA_5551, Format::BASE_FORMAT_R5G5B5A1_UNORM_PACK16 },       // RGBA5551
    { NATIVEBUFFER_PIXEL_FMT_RGB_555, Format::BASE_FORMAT_UNDEFINED },                     // RGB555
    { NATIVEBUFFER_PIXEL_FMT_RGBX_8888, Format::BASE_FORMAT_UNDEFINED },                   // RGBX8888
    { NATIVEBUFFER_PIXEL_FMT_RGBA_8888, Format::BASE_FORMAT_R8G8B8A8_UNORM },              // RGB888
    { NATIVEBUFFER_PIXEL_FMT_RGB_888, Format::BASE_FORMAT_R8G8B8_UNORM },                  // RGBA8888
    { NATIVEBUFFER_PIXEL_FMT_BGR_565, Format::BASE_FORMAT_B5G6R5_UNORM_PACK16 },           // BGR565
    { NATIVEBUFFER_PIXEL_FMT_BGRX_4444, Format::BASE_FORMAT_UNDEFINED },                   // BGRX4444
    { NATIVEBUFFER_PIXEL_FMT_BGRA_4444, Format::BASE_FORMAT_B4G4R4A4_UNORM_PACK16 },       // BGRA4444
    { NATIVEBUFFER_PIXEL_FMT_BGRX_5551, Format::BASE_FORMAT_UNDEFINED },                   // BGRX5551
    { NATIVEBUFFER_PIXEL_FMT_BGRA_5551, Format::BASE_FORMAT_B5G5R5A1_UNORM_PACK16 },       // BGRA5551
    { NATIVEBUFFER_PIXEL_FMT_BGRX_8888, Format::BASE_FORMAT_UNDEFINED },                   // BGRX8888
    { NATIVEBUFFER_PIXEL_FMT_BGRA_8888, Format::BASE_FORMAT_B8G8R8A8_UNORM },              // BGRA8888
    { NATIVEBUFFER_PIXEL_FMT_YUV_422_I, Format::BASE_FORMAT_UNDEFINED },                   // YUV422 interleaved
    { NATIVEBUFFER_PIXEL_FMT_YCBCR_422_SP, Format::BASE_FORMAT_G8_B8R8_2PLANE_422_UNORM }, // YCBCR422 semi-plannar
    { NATIVEBUFFER_PIXEL_FMT_YCRCB_422_SP, Format::BASE_FORMAT_UNDEFINED },                // YCRCB422 semi-plannar
    { NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, Format::BASE_FORMAT_G8_B8R8_2PLANE_420_UNORM }, // YCBCR420 semi-plannar
    { NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP, Format::BASE_FORMAT_UNDEFINED },                // YCRCB420 semi-plannar
    { NATIVEBUFFER_PIXEL_FMT_YCBCR_422_P, Format::BASE_FORMAT_G8_B8_R8_3PLANE_422_UNORM }, // YCBCR422 plannar
    { NATIVEBUFFER_PIXEL_FMT_YCRCB_422_P, Format::BASE_FORMAT_UNDEFINED },                 // YCRCB422 plannar
    { NATIVEBUFFER_PIXEL_FMT_YCBCR_420_P, Format::BASE_FORMAT_G8_B8_R8_3PLANE_420_UNORM }, // YCBCR420 plannar NV12
    { NATIVEBUFFER_PIXEL_FMT_YCRCB_420_P, Format::BASE_FORMAT_UNDEFINED },                 // YCRCB420 plannar NV21
    { NATIVEBUFFER_PIXEL_FMT_YUYV_422_PKG, Format::BASE_FORMAT_UNDEFINED },                // YUYV422 packed
    { NATIVEBUFFER_PIXEL_FMT_UYVY_422_PKG, Format::BASE_FORMAT_UNDEFINED },                // UYVY422 packed
    { NATIVEBUFFER_PIXEL_FMT_YVYU_422_PKG, Format::BASE_FORMAT_UNDEFINED },                // YVYU422 packed
    { NATIVEBUFFER_PIXEL_FMT_VYUY_422_PKG, Format::BASE_FORMAT_UNDEFINED },                // VYUY422 packed
    { NATIVEBUFFER_PIXEL_FMT_RGBA_1010102, Format::BASE_FORMAT_A2R10G10B10_UNORM_PACK32 }, // RGBA_1010102 packed
    { NATIVEBUFFER_PIXEL_FMT_YCBCR_P010, Format::BASE_FORMAT_UNDEFINED }, // YCBCR420 semi-planar 10bit packed
    { NATIVEBUFFER_PIXEL_FMT_YCRCB_P010, Format::BASE_FORMAT_UNDEFINED }, // YCRCB420 semi-planar 10bit packed
    { NATIVEBUFFER_PIXEL_FMT_RAW10, Format::BASE_FORMAT_UNDEFINED },      // Raw 10bit packed
};

BASE_NS::Format GetCoreFormatFromNativeBufferFormat(uint32_t nativeBufferFormat)
{
    if (nativeBufferFormat < BASE_NS::countof(FORMATS)) {
        return FORMATS[nativeBufferFormat].second;
    }
    return Format::BASE_FORMAT_UNDEFINED;
}
} // namespace

GpuImageDesc GetImageDescFromHwBufferDesc(uintptr_t platformHwBuffer)
{
    auto* aHwBuffer = reinterpret_cast<OH_NativeBuffer*>(platformHwBuffer);
    OH_NativeBuffer_Config config;
    OH_NativeBuffer_GetConfig(aHwBuffer, &config);
    OH_NativeBuffer_ColorSpace colorSpace = OH_COLORSPACE_NONE;
    if (OH_NativeBuffer_GetColorSpace(aHwBuffer, &colorSpace) != 0) {
        PLUGIN_LOG_W("Could not get NativeBuffer colors space.");
    } else {
        PLUGIN_LOG_I("NativeBuffer colors space %d.", colorSpace);
    }
    ImageViewType imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
    ImageCreateFlags createFlags = 0;

    BASE_NS::Format format = GetCoreFormatFromNativeBufferFormat(config.format);
    if ((colorSpace == OH_COLORSPACE_SRGB_FULL) || (colorSpace == OH_COLORSPACE_DISPLAY_SRGB)) {
        if (format == BASE_NS::Format::BASE_FORMAT_R8G8B8A8_UNORM) {
            format = BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SRGB;
        } else if (format == BASE_NS::Format::BASE_FORMAT_R8G8B8_UNORM) {
            format = BASE_NS::Format::BASE_FORMAT_R8G8B8_SRGB;
        } else if (format == BASE_NS::Format::BASE_FORMAT_B8G8R8A8_UNORM) {
            format = BASE_NS::Format::BASE_FORMAT_B8G8R8A8_SRGB;
        }
    }

    ImageUsageFlags usage = CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (format == BASE_NS::BASE_FORMAT_UNDEFINED) {
        usage &= CORE_IMAGE_USAGE_SAMPLED_BIT;
    }

    return GpuImageDesc {
        ImageType::CORE_IMAGE_TYPE_2D,
        imageViewType,
        format,
        CORE_IMAGE_TILING_OPTIMAL,
        usage,
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        createFlags,
        CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS,
        config.width,
        config.height,
        1,
        1,
        1,
        SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,
        { CORE_COMPONENT_SWIZZLE_R, CORE_COMPONENT_SWIZZLE_G, CORE_COMPONENT_SWIZZLE_B, CORE_COMPONENT_SWIZZLE_A },
    };
}
RENDER_END_NAMESPACE()
