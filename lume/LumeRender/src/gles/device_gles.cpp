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

#include "device_gles.h"

#include <algorithm>

#include <base/containers/string.h>
#include <base/math/vector.h>
#include <base/util/compile_time_hashes.h>
#include <render/namespace.h>

#include "device/gpu_program_util.h"
#include "device/gpu_resource_manager.h"
#include "device/shader_manager.h"
#include "device/shader_module.h"
#include "gles/gl_functions.h"
#include "gles/gpu_buffer_gles.h"
#include "gles/gpu_image_gles.h"
#include "gles/gpu_program_gles.h"
#include "gles/gpu_sampler_gles.h"
#include "gles/gpu_semaphore_gles.h"
#include "gles/node_context_descriptor_set_manager_gles.h"
#include "gles/node_context_pool_manager_gles.h"
#include "gles/pipeline_state_object_gles.h"
#include "gles/render_backend_gles.h"
#include "gles/render_frame_sync_gles.h"
#include "gles/shader_module_gles.h"
#include "gles/swapchain_gles.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
// Make all temporary binds to unit GL_TEXTURE15. (should use the last available unit, so as to least affect actual
// usage) "The number of texture units is implementation-dependent, but must be at least 32. texture must be one of
// GL_TEXTUREi, where i ranges from zero to the value of GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS minus one." Currently our
// there is an implementation limit in our resource caching which limits it to 16... (this is why we use 16 instead of
// 32)
constexpr const uint32_t TEMP_BIND_UNIT = 15;
constexpr const string_view EXT_BUFFER_STORAGE = "GL_EXT_buffer_storage";
#if RENDER_GL_DEBUG
#define DUMP(a)                       \
    {                                 \
        GLint val;                    \
        glGetIntegerv(a, &val);       \
        PLUGIN_LOG_V(#a ": %d", val); \
    }

#define DUMP_INDEX(a, index)                     \
    {                                            \
        GLint val;                               \
        glGetIntegeri_v(a, index, &val);         \
        PLUGIN_LOG_V(#a "[%d]: %d", index, val); \
    }

static bool (*filterErrorFunc)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const string_view message, const void* userParam) noexcept = nullptr;

static auto SourceName(GLenum source)
{
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            return "GL_DEBUG_SOURCE_API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            return "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            return "GL_DEBUG_SOURCE_SHADER_COMPILER";
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            return "GL_DEBUG_SOURCE_THIRD_PARTY";
        case GL_DEBUG_SOURCE_APPLICATION:
            return "GL_DEBUG_SOURCE_APPLICATION";
        case GL_DEBUG_SOURCE_OTHER:
            return "GL_DEBUG_SOURCE_OTHER";

        default:
            break;
    }
    return "UNKNOWN";
}

static auto TypeName(GLenum type)
{
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            return "GL_DEBUG_TYPE_ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
        case GL_DEBUG_TYPE_PORTABILITY:
            return "GL_DEBUG_TYPE_PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE:
            return "GL_DEBUG_TYPE_PERFORMANCE";
        case GL_DEBUG_TYPE_MARKER:
            return "GL_DEBUG_TYPE_MARKER";
        case GL_DEBUG_TYPE_PUSH_GROUP:
            return "GL_DEBUG_TYPE_PUSH_GROUP";
        case GL_DEBUG_TYPE_POP_GROUP:
            return "GL_DEBUG_TYPE_POP_GROUP";
        case GL_DEBUG_TYPE_OTHER:
            return "GL_DEBUG_TYPE_OTHER";

        default:
            break;
    }
    return "UNKNOWN";
}

static auto SeverityName(GLenum severity)
{
    switch (severity) {
        case GL_DEBUG_SEVERITY_LOW:
            return "GL_DEBUG_SEVERITY_LOW";
        case GL_DEBUG_SEVERITY_MEDIUM:
            return "GL_DEBUG_SEVERITY_MEDIUM";
        case GL_DEBUG_SEVERITY_HIGH:
            return "GL_DEBUG_SEVERITY_HIGH";
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            return "GL_DEBUG_SEVERITY_NOTIFICATION";

        default:
            break;
    }
    return "UNKNOWN";
}

#ifndef APIENTRY
#define APIENTRY
#endif
static void APIENTRY OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar* message, const void* userParam) noexcept
{
    if (type == GL_DEBUG_TYPE_PUSH_GROUP) {
        return;
    } else if (type == GL_DEBUG_TYPE_POP_GROUP) {
        return;
    } else if ((filterErrorFunc) && (filterErrorFunc(source, type, id, severity, length, message, userParam))) {
        return;
    } else if (type == GL_DEBUG_TYPE_ERROR) {
        PLUGIN_LOG_E("---------------------opengl-callback-start------------\n"
                     "source: %s\n"
                     "type: %s\n"
                     "id: %u\n"
                     "severity: %s\n"
                     "message: %s\n"
                     "---------------------opengl-callback-end--------------\n",
            SourceName(source), TypeName(type), id, SeverityName(severity), message);
    } else {
        PLUGIN_LOG_D("---------------------opengl-callback-start------------\n"
                     "source: %s\n"
                     "type: %s\n"
                     "id: %u\n"
                     "severity: %s\n"
                     "message: %s\n"
                     "---------------------opengl-callback-end--------------\n",
            SourceName(source), TypeName(type), id, SeverityName(severity), message);
    }
}
#else
#define DUMP(a)
#define DUMP_INDEX(a, index)
#endif

struct FormatFeatures {
    GLenum internalFormat;
    FormatFeatureFlags flags;
};

// image store and atomic operations seem to go hand in hand
static constexpr const FormatFeatureFlags ATOMIC_STORE =
    CORE_FORMAT_FEATURE_STORAGE_IMAGE_BIT | CORE_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT;
// no writable texture buffers in gl?
static constexpr const FormatFeatureFlags TEXEL_BUF = CORE_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT;
// color renderable
static constexpr const FormatFeatureFlags CR = CORE_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
// texture filterable
static constexpr const FormatFeatureFlags TF = CORE_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
//  required texture formats. assume can be sampled, and transfered to/from
static constexpr const FormatFeatureFlags TEX =
    CORE_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | CORE_FORMAT_FEATURE_TRANSFER_SRC_BIT | CORE_FORMAT_FEATURE_TRANSFER_SRC_BIT;
// required depth format
static constexpr const FormatFeatureFlags DS = CORE_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | TEX | TF;

static constexpr const FormatFeatureFlags TF_TEX = TF | TEX;
#if RENDER_HAS_GL_BACKEND
static constexpr const FormatFeatureFlags CR_TEX = CR | TEX; // color renderable, texture format
#endif
static constexpr const FormatFeatureFlags CR_REND_TEX = CR | TEX; // color renderable, renderbuffer, texture format
#if RENDER_HAS_GLES_BACKEND
static constexpr const FormatFeatureFlags CR_TF_REND_TEX = CR | TF | TEX;
#endif

