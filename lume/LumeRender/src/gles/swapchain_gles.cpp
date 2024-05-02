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

#include "gles/swapchain_gles.h"

#include <base/math/vector.h>
#include <core/intf_engine.h>
#include <render/namespace.h>

#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "gles/gpu_image_gles.h"
#include "gles/surface_information.h"
#include "util/log.h"

using namespace BASE_NS;

// NOTE: think more about the initialization. (context creation etc..)
RENDER_BEGIN_NAMESPACE()
namespace {
struct FormatInfo {
    Format format;
    uint32_t r, g, b, a;
    GLenum glFormat;  // texture
    GLenum glFormat2; // renderable
};

// NOTE: add more mappings if needed.
static constexpr FormatInfo FORMATS[] = { { BASE_FORMAT_R8G8B8_UNORM, 8, 8, 8, 0, GL_RGB8, GL_RGB8 },
    { BASE_FORMAT_R8G8B8A8_UNORM, 8, 8, 8, 8, GL_RGBA8, GL_RGBA8 },
#if RENDER_HAS_GL_BACKEND
    { BASE_FORMAT_R16G16B16_UNORM, 16, 16, 16, 0, GL_RGB16, GL_RGB16 },
    { BASE_FORMAT_R16G16B16A16_UNORM, 16, 16, 16, 16, GL_RGBA16, GL_RGBA16 },
#endif
    { BASE_FORMAT_UNDEFINED, 0, 0, 0, 0, GL_NONE, GL_NONE } };

static constexpr FormatInfo FORMATS_SRGB[] = { { BASE_FORMAT_R8G8B8_SRGB, 8, 8, 8, 0, GL_SRGB8, GL_SRGB8_ALPHA8 },
    { BASE_FORMAT_R8G8B8A8_SRGB, 8, 8, 8, 8, GL_SRGB8_ALPHA8, GL_SRGB8_ALPHA8 },
    { BASE_FORMAT_UNDEFINED, 0, 0, 0, 0, GL_NONE, GL_NONE } };

SampleCountFlags SamplesToSampleCountFlags(uint32_t samples)
{
    SampleCountFlags sampleCount = 0;
    const SampleCountFlagBits bits[] = { CORE_SAMPLE_COUNT_1_BIT, CORE_SAMPLE_COUNT_2_BIT, CORE_SAMPLE_COUNT_4_BIT,
        CORE_SAMPLE_COUNT_8_BIT, CORE_SAMPLE_COUNT_16_BIT, CORE_SAMPLE_COUNT_32_BIT, CORE_SAMPLE_COUNT_64_BIT };
    for (int id = 0;; id++) {
        if (bits[id] == 0) {
            break;
        }
        if (samples == 0) {
            break;
        }
        if (samples & 1) {
            sampleCount |= (SampleCountFlags)bits[id];
        }
        samples >>= 1;
    }
    if (sampleCount == 0) {
        sampleCount = CORE_SAMPLE_COUNT_1_BIT;
    }
    return sampleCount;
}
Format GetDepthFormat(uint32_t depthSize, uint32_t stencilSize)
{
    auto depthFormat = BASE_FORMAT_UNDEFINED;
    // This mapping might not be exact.
    if (stencilSize == 0) {
        if (depthSize == 16) {
            depthFormat = BASE_FORMAT_D16_UNORM;
        } else if (depthSize == 24) {
            depthFormat = BASE_FORMAT_X8_D24_UNORM_PACK32;
        } else if (depthSize == 32) {
            depthFormat = BASE_FORMAT_D32_SFLOAT;
        }
    } else {
        if (depthSize == 16) {
            PLUGIN_LOG_E("unsupported depth stencil format D16_UNORM_S8_UINT fallback to D24_UNORM_S8_UINT");
            depthFormat = BASE_FORMAT_D24_UNORM_S8_UINT;
        } else if (depthSize == 24) {
            depthFormat = BASE_FORMAT_D24_UNORM_S8_UINT;
        } else if (depthSize == 32) {
            PLUGIN_LOG_E("unsupported depth stencil format D32_SFLOAT_S8_UINT fallback to D24_UNORM_S8_UINT");
            depthFormat = BASE_FORMAT_D24_UNORM_S8_UINT;
        }
    }
    return depthFormat;
}

Format RgbToFormat(uint32_t r, uint32_t g, uint32_t b, uint32_t a, bool srgb)
{
    const FormatInfo* format = nullptr;
    if (!srgb) {
        format = FORMATS;
    } else {
        format = FORMATS_SRGB;
    }
    for (int i = 0;; i++) {
        if (format[i].format == BASE_FORMAT_UNDEFINED) {
            // no match.
            break;
        }
        if ((format[i].r == r) && (format[i].g == g) && (format[i].b == b) && (format[i].a == a)) {
            return format[i].format;
        }
    }
    PLUGIN_LOG_E("Unsupported swapchain color channels");
    return BASE_FORMAT_UNDEFINED;
}

#if RENDER_GL_FLIP_Y_SWAPCHAIN
GLenum FormatToGlFormat(Format colorFormat)
{
    const FormatInfo* steps[] = { FORMATS, FORMATS_SRGB, nullptr };
    for (int step = 0;; step++) {
        const FormatInfo* format = steps[step];
        if (steps[step] == nullptr) {
            // no match in any lists..
            break;
        }

        for (int i = 0;; i++) {
            if (format[i].format == BASE_FORMAT_MAX_ENUM) {
                // no match.
                break;
            }
            if (format[i].format == colorFormat) {
                return format[i].glFormat2;
            }
        }
    }
    PLUGIN_LOG_E("Unsupported swapchain format");
    return GL_NONE;
}
#endif
GlesImplementation::SurfaceInfo ExtractInfo(DeviceGLES& device, const uint64_t surfaceHandle) noexcept
{
    GlesImplementation::SurfaceInfo info;
    const auto& EGLState = device.GetEglState();
#if RENDER_HAS_GLES_BACKEND
    if (device.GetBackendType() == DeviceBackendType::OPENGLES) {
        const auto& devicePlatformData = (const DevicePlatformDataGLES&)device.GetPlatformData();
        EGLSurface surface = (EGLSurface)surfaceHandle;
        if (!EGLState.GetSurfaceInformation(devicePlatformData, surface, info)) {
            PLUGIN_LOG_E("Could not query surface information");
        }
        if (!surface) {
            PLUGIN_LOG_E("Surface is null");
        }
    }
#endif
#if RENDER_HAS_GL_BACKEND
    if (device.GetBackendType() == DeviceBackendType::OPENGL) {
#if _WIN32
        HDC surface = (HDC)surfaceHandle;
        if (!EGLState.GetSurfaceInformation(surface, info)) {
            PLUGIN_LOG_E("Could not query surface information");
        }
        if (!surface) {
            PLUGIN_LOG_E("Surface is null");
        }
#else
#error Core::DeviceBackendType::OPENGL not implemented for this platform yet.
#endif
    }
#endif
    return info;
}

GpuImageDesc GenerateDescriptor(bool depth, Format format, uint32_t width, uint32_t height, uint32_t samples) noexcept
{
    GpuImageDesc res = {
        ImageType::CORE_IMAGE_TYPE_2D,                                            // imageType
        ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,                                   // imageViewType
        format,                                                                   // format
        ImageTiling::CORE_IMAGE_TILING_OPTIMAL,                                   // imageTiling
        0,                                                                        // usageFlags
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,            // memoryPropertyFlags
        0,                                                                        // createFlags
        EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, // engineCreationFlags
        width,                                                                    // width
        height,                                                                   // height
        1,                                                                        // depth
        1,                                                                        // mipCount
        1,                                                                        // layerCount
        SamplesToSampleCountFlags(samples),                                       // sampleCountFlags
        {}                                                                        // componentMapping
    };
    if (depth) {
        res.usageFlags = ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                         ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                         ImageUsageFlagBits::CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        res.memoryPropertyFlags |= MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
    } else {
        res.usageFlags = ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                         ImageUsageFlagBits::CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        res.engineCreationFlags |= EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS;
    }
    return res;
}
#if RENDER_GL_FLIP_Y_SWAPCHAIN
void GenerateTextures(
    DeviceGLES& device, uint32_t count, GLenum colorf, uint32_t sampleCount, SwapchainPlatformDataGL& plat)
{
    constexpr const uint32_t TEMP_BIND_UNIT = 15; // Use texture unit 15 as a temporary bind spot.
    uint32_t binding;
    if (sampleCount > 1) {
        binding = device.BoundTexture(TEMP_BIND_UNIT, GL_TEXTURE_2D_MULTISAMPLE);
    } else {
        binding = device.BoundTexture(TEMP_BIND_UNIT, GL_TEXTURE_2D);
    }
    Math::UVec2 size { plat.swapchainImages.width, plat.swapchainImages.height };
    plat.swapchainImages.images.resize(1); // Supports multiple images, see if it helps performance.

    glGenTextures(static_cast<GLsizei>(plat.swapchainImages.images.size()), plat.swapchainImages.images.data());
    for (size_t i = 0; i < plat.swapchainImages.images.size(); i++) {
        GLuint tid = plat.swapchainImages.images[i];
        if (sampleCount > 1) {
            device.TexStorage2DMultisample(tid, GL_TEXTURE_2D_MULTISAMPLE, sampleCount, colorf, size, false);
        } else {
            device.TexStorage2D(tid, GL_TEXTURE_2D, 1, colorf, size);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    // Currently missing pseudo depth buffer for "CORE_DEFAULT_BACKBUFFER_DEPTH"
    if (sampleCount > 1) {
        device.BindTexture(TEMP_BIND_UNIT, GL_TEXTURE_2D_MULTISAMPLE, binding);
    } else {
        device.BindTexture(TEMP_BIND_UNIT, GL_TEXTURE_2D, binding);
    }
}
void GenerateFBO(DeviceGLES& device, SwapchainPlatformDataGL& plat, bool msaa)
{
    // create fbos to be used during present (blit from this fbo to backbuffer)
    plat.fbos.resize(plat.swapchainImages.images.size());
    glGenFramebuffers(static_cast<GLsizei>(plat.fbos.size()), plat.fbos.data());
    for (size_t i = 0; i < plat.fbos.size(); i++) {
        device.BindFrameBuffer(plat.fbos[i]);
        GLenum texType;
        if (msaa) {
            texType = GL_TEXTURE_2D_MULTISAMPLE;
        } else {
            texType = GL_TEXTURE_2D;
        }
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texType, plat.swapchainImages.images[i], 0);
        GLenum status;
        status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            PLUGIN_LOG_E("Framebuffer incomplete (status: %x)", status);
        }
    }
    device.BindFrameBuffer(0);
}
#endif
} // namespace