static constexpr const FormatFeatures IMAGE_FORMAT_FEATURES[] = {
#if RENDER_HAS_GL_BACKEND
    { GL_R8, CR_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_R8_SNORM, CR_TEX | ATOMIC_STORE },
    { GL_R16, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_R16_SNORM, CR_TEX | ATOMIC_STORE },
    { GL_RG8, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RG8_SNORM, CR_TEX | ATOMIC_STORE },
    { GL_RG16, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RG16_SNORM, CR_TEX | ATOMIC_STORE },
    // R3_G3_B2 not in base format
    { GL_RGB4, CR_TEX },
    // RGB5 not in base format
    { GL_RGB565, CR_REND_TEX },
    { GL_RGB8, CR_TEX },
    { GL_RGB8_SNORM, CR_TEX },
    { GL_RGB10, CR_TEX },
    { GL_RGB12, CR_TEX },
    { GL_RGB16, CR_TEX },
    { GL_RGB16_SNORM, CR_TEX },
    // RGBA2 not in base format
    { GL_RGBA4, CR_REND_TEX },
    { GL_RGB5_A1, CR_REND_TEX },
    { GL_RGBA8, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGBA8_SNORM, CR_TEX | ATOMIC_STORE },
    { GL_RGB10_A2, CR_REND_TEX | ATOMIC_STORE },
    { GL_RGB10_A2UI, CR_REND_TEX | ATOMIC_STORE },
    { GL_RGBA12, CR_TEX },
    { GL_RGBA16, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGBA16_SNORM, CR_TEX | ATOMIC_STORE },
    { GL_SRGB8, CR_TEX },
    { GL_SRGB8_ALPHA8, CR_REND_TEX },
    { GL_R16F, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RG16F, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGB16F, CR_TEX },
    { GL_RGBA16F, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_R32F, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RG32F, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGB32F, CR_TEX | TEXEL_BUF | TEXEL_BUF },
    { GL_RGBA32F, CR_REND_TEX | ATOMIC_STORE },
    { GL_R11F_G11F_B10F, CR_REND_TEX | ATOMIC_STORE },
    { GL_RGB9_E5, TEX },
    { GL_R8I, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_R8UI, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_R16I, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_R16UI, CR_REND_TEX | TEXEL_BUF },
    { GL_R32I, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_R32UI, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RG8I, CR_REND_TEX | TEXEL_BUF },
    { GL_RG8UI, CR_REND_TEX | TEXEL_BUF },
    { GL_RG16I, CR_REND_TEX | TEXEL_BUF },
    { GL_RG16UI, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RG32I, CR_REND_TEX | TEXEL_BUF },
    { GL_RG32UI, CR_REND_TEX | TEXEL_BUF },
    { GL_RGB8I, CR_TEX },
    { GL_RGB8UI, CR_TEX },
    { GL_RGB16I, CR_TEX },
    { GL_RGB16UI, CR_TEX },
    { GL_RGB32I, CR_TEX | TEXEL_BUF },
    { GL_RGB32UI, CR_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGBA8I, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGBA8UI, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGBA16I, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGBA16UI, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGBA32I, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGBA32UI, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
#elif RENDER_HAS_GLES_BACKEND
    { GL_R8, CR_TF_REND_TEX | TEXEL_BUF },
    { GL_R8_SNORM, TF_TEX },
    { GL_RG8, CR_TF_REND_TEX | TEXEL_BUF },
    { GL_RG8_SNORM, TF_TEX },
    { GL_RGB8, CR_TF_REND_TEX },
    { GL_RGB8_SNORM, TF_TEX },
    { GL_RGB565, CR_TF_REND_TEX },
    { GL_RGBA4, CR_TF_REND_TEX },
    { GL_RGB5_A1, CR_TF_REND_TEX },
    { GL_RGBA8, CR_TF_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGBA8_SNORM, TF_TEX | ATOMIC_STORE },
    { GL_RGB10_A2, CR_TF_REND_TEX },
    { GL_RGB10_A2UI, CR_REND_TEX },
    { GL_SRGB8, TF_TEX },
    { GL_SRGB8_ALPHA8, CR_TF_REND_TEX },
    { GL_R16F, CR_TF_REND_TEX | TEXEL_BUF },
    { GL_RG16F, CR_TF_REND_TEX | TEXEL_BUF },
    { GL_RGB16F, TF_TEX },
    { GL_RGBA16F, CR_TF_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_R32F, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RG32F, CR_REND_TEX | TEXEL_BUF },
    { GL_RGB32F, TEX | TEXEL_BUF },
    { GL_RGBA32F, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_R11F_G11F_B10F, CR_TF_REND_TEX },
    { GL_RGB9_E5, TF_TEX },
    { GL_R8I, CR_REND_TEX | TEXEL_BUF },
    { GL_R8UI, CR_REND_TEX | TEXEL_BUF },
    { GL_R16I, CR_REND_TEX | TEXEL_BUF },
    { GL_R16UI, CR_REND_TEX | TEXEL_BUF },
    { GL_R32I, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_R32UI, CR_REND_TEX | TEXEL_BUF },
    { GL_RG8I, CR_REND_TEX | TEXEL_BUF },
    { GL_RG8UI, CR_REND_TEX | TEXEL_BUF },
    { GL_RG16I, CR_REND_TEX | TEXEL_BUF },
    { GL_RG16UI, CR_REND_TEX | TEXEL_BUF },
    { GL_RG32I, CR_REND_TEX | TEXEL_BUF },
    { GL_RG32UI, CR_REND_TEX | TEXEL_BUF },
    { GL_RGB8I, TEX },
    { GL_RGB8UI, TEX },
    { GL_RGB16I, TEX },
    { GL_RGB16UI, TEX },
    { GL_RGB32I, TEX | TEXEL_BUF },
    { GL_RGB32UI, TEX | TEXEL_BUF },
    { GL_RGBA8I, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGBA8UI, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGBA16I, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGBA16UI, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGBA32I, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
    { GL_RGBA32UI, CR_REND_TEX | ATOMIC_STORE | TEXEL_BUF },
#endif
    { GL_DEPTH_COMPONENT16, DS },
    { GL_DEPTH_COMPONENT24, DS },
    { GL_DEPTH_COMPONENT32F, DS },
    { GL_DEPTH24_STENCIL8, DS },
    { GL_DEPTH32F_STENCIL8, DS },
    { GL_STENCIL_INDEX8, DS },

#if (defined(GL_EXT_texture_sRGB_R8) && (GL_EXT_texture_sRGB_R8))
    { GL_SR8_EXT, TF_TEX },
#endif

#if (defined(GL_EXT_texture_sRGB_RG8) && (GL_EXT_texture_sRGB_RG8))
    { GL_SRG8_EXT, TF_TEX },
#endif

#if defined(GL_EXT_texture_format_BGRA8888) && (GL_EXT_texture_format_BGRA8888)
    { GL_BGRA_EXT, CR_REND_TEX },
#endif

#if defined(GL_EXT_texture_norm16) && (GL_EXT_texture_norm16)
    { GL_R16_EXT, CR_TF_REND_TEX },
    { GL_RG16_EXT, CR_TF_REND_TEX },
    { GL_RGB16_EXT, TF_TEX },
    { GL_RGBA16_EXT, CR_TF_REND_TEX },
    { GL_R16_SNORM_EXT, TF_TEX },
    { GL_RG16_SNORM_EXT, TF_TEX },
    { GL_RGB16_SNORM_EXT, TF_TEX },
    { GL_RGBA16_SNORM_EXT, TF_TEX },
#endif

    { GL_COMPRESSED_R11_EAC, TF_TEX },
    { GL_COMPRESSED_SIGNED_R11_EAC, TF_TEX },
    { GL_COMPRESSED_RG11_EAC, TF_TEX },
    { GL_COMPRESSED_SIGNED_RG11_EAC, TF_TEX },
    { GL_COMPRESSED_RGB8_ETC2, TF_TEX },
    { GL_COMPRESSED_SRGB8_ETC2, TF_TEX },
    { GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, TF_TEX },
    { GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, TF_TEX },
    { GL_COMPRESSED_RGBA8_ETC2_EAC, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC, TF_TEX },
#if RENDER_HAS_GLES_BACKEND
    { GL_COMPRESSED_RGBA_ASTC_4x4, TF_TEX },
    { GL_COMPRESSED_RGBA_ASTC_5x4, TF_TEX },
    { GL_COMPRESSED_RGBA_ASTC_5x5, TF_TEX },
    { GL_COMPRESSED_RGBA_ASTC_6x5, TF_TEX },
    { GL_COMPRESSED_RGBA_ASTC_6x6, TF_TEX },
    { GL_COMPRESSED_RGBA_ASTC_8x5, TF_TEX },
    { GL_COMPRESSED_RGBA_ASTC_8x6, TF_TEX },
    { GL_COMPRESSED_RGBA_ASTC_8x8, TF_TEX },
    { GL_COMPRESSED_RGBA_ASTC_10x5, TF_TEX },
    { GL_COMPRESSED_RGBA_ASTC_10x6, TF_TEX },
    { GL_COMPRESSED_RGBA_ASTC_10x8, TF_TEX },
    { GL_COMPRESSED_RGBA_ASTC_10x10, TF_TEX },
    { GL_COMPRESSED_RGBA_ASTC_12x10, TF_TEX },
    { GL_COMPRESSED_RGBA_ASTC_12x12, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10, TF_TEX },
    { GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12, TF_TEX },
#endif
#if defined(GL_EXT_texture_compression_s3tc) && (GL_EXT_texture_compression_s3tc)
    { GL_COMPRESSED_RGB_S3TC_DXT1_EXT, TF_TEX },
    { GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, TF_TEX },
    { GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, TF_TEX },
    { GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, TF_TEX },
#endif
#if defined(GL_ARB_texture_compression_bptc) && (GL_ARB_texture_compression_bptc)
    { GL_COMPRESSED_RGBA_BPTC_UNORM_ARB, TF_TEX },
    { GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB, TF_TEX },
    { GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB, TF_TEX },
    { GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB, TF_TEX },
#endif
#if defined(GL_EXT_texture_compression_rgtc) && (GL_EXT_texture_compression_rgtc)
    { GL_COMPRESSED_RED_RGTC1_EXT, TF_TEX },
    { GL_COMPRESSED_SIGNED_RED_RGTC1_EXT, TF_TEX },
    { GL_COMPRESSED_RED_GREEN_RGTC2_EXT, TF_TEX },
    { GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT, TF_TEX },
#endif
};

// Dont allow SRGB_R8 and SRGB_R8G8 internal formats, instead use the GL_SRGB8 with swizzle as workaround.
#define USE_EXTENSION_FORMATS

#define BLOCK_BITS_8 1
#define BLOCK_BITS_16 2
#define BLOCK_BITS_32 4
#define BLOCK_BITS_64 8
#define BLOCK_BITS_128 16

// GL_EXT_texture_sRGB_R8 extension
#if (defined(GL_EXT_texture_sRGB_R8) && (GL_EXT_texture_sRGB_R8))
static constexpr DeviceGLES::ImageFormat IMAGE_FORMATS_EXT_SRGB_R8[] = {
    { BASE_FORMAT_R8_SRGB, GL_RED, GL_SR8_EXT, GL_UNSIGNED_BYTE, 1, { false, 0, 0, 0 },
        { GL_RED, GL_ZERO, GL_ZERO, GL_ONE } },
};
#endif

// GL_EXT_texture_sRGB_RG8 extension
#if (defined(GL_EXT_texture_sRGB_RG8) && (GL_EXT_texture_sRGB_RG8))
static constexpr DeviceGLES::ImageFormat IMAGE_FORMATS_EXT_SRGB_RG8[] = {
    { BASE_FORMAT_R8G8_SRGB, GL_RG, GL_SRG8_EXT, GL_UNSIGNED_BYTE, 1, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_ZERO, GL_ONE } },
};
#endif

// GL_EXT_texture_sRGB extension
#if defined(GL_EXT_texture_sRGB) && (GL_EXT_texture_sRGB)
static constexpr DeviceGLES::ImageFormat IMAGE_FORMATS_EXT_SRGB[] = {
    { BASE_FORMAT_BC1_RGB_SRGB_BLOCK, GL_RGB, GL_COMPRESSED_SRGB_S3TC_DXT1_EXT, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_64 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_BC1_RGBA_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_64 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_BC2_SRGB_BLOCK, GL_RGB, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_BC3_SRGB_BLOCK, GL_RGB, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
};
#endif

// GL_EXT_texture_format_BGRA8888 extension
#if defined(GL_EXT_texture_format_BGRA8888) && (GL_EXT_texture_format_BGRA8888)
static constexpr DeviceGLES::ImageFormat IMAGE_FORMATS_EXT_BGRA[] = {
    { BASE_FORMAT_B8G8R8A8_UNORM, GL_BGRA_EXT, GL_BGRA_EXT, GL_UNSIGNED_BYTE, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
};
#endif

#if defined(GL_EXT_texture_norm16) && (GL_EXT_texture_norm16)
static constexpr DeviceGLES::ImageFormat IMAGE_FORMATS_EXT_NORM16[] = {
    { BASE_FORMAT_R16_UNORM, GL_RED, GL_R16_EXT, GL_UNSIGNED_SHORT, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16_UNORM, GL_RG, GL_RG16_EXT, GL_UNSIGNED_SHORT, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16B16_UNORM, GL_RGB, GL_RGB16_EXT, GL_UNSIGNED_SHORT, 6, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16B16A16_UNORM, GL_RGBA, GL_RGBA16_EXT, GL_UNSIGNED_SHORT, 8, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16_SNORM, GL_RED, GL_R16_SNORM_EXT, GL_SHORT, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16_SNORM, GL_RG, GL_RG16_SNORM_EXT, GL_SHORT, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16B16_SNORM, GL_RGB, GL_RGB16_SNORM_EXT, GL_SHORT, 6, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16B16A16_SNORM, GL_RGBA, GL_RGBA16_SNORM_EXT, GL_SHORT, 8, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
};
#endif

// GL_EXT_texture_compression_s3tc extension
#if defined(GL_EXT_texture_compression_s3tc) && (GL_EXT_texture_compression_s3tc)
static constexpr DeviceGLES::ImageFormat IMAGE_FORMATS_EXT_S3TC[] = {
    { BASE_FORMAT_BC1_RGB_UNORM_BLOCK, GL_RGB, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_64 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_BC1_RGBA_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_64 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_BC2_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_BC3_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
};
#endif

// GL_ARB_texture_compression_bptc extension
#if defined(GL_ARB_texture_compression_bptc) && (GL_ARB_texture_compression_bptc)
static constexpr DeviceGLES::ImageFormat IMAGE_FORMATS_EXT_BPTC[] = {
    { BASE_FORMAT_BC7_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_BPTC_UNORM_ARB, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_BC7_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_BC6H_SFLOAT_BLOCK, GL_RGB, GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB, GL_FLOAT, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_BC6H_UFLOAT_BLOCK, GL_RGB, GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB, GL_FLOAT, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
};
#endif

// GL_EXT_texture_norm16 extension
#if defined(GL_EXT_texture_compression_rgtc) && (GL_EXT_texture_compression_rgtc)
static constexpr DeviceGLES::ImageFormat IMAGE_FORMATS_EXT_RGTC[] = {
    { BASE_FORMAT_BC4_UNORM_BLOCK, GL_RED, GL_COMPRESSED_RED_RGTC1_EXT, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_64 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_BC4_SNORM_BLOCK, GL_RED, GL_COMPRESSED_SIGNED_RED_RGTC1_EXT, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_64 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_BC5_UNORM_BLOCK, GL_RG, GL_COMPRESSED_RED_GREEN_RGTC2_EXT, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_BC5_SNORM_BLOCK, GL_RG, GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
};
#endif

static constexpr DeviceGLES::ImageFormat IMAGE_FORMATS_FALLBACK[] = {
    { BASE_FORMAT_R4G4_UNORM_PACK8, GL_RG, GL_RGBA4, GL_UNSIGNED_BYTE, 1, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_ZERO, GL_ONE } },
    { BASE_FORMAT_B4G4R4A4_UNORM_PACK16, GL_RGBA, GL_RGBA4, GL_UNSIGNED_SHORT_4_4_4_4, 2, { false, 0, 0, 0 },
        { GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA } },
    { BASE_FORMAT_B5G6R5_UNORM_PACK16, GL_RGB, GL_RGB565, GL_UNSIGNED_SHORT_5_6_5, 2, { false, 0, 0, 0 },
        { GL_BLUE, GL_GREEN, GL_RED, GL_ONE } },
    { BASE_FORMAT_B5G5R5A1_UNORM_PACK16, GL_RGBA, GL_RGB5_A1, GL_UNSIGNED_SHORT_5_5_5_1, 2, { false, 0, 0, 0 },
        { GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA } },
    { BASE_FORMAT_A1R5G5B5_UNORM_PACK16, GL_RGBA, GL_RGB5_A1, GL_UNSIGNED_SHORT_5_5_5_1, 2, { false, 0, 0, 0 },
        { GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA } },

    // not available in desktop, available as an extension in opengles. "GL_EXT_texture_sRGB_RG8"
    { BASE_FORMAT_R8_SRGB, GL_RED, GL_SRGB8, GL_UNSIGNED_BYTE, 1, { false, 0, 0, 0 },
        { GL_RED, GL_ZERO, GL_ZERO, GL_ONE } },
    { BASE_FORMAT_R8G8_SRGB, GL_RG, GL_SRGB8, GL_UNSIGNED_BYTE, 1, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_ZERO, GL_ONE } },

    { BASE_FORMAT_A2R10G10B10_UNORM_PACK32, GL_RGBA, GL_RGB10_A2, GL_UNSIGNED_INT_2_10_10_10_REV, 4, { false, 0, 0, 0 },
        { GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA } },
    { BASE_FORMAT_A2R10G10B10_UINT_PACK32, GL_RGBA, GL_RGB10_A2UI, GL_UNSIGNED_INT_2_10_10_10_REV, 4,
        { false, 0, 0, 0 }, { GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA } },

    // available as an extension in opengles. "GL_EXT_texture_norm16" -> fallback to half float
    { BASE_FORMAT_R16_UNORM, GL_RED, GL_R16F, GL_HALF_FLOAT, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16_UNORM, GL_RG, GL_RG16F, GL_HALF_FLOAT, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16B16_UNORM, GL_RGB, GL_RGB16F, GL_HALF_FLOAT, 6, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16B16A16_UNORM, GL_RGBA, GL_RGBA16F, GL_HALF_FLOAT, 8, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
};

// NOTE: verify this table. add missing formats.
static constexpr DeviceGLES::ImageFormat IMAGE_FORMATS[] = {
    { BASE_FORMAT_UNDEFINED, GL_NONE, GL_NONE, GL_NONE, 0, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    // These are required in GL and GLES
    { BASE_FORMAT_R8_UNORM, GL_RED, GL_R8, GL_UNSIGNED_BYTE, 1, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8_SNORM, GL_RED, GL_R8_SNORM, GL_BYTE, 1, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8G8_UNORM, GL_RG, GL_RG8, GL_UNSIGNED_BYTE, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8G8_SNORM, GL_RG, GL_RG8, GL_BYTE, 2, { false, 0, 0, 0 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R5G6B5_UNORM_PACK16, GL_RGB, GL_RGB565, GL_UNSIGNED_SHORT_5_6_5, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8G8B8_UNORM, GL_RGB, GL_RGB8, GL_UNSIGNED_BYTE, 3, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8G8B8_SNORM, GL_RGB, GL_RGB8, GL_BYTE, 3, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R4G4B4A4_UNORM_PACK16, GL_RGBA, GL_RGBA4, GL_UNSIGNED_SHORT_4_4_4_4, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R5G5B5A1_UNORM_PACK16, GL_RGBA, GL_RGB5_A1, GL_UNSIGNED_SHORT_5_5_5_1, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8G8B8A8_UNORM, GL_RGBA, GL_RGBA8, GL_UNSIGNED_BYTE, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8G8B8A8_SNORM, GL_RGBA, GL_RGBA8, GL_BYTE, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_A2B10G10R10_UNORM_PACK32, GL_RGBA, GL_RGB10_A2, GL_UNSIGNED_INT_2_10_10_10_REV, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_A2B10G10R10_UINT_PACK32, GL_RGBA, GL_RGB10_A2UI, GL_UNSIGNED_INT_2_10_10_10_REV, 4,
        { false, 0, 0, 0 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8G8B8_SRGB, GL_RGB, GL_SRGB8, GL_UNSIGNED_BYTE, 3, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8G8B8A8_SRGB, GL_RGBA, GL_SRGB8_ALPHA8, GL_UNSIGNED_BYTE, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16_SFLOAT, GL_RED, GL_R16F, GL_HALF_FLOAT, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16_SFLOAT, GL_RG, GL_RG16F, GL_HALF_FLOAT, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16B16_SFLOAT, GL_RGB, GL_RGB16F, GL_HALF_FLOAT, 6, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16B16A16_SFLOAT, GL_RGBA, GL_RGBA16F, GL_HALF_FLOAT, 8, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R32_SFLOAT, GL_RED, GL_R32F, GL_FLOAT, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R32G32_SFLOAT, GL_RG, GL_RG32F, GL_FLOAT, 8, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R32G32B32_SFLOAT, GL_RGB, GL_RGB32F, GL_FLOAT, 12, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R32G32B32A32_SFLOAT, GL_RGBA, GL_RGBA32F, GL_FLOAT, 16, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    // Special R11 G11 B10 float format. This format does not work with compute on GLES, but works as a texture. (this
    // is handled elsewhere)
    { BASE_FORMAT_B10G11R11_UFLOAT_PACK32, GL_RGB, GL_R11F_G11F_B10F, GL_UNSIGNED_INT_10F_11F_11F_REV, 4,
        { false, 0, 0, 0 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_E5B9G9R9_UFLOAT_PACK32, GL_RGB, GL_RGB9_E5, GL_UNSIGNED_INT_5_9_9_9_REV, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8_SINT, GL_RED_INTEGER, GL_R8I, GL_BYTE, 1, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8_UINT, GL_RED_INTEGER, GL_R8UI, GL_UNSIGNED_BYTE, 1, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16_SINT, GL_RED_INTEGER, GL_R16I, GL_SHORT, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16_UINT, GL_RED_INTEGER, GL_R16UI, GL_UNSIGNED_SHORT, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R32_SINT, GL_RED_INTEGER, GL_R32I, GL_INT, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R32_UINT, GL_RED_INTEGER, GL_R32UI, GL_UNSIGNED_INT, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8G8_SINT, GL_RG_INTEGER, GL_RG8I, GL_BYTE, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8G8_UINT, GL_RG_INTEGER, GL_R8UI, GL_UNSIGNED_BYTE, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16_SINT, GL_RG_INTEGER, GL_RG16I, GL_SHORT, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16_UINT, GL_RG_INTEGER, GL_RG16UI, GL_UNSIGNED_SHORT, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R32G32_SINT, GL_RG_INTEGER, GL_RG32I, GL_INT, 8, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R32G32_UINT, GL_RG_INTEGER, GL_RG32UI, GL_UNSIGNED_INT, 8, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8G8B8_SINT, GL_RGB_INTEGER, GL_RGB8I, GL_BYTE, 3, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8G8B8_UINT, GL_RGB_INTEGER, GL_RGB8UI, GL_UNSIGNED_BYTE, 3, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16B16_SINT, GL_RGB_INTEGER, GL_RGB16I, GL_SHORT, 6, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16B16_UINT, GL_RGB_INTEGER, GL_RGB16UI, GL_UNSIGNED_SHORT, 6, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R32G32B32_SINT, GL_RGB_INTEGER, GL_RGB32I, GL_INT, 12, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R32G32B32_UINT, GL_RGB_INTEGER, GL_RGB32UI, GL_UNSIGNED_INT, 12, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8G8B8A8_SINT, GL_RGBA_INTEGER, GL_RGBA8I, GL_BYTE, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R8G8B8A8_UINT, GL_RGBA_INTEGER, GL_RGBA8UI, GL_UNSIGNED_BYTE, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16B16A16_SINT, GL_RGBA_INTEGER, GL_RGBA16I, GL_SHORT, 8, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16B16A16_UINT, GL_RGBA_INTEGER, GL_RGBA16UI, GL_UNSIGNED_SHORT, 8, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R32G32B32A32_SINT, GL_RGBA_INTEGER, GL_RGBA32I, GL_INT, 16, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R32G32B32A32_UINT, GL_RGBA_INTEGER, GL_RGBA32UI, GL_UNSIGNED_INT, 16, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_D16_UNORM, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT16, GL_UNSIGNED_SHORT, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_X8_D24_UNORM_PACK32, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT24, GL_UNSIGNED_INT, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_D32_SFLOAT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT32F, GL_FLOAT, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_S8_UINT, GL_STENCIL_INDEX, GL_STENCIL_INDEX8, GL_UNSIGNED_BYTE, 1, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_D24_UNORM_S8_UINT, GL_DEPTH_STENCIL, GL_DEPTH24_STENCIL8, GL_UNSIGNED_INT_24_8, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_D32_SFLOAT_S8_UINT, GL_DEPTH_STENCIL, GL_DEPTH32F_STENCIL8, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 8,
        { false, 0, 0, 0 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    // EAC
    { BASE_FORMAT_EAC_R11_UNORM_BLOCK, GL_RED, GL_COMPRESSED_R11_EAC, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_64 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_EAC_R11_SNORM_BLOCK, GL_RED, GL_COMPRESSED_SIGNED_R11_EAC, GL_BYTE, 0, { true, 4, 4, BLOCK_BITS_64 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_EAC_R11G11_UNORM_BLOCK, GL_RG, GL_COMPRESSED_RG11_EAC, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_EAC_R11G11_SNORM_BLOCK, GL_RG, GL_COMPRESSED_SIGNED_RG11_EAC, GL_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    // ETC
    { BASE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, GL_RGB, GL_COMPRESSED_RGB8_ETC2, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_64 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, GL_RGB, GL_COMPRESSED_SRGB8_ETC2, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_64 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_64 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_64 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA8_ETC2_EAC, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
#if RENDER_HAS_GL_BACKEND
    // required by GL
    { BASE_FORMAT_R16_UNORM, GL_RED, GL_R16, GL_UNSIGNED_SHORT, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16_SNORM, GL_RED, GL_R16, GL_SHORT, 2, { false, 0, 0, 0 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16_UNORM, GL_RG, GL_RG16, GL_UNSIGNED_SHORT, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16_SNORM, GL_RG, GL_RG16, GL_SHORT, 4, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    // GL_R3_G3_B2, RGB4, RGB5, RGB10, RGB12 not in base formats
    { BASE_FORMAT_R16G16B16_UNORM, GL_RGB, GL_RGB16, GL_UNSIGNED_SHORT, 6, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16B16_SNORM, GL_RGB, GL_RGB16, GL_SHORT, 6, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    // RGBA2, RGBA12 not in base formats
    { BASE_FORMAT_R16G16B16A16_UNORM, GL_RGBA, GL_RGBA16, GL_UNSIGNED_SHORT, 8, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_R16G16B16A16_SNORM, GL_RGBA, GL_RGBA16, GL_SHORT, 8, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    // STENCIL_INDEX1, STENCIL_INDEX4, STENCIL_INDEX16 not in base formats
    { BASE_FORMAT_B4G4R4A4_UNORM_PACK16, GL_BGRA, GL_RGBA4, GL_UNSIGNED_SHORT_4_4_4_4_REV, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_A1R5G5B5_UNORM_PACK16, GL_BGRA, GL_RGB5_A1, GL_UNSIGNED_SHORT_1_5_5_5_REV, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_B5G6R5_UNORM_PACK16, GL_BGR, GL_RGB565, GL_UNSIGNED_SHORT_5_6_5_REV, 2, { false, 0, 0, 0 },
        { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
#elif RENDER_HAS_GLES_BACKEND
    // required by GLES
    { BASE_FORMAT_ASTC_4x4_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_4x4_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_5x4_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_5x4_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 5, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_5x5_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_5x5_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 5, 5, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_6x5_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_6x5_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 6, 5, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_6x6_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_6x6_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 6, 6, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_8x5_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_8x5_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 8, 5, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_8x6_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_8x6_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 8, 6, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_8x8_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_8x8_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 8, 8, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_10x5_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_10x5_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 10, 5, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_10x6_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_10x6_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 10, 6, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_10x8_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_10x8_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 10, 8, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_10x10_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_10x10_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 10, 10, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_12x10_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_12x10_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 12, 10, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_12x12_UNORM_BLOCK, GL_RGBA, GL_COMPRESSED_RGBA_ASTC_12x12_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 12, 12, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_4x4_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 4, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_5x4_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 5, 4, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_5x5_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 5, 5, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_6x5_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 6, 5, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 6, 6, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_8x5_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 8, 5, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_8x6_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 8, 6, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_8x8_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 8, 8, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_10x5_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 10, 5, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_10x6_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 10, 6, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_10x8_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 10, 8, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_10x10_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 10, 10, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_12x10_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 12, 10, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
    { BASE_FORMAT_ASTC_12x12_SRGB_BLOCK, GL_RGBA, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR, GL_UNSIGNED_BYTE, 0,
        { true, 12, 12, BLOCK_BITS_128 }, { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA } },
#endif
};

void FillExtensionFormats(const DeviceGLES& device, BASE_NS::vector<DeviceGLES::ImageFormat>& sf)
{
#if (defined(GL_EXT_texture_sRGB_R8) && (GL_EXT_texture_sRGB_R8))
    if (device.HasExtension("GL_EXT_texture_sRGB_R8")) {
        sf.insert(sf.end(), std::begin(IMAGE_FORMATS_EXT_SRGB_R8), std::end(IMAGE_FORMATS_EXT_SRGB_R8));
    }
#endif

#if (defined(GL_EXT_texture_sRGB_RG8) && (GL_EXT_texture_sRGB_RG8))
    if (device.HasExtension("GL_EXT_texture_sRGB_RG8")) {
        sf.insert(sf.end(), std::begin(IMAGE_FORMATS_EXT_SRGB_RG8), std::end(IMAGE_FORMATS_EXT_SRGB_RG8));
    }
#endif

#if defined(GL_EXT_texture_sRGB) && (GL_EXT_texture_sRGB)
    if (device.HasExtension("GL_EXT_texture_sRGB")) {
        sf.insert(sf.end(), std::begin(IMAGE_FORMATS_EXT_SRGB), std::end(IMAGE_FORMATS_EXT_SRGB));
    }
#endif

#if defined(GL_EXT_texture_format_BGRA8888) && (GL_EXT_texture_format_BGRA8888)
    if (device.HasExtension("GL_EXT_texture_format_BGRA8888")) {
        sf.insert(sf.end(), std::begin(IMAGE_FORMATS_EXT_BGRA), std::end(IMAGE_FORMATS_EXT_BGRA));
    }
#endif

#if defined(GL_EXT_texture_norm16) && (GL_EXT_texture_norm16)
    if (device.HasExtension("GL_EXT_texture_norm16")) {
        sf.insert(sf.end(), std::begin(IMAGE_FORMATS_EXT_NORM16), std::end(IMAGE_FORMATS_EXT_NORM16));
    }
#endif

#if defined(GL_EXT_texture_compression_s3tc) && (GL_EXT_texture_compression_s3tc)
    if (device.HasExtension("GL_EXT_texture_compression_s3tc")) {
        sf.insert(sf.end(), std::begin(IMAGE_FORMATS_EXT_S3TC), std::end(IMAGE_FORMATS_EXT_S3TC));
    }
#endif

#if defined(GL_ARB_texture_compression_bptc) && (GL_ARB_texture_compression_bptc)
    if (device.HasExtension("GL_ARB_texture_compression_bptc")) {
        sf.insert(sf.end(), std::begin(IMAGE_FORMATS_EXT_BPTC), std::end(IMAGE_FORMATS_EXT_BPTC));
    }
#endif

#if defined(GL_EXT_texture_compression_rgtc) && (GL_EXT_texture_compression_rgtc)
    if (device.HasExtension("GL_EXT_texture_compression_rgtc")) {
        sf.insert(sf.end(), std::begin(IMAGE_FORMATS_EXT_RGTC), std::end(IMAGE_FORMATS_EXT_RGTC));
    }
#endif
}
} // namespace

// Some OpenGL/ES features are supported and using them will lead to an assertion unless
// the following define is added locally or as part of the build command: #define HANDLE_UNSUPPORTED_ENUMS
inline uint32_t DeviceGLES::TargetToBinding(uint32_t target)
{
    if (target == GL_UNIFORM_BUFFER) {
        return GL_UNIFORM_BUFFER_BINDING;
    } else if (target == GL_SHADER_STORAGE_BUFFER) {
        return GL_SHADER_STORAGE_BUFFER_BINDING;
    } else if (target == GL_PIXEL_UNPACK_BUFFER) {
        return GL_PIXEL_UNPACK_BUFFER_BINDING;
    } else if (target == GL_COPY_READ_BUFFER) {
        return GL_COPY_READ_BUFFER_BINDING;
    } else if (target == GL_COPY_WRITE_BUFFER) {
        return GL_COPY_WRITE_BUFFER_BINDING;
#ifdef HANDLE_UNSUPPORTED_ENUMS
    } else if (target == GL_ATOMIC_COUNTER_BUFFER) {
        return GL_ATOMIC_COUNTER_BUFFER_BINDING;
    } else if (target == GL_TRANSFORM_FEEDBACK_BUFFER) {
        return GL_TRANSFORM_FEEDBACK_BUFFER_BINDING;
    } else if (target == GL_PIXEL_PACK_BUFFER) {
        return GL_PIXEL_PACK_BUFFER_BINDING;
    } else if (target == GL_QUERY_BUFFER) {
        return GL_QUERY_BUFFER_BINDING;
    } else if (target == GL_ARRAY_BUFFER) {
        return GL_ARRAY_BUFFER_BINDING;
    } else if (target == GL_DISPATCH_INDIRECT_BUFFER) {
        return GL_DISPATCH_INDIRECT_BUFFER_BINDING;
    } else if (target == GL_DRAW_INDIRECT_BUFFER) {
        return GL_DRAW_INDIRECT_BUFFER_BINDING;
    } else if (target == GL_ELEMENT_ARRAY_BUFFER) { // stored in VAO state...
        return GL_ELEMENT_ARRAY_BUFFER_BINDING;
    } else if (target == GL_TEXTURE_BUFFER) {
        return GL_TEXTURE_BUFFER_BINDING;
#endif
    }
    PLUGIN_ASSERT_MSG(false, "UNHANDLED BUFFER BIND TARGET UNIT");
    return GL_NONE;
}

inline DeviceGLES::BufferBindId DeviceGLES::IndexedTargetToTargetId(uint32_t target)
{
    if (target == GL_UNIFORM_BUFFER) {
        return BufferBindId::UNIFORM_BUFFER_BIND;
    } else if (target == GL_SHADER_STORAGE_BUFFER) {
        return BufferBindId::SHADER_STORAGE_BUFFER_BIND;
#ifdef HANDLE_UNSUPPORTED_ENUMS
    } else if (target == GL_ATOMIC_COUNTER_BUFFER) {
        return BufferBindId::ATOMIC_COUNTER_BUFFER;
    } else if (target == GL_TRANSFORM_FEEDBACK_BUFFER) {
        return BufferBindId::TRANSFORM_FEEDBACK_BUFFER;
#endif
    }
    PLUGIN_ASSERT_MSG(false, "UNHANDLED BUFFER BIND TARGET UNIT");
    return BufferBindId::MAX_BUFFER_BIND_ID;
}

inline uint32_t DeviceGLES::IndexedTargetIdToTarget(DeviceGLES::BufferBindId target)
{
    if (target == BufferBindId::UNIFORM_BUFFER_BIND) {
        return GL_UNIFORM_BUFFER;
    } else if (target == BufferBindId::SHADER_STORAGE_BUFFER_BIND) {
        return GL_SHADER_STORAGE_BUFFER;
#ifdef HANDLE_UNSUPPORTED_ENUMS
    } else if (target == BufferBindId::ATOMIC_COUNTER_BUFFER_BIND) {
        return GL_ATOMIC_COUNTER_BUFFER;
    } else if (target == BufferBindId::TRANSFORM_FEEDBACK_BUFFER_BIND) {
        return GL_TRANSFORM_FEEDBACK_BUFFER;
#endif
    }
    PLUGIN_ASSERT_MSG(false, "UNHANDLED BUFFER BIND TARGET UNIT");
    return 0;
}

inline DeviceGLES::BufferTargetId DeviceGLES::GenericTargetToTargetId(uint32_t target)
{
    if (target == GL_PIXEL_UNPACK_BUFFER) {
        return BufferTargetId::PIXEL_UNPACK_BUFFER;
    } else if (target == GL_PIXEL_PACK_BUFFER) {
        return BufferTargetId::PIXEL_PACK_BUFFER;
    } else if (target == GL_COPY_READ_BUFFER) {
        return BufferTargetId::COPY_READ_BUFFER;
    } else if (target == GL_COPY_WRITE_BUFFER) {
        return BufferTargetId::COPY_WRITE_BUFFER;
    } else if (target == GL_UNIFORM_BUFFER) {
        return BufferTargetId::UNIFORM_BUFFER;
    } else if (target == GL_SHADER_STORAGE_BUFFER) {
        return BufferTargetId::SHADER_STORAGE_BUFFER;
    } else if (target == GL_DISPATCH_INDIRECT_BUFFER) {
        return BufferTargetId::DISPATCH_INDIRECT_BUFFER;
    } else if (target == GL_DRAW_INDIRECT_BUFFER) {
        return BufferTargetId::DRAW_INDIRECT_BUFFER;
#ifdef HANDLE_UNSUPPORTED_ENUMS
    } else if (target == GL_ATOMIC_COUNTER_BUFFER) {
        return BufferTargetId::ATOMIC_COUNTER_BUFFER;
    } else if (target == GL_QUERY_BUFFER) {
        return BufferTargetId::QUERY_BUFFER;
    } else if (target == GL_TRANSFORM_FEEDBACK_BUFFER) {
        return BufferTargetId::TRANSFORM_FEEDBACK_BUFFER;
    } else if (target == GL_ARRAY_BUFFER) {
        return BufferTargetId::ARRAY_BUFFER;
    } else if (target == GL_ELEMENT_ARRAY_BUFFER) { // stored in VAO state...
        return BufferTargetId::ELEMENT_ARRAY_BUFFER;
    } else if (target == GL_TEXTURE_BUFFER) {
        return BufferTargetId::TEXTURE_BUFFER;
#endif
    }
    PLUGIN_ASSERT_MSG(false, "UNHANDLED BUFFER BIND TARGET");
    return BufferTargetId::MAX_BUFFER_TARGET_ID;
}

inline uint32_t DeviceGLES::GenericTargetIdToTarget(BufferTargetId target)
{
    if (target == BufferTargetId::PIXEL_UNPACK_BUFFER) {
        return GL_PIXEL_UNPACK_BUFFER;
    } else if (target == BufferTargetId::PIXEL_PACK_BUFFER) {
        return GL_PIXEL_PACK_BUFFER;
    } else if (target == BufferTargetId::COPY_READ_BUFFER) {
        return GL_COPY_READ_BUFFER;
    } else if (target == BufferTargetId::COPY_WRITE_BUFFER) {
        return GL_COPY_WRITE_BUFFER;
    } else if (target == BufferTargetId::UNIFORM_BUFFER) {
        return GL_UNIFORM_BUFFER;
    } else if (target == BufferTargetId::SHADER_STORAGE_BUFFER) {
        return GL_SHADER_STORAGE_BUFFER;
    } else if (target == BufferTargetId::DISPATCH_INDIRECT_BUFFER) {
        return GL_DISPATCH_INDIRECT_BUFFER;
    } else if (target == BufferTargetId::DRAW_INDIRECT_BUFFER) {
        return GL_DRAW_INDIRECT_BUFFER;
#ifdef HANDLE_UNSUPPORTED_ENUMS
    } else if (target == BufferTargetId::ATOMIC_COUNTER_BUFFER) {
        return GL_ATOMIC_COUNTER_BUFFER;
    } else if (target == BufferTargetId::QUERY_BUFFER) {
        return GL_QUERY_BUFFER;
    } else if (target == BufferTargetId::TRANSFORM_FEEDBACK_BUFFER) {
        return GL_TRANSFORM_FEEDBACK_BUFFER;
    } else if (target == BufferTargetId::ARRAY_BUFFER) {
        return GL_ARRAY_BUFFER;
    } else if (target == BufferTargetId::ELEMENT_ARRAY_BUFFER) { // stored in VAO state...
        return GL_ELEMENT_ARRAY_BUFFER;
    } else if (target == BufferTargetId::TEXTURE_BUFFER) {
        return GL_TEXTURE_BUFFER;
#endif
    }
    PLUGIN_ASSERT_MSG(false, "UNHANDLED BUFFER BIND TARGET");
    return 0;
}

inline DeviceGLES::TextureTargetId DeviceGLES::TextureTargetToTargetId(uint32_t target)
{
    if (target == GL_TEXTURE_2D) {
        return TextureTargetId::TEXTURE_2D;
    } else if ((target == GL_TEXTURE_CUBE_MAP_POSITIVE_X) || (target == GL_TEXTURE_CUBE_MAP_NEGATIVE_X) ||
               (target == GL_TEXTURE_CUBE_MAP_POSITIVE_Y) || (target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y) ||
               (target == GL_TEXTURE_CUBE_MAP_POSITIVE_Z) || (target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) ||
               (target == GL_TEXTURE_CUBE_MAP)) {
        return TextureTargetId::TEXTURE_CUBE_MAP;
#if RENDER_HAS_GLES_BACKEND
    } else if (target == GL_TEXTURE_EXTERNAL_OES) {
        return TextureTargetId::TEXTURE_EXTERNAL_OES;
#endif
    } else if (target == GL_TEXTURE_2D_MULTISAMPLE) {
        return TextureTargetId::TEXTURE_2D_MULTISAMPLE;
    } else if (target == GL_TEXTURE_2D_ARRAY) {
        return TextureTargetId::TEXTURE_2D_ARRAY;
    } else if (target == GL_TEXTURE_3D) {
        return TextureTargetId::TEXTURE_3D;
    }
    PLUGIN_ASSERT_MSG(false, "UNHANDLED TEXTURE TARGET UNIT");
    return TextureTargetId::MAX_TEXTURE_TARGET_ID;
}

inline uint32_t DeviceGLES::TextureTargetIdToTarget(DeviceGLES::TextureTargetId target)
{
    if (target == TextureTargetId::TEXTURE_2D) {
        return GL_TEXTURE_2D;
    } else if (target == TextureTargetId::TEXTURE_CUBE_MAP) {
        return GL_TEXTURE_CUBE_MAP;
#if RENDER_HAS_GLES_BACKEND
    } else if (target == TextureTargetId::TEXTURE_EXTERNAL_OES) {
        return GL_TEXTURE_EXTERNAL_OES;
#endif
    } else if (target == TextureTargetId::TEXTURE_2D_MULTISAMPLE) {
        return GL_TEXTURE_2D_MULTISAMPLE;
    } else if (target == TextureTargetId::TEXTURE_2D_ARRAY) {
        return GL_TEXTURE_2D_ARRAY;
    } else if (target == TextureTargetId::TEXTURE_3D) {
        return GL_TEXTURE_3D;
    }
    PLUGIN_ASSERT_MSG(false, "UNHANDLED TEXTURE TARGET UNIT");
    return 0;
}

void DeviceGLES::Activate(RenderHandle swapchain)
{
    if (HasSwapchain()) {
        eglState_.SetContext(static_cast<const SwapchainGLES*>(GetSwapchain(swapchain)));
    } else {
        // bind the dummy surface as there is no swapchain.
        eglState_.SetContext(nullptr);
    }
}

void DeviceGLES::SwapBuffers(const SwapchainGLES& swapchain)
{
    eglState_.SwapBuffers(swapchain);
}

#if RENDER_HAS_GL_BACKEND
const WGLHelpers::WGLState& DeviceGLES::GetEglState()
#endif
#if RENDER_HAS_GLES_BACKEND
    const EGLHelpers::EGLState& DeviceGLES::GetEglState()
#endif
{
    return eglState_;
}

#if RENDER_HAS_GLES_BACKEND
bool DeviceGLES::IsDepthResolveSupported() const
{
    return backendConfig_.allowDepthResolve;
}
#endif

DeviceGLES::DeviceGLES(RenderContext& renderContext, DeviceCreateInfo const& createInfo)
    : Device(renderContext, createInfo)
{
    eglState_.CreateContext(createInfo);
    if (!eglState_.IsValid()) {
        PLUGIN_LOG_F("Failed to create a context");
        return;
    }
    eglState_.GlInitialize();
#if RENDER_GL_DEBUG
    filterErrorFunc = (decltype(filterErrorFunc))eglState_.ErrorFilter();
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OnGlError, NULL);
    GLuint unusedIds = 0;
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, true);
#endif
    PLUGIN_LOG_I("GL_VENDOR: %s", glGetString(GL_VENDOR));
    PLUGIN_LOG_I("GL_RENDERER: %s", glGetString(GL_RENDERER));
    PLUGIN_LOG_I("GL_VERSION: %s", glGetString(GL_VERSION));
    PLUGIN_LOG_I("GL_SHADING_LANGUAGE_VERSION: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    GLint n = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    extensions_.reserve(n + 1U);
    for (GLuint i = 0U; i < static_cast<GLuint>(n); ++i) {
        const auto ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        PLUGIN_LOG_V("  GL_EXTENSION: %s", ext);
        extensions_.emplace_back(ext);
    }
    std::sort(extensions_.begin(), extensions_.end(),
        [](const string_view& lhs, const string_view& rhs) { return lhs < rhs; });

#if RENDER_HAS_GL_BACKEND
    // Extension in OpenGL ES, but part of core in OpenGL.
    if (const auto pos = std::lower_bound(extensions_.cbegin(), extensions_.cend(), EXT_BUFFER_STORAGE,
            [](const string_view& element, const string_view& value) { return element < value; });
        (pos == extensions_.cend()) || (*pos != EXT_BUFFER_STORAGE)) {
        extensions_.insert(pos, EXT_BUFFER_STORAGE);
    }
    // Seamless cubemaps are always on in vulkan and gles 3.0..
    // (3.0 made it required, not supported prior to 3.0 es)
    // on desktop gl, it's optional.
    // (but must be supported since 3.2)
    // So just enable it always, so that desktop GL works as vulkan and GLES.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif
#if RENDER_GL_DEBUG
    // Dump interesting constants.
    DUMP(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS);
    DUMP(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS);
    DUMP(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS);
    DUMP(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS);
    DUMP(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS);
    DUMP(GL_MAX_SHADER_STORAGE_BLOCK_SIZE);
#endif
    boundReadFbo_ = boundWriteFbo_ = 0;
    eglState_.RestoreContext();
#ifdef RENDER_OPTIMIZE_FOR_INTEGRATED_GPU
    // NOTE: we expect e.g. mobile devices to have integrated memory, where we can bypass staging and write directly to
    // linear gpu buffers without additional copies and performance decrease
    deviceSharedMemoryPropertyFlags_ = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
#endif
    SetDeviceStatus(true);

    // First dump required formats and then add based on supported extensions
    supportedFormats_.insert(supportedFormats_.end(), std::begin(IMAGE_FORMATS), std::end(IMAGE_FORMATS));

    FillExtensionFormats(*this, supportedFormats_);

    // Keep the list sorted for faster lookup
    std::sort(supportedFormats_.begin(), supportedFormats_.end(),
        [](const ImageFormat& lhs, const ImageFormat& rhs) { return lhs.coreFormat < rhs.coreFormat; });

    const GpuResourceManager::CreateInfo grmCreateInfo {
        GpuResourceManager::GPU_RESOURCE_MANAGER_OPTIMIZE_STAGING_MEMORY,
    };
    gpuResourceMgr_ = make_unique<GpuResourceManager>(*this, grmCreateInfo);
    shaderMgr_ = make_unique<ShaderManager>(*this);

    lowLevelDevice_ = make_unique<LowLevelDeviceGLES>(*this);
}

DeviceGLES::~DeviceGLES()
{
    if (eglState_.IsValid()) {
        Activate(); // make sure we are active during teardown..
        WaitForIdle();

        // must release handles before taking down gpu resource manager.
        swapchains_.clear();

        gpuResourceMgr_.reset();
        shaderMgr_.reset();
        eglState_.DestroyContext();
        Deactivate(); // make sure the previous context is still active..
    }
}

bool DeviceGLES::HasExtension(const string_view extension) const
{
    return std::binary_search(extensions_.begin(), extensions_.end(), extension,
        [](const string_view& element, const string_view value) { return element < value; });
}

DeviceBackendType DeviceGLES::GetBackendType() const
{
    return backendType_;
}

const DevicePlatformData& DeviceGLES::GetPlatformData() const
{
    return eglState_.GetPlatformData();
}

ILowLevelDevice& DeviceGLES::GetLowLevelDevice() const
{
    return *lowLevelDevice_;
}

FormatProperties DeviceGLES::GetFormatProperties(const Format format) const
{
    FormatProperties properties;
    auto& glFormat = GetGlImageFormat(format);
    if (glFormat.internalFormat != GL_NONE) {
        if (auto pos = std::find_if(std::begin(IMAGE_FORMAT_FEATURES), std::end(IMAGE_FORMAT_FEATURES),
                [internalFormat = glFormat.internalFormat](
                    const FormatFeatures& features) { return features.internalFormat == internalFormat; });
            pos != std::end(IMAGE_FORMAT_FEATURES)) {
            // split texel buffer support to bufferFeatures
            properties.linearTilingFeatures = properties.optimalTilingFeatures =
                pos->flags & ~CORE_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT;
            // assume if the format can be sampled it can be used as a vertex buffer.
            properties.bufferFeatures =
                ((pos->flags & CORE_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ? CORE_FORMAT_FEATURE_VERTEX_BUFFER_BIT : 0U) |
                (pos->flags & CORE_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT);
#if RENDER_HAS_GL_BACKEND
            // desktop GL can filter anything
            properties.linearTilingFeatures |= TF;
            properties.optimalTilingFeatures |= TF;
#endif
            // can probably blit if not compressed
            if (!glFormat.compression.compressed) {
                properties.linearTilingFeatures |= CORE_FORMAT_FEATURE_BLIT_DST_BIT | CORE_FORMAT_FEATURE_BLIT_SRC_BIT;
                properties.optimalTilingFeatures |= CORE_FORMAT_FEATURE_BLIT_DST_BIT | CORE_FORMAT_FEATURE_BLIT_SRC_BIT;
            }
            properties.bytesPerPixel = glFormat.bytesperpixel;
        }
    }
    return properties;
}

AccelerationStructureBuildSizes DeviceGLES::GetAccelerationStructureBuildSizes(
    const AccelerationStructureBuildGeometryInfo& geometry,
    BASE_NS::array_view<const AccelerationStructureGeometryTrianglesInfo> triangles,
    BASE_NS::array_view<const AccelerationStructureGeometryAabbsInfo> aabbs,
    BASE_NS::array_view<const AccelerationStructureGeometryInstancesInfo> instances) const
{
    return {};
}

PlatformGpuMemoryAllocator* DeviceGLES::GetPlatformGpuMemoryAllocator()
{
    PLUGIN_ASSERT_MSG(false, "DeviceGLES::GetPlatformGpuMemoryAllocator called!");
    return nullptr;
}

// (re-)create swapchain
unique_ptr<Swapchain> DeviceGLES::CreateDeviceSwapchain(const SwapchainCreateInfo& swapchainCreateInfo)
{
    PLUGIN_ASSERT(IsActive());
    auto swapchain = make_unique<SwapchainGLES>(*this, swapchainCreateInfo);
    // Switch to the new swapchain.
    eglState_.SetContext(swapchain.get());
    return swapchain;
}

void DeviceGLES::DestroyDeviceSwapchain()
{
    PLUGIN_ASSERT(IsActive());
    // TODO:
    // Drop to dummycontext (ie. 1x1 surface etc...)
    eglState_.SetContext(nullptr);
}

bool DeviceGLES::IsActive() const
{
    return isActive_ > 0;
}

void DeviceGLES::Activate()
{
    activeMutex_.lock();
    PLUGIN_ASSERT_MSG(isActive_ == 0, "Activate called while already inactive");
    if (isActive_ == 0) {
        eglState_.SaveContext();

        constexpr RenderHandle defaultSwapchain {};
        Activate(defaultSwapchain);
    }
    isActive_++;
}

void DeviceGLES::Deactivate()
{
    PLUGIN_ASSERT_MSG(isActive_ > 0, "Deactivate called while already inactive");
    if (isActive_ > 0) {
        isActive_--;
    }
    if (isActive_ == 0) {
        eglState_.RestoreContext();
        isActive_ = false;
    }
    activeMutex_.unlock();
}

GpuQueue DeviceGLES::GetValidGpuQueue(const GpuQueue& gpuQueue) const
{
    return { GpuQueue::QueueType::GRAPHICS, 0 }; // no queues -> graphics
}

uint32_t DeviceGLES::GetGpuQueueCount() const
{
    return 1;
}

void DeviceGLES::InitializePipelineCache(array_view<const uint8_t> initialData)
{
    // NOTE: not implemented.
}

vector<uint8_t> DeviceGLES::GetPipelineCache() const
{
    // NOTE: not implemented. could probably be done by gathering glGetProgramBinary results together.
    return {};
}

void DeviceGLES::WaitForIdle()
{
    const bool activeState = IsActive();
    if (!activeState) {
        Activate();
    }
    if (!isRenderbackendRunning_) {
        PLUGIN_LOG_D("Device - WaitForIdle");
        glFinish();
    } else {
        PLUGIN_LOG_E("Device WaitForIdle can only called when render backend is not running");
    }
    if (!activeState) {
        Deactivate();
    }
}

#if (RENDER_HAS_GL_BACKEND)
unique_ptr<Device> CreateDeviceGL(RenderContext& renderContext, DeviceCreateInfo const& createInfo)
{
    if (auto device = make_unique<DeviceGLES>(renderContext, createInfo); device) {
        const auto& plat = static_cast<const DevicePlatformDataGL&>(device->GetPlatformData());
        if (plat.context != nullptr) {
            return device;
        }
    }
    return nullptr;
}
#endif
#if (RENDER_HAS_GLES_BACKEND)
unique_ptr<Device> CreateDeviceGLES(RenderContext& renderContext, DeviceCreateInfo const& createInfo)
{
    if (auto device = make_unique<DeviceGLES>(renderContext, createInfo); device) {
        const auto& plat = static_cast<const DevicePlatformDataGLES&>(device->GetPlatformData());
        if (plat.context != EGL_NO_CONTEXT) {
            return device;
        }
    }
    return nullptr;
}
#endif

void DeviceGLES::ReleaseShader(uint32_t type, uint32_t shader)
{
    vector<ShaderCache::Entry>* cache = nullptr;
    if (type == GL_FRAGMENT_SHADER) {
        cache = &caches[DeviceGLES::FRAGMENT_CACHE].cache;
    } else if (type == GL_VERTEX_SHADER) {
        cache = &caches[DeviceGLES::VERTEX_CACHE].cache;
    } else if (type == GL_COMPUTE_SHADER) {
        cache = &caches[DeviceGLES::COMPUTE_CACHE].cache;
    } else {
        return;
    }

    const auto pos = std::find_if(
        cache->begin(), cache->end(), [shader](const ShaderCache::Entry& entry) { return entry.shader == shader; });
    if (pos != cache->end()) {
        ShaderCache::Entry& entry = *pos;
        entry.refCount--;
        if (entry.refCount == 0) {
            glDeleteShader(entry.shader);
            cache->erase(pos);
        }
        return;
    }

    PLUGIN_ASSERT_MSG(false, "Tried to release a non-existant shader?");
}

void DeviceGLES::ReleaseProgram(uint32_t program)
{
    PLUGIN_ASSERT_MSG(isActive_, "Device not active when releasing shaders");
    for (auto it = programs_.begin(); it != programs_.end(); it++) {
        auto& t = *it;
        if (t.program == program) {
            t.refCount--;
            if (t.refCount == 0) {
                if (t.fragShader) {
                    ReleaseShader(GL_FRAGMENT_SHADER, t.fragShader);
                }
                if (t.vertShader) {
                    ReleaseShader(GL_VERTEX_SHADER, t.vertShader);
                }
                if (t.compShader) {
                    ReleaseShader(GL_COMPUTE_SHADER, t.compShader);
                }
                glDeleteProgram(t.program);
                programs_.erase(it);
            }
            return;
        }
    }
    PLUGIN_ASSERT_MSG(false, "Tried to release a non-existant program?");
}

const DeviceGLES::ShaderCache::Entry& DeviceGLES::CacheShader(int type, const string_view source)
{
    PLUGIN_ASSERT(type < MAX_CACHES);
    if (source.empty()) {
        static constexpr DeviceGLES::ShaderCache::Entry invalid {};
        return invalid;
    }
    constexpr GLenum types[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_COMPUTE_SHADER };
    // NOTE: check other hash functions, also i guess verify (strcmp) on collision would be prudent.
    const uint64_t hash = FNV1aHash(source.data(), source.size());
    PLUGIN_ASSERT(hash != 0);
    for (auto& t : caches[type].cache) {
        if (t.hash == hash) {
            caches[type].hit++;
            t.refCount++;
            return t;
        }
    }
    caches[type].miss++;
    DeviceGLES::ShaderCache::Entry entry;
    entry.hash = hash;
    entry.shader = glCreateShader(types[type]);
    entry.refCount = 1;
    const GLint len = static_cast<GLint>(source.length());
    const auto data = source.data();
    glShaderSource(entry.shader, 1, &data, &len);
    glCompileShader(entry.shader);
    GLint result = GL_FALSE;
    glGetShaderiv(entry.shader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        GLint logLength = 0;
        glGetShaderiv(entry.shader, GL_INFO_LOG_LENGTH, &logLength);
        string messages;
        messages.resize(static_cast<size_t>(logLength));
        glGetShaderInfoLog(entry.shader, logLength, 0, messages.data());
        PLUGIN_LOG_F("Shader compilation error: %s", messages.c_str());
        glDeleteShader(entry.shader);
        entry.shader = 0U;
    }
    caches[type].cache.push_back(entry);
    return caches[type].cache.back();
}

uint32_t DeviceGLES::CacheProgram(
    const string_view vertSource, const string_view fragSource, const string_view compSource)
{
    PLUGIN_ASSERT_MSG(isActive_, "Device not active when building shaders");
    // Hash and cache shader sources.
    const auto& vEntry = CacheShader(DeviceGLES::VERTEX_CACHE, vertSource);
    const auto& fEntry = CacheShader(DeviceGLES::FRAGMENT_CACHE, fragSource);
    const auto& cEntry = CacheShader(DeviceGLES::COMPUTE_CACHE, compSource);
    // Then check if we have the program already cached (ie. matching shaders linked)
    for (ProgramCache& t : programs_) {
        if ((t.hashVert != vEntry.hash) || (t.hashFrag != fEntry.hash) || (t.hashComp != cEntry.hash)) {
            continue;
        }
        pCacheHit_++;
        t.refCount++;
        return t.program;
    }
    // Create new program
    pCacheMiss_++;
    const GLuint program = glCreateProgram();
#if defined(CORE_USE_SEPARATE_SHADER_OBJECTS) && (CORE_USE_SEPARATE_SHADER_OBJECTS == 1)
    // enable separable programs.
    glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);
#endif
    // Attach and link
    if (vEntry.shader) {
        glAttachShader(program, vEntry.shader);
    }
    if (fEntry.shader) {
        glAttachShader(program, fEntry.shader);
    }
    if (cEntry.shader) {
        glAttachShader(program, cEntry.shader);
    }
    glLinkProgram(program);
    GLint result = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        string messages;
        messages.resize(static_cast<size_t>(logLength));
        glGetProgramInfoLog(program, logLength, 0, messages.data());
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_E("gl_shader_linking_error_" + to_string(program),
            "RENDER_VALIDATION: Shader linking error: %s", messages.c_str());
#endif
        glDeleteProgram(program);
        return 0U;
    }
    // Add the program to cache
    programs_.push_back(
        { program, vEntry.shader, fEntry.shader, cEntry.shader, vEntry.hash, fEntry.hash, cEntry.hash, 1 });
    return program;
}

void DeviceGLES::UseProgram(uint32_t program)
{
    if (boundProgram_ != program) {
        boundProgram_ = program;
        glUseProgram(static_cast<GLuint>(program));
    }
}

void DeviceGLES::BindBuffer(uint32_t target, uint32_t buffer)
{
    const uint32_t targetId = GenericTargetToTargetId(target);
    auto& state = bufferBound_[targetId];
    if ((!state.bound) || (state.buffer != buffer)) {
        state.bound = true;
        state.buffer = buffer;
        glBindBuffer(target, static_cast<GLuint>(buffer));
    }
}

void DeviceGLES::BindBufferRange(uint32_t target, uint32_t binding, uint32_t buffer, uint64_t offset, uint64_t size)
{
    const uint32_t targetId = IndexedTargetToTargetId(target);
    auto& slot = boundBuffers_[targetId][binding];

    if ((slot.cached == false) || (slot.buffer != buffer) || (slot.offset != offset) || (slot.size != size)) {
        slot.cached = true;
        slot.buffer = buffer;
        slot.offset = offset;
        slot.size = size;
        glBindBufferRange(target, binding, buffer, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size));
        // BindBufferRange sets the "generic" binding too. so make sure cache state is correct.
        const uint32_t targetId2 = GenericTargetToTargetId(target);
        auto& state = bufferBound_[targetId2];
        state.bound = true;
        state.buffer = buffer;
    }
    // NOTE: we are not forcing the generic bind point here. use BindBuffer to set the generic one if needed!
}

void DeviceGLES::BindSampler(uint32_t textureUnit, uint32_t sampler)
{
    if ((sampler + 1) != boundSampler_[textureUnit]) {
        boundSampler_[textureUnit] = sampler + 1;
        glBindSampler(textureUnit, sampler);
    }
}

uint32_t DeviceGLES::BoundReadFrameBuffer() const
{
    return boundReadFbo_;
}

uint32_t DeviceGLES::BoundWriteFrameBuffer() const
{
    return boundWriteFbo_;
}

uint32_t DeviceGLES::BoundProgram() const
{
    return boundProgram_;
}

uint32_t DeviceGLES::BoundBuffer(uint32_t target) const
{
    const uint32_t targetId = GenericTargetToTargetId(target);
    if (targetId >= MAX_BUFFER_BIND_ID) {
        return 0;
    }
    const auto& slot = bufferBound_[targetId];
    if (!slot.bound) {
        return 0;
    }
    return slot.buffer;
}

uint32_t DeviceGLES::BoundBuffer(uint32_t target, uint32_t binding) const
{
    const uint32_t targetId = IndexedTargetToTargetId(target);
    if (targetId >= MAX_BUFFER_BIND_ID || binding >= MAX_BINDING_VALUE) {
        return 0;
    }
    const auto& slot = boundBuffers_[targetId][binding];
    if (!slot.cached) {
        return 0;
    }
    return slot.buffer;
}

uint32_t DeviceGLES::BoundSampler(uint32_t textureUnit) const
{
    if (textureUnit >= MAX_SAMPLERS) {
        return 0;
    }
    const uint32_t bound = boundSampler_[textureUnit];
    return bound ? (bound - 1) : bound;
}

uint32_t DeviceGLES::BoundTexture(uint32_t textureUnit, uint32_t target) const
{
    const uint32_t targetId = TextureTargetToTargetId(target);
    if (textureUnit >= MAX_TEXTURE_UNITS || targetId >= MAX_TEXTURE_TARGET_ID) {
        return 0;
    }
    const uint32_t bound = boundTexture_[textureUnit][targetId];
    if (bound == 0) {
        return 0; // bound 0 == nothing has been bound via cache yet.
    }
    return bound - 1;
}

void DeviceGLES::BindImageTexture(
    uint32_t unit, uint32_t texture, uint32_t level, bool layered, uint32_t layer, uint32_t access, uint32_t format)
{
    auto& image = boundImage_[unit];
    if ((!image.bound) || (image.texture != texture) || (image.level != level) || (image.layered != layered) ||
        (image.access != access) || (image.format != format)) {
        image.bound = true;
        image.texture = texture;
        image.level = level;
        image.layered = layered;
        image.access = access;
        image.format = format;
        glBindImageTexture(static_cast<GLuint>(unit), static_cast<GLuint>(texture), static_cast<GLint>(level),
            static_cast<GLboolean>(layered), static_cast<GLint>(layer), static_cast<GLenum>(access),
            static_cast<GLenum>(format));
    }
}

void DeviceGLES::SetActiveTextureUnit(uint32_t textureUnit)
{
    if ((textureUnit + 1) != activeTextureUnit_) {
        activeTextureUnit_ = textureUnit + 1;
        glActiveTexture(GL_TEXTURE0 + textureUnit);
    }
}

void DeviceGLES::BindTexture(uint32_t textureUnit, uint32_t target, uint32_t texture)
{
    const uint32_t targetId = TextureTargetToTargetId(target);
#if RENDER_HAS_GLES_BACKEND
    if (target == GL_TEXTURE_EXTERNAL_OES) {
        // Work around for oes textures needing a bind to zero to update.
        SetActiveTextureUnit(textureUnit);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
        boundTexture_[textureUnit][targetId] = 0;
        // Force the default sampler for OES textures.
        BindSampler(textureUnit, 0);
    }
#endif
    if ((texture + 1) != boundTexture_[textureUnit][targetId]) {
        SetActiveTextureUnit(textureUnit);
        boundTexture_[textureUnit][targetId] = texture + 1;
        // remap the cubemap layer ids...
        switch (target) {
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
                break;
            default:
                glBindTexture(target, texture);
                break;
        }
    }
}

void DeviceGLES::TexSwizzle(uint32_t image, uint32_t target, const Math::UVec4& swizzle)
{
    // set only if not default..
    if ((swizzle.x != GL_RED) || (swizzle.y != GL_GREEN) || (swizzle.z != GL_BLUE) || (swizzle.w != GL_ALPHA)) {
        SetActiveTextureUnit(TEMP_BIND_UNIT);
        BindTexture(TEMP_BIND_UNIT, target, image);
        glTexParameteri(static_cast<GLenum>(target), GL_TEXTURE_SWIZZLE_R, static_cast<GLint>(swizzle.x));
        glTexParameteri(static_cast<GLenum>(target), GL_TEXTURE_SWIZZLE_G, static_cast<GLint>(swizzle.y));
        glTexParameteri(static_cast<GLenum>(target), GL_TEXTURE_SWIZZLE_B, static_cast<GLint>(swizzle.z));
        glTexParameteri(static_cast<GLenum>(target), GL_TEXTURE_SWIZZLE_A, static_cast<GLint>(swizzle.w));
    }
}

void DeviceGLES::TexStorage2D(
    uint32_t image, uint32_t target, uint32_t levels, uint32_t internalformat, const Math::UVec2& extent)
{
    SetActiveTextureUnit(TEMP_BIND_UNIT);
    BindTexture(TEMP_BIND_UNIT, target, image);
    glTexStorage2D(static_cast<GLenum>(target), static_cast<GLsizei>(levels), static_cast<GLenum>(internalformat),
        static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y));
}

void DeviceGLES::TexStorage2DMultisample(uint32_t image, uint32_t target, uint32_t samples, uint32_t internalformat,
    const Math::UVec2& extent, bool fixedsamplelocations)
{
    SetActiveTextureUnit(TEMP_BIND_UNIT);
    BindTexture(TEMP_BIND_UNIT, target, image);
    glTexStorage2DMultisample(static_cast<GLenum>(target), static_cast<GLsizei>(samples),
        static_cast<GLenum>(internalformat), static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y),
        fixedsamplelocations);
}

void DeviceGLES::TexStorage3D(
    uint32_t image, uint32_t target, uint32_t levels, uint32_t internalformat, const Math::UVec3& extent)
{
    SetActiveTextureUnit(TEMP_BIND_UNIT);
    BindTexture(TEMP_BIND_UNIT, target, image);
    glTexStorage3D((GLenum)target, (GLsizei)levels, (GLenum)internalformat, (GLsizei)extent.x, (GLsizei)extent.y,
        (GLsizei)extent.z);
}

void DeviceGLES::TexSubImage2D(uint32_t image, uint32_t target, uint32_t level, const Math::UVec2& offset,
    const Math::UVec2& extent, uint32_t format, uint32_t type, const void* pixels)
{
    SetActiveTextureUnit(TEMP_BIND_UNIT);
    BindTexture(TEMP_BIND_UNIT, target, image);
    glTexSubImage2D((GLenum)target, (GLint)level, (GLint)offset.x, (GLint)offset.y, (GLsizei)extent.x,
        (GLsizei)extent.y, (GLenum)format, (GLenum)type, pixels);
}

void DeviceGLES::TexSubImage3D(uint32_t image, uint32_t target, uint32_t level, const Math::UVec3& offset,
    const Math::UVec3& extent, uint32_t format, uint32_t type, const void* pixels)
{
    SetActiveTextureUnit(TEMP_BIND_UNIT);
    BindTexture(TEMP_BIND_UNIT, target, image);
    glTexSubImage3D((GLenum)target, (GLint)level, (GLint)offset.x, (GLint)offset.y, (GLint)offset.z, (GLsizei)extent.x,
        (GLsizei)extent.y, (GLsizei)extent.z, (GLenum)format, (GLenum)type, pixels);
}

void DeviceGLES::CompressedTexSubImage2D(uint32_t image, uint32_t target, uint32_t level, const Math::UVec2& offset,
    const Math::UVec2& extent, uint32_t format, uint32_t imageSize, const void* data)
{
    SetActiveTextureUnit(TEMP_BIND_UNIT);
    BindTexture(TEMP_BIND_UNIT, target, image);
    glCompressedTexSubImage2D((GLenum)target, (GLint)level, (GLint)offset.x, (GLint)offset.y, (GLsizei)extent.x,
        (GLsizei)extent.y, (GLenum)format, (GLint)imageSize, data);
}

void DeviceGLES::CompressedTexSubImage3D(uint32_t image, uint32_t target, uint32_t level, const Math::UVec3& offset,
    const Math::UVec3& extent, uint32_t format, uint32_t imageSize, const void* data)
{
    SetActiveTextureUnit(TEMP_BIND_UNIT);
    BindTexture(TEMP_BIND_UNIT, target, image);
    glCompressedTexSubImage3D((GLenum)target, (GLint)level, (GLint)offset.x, (GLint)offset.y, (GLint)offset.z,
        (GLsizei)extent.x, (GLsizei)extent.y, (GLsizei)extent.z, (GLenum)format, (GLint)imageSize, data);
}

const DeviceGLES::ImageFormat& DeviceGLES::GetGlImageFormat(const Format format) const
{
    if (const auto pos = std::lower_bound(supportedFormats_.begin(), supportedFormats_.end(), format,
            [](const ImageFormat& element, const Format value) { return element.coreFormat < value; });
        (pos != supportedFormats_.end()) && (pos->coreFormat == format)) {
        return *pos;
    }
    if (const auto pos = std::lower_bound(std::begin(IMAGE_FORMATS_FALLBACK), std::end(IMAGE_FORMATS_FALLBACK), format,
            [](const ImageFormat& element, const Format value) { return element.coreFormat < value; });
        (pos != std::end(IMAGE_FORMATS_FALLBACK)) && (pos->coreFormat == format)) {
        PLUGIN_LOG_I("using fallback for format %u", format);
        return *pos;
    }
    PLUGIN_LOG_I("asking for unsupported format %u", format);
    return supportedFormats_[0];
}

void DeviceGLES::DeleteTexture(uint32_t texture)
{
    UnBindTexture(texture);
    glDeleteTextures(1, &texture);
}

void DeviceGLES::DeleteBuffer(uint32_t buffer)
{
    UnBindBuffer(buffer);
    glDeleteBuffers(1, &buffer);
}

void DeviceGLES::DeleteSampler(uint32_t sampler)
{
    UnBindSampler(sampler);
    glDeleteSamplers(1, &sampler);
}

uint32_t DeviceGLES::CreateVertexArray()
{
    GLuint vao;
    glGenVertexArrays(1, &vao);
    if (vaoStatesInUse_ == vaoStates_.size()) {
        for (auto it = vaoStates_.begin(); it != vaoStates_.end(); it++) {
            if (it->vao == 0) {
                // re-use old "object"
                it->vao = vao;
                vaoStatesInUse_++;
                return static_cast<uint32_t>(1 + (it - vaoStates_.begin()));
            }
        }
    }
    VAOState v;
    v.vao = vao;
    vaoStates_.push_back(v);
    vaoStatesInUse_++;
    return static_cast<uint32_t>(vaoStates_.size());
}

void DeviceGLES::DeleteVertexArray(uint32_t vao)
{
    PLUGIN_ASSERT(!vaoStates_.empty());
    if (vao > 0) {
        UnBindVertexArray(vao);
        auto& state = vaoStates_[vao - 1];
        glDeleteVertexArrays(1, &state.vao);
        state = {}; // clear the object.
        vaoStatesInUse_--;
    }
}

void DeviceGLES::DeleteFrameBuffer(uint32_t fbo)
{
    PLUGIN_ASSERT(IsActive());
    UnBindFrameBuffer(fbo);
    glDeleteFramebuffers(1, &fbo);
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
    PLUGIN_LOG_D("fbo id <: %u", fbo);
#endif
}

void DeviceGLES::UnBindTexture(uint32_t texture)
{
    uint32_t unit = 0;
    for (auto& textureUnit : boundTexture_) {
        uint32_t targetId = 0;
        for (uint32_t& typeBinding : textureUnit) {
            if (typeBinding == texture + 1) {
                SetActiveTextureUnit(unit);
                const uint32_t target = TextureTargetIdToTarget(TextureTargetId { targetId });
                glBindTexture(target, 0);
                typeBinding = 0;
            }
            targetId++;
        }
        unit++;
    }

    unit = 0;
    for (auto& image : boundImage_) {
        if ((image.bound) && (image.texture == texture)) {
            glBindImageTexture(static_cast<GLuint>(unit), 0, 0, false, 0, GL_READ_ONLY, GL_R32UI);
            // set default state...
            image.bound = false;
            image.texture = 0;
            image.level = 0;
            image.layered = false;
            image.access = GL_READ_ONLY;
            image.format = GL_R32UI;
        }
        unit++;
    }
}

void DeviceGLES::UnBindBuffer(uint32_t buffer)
{
    uint32_t tid = 0;
    for (auto& buffers : boundBuffers_) {
        const uint32_t targetId = IndexedTargetIdToTarget(BufferBindId { tid });
        uint32_t bid = 0;
        for (auto& slot : buffers) {
            if (slot.buffer == buffer) {
                glBindBufferRange(targetId, bid, 0, 0, 0);
                // nothing bound
                slot.cached = false;
                slot.buffer = 0;
                slot.offset = 0;
                slot.size = 0;
            }
            bid++;
        }
        tid++;
    }

    tid = 0;
    for (auto& slot : bufferBound_) {
        if (slot.buffer == buffer) {
            const uint32_t targetId = GenericTargetIdToTarget(BufferTargetId { tid });
            glBindBuffer(targetId, 0);
            // nothing bound
            slot.bound = false;
            slot.buffer = 0;
        }
        tid++;
    }

    // scan VAOs also..
    UnBindBufferFromVertexArray(buffer);
}

void DeviceGLES::UnBindBufferFromVertexArray(uint32_t buffer)
{
    uint32_t vao = 1;
    uint32_t wasbound = BoundVertexArray();
    for (auto& state : vaoStates_) {
        if (state.vao > 0) {
            auto& elementBuffer = state.elementBuffer;
            auto& vertexBufferBinds = state.vertexBufferBinds;
            int slot = 0;
            for (auto& t : vertexBufferBinds) {
                if ((t.bound) && (t.buffer == buffer)) {
                    // detach the buffer from vao.
                    BindVertexArray(vao);
                    glBindVertexBuffer((GLuint)slot, 0, 0, 0);
                    t.bound = false;
                    t.buffer = 0;
                    t.offset = 0;
                    t.stride = 0;
                }
                slot++;
            }
            if ((elementBuffer.bound) && (elementBuffer.buffer == buffer)) {
                // detach the buffer from vao.
                BindVertexArray(vao);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                elementBuffer.bound = false;
                elementBuffer.buffer = 0;
            }
        }
        vao++;
    }
    BindVertexArray(wasbound);
}

void DeviceGLES::UnBindSampler(uint32_t sampler)
{
    for (uint32_t& boundSampler : boundSampler_) {
        if ((sampler + 1) == boundSampler) {
            glBindSampler((sampler + 1), 0);
            boundSampler = 0;
        }
    }
}

void DeviceGLES::UnBindVertexArray(uint32_t vao)
{
    if (boundVao_ == vao) {
        glBindVertexArray(0);
        boundVao_ = 0;
    }
}

void DeviceGLES::UnBindFrameBuffer(uint32_t fbo)
{
    if ((fbo == boundReadFbo_) && (fbo == boundWriteFbo_)) {
        boundReadFbo_ = 0;
        boundWriteFbo_ = 0;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else if (boundWriteFbo_ == fbo) {
        boundWriteFbo_ = 0;
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    } else if (boundReadFbo_ == fbo) {
        boundReadFbo_ = 0;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }
}

uint32_t DeviceGLES::BoundVertexArray() const
{
    return boundVao_;
}

void DeviceGLES::BindFrameBuffer(uint32_t fbo)
{
    if ((boundReadFbo_ != fbo) && (boundWriteFbo_ != fbo)) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        boundReadFbo_ = boundWriteFbo_ = fbo;
    } else if (boundWriteFbo_ != fbo) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        boundWriteFbo_ = fbo;
    } else if (boundReadFbo_ != fbo) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        boundReadFbo_ = fbo;
    }
}

void DeviceGLES::BindReadFrameBuffer(uint32_t fbo)
{
    if (boundReadFbo_ != fbo) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        boundReadFbo_ = fbo;
    }
}

void DeviceGLES::BindWriteFrameBuffer(uint32_t fbo)
{
    if (boundWriteFbo_ != fbo) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        boundWriteFbo_ = fbo;
    }
}

void DeviceGLES::BindVertexArray(uint32_t vao)
{
    if (boundVao_ != vao) {
        PLUGIN_ASSERT(vao <= vaoStates_.size());
        if (vao > 0) {
            glBindVertexArray(vaoStates_[vao - 1].vao);
        } else {
            glBindVertexArray(0);
        }
        boundVao_ = vao;
    }
}

void DeviceGLES::BindVertexBuffer(uint32_t slot, uint32_t buffer, intptr_t offset, intptr_t stride)
{
    PLUGIN_ASSERT(boundVao_ > 0);
    PLUGIN_ASSERT(boundVao_ <= vaoStates_.size());
    auto& vertexBufferBinds = vaoStates_[boundVao_ - 1].vertexBufferBinds;
    bool bind = true;
    if (vertexBufferBinds[slot].bound) {
        bind = (vertexBufferBinds[slot].buffer != buffer) || (vertexBufferBinds[slot].offset != offset) ||
               (vertexBufferBinds[slot].stride != stride);
    }
    if (bind) {
        vertexBufferBinds[slot].bound = true;
        vertexBufferBinds[slot].buffer = buffer;
        vertexBufferBinds[slot].offset = offset;
        vertexBufferBinds[slot].stride = stride;
        glBindVertexBuffer((GLuint)slot, (GLuint)buffer, (GLintptr)offset, (GLsizei)stride);
    }
}

void DeviceGLES::VertexBindingDivisor(uint32_t slot, uint32_t divisor)
{
    PLUGIN_ASSERT(boundVao_ > 0);
    PLUGIN_ASSERT(boundVao_ <= vaoStates_.size());
    auto& vertexBufferBinds = vaoStates_[boundVao_ - 1].vertexBufferBinds;
    if (vertexBufferBinds[slot].divisor != divisor) {
        vertexBufferBinds[slot].divisor = divisor;
        glVertexBindingDivisor(slot, divisor);
    }
}

void DeviceGLES::BindElementBuffer(uint32_t buffer)
{
    PLUGIN_ASSERT(boundVao_ > 0);
    PLUGIN_ASSERT(boundVao_ <= vaoStates_.size());
    auto& elementBuffer = vaoStates_[boundVao_ - 1].elementBuffer;
    bool bind = true;
    if (elementBuffer.bound) {
        bind = (buffer != elementBuffer.buffer);
    }
    if (bind) {
        elementBuffer.bound = true;
        elementBuffer.buffer = buffer;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
    }
}

bool DeviceGLES::AllowThreadedProcessing() const
{
    return HasExtension(EXT_BUFFER_STORAGE);
}

unique_ptr<GpuBuffer> DeviceGLES::CreateGpuBuffer(const GpuBufferDesc& desc)
{
    return make_unique<GpuBufferGLES>(*this, desc);
}

unique_ptr<GpuBuffer> DeviceGLES::CreateGpuBuffer(const GpuAccelerationStructureDesc& desc)
{
    return make_unique<GpuBufferGLES>(*this, desc.bufferDesc);
}

unique_ptr<GpuImage> DeviceGLES::CreateGpuImage(const GpuImageDesc& desc)
{
    return make_unique<GpuImageGLES>(*this, desc);
}

unique_ptr<GpuImage> DeviceGLES::CreateGpuImageView(const GpuImageDesc& desc, const GpuImagePlatformData& platformData)
{
    PLUGIN_ASSERT(IsActive());
    return make_unique<GpuImageGLES>(*this, desc, platformData);
}

vector<unique_ptr<GpuImage>> DeviceGLES::CreateGpuImageViews(const Swapchain& platformSwapchain)
{
    vector<unique_ptr<GpuImage>> gpuImages;
    const SwapchainGLES& swapchain = (const SwapchainGLES&)platformSwapchain;
    const GpuImageDesc& desc = swapchain.GetDesc();
    const auto& swapchainPlat = swapchain.GetPlatformData();

    gpuImages.resize(swapchainPlat.swapchainImages.images.size());

    PLUGIN_ASSERT(IsActive());
    for (size_t idx = 0; idx < gpuImages.size(); ++idx) {
        GpuImagePlatformDataGL gpuImagePlat {};
        gpuImagePlat.image = swapchainPlat.swapchainImages.images[idx];
        gpuImagePlat.swizzle = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };
        gpuImages[idx] = CreateGpuImageView(desc, gpuImagePlat);
    }
    return gpuImages;
}

unique_ptr<GpuSampler> DeviceGLES::CreateGpuSampler(const GpuSamplerDesc& desc)
{
    return make_unique<GpuSamplerGLES>(*this, desc);
}

unique_ptr<RenderFrameSync> DeviceGLES::CreateRenderFrameSync()
{
    return make_unique<RenderFrameSyncGLES>(*this);
}

unique_ptr<RenderBackend> DeviceGLES::CreateRenderBackend(
    GpuResourceManager& gpuResourceMgr, const CORE_NS::IParallelTaskQueue::Ptr&)
{
    return make_unique<RenderBackendGLES>(*this, gpuResourceMgr);
}

unique_ptr<ShaderModule> DeviceGLES::CreateShaderModule(const ShaderModuleCreateInfo& data)
{
    return make_unique<ShaderModuleGLES>(*this, data);
}

unique_ptr<ShaderModule> DeviceGLES::CreateComputeShaderModule(const ShaderModuleCreateInfo& data)
{
    return make_unique<ShaderModuleGLES>(*this, data);
}

unique_ptr<GpuShaderProgram> DeviceGLES::CreateGpuShaderProgram(const GpuShaderProgramCreateData& data)
{
    return make_unique<GpuShaderProgramGLES>(*this, data);
}

unique_ptr<GpuComputeProgram> DeviceGLES::CreateGpuComputeProgram(const GpuComputeProgramCreateData& data)
{
    return make_unique<GpuComputeProgramGLES>(*this, data);
}

unique_ptr<NodeContextDescriptorSetManager> DeviceGLES::CreateNodeContextDescriptorSetManager()
{
    return make_unique<NodeContextDescriptorSetManagerGLES>(*this);
}

unique_ptr<NodeContextPoolManager> DeviceGLES::CreateNodeContextPoolManager(
    GpuResourceManager& gpuResourceMgr, const GpuQueue& gpuQueue)
{
    return make_unique<NodeContextPoolManagerGLES>(*this, gpuResourceMgr);
}

unique_ptr<GraphicsPipelineStateObject> DeviceGLES::CreateGraphicsPipelineStateObject(
    const GpuShaderProgram& gpuProgram, const GraphicsState& graphicsState, const PipelineLayout& pipelineLayout,
    const VertexInputDeclarationView& vertexInputDeclaration,
    const ShaderSpecializationConstantDataView& specializationConstants,
    const array_view<const DynamicStateEnum> dynamicStates, const RenderPassDesc& renderPassDesc,
    const array_view<const RenderPassSubpassDesc>& renderPassSubpassDescs, const uint32_t subpassIndex,
    const LowLevelRenderPassData* renderPassData, const LowLevelPipelineLayoutData* pipelineLayoutData)
{
    PLUGIN_ASSERT(!renderPassData);
    PLUGIN_ASSERT(!pipelineLayoutData);
    auto pipeline = make_unique<GraphicsPipelineStateObjectGLES>(*this, gpuProgram, graphicsState, pipelineLayout,
        vertexInputDeclaration, specializationConstants, dynamicStates, renderPassDesc, renderPassSubpassDescs,
        subpassIndex);
    return unique_ptr<GraphicsPipelineStateObject> { pipeline->GetPlatformData().graphicsShader ? pipeline.release()
                                                                                                : nullptr };
}

unique_ptr<ComputePipelineStateObject> DeviceGLES::CreateComputePipelineStateObject(const GpuComputeProgram& gpuProgram,
    const PipelineLayout& pipelineLayout, const ShaderSpecializationConstantDataView& specializationConstants,
    const LowLevelPipelineLayoutData* pipelineLayoutData)
{
    PLUGIN_ASSERT(!pipelineLayoutData);
    auto pipeline =
        make_unique<ComputePipelineStateObjectGLES>(*this, gpuProgram, pipelineLayout, specializationConstants);
    return unique_ptr<ComputePipelineStateObject> { pipeline->GetPlatformData().computeShader ? pipeline.release()
                                                                                              : nullptr };
}

unique_ptr<GpuSemaphore> DeviceGLES::CreateGpuSemaphore()
{
    return make_unique<GpuSemaphoreGles>(*this);
}

unique_ptr<GpuSemaphore> DeviceGLES::CreateGpuSemaphoreView(const uint64_t handle)
{
    return make_unique<GpuSemaphoreGles>(*this, handle);
}

void DeviceGLES::SetBackendConfig(const BackendConfig& config)
{
#if RENDER_HAS_GLES_BACKEND
    backendConfig_.allowDepthResolve = static_cast<const BackendConfigGLES&>(config).allowDepthResolve &&
                                       HasExtension("GL_EXT_multisampled_render_to_texture2");
#endif
}

LowLevelDeviceGLES::LowLevelDeviceGLES(DeviceGLES& deviceGLES)
    : deviceGLES_(deviceGLES), gpuResourceMgr_(static_cast<GpuResourceManager&>(deviceGLES.GetGpuResourceManager()))
{}

DeviceBackendType LowLevelDeviceGLES::GetBackendType() const
{
    return deviceGLES_.GetBackendType();
}

#if RENDER_HAS_EXPERIMENTAL
void LowLevelDeviceGLES::Activate()
{
    deviceGLES_.Activate();
}

void LowLevelDeviceGLES::Deactivate()
{
    deviceGLES_.Deactivate();
}

void LowLevelDeviceGLES::SwapBuffers()
{
    if (deviceGLES_.IsActive() && deviceGLES_.HasSwapchain()) {
        RenderHandle defaultSwapChain {};
        auto sc = static_cast<const SwapchainGLES*>(deviceGLES_.GetSwapchain(defaultSwapChain));
        deviceGLES_.SwapBuffers(*sc);
    }
}
#endif

RENDER_END_NAMESPACE()