SwapchainGLES::SwapchainGLES(Device& device, const SwapchainCreateInfo& swapchainCreateInfo)
    : device_((DeviceGLES&)device), flags_(swapchainCreateInfo.swapchainFlags)
{
    // check for surface creation automatically
    if ((swapchainCreateInfo.surfaceHandle == 0) && swapchainCreateInfo.window.window) {
        plat_.surface =
            device_.GetEglState().CreateSurface(swapchainCreateInfo.window.window, swapchainCreateInfo.window.instance);
        ownsSurface_ = true;
    } else {
        plat_.surface = (uintptr_t)swapchainCreateInfo.surfaceHandle;
    }

    // Fetch information about the backbuffer.
    // Create pseudo handles (or actual handles, depending if direct backbuffer rendering is enabled or not)
    GlesImplementation::SurfaceInfo info = ExtractInfo(device_, plat_.surface);
    const Format colorFormat = RgbToFormat(info.red_size, info.green_size, info.blue_size, info.alpha_size, info.srgb);
    PLUGIN_LOG_I("Input surface for swapchain is [%x] %dx%d R:%d G:%d B:%d A:%d D:%d S:%d samples:%d srgb:%s",
        info.configId, info.width, info.height, info.red_size, info.green_size, info.blue_size, info.alpha_size,
        info.depth_size, info.stencil_size, info.samples, info.srgb ? "true" : "false");
    info.width = Math::max(info.width, 1u);
    info.height = Math::max(info.height, 1u);
    plat_.swapchainImageIndex = 0;
    plat_.vsync = (flags_ & SwapchainFlagBits::CORE_SWAPCHAIN_VSYNC_BIT);
    plat_.swapchainImages.width = info.width;
    plat_.swapchainImages.height = info.height;
#if RENDER_GL_FLIP_Y_SWAPCHAIN
    // Create "swapchain" images to be used.
    // These are actual textures to be used during rendering. these will be blitted/flipped to backbuffer during
    // present.
    GenerateTextures(device_, 1, FormatToGlFormat(colorFormat), info.samples, plat_);
    GenerateFBO(device_, plat_, (info.samples > 1));
#else
    // Initialize one "swapchain" texture with GL name '0'
    // It will be identified based on that in nodecontextpoolmanager which creates (or in this case just uses default)
    // FBO.
    plat_.swapchainImages.images.resize(1);
#endif
    desc_ = GenerateDescriptor(false, colorFormat, info.width, info.height, info.samples);
    if (flags_ & SwapchainFlagBits::CORE_SWAPCHAIN_DEPTH_BUFFER_BIT) {
        if (info.depth_size == 0) {
            PLUGIN_LOG_V("Default backbuffer has no depth, but depth requested!");
        } else {
            const auto depthFormat = GetDepthFormat(info.depth_size, info.stencil_size);
            plat_.depthPlatformData = GpuImageGLES::GetPlatformData(device_, depthFormat);
            descDepthBuffer_ = GenerateDescriptor(true, depthFormat, info.width, info.height, info.samples);
        }
    }
}

SwapchainGLES::~SwapchainGLES()
{
#if RENDER_GL_FLIP_Y_SWAPCHAIN
    device_.Activate();
    for (auto id : plat_.fbos) {
        device_.DeleteFrameBuffer(id);
    }
    for (size_t i = 0; i < plat_.swapchainImages.images.size(); i++) {
        device_.DeleteTexture(plat_.swapchainImages.images[i]);
    }
    device_.Deactivate();
#endif
    if (ownsSurface_) {
        device_.GetEglState().DestroySurface(plat_.surface);
    }
}

const SwapchainPlatformDataGL& SwapchainGLES::GetPlatformData() const
{
    return plat_;
}

const GpuImageDesc& SwapchainGLES::GetDesc() const
{
    return desc_;
}

const GpuImageDesc& SwapchainGLES::GetDescDepthBuffer() const
{
    return descDepthBuffer_;
}

uint32_t SwapchainGLES::GetFlags() const
{
    return flags_;
}

uint32_t SwapchainGLES::GetNextImage() const
{
    PLUGIN_ASSERT(plat_.swapchainImages.images.size() <= 1U);
    // NOTE: does not flip images
    return plat_.swapchainImageIndex;
}

uint64_t SwapchainGLES::GetSurfaceHandle() const
{
    return (uint64_t)(plat_.surface);
}
RENDER_END_NAMESPACE()
