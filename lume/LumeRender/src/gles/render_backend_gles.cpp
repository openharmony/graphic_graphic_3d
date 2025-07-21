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

#include "render_backend_gles.h"

#include <algorithm>

#include <base/containers/fixed_string.h>
#include <render/datastore/render_data_store_render_pods.h> // NodeGraphBackbufferConfiguration...
#include <render/namespace.h>

#if (RENDER_PERF_ENABLED == 1)
#include <core/perf/cpu_perf_scope.h>
#include <core/perf/intf_performance_data_manager.h>

#include "perf/gpu_query.h"
#include "perf/gpu_query_manager.h"
#endif
#include "device/gpu_resource_manager.h"
#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "gles/gpu_buffer_gles.h"
#include "gles/gpu_image_gles.h"
#include "gles/gpu_program_gles.h"
#include "gles/gpu_query_gles.h"
#include "gles/gpu_sampler_gles.h"
#include "gles/gpu_semaphore_gles.h"
#include "gles/node_context_descriptor_set_manager_gles.h"
#include "gles/node_context_pool_manager_gles.h"
#include "gles/pipeline_state_object_gles.h"
#include "gles/render_frame_sync_gles.h"
#include "gles/swapchain_gles.h"
#include "nodecontext/pipeline_descriptor_set_binder.h"
#include "nodecontext/render_command_list.h"
#include "nodecontext/render_node_graph_node_store.h" // RenderCommandFrameData
#include "util/log.h"
#include "util/render_frame_util.h"

#define IS_BIT(value, bit) ((((value) & (bit)) == (bit)) ? true : false)
#define IS_BIT_GL(value, bit) ((((value) & (bit)) == (bit)) ? (GLboolean)GL_TRUE : (GLboolean)GL_FALSE)

using namespace BASE_NS;

// NOTE: implement missing commands, add state caching and cleanup a bit more.
RENDER_BEGIN_NAMESPACE()
namespace Gles {
// Indices to colorBlendConstants
static constexpr uint32_t RED_INDEX = 0;
static constexpr uint32_t GREEN_INDEX = 1;
static constexpr uint32_t BLUE_INDEX = 2;
static constexpr uint32_t ALPHA_INDEX = 3;
static constexpr uint32_t CUBEMAP_LAYERS = 6;
} // namespace Gles

namespace {
constexpr GLenum LAYER_ID[] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0 };

GLenum GetCubeMapTarget(GLenum type, uint32_t layer)
{
    if (type == GL_TEXTURE_CUBE_MAP) {
        PLUGIN_ASSERT_MSG(layer < Gles::CUBEMAP_LAYERS, "Invalid cubemap index %u", layer);
        return LAYER_ID[layer];
    }
    PLUGIN_ASSERT_MSG(false, "Unhandled type in getTarget! %x", type);
    return GL_NONE;
}

GLenum GetTarget(GLenum type, uint32_t layer, uint32_t sampleCount)
{
    if (type == GL_TEXTURE_2D) {
        if (sampleCount > 1) {
            return GL_TEXTURE_2D_MULTISAMPLE;
        }
        return GL_TEXTURE_2D;
    }
    if (type == GL_TEXTURE_CUBE_MAP) {
        PLUGIN_ASSERT_MSG(sampleCount == 1, "Cubemap texture can't have MSAA");
        return GetCubeMapTarget(type, layer);
    }
    PLUGIN_ASSERT_MSG(false, "Unhandled type in getTarget! %x", type);
    return GL_NONE;
}

struct BlitArgs {
    uint32_t mipLevel {};
    Size3D rect0 {};
    Size3D rect1 {};
    uint32_t height {};
};

void DoBlit(const Filter filter, const BlitArgs& src, const BlitArgs& dst)
{
    // Handle top-left / bottom-left origin conversion
    auto sy = static_cast<GLint>(src.rect0.height);
    const auto sh = static_cast<const GLint>(src.rect1.height);
    const auto sfh = static_cast<GLint>(src.height >> src.mipLevel);
    sy = sfh - (sy + sh);
    auto dy = static_cast<GLint>(dst.rect0.height);
    const auto dh = static_cast<const GLint>(dst.rect1.height);
    const auto dfh = static_cast<GLint>(dst.height >> dst.mipLevel);
    dy = dfh - (dy + dh);
    GLenum glfilter = GL_NEAREST;
    if (filter == CORE_FILTER_NEAREST) {
        glfilter = GL_NEAREST;
    } else if (filter == CORE_FILTER_LINEAR) {
        glfilter = GL_LINEAR;
    } else {
        PLUGIN_ASSERT_MSG(false, "RenderCommandBlitImage Invalid filter mode");
    }
    glBlitFramebuffer(static_cast<GLint>(src.rect0.width), sy, static_cast<GLint>(src.rect1.width), sfh,
        static_cast<GLint>(dst.rect0.width), dy, static_cast<GLint>(dst.rect1.width), dfh, GL_COLOR_BUFFER_BIT,
        glfilter);
}

GLenum GetPrimFromTopology(PrimitiveTopology op)
{
    switch (op) {
        case CORE_PRIMITIVE_TOPOLOGY_POINT_LIST:
            return GL_POINTS;
        case CORE_PRIMITIVE_TOPOLOGY_LINE_LIST:
            return GL_LINES;
        case CORE_PRIMITIVE_TOPOLOGY_LINE_STRIP:
            return GL_LINE_STRIP;
        case CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
            return GL_TRIANGLES;
        case CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
            return GL_TRIANGLE_STRIP;
        case CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
            return GL_TRIANGLE_FAN;
#if defined(GL_ES_VERSION_3_2) || defined(GL_VERSION_3_2)
            // The following are valid after gles 3.2
        case CORE_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
            return GL_LINES_ADJACENCY;
        case CORE_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
            return GL_LINE_STRIP_ADJACENCY;
        case CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
            return GL_TRIANGLES_ADJACENCY;
        case CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
            return GL_TRIANGLE_STRIP_ADJACENCY;
        case CORE_PRIMITIVE_TOPOLOGY_PATCH_LIST:
            return GL_PATCHES;
#endif
        default:
            PLUGIN_ASSERT_MSG(false, "Unsupported primitive topology");
            break;
    }
    return GL_POINTS;
}

GLenum GetBlendOp(BlendOp func)
{
    switch (func) {
        case CORE_BLEND_OP_ADD:
            return GL_FUNC_ADD;
        case CORE_BLEND_OP_SUBTRACT:
            return GL_FUNC_SUBTRACT;
        case CORE_BLEND_OP_REVERSE_SUBTRACT:
            return GL_FUNC_REVERSE_SUBTRACT;
        case CORE_BLEND_OP_MIN:
            return GL_MIN;
        case CORE_BLEND_OP_MAX:
            return GL_MAX;
        default:
            break;
    }
    return GL_FUNC_ADD;
}

GLenum GetBlendFactor(BlendFactor factor)
{
    switch (factor) {
        case CORE_BLEND_FACTOR_ZERO:
            return GL_ZERO;
        case CORE_BLEND_FACTOR_ONE:
            return GL_ONE;
        case CORE_BLEND_FACTOR_SRC_COLOR:
            return GL_SRC_COLOR;
        case CORE_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
            return GL_ONE_MINUS_SRC_COLOR;
        case CORE_BLEND_FACTOR_DST_COLOR:
            return GL_DST_COLOR;
        case CORE_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
            return GL_ONE_MINUS_DST_COLOR;
        case CORE_BLEND_FACTOR_SRC_ALPHA:
            return GL_SRC_ALPHA;
        case CORE_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
            return GL_ONE_MINUS_SRC_ALPHA;
        case CORE_BLEND_FACTOR_DST_ALPHA:
            return GL_DST_ALPHA;
        case CORE_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
            return GL_ONE_MINUS_DST_ALPHA;
        case CORE_BLEND_FACTOR_CONSTANT_COLOR:
            return GL_CONSTANT_COLOR;
        case CORE_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
            return GL_ONE_MINUS_CONSTANT_COLOR;
        case CORE_BLEND_FACTOR_CONSTANT_ALPHA:
            return GL_CONSTANT_ALPHA;
        case CORE_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
            return GL_ONE_MINUS_CONSTANT_ALPHA;
        case CORE_BLEND_FACTOR_SRC_ALPHA_SATURATE:
            return GL_SRC_ALPHA_SATURATE;
            // NOTE: check the GLES3.2...
            /* following requires EXT_blend_func_extended (dual source blending) */
        case CORE_BLEND_FACTOR_SRC1_COLOR:
        case CORE_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:
        case CORE_BLEND_FACTOR_SRC1_ALPHA:
        case CORE_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:
        default:
            break;
    }
    return GL_ONE;
}

GLenum GetCompareOp(CompareOp aOp)
{
    switch (aOp) {
        case CORE_COMPARE_OP_NEVER:
            return GL_NEVER;
        case CORE_COMPARE_OP_LESS:
            return GL_LESS;
        case CORE_COMPARE_OP_EQUAL:
            return GL_EQUAL;
        case CORE_COMPARE_OP_LESS_OR_EQUAL:
            return GL_LEQUAL;
        case CORE_COMPARE_OP_GREATER:
            return GL_GREATER;
        case CORE_COMPARE_OP_NOT_EQUAL:
            return GL_NOTEQUAL;
        case CORE_COMPARE_OP_GREATER_OR_EQUAL:
            return GL_GEQUAL;
        case CORE_COMPARE_OP_ALWAYS:
            return GL_ALWAYS;
        default:
            break;
    }
    return GL_ALWAYS;
}

GLenum GetStencilOp(StencilOp aOp)
{
    switch (aOp) {
        case CORE_STENCIL_OP_KEEP:
            return GL_KEEP;
        case CORE_STENCIL_OP_ZERO:
            return GL_ZERO;
        case CORE_STENCIL_OP_REPLACE:
            return GL_REPLACE;
        case CORE_STENCIL_OP_INCREMENT_AND_CLAMP:
            return GL_INCR;
        case CORE_STENCIL_OP_DECREMENT_AND_CLAMP:
            return GL_DECR;
        case CORE_STENCIL_OP_INVERT:
            return GL_INVERT;
        case CORE_STENCIL_OP_INCREMENT_AND_WRAP:
            return GL_INCR_WRAP;
        case CORE_STENCIL_OP_DECREMENT_AND_WRAP:
            return GL_DECR_WRAP;
        default:
            break;
    }
    return GL_KEEP;
}

void SetState(GLenum type, bool enabled)
{
    if (enabled) {
        glEnable(type);
    } else {
        glDisable(type);
    }
}

void SetCullMode(const GraphicsState::RasterizationState& rs)
{
    SetState(GL_CULL_FACE, (rs.cullModeFlags != CORE_CULL_MODE_NONE));

    switch (rs.cullModeFlags) {
        case CORE_CULL_MODE_FRONT_BIT:
            glCullFace(GL_FRONT);
            break;
        case CORE_CULL_MODE_BACK_BIT:
            glCullFace(GL_BACK);
            break;
        case CORE_CULL_MODE_FRONT_AND_BACK:
            glCullFace(GL_FRONT_AND_BACK);
            break;
        case CORE_CULL_MODE_NONE:
        default:
            break;
    }
}

void SetFrontFace(const GraphicsState::RasterizationState& rs)
{
    switch (rs.frontFace) {
        case CORE_FRONT_FACE_COUNTER_CLOCKWISE:
            glFrontFace(GL_CCW);
            break;
        case CORE_FRONT_FACE_CLOCKWISE:
            glFrontFace(GL_CW);
            break;
        default:
            break;
    }
}

#if RENDER_HAS_GL_BACKEND
void SetPolygonMode(const GraphicsState::RasterizationState& rs)
{
    GLenum mode;
    switch (rs.polygonMode) {
        default:
        case CORE_POLYGON_MODE_FILL:
            mode = GL_FILL;
            break;
        case CORE_POLYGON_MODE_LINE:
            mode = GL_LINE;
            break;
        case CORE_POLYGON_MODE_POINT:
            mode = GL_POINT;
            break;
    }
    glPolygonMode(GL_FRONT_AND_BACK, mode);
}
#endif

void Invalidate(GLenum framebuffer, int32_t count, const GLenum invalidate[], const RenderPassDesc& rpd,
    const LowlevelFramebufferGL& frameBuffer)
{
    if (count > 0) {
        if ((frameBuffer.width == rpd.renderArea.extentWidth) && (frameBuffer.height == rpd.renderArea.extentHeight)) {
            // Invalidate the whole buffer.  (attachment sizes match render area)
            glInvalidateFramebuffer(framebuffer, static_cast<GLsizei>(count), invalidate);
        } else {
            // invalidate only a part of the render target..
            // NOTE: verify that this works, we might need to flip the Y axis the same way as scissors etc.
            const auto X = static_cast<const GLint>(rpd.renderArea.offsetX);
            const auto Y = static_cast<const GLint>(rpd.renderArea.offsetY);
            const auto W = static_cast<const GLsizei>(rpd.renderArea.extentWidth);
            const auto H = static_cast<const GLsizei>(rpd.renderArea.extentHeight);
            glInvalidateSubFramebuffer(framebuffer, static_cast<GLsizei>(count), invalidate, X, Y, W, H);
        }
    }
}

struct BlitData {
    const GpuImagePlatformDataGL& iPlat;
    const GpuImageDesc& imageDesc;
    const BufferImageCopy& bufferImageCopy;
    uintptr_t data { 0 };
    uint64_t size { 0 };
    uint64_t sizeOfData { 0 };
    bool compressed { false };
};

void BlitArray(DeviceGLES& device_, const BlitData& bd)
{
    const auto& iPlat = bd.iPlat;
    const auto& bufferImageCopy = bd.bufferImageCopy;
    const auto& imageSubresource = bufferImageCopy.imageSubresource;
    const auto& imageDesc = bd.imageDesc;
    const uint32_t mip = imageSubresource.mipLevel;
    const Math::UVec3 imageSize { imageDesc.width >> mip, imageDesc.height >> mip, imageDesc.depth };
    // NOTE: image offset depth is ignored
    const Math::UVec2 offset { bufferImageCopy.imageOffset.width, bufferImageCopy.imageOffset.height };
    const Math::UVec3 extent3D { Math::min(imageSize.x - offset.x, bufferImageCopy.imageExtent.width),
        Math::min(imageSize.y - offset.y, bufferImageCopy.imageExtent.height),
        Math::min(imageSize.z, bufferImageCopy.imageExtent.depth) };
    const bool valid = (offset.x < imageSize.x) && (offset.y < imageSize.y);
    if (valid) {
        uintptr_t data = bd.data;
        const uint32_t layerCount = imageSubresource.baseArrayLayer + imageSubresource.layerCount;
        for (uint32_t layer = imageSubresource.baseArrayLayer; layer < layerCount; layer++) {
            const Math::UVec3 offset3D { offset.x, offset.y, layer };
            if (bd.compressed) {
                device_.CompressedTexSubImage3D(iPlat.image, iPlat.type, imageSubresource.mipLevel, offset3D, extent3D,
                    iPlat.internalFormat, static_cast<uint32_t>(bd.sizeOfData), reinterpret_cast<const void*>(data));
            } else {
                device_.TexSubImage3D(iPlat.image, iPlat.type, imageSubresource.mipLevel, offset3D, extent3D,
                    iPlat.format, iPlat.dataType, reinterpret_cast<const void*>(data));
            }
            data += static_cast<ptrdiff_t>(bd.sizeOfData);
        }
    }
}

void Blit2D(DeviceGLES& device_, const BlitData& bd)
{
    const auto& iPlat = bd.iPlat;
    const auto& bufferImageCopy = bd.bufferImageCopy;
    const auto& imageSubresource = bufferImageCopy.imageSubresource;
    const auto& imageDesc = bd.imageDesc;
    const uint32_t mip = imageSubresource.mipLevel;
    const Math::UVec2 imageSize { imageDesc.width >> mip, imageDesc.height >> mip };
    const Math::UVec2 offset { bufferImageCopy.imageOffset.width, bufferImageCopy.imageOffset.height };
    const Math::UVec2 extent { Math::min(imageSize.x - offset.x, bufferImageCopy.imageExtent.width),
        Math::min(imageSize.y - offset.y, bufferImageCopy.imageExtent.height) };
    PLUGIN_ASSERT_MSG(imageSubresource.baseArrayLayer == 0 && imageSubresource.layerCount == 1,
        "RenderCommandCopyBufferImage Texture2D with baseArrayLayer!=0 && layerCount!= 1");
    const bool valid = (offset.x < imageSize.x) && (offset.y < imageSize.y);
    const uintptr_t data = bd.data;
    if (valid && bd.compressed) {
        device_.CompressedTexSubImage2D(iPlat.image, iPlat.type, imageSubresource.mipLevel, offset, extent,
            iPlat.internalFormat, static_cast<uint32_t>(bd.sizeOfData), reinterpret_cast<const void*>(data));
    } else if (valid) {
        device_.TexSubImage2D(iPlat.image, iPlat.type, imageSubresource.mipLevel, offset, extent, iPlat.format,
            iPlat.dataType, reinterpret_cast<const void*>(data));
    }
}

void Blit3D(DeviceGLES& device_, const BlitData& bd)
{
    const auto& iPlat = bd.iPlat;
    const auto& bufferImageCopy = bd.bufferImageCopy;
    const auto& imageSubresource = bufferImageCopy.imageSubresource;
    const auto& imageDesc = bd.imageDesc;
    const uint32_t mip = imageSubresource.mipLevel;
    const Math::UVec3 imageSize { imageDesc.width >> mip, imageDesc.height >> mip, imageDesc.depth >> mip };
    const Math::UVec3 offset { bufferImageCopy.imageOffset.width, bufferImageCopy.imageOffset.height,
        bufferImageCopy.imageOffset.depth };
    Math::UVec3 extent3D { Math::min(imageSize.x - offset.x, bufferImageCopy.imageExtent.width),
        Math::min(imageSize.y - offset.y, bufferImageCopy.imageExtent.height), Math::min(imageSize.z - offset.z, 1U) };
    const bool valid = (offset.x < imageSize.x) && (offset.y < imageSize.y);
    if (valid) {
        uintptr_t data = bd.data;
        for (uint32_t slice = 0U; slice < imageSize.z; ++slice) {
            const Math::UVec3 offset3D { offset.x, offset.y, slice };
            if (bd.compressed) {
                device_.CompressedTexSubImage3D(iPlat.image, iPlat.type, imageSubresource.mipLevel, offset3D, extent3D,
                    iPlat.internalFormat, static_cast<uint32_t>(bd.sizeOfData), reinterpret_cast<const void*>(data));
            } else {
                device_.TexSubImage3D(iPlat.image, iPlat.type, imageSubresource.mipLevel, offset3D, extent3D,
                    iPlat.format, iPlat.dataType, reinterpret_cast<const void*>(data));
            }
            // offsets one slice
            data += static_cast<uintptr_t>(bd.sizeOfData);
        }
    }
}

void BlitCube(DeviceGLES& device_, const BlitData& bd)
{
    const auto& iPlat = bd.iPlat;
    const auto& bufferImageCopy = bd.bufferImageCopy;
    const auto& imageSubresource = bufferImageCopy.imageSubresource;
    const Math::UVec2 offset { bufferImageCopy.imageOffset.width, bufferImageCopy.imageOffset.height };
    const Math::UVec2 extent { bufferImageCopy.imageExtent.width, bufferImageCopy.imageExtent.height };
    constexpr GLenum faceId[] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0 };
    PLUGIN_UNUSED(Gles::CUBEMAP_LAYERS);
    PLUGIN_ASSERT_MSG(imageSubresource.baseArrayLayer == 0 && imageSubresource.layerCount == Gles::CUBEMAP_LAYERS,
        "RenderCommandCopyBufferImage Cubemap with baseArrayLayer!=0 && layerCount!= 6");
    uintptr_t data = bd.data;
    const uint32_t lastLayer = imageSubresource.baseArrayLayer + imageSubresource.layerCount;
    for (uint32_t i = imageSubresource.baseArrayLayer; i < lastLayer; i++) {
        const GLenum face = faceId[i]; // convert layer index to cube map face id.
        if (face == 0) {
            // reached the end of cubemap faces (see faceId)
            // so must stop copying.
            break;
        }
        if (bd.compressed) {
            device_.CompressedTexSubImage2D(iPlat.image, face, imageSubresource.mipLevel, offset, extent,
                iPlat.internalFormat, static_cast<uint32_t>(bd.sizeOfData), reinterpret_cast<const void*>(data));
        } else {
            device_.TexSubImage2D(iPlat.image, face, imageSubresource.mipLevel, offset, extent, iPlat.format,
                iPlat.dataType, reinterpret_cast<const void*>(data));
        }
        data += static_cast<uintptr_t>(bd.sizeOfData);
    }
}
template<bool usePixelUnpackBuffer>

BlitData SetupBlit(DeviceGLES& device_, const BufferImageCopy& bufferImageCopy, GpuBufferGLES& srcGpuBuffer,
    const GpuImageGLES& dstGpuImage)
{
    const auto& iPlat = dstGpuImage.GetPlatformData();
    const auto& imageOffset = bufferImageCopy.imageOffset;
    PLUGIN_UNUSED(imageOffset);
    const auto& imageExtent = bufferImageCopy.imageExtent;
    auto width = (!bufferImageCopy.bufferImageHeight || bufferImageCopy.bufferRowLength)
                     ? bufferImageCopy.imageExtent.width
                     : bufferImageCopy.bufferRowLength;
    auto height = (!bufferImageCopy.bufferImageHeight || bufferImageCopy.bufferRowLength)
                      ? bufferImageCopy.imageExtent.height
                      : bufferImageCopy.bufferImageHeight;
    // size is calculated for single layer / slice
    const uint64_t size =
        static_cast<uint64_t>(iPlat.bytesperpixel) * static_cast<uint64_t>(width) * static_cast<uint64_t>(height);
    uintptr_t data = bufferImageCopy.bufferOffset;
    if constexpr (usePixelUnpackBuffer) {
        const auto& plat = srcGpuBuffer.GetPlatformData();
        device_.BindBuffer(GL_PIXEL_UNPACK_BUFFER, plat.buffer);
    } else {
        // Use the mapped pointer for glTexSubImage2D, this is a workaround on GL_INVALID_OPERATION on PVR GLES
        // simulator and crash with ETC2 textures on NVIDIA..
        data += reinterpret_cast<uintptr_t>(srcGpuBuffer.Map());
    }
    uint64_t sizeOfData = size;
    const auto& compinfo = iPlat.compression;
    if (compinfo.compressed) {
        // how many blocks in width
        const int64_t blockW = (imageExtent.width + (compinfo.blockW - 1)) / compinfo.blockW;
        // how many blocks in height
        const int64_t blockH = (imageExtent.height + (compinfo.blockH - 1)) / compinfo.blockH;
        // size in bytes..
        sizeOfData = static_cast<uint64_t>(((blockW * blockH) * compinfo.bytesperblock));

        // Warn for partial copies. we do not handle those at the moment.
        if (bufferImageCopy.bufferRowLength != 0) {
            if (bufferImageCopy.bufferRowLength != blockW * compinfo.blockW) {
                PLUGIN_LOG_W("Partial copies of compressed texture data is not currently supported. "
                             "Stride must match image width (with block align). "
                             "bufferImageCopy.bufferRowLength(%d) "
                             "imageExtent.width(%d) ",
                    bufferImageCopy.bufferRowLength, imageExtent.width);
            }
        }
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
    } else {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(bufferImageCopy.bufferRowLength));
        glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, static_cast<GLint>(bufferImageCopy.bufferImageHeight));
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Make sure the align is tight.
    return { iPlat, dstGpuImage.GetDesc(), bufferImageCopy, data, size, sizeOfData, compinfo.compressed };
}

template<bool usePixelUnpackBuffer>
void FinishBlit(DeviceGLES& device_, const GpuBufferGLES& srcGpuBuffer)
{
    if constexpr (usePixelUnpackBuffer) {
        device_.BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    } else {
        srcGpuBuffer.Unmap();
    }
}

template<typename T, size_t N>
constexpr size_t Compare(const T (&a)[N], const T (&b)[N])
{
    for (size_t i = 0; i < N; i++) {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

template<typename T, size_t N>

constexpr size_t Set(T (&a)[N], const T (&b)[N])
{
    for (size_t i = 0; i < N; i++) {
        a[i] = b[i];
    }
    return true;
}

bool CompareBlendFactors(
    const GraphicsState::ColorBlendState::Attachment& a, const GraphicsState::ColorBlendState::Attachment& b)
{
    return (a.srcColorBlendFactor == b.srcColorBlendFactor) && (a.srcAlphaBlendFactor == b.srcAlphaBlendFactor) &&
           (a.dstColorBlendFactor == b.dstColorBlendFactor) && (a.dstAlphaBlendFactor == b.dstAlphaBlendFactor);
}

void SetBlendFactors(GraphicsState::ColorBlendState::Attachment& a, const GraphicsState::ColorBlendState::Attachment& b)
{
    a.srcColorBlendFactor = b.srcColorBlendFactor;
    a.srcAlphaBlendFactor = b.srcAlphaBlendFactor;
    a.dstColorBlendFactor = b.dstColorBlendFactor;
    a.dstAlphaBlendFactor = b.dstAlphaBlendFactor;
}

bool CompareBlendOps(
    const GraphicsState::ColorBlendState::Attachment& a, const GraphicsState::ColorBlendState::Attachment& b)
{
    return (a.colorBlendOp == b.colorBlendOp) && (a.alphaBlendOp == b.alphaBlendOp);
}

void SetBlendOps(GraphicsState::ColorBlendState::Attachment& a, const GraphicsState::ColorBlendState::Attachment& b)
{
    a.colorBlendOp = b.colorBlendOp;
    a.alphaBlendOp = b.alphaBlendOp;
}

bool CompareStencilOp(const GraphicsState::StencilOpState& a, const GraphicsState::StencilOpState& b)
{
    return (a.failOp == b.failOp) && (a.depthFailOp == b.depthFailOp) && (a.passOp == b.passOp);
}

void SetStencilOp(GraphicsState::StencilOpState& a, const GraphicsState::StencilOpState& b)
{
    a.failOp = b.failOp;
    a.depthFailOp = b.depthFailOp;
    a.passOp = b.passOp;
}

void SetStencilCompareOp(GraphicsState::StencilOpState& a, const GraphicsState::StencilOpState& b)
{
    a.compareOp = b.compareOp;
    a.compareMask = b.compareMask;
    a.reference = b.reference;
}

#if RENDER_VALIDATION_ENABLED
void ValidateCopyImage(const Offset3D& offset, const Size3D& extent, uint32_t mipLevel, const GpuImageDesc& imageDesc)
{
    if (mipLevel >= imageDesc.mipCount) {
        PLUGIN_LOG_W("RENDER_VALIDATION: CopyImage mipLevel must be less than image mipCount.");
    }
    if ((offset.x < 0) || (offset.y < 0) || (offset.z < 0)) {
        PLUGIN_LOG_W("RENDER_VALIDATION: CopyImage offset must not be negative.");
    }
    if (((offset.x + extent.width) > imageDesc.width) || ((offset.y + extent.height) > imageDesc.height) ||
        ((offset.z + extent.depth) > imageDesc.depth)) {
        PLUGIN_LOG_W("RENDER_VALIDATION: CopyImage offset + extent does not fit in image.");
    }
}

void ValidateCopyImage(const ImageCopy& imageCopy, const GpuImageDesc& srcImageDesc, const GpuImageDesc& dstImageDesc)
{
    ValidateCopyImage(imageCopy.srcOffset, imageCopy.extent, imageCopy.srcSubresource.mipLevel, srcImageDesc);
    ValidateCopyImage(imageCopy.dstOffset, imageCopy.extent, imageCopy.dstSubresource.mipLevel, dstImageDesc);
}
#endif

constexpr void ClampOffset(int32_t& srcOffset, int32_t& dstOffset, uint32_t& size)
{
    if (srcOffset < 0) {
        auto iSize = static_cast<int32_t>(size);
        size = static_cast<uint32_t>(iSize + srcOffset);
        dstOffset -= srcOffset;
        srcOffset = 0;
    }
}

constexpr void ClampOffset(Offset3D& srcOffset, Offset3D& dstOffset, Size3D& size)
{
    ClampOffset(srcOffset.x, dstOffset.x, size.width);
    ClampOffset(srcOffset.y, dstOffset.y, size.height);
    ClampOffset(srcOffset.z, dstOffset.z, size.depth);
}

constexpr void ClampSize(int32_t offset, uint32_t maxSize, uint32_t& size)
{
    if (size > static_cast<uint32_t>(static_cast<int32_t>(maxSize) - offset)) {
        size = static_cast<uint32_t>(static_cast<int32_t>(maxSize) - offset);
    }
}

constexpr void ClampSize(const Offset3D& offset, const GpuImageDesc& desc, Size3D& size)
{
    ClampSize(offset.x, desc.width, size.width);
    ClampSize(offset.y, desc.height, size.height);
    ClampSize(offset.z, desc.depth, size.depth);
}

// helper which covers barriers supported by Barrier and BarrierByRegion
constexpr GLbitfield CommonBarrierBits(AccessFlags accessFlags, RenderHandleType resourceType)
{
    GLbitfield barriers = 0;
    if (accessFlags & CORE_ACCESS_UNIFORM_READ_BIT) {
        barriers |= GL_UNIFORM_BARRIER_BIT;
    }
    if (accessFlags & CORE_ACCESS_SHADER_READ_BIT) {
        // shader read covers UBO, SSBO, storage image etc. use resource type to limit the options.
        if (resourceType == RenderHandleType::GPU_IMAGE) {
            barriers |= GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
        } else if (resourceType == RenderHandleType::GPU_BUFFER) {
            barriers |= GL_UNIFORM_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT;
        } else {
            barriers |= GL_UNIFORM_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT |
                        GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
        }
    }
    if (accessFlags & CORE_ACCESS_SHADER_WRITE_BIT) {
        if (resourceType == RenderHandleType::GPU_IMAGE) {
            barriers |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
        } else if (resourceType == RenderHandleType::GPU_BUFFER) {
            barriers |= GL_SHADER_STORAGE_BARRIER_BIT;
        } else {
            barriers |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT;
        }
    }
    if (accessFlags & (CORE_ACCESS_INPUT_ATTACHMENT_READ_BIT | CORE_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                          CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) {
        barriers |= GL_FRAMEBUFFER_BARRIER_BIT;
    }
    // GL_ATOMIC_COUNTER_BARRIER_BIT is not used at the moment
    return barriers;
}
} // namespace

RenderBackendGLES::RenderBackendGLES(Device& device, GpuResourceManager& gpuResourceManager)
    : RenderBackend(), device_(static_cast<DeviceGLES&>(device)), gpuResourceMgr_(gpuResourceManager)
{
#if (RENDER_PERF_ENABLED == 1)
    validGpuQueries_ = false;
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
    gpuQueryMgr_ = make_unique<GpuQueryManager>();
#if RENDER_HAS_GL_BACKEND
    if (device_.GetBackendType() == DeviceBackendType::OPENGL) {
        validGpuQueries_ = true;
    }
#endif
#if RENDER_HAS_GLES_BACKEND
    if (device_.GetBackendType() == DeviceBackendType::OPENGLES) {
        // Check if GL_EXT_disjoint_timer_query is available.
        validGpuQueries_ = device_.HasExtension("GL_EXT_disjoint_timer_query");
    }
#endif
#endif // RENDER_GPU_TIMESTAMP_QUERIES_ENABLED
#endif // RENDER_PERF_ENABLED
#if RENDER_HAS_GLES_BACKEND
    if (device_.GetBackendType() == DeviceBackendType::OPENGLES) {
        multisampledRenderToTexture_ = device_.HasExtension("GL_EXT_multisampled_render_to_texture2");
    }
#endif
    PLUGIN_ASSERT(device_.IsActive());
    PrimeCache(GraphicsState {}); // Initializes cache.
    glGenFramebuffers(1, &blitImageSourceFbo_);
    glGenFramebuffers(1, &blitImageDestinationFbo_);
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
    PLUGIN_LOG_D("fbo id >: %u", blitImageSourceFbo_);
    PLUGIN_LOG_D("fbo id >: %u", blitImageDestinationFbo_);
#endif
#if !RENDER_HAS_GLES_BACKEND
    glEnable(GL_PROGRAM_POINT_SIZE);
#endif
}

RenderBackendGLES::~RenderBackendGLES()
{
    PLUGIN_ASSERT(device_.IsActive());
    device_.DeleteFrameBuffer(blitImageSourceFbo_);
    device_.DeleteFrameBuffer(blitImageDestinationFbo_);
}

void RenderBackendGLES::Present(const RenderBackendBackBufferConfiguration& backBufferConfig)
{
    if (!backBufferConfig.swapchainData.empty()) {
        if (device_.HasSwapchain()) {
#if (RENDER_PERF_ENABLED == 1)
            commonCpuTimers_.present.Begin();
#endif
            for (const auto& swapchainData : backBufferConfig.swapchainData) {
#if (RENDER_DEV_ENABLED == 1)
                if (swapchainData.config.gpuSemaphoreHandle) {
                    // NOTE: not implemented
                    PLUGIN_LOG_E("NodeGraphBackBufferConfiguration semaphore not signaled");
                }
#endif
                const auto* swp = static_cast<const SwapchainGLES*>(device_.GetSwapchain(swapchainData.handle));
                if (swp) {
#if RENDER_GL_FLIP_Y_SWAPCHAIN
                    // Blit and flip our swapchain frame to backbuffer..
                    const auto& sdesc = swp->GetDesc();
                    if (scissorEnabled_) {
                        glDisable(GL_SCISSOR_TEST);
                        scissorEnabled_ = false;
                    }
                    const auto& platSwapchain = swp->GetPlatformData();
                    device_.BindReadFrameBuffer(platSwapchain.fbos[presentationInfo_.swapchainImageIndex]);
                    device_.BindWriteFrameBuffer(0); // FBO 0  is the surface bound to current context..
                    glBlitFramebuffer(0, 0, (GLint)sdesc.width, (GLint)sdesc.height, 0, (GLint)sdesc.height,
                        (GLint)sdesc.width, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
                    device_.BindReadFrameBuffer(0);
#endif
                    device_.SwapBuffers(*swp);
                }
            }
#if (RENDER_PERF_ENABLED == 1)
            commonCpuTimers_.present.End();
#endif
        }
    }
}

void RenderBackendGLES::ResetState()
{
    boundProgram_ = {};
    boundIndexBuffer_ = {};
    vertexAttribBinds_ = 0;
    renderingToDefaultFbo_ = false;
    boundComputePipeline_ = nullptr;
    boundGraphicsPipeline_ = nullptr;
    currentPsoHandle_ = {};
    renderArea_ = {};
    activeRenderPass_ = {};
    currentSubPass_ = 0;
    currentFrameBuffer_ = nullptr;
    inRenderpass_ = 0;
    descriptorUpdate_ = false;
    vertexBufferUpdate_ = false;
    indexBufferUpdate_ = false;
}

void RenderBackendGLES::ResetBindings()
{
    boundComputePipeline_ = nullptr;
    boundGraphicsPipeline_ = nullptr;
    currentPsoHandle_ = {};
}

void RenderBackendGLES::Render(
    RenderCommandFrameData& renderCommandFrameData, const RenderBackendBackBufferConfiguration& backBufferConfig)
{
    // NOTE: all command lists are validated before entering here
    PLUGIN_ASSERT(device_.IsActive());
#if (RENDER_PERF_ENABLED == 1)
    commonCpuTimers_.full.Begin();
    commonCpuTimers_.acquire.Begin();
#endif
    presentationInfo_ = {};

    if (device_.HasSwapchain() && (!backBufferConfig.swapchainData.empty())) {
        for (const auto& swapData : backBufferConfig.swapchainData) {
            if (const auto* swp = static_cast<const SwapchainGLES*>(device_.GetSwapchain(swapData.handle))) {
                presentationInfo_.swapchainImageIndex = swp->GetNextImage();
                const Device::SwapchainData swapchainData = device_.GetSwapchainData(swapData.handle);
                if (presentationInfo_.swapchainImageIndex < swapchainData.imageViewCount) {
                    // remap image to backbuffer
                    const RenderHandle currentSwapchainHandle =
                        swapchainData.imageViews[presentationInfo_.swapchainImageIndex];
                    // special swapchain remapping
                    gpuResourceMgr_.RenderBackendImmediateRemapGpuImageHandle(swapData.handle, currentSwapchainHandle);
                }
            }
        }
    }
#if (RENDER_PERF_ENABLED == 1)
    commonCpuTimers_.acquire.End();

    StartFrameTimers(renderCommandFrameData);
    commonCpuTimers_.execute.Begin();
#endif
    // global begin backend frame
    auto& descriptorSetMgr = (DescriptorSetManagerGles&)device_.GetDescriptorSetManager();
    descriptorSetMgr.BeginBackendFrame();

    // Reset bindings.
    ResetState();

    // Update global descset if needed
    UpdateGlobalDescriptorSets();

    for (const auto& ref : renderCommandFrameData.renderCommandContexts) {
        // Reset bindings between command lists..
        ResetBindings();
        RenderSingleCommandList(ref);
    }
#if (RENDER_PERF_ENABLED == 1)
    commonCpuTimers_.execute.End();
#endif
    RenderProcessEndCommandLists(renderCommandFrameData, backBufferConfig);
#if (RENDER_PERF_ENABLED == 1)
    commonCpuTimers_.full.End();
    EndFrameTimers();
#endif
}

void RenderBackendGLES::RenderProcessEndCommandLists(
    RenderCommandFrameData& renderCommandFrameData, const RenderBackendBackBufferConfiguration& backBufferConfig)
{
    if (auto* frameSync = static_cast<RenderFrameSyncGLES*>(renderCommandFrameData.renderFrameSync); frameSync) {
        frameSync->GetFrameFence();
    }
    // signal external GPU fences
    if (renderCommandFrameData.renderFrameUtil && renderCommandFrameData.renderFrameUtil->HasGpuSignals()) {
        auto externalSignals = renderCommandFrameData.renderFrameUtil->GetFrameGpuSignalData();
        const auto externalSemaphores = renderCommandFrameData.renderFrameUtil->GetGpuSemaphores();
        PLUGIN_ASSERT(externalSignals.size() == externalSemaphores.size());
        if (externalSignals.size() == externalSemaphores.size()) {
            for (size_t sigIdx = 0; sigIdx < externalSignals.size(); ++sigIdx) {
                // needs to be false
                if (!externalSignals[sigIdx].signaled && (externalSemaphores[sigIdx])) {
                    if (const auto* gs = (const GpuSemaphoreGles*)externalSemaphores[sigIdx].get(); gs) {
                        auto& plat = const_cast<GpuSemaphorePlatformDataGles&>(gs->GetPlatformData());
                        // NOTE: currently could create only one GPU sync
                        GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
                        plat.sync = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(sync));
                        externalSignals[sigIdx].gpuSignalResourceHandle = plat.sync;
                        externalSignals[sigIdx].signaled = true;

                        // NOTE: client is expected to add code for the wait with glClientWaitSync(sync, X, 0)
                    }
                }
            }
        }
    }
}

void RenderBackendGLES::RenderCommandUndefined(const RenderCommandWithType& renderCommand)
{
    PLUGIN_ASSERT_MSG(false, "non-valid render command");
}

void RenderBackendGLES::RenderSingleCommandList(const RenderCommandContext& renderCommandCtx)
{
    // these are validated in render graph
    managers_ = { renderCommandCtx.nodeContextPsoMgr, renderCommandCtx.nodeContextPoolMgr,
        renderCommandCtx.nodeContextDescriptorSetMgr, renderCommandCtx.renderBarrierList };

    managers_.poolMgr->BeginBackendFrame();
    managers_.psoMgr->BeginBackendFrame();

    // update cmd list context descriptor sets
    UpdateCommandListDescriptorSets(*renderCommandCtx.renderCommandList, *renderCommandCtx.nodeContextDescriptorSetMgr);

#if (RENDER_PERF_ENABLED == 1) || (RENDER_DEBUG_MARKERS_ENABLED == 1)
    const auto& debugName = renderCommandCtx.debugName;
#endif
#if (RENDER_PERF_ENABLED == 1)
    perfCounters_ = {};
    PLUGIN_ASSERT(timers_.count(debugName) == 1);
    PerfDataSet& perfDataSet = timers_[debugName];
    perfDataSet.cpuTimer.Begin();
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
    if (validGpuQueries_) {
#ifdef GL_GPU_DISJOINT_EXT
        /* Clear disjoint error */
        GLint disjointOccurred = 0;
        glGetIntegerv(GL_GPU_DISJOINT_EXT, &disjointOccurred);
#endif
        GpuQuery* gpuQuery = gpuQueryMgr_->Get(perfDataSet.gpuHandle);
        PLUGIN_ASSERT(gpuQuery);

        const auto& platData = static_cast<const GpuQueryPlatformDataGLES&>(gpuQuery->GetPlatformData());
        PLUGIN_ASSERT(platData.queryObject);
        glBeginQuery(GL_TIME_ELAPSED_EXT, platData.queryObject);
    }
#endif // RENDER_GPU_TIMESTAMP_QUERIES_ENABLED
#endif // RENDER_PERF_ENABLED
#if (RENDER_DEBUG_MARKERS_ENABLED == 1)
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, (const GLchar*)debugName.data());
#endif
    commandListValid_ = true;
    for (const auto& ref : renderCommandCtx.renderCommandList->GetRenderCommands()) {
        PLUGIN_ASSERT(ref.rc);
        if (commandListValid_) {
#if RENDER_DEBUG_COMMAND_MARKERS_ENABLED
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, (const GLchar*)COMMAND_NAMES[(uint32_t)ref.type]);
#endif
            (this->*(COMMAND_HANDLERS[static_cast<uint32_t>(ref.type)]))(ref);
#if RENDER_DEBUG_COMMAND_MARKERS_ENABLED
            glPopDebugGroup();
#endif
        }
    }
#if (RENDER_DEBUG_MARKERS_ENABLED == 1)
    glPopDebugGroup();
#endif
#if (RENDER_PERF_ENABLED == 1)
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
    if (validGpuQueries_) {
        glEndQuery(GL_TIME_ELAPSED_EXT);
    }
#endif // RENDER_GPU_TIMESTAMP_QUERIES_ENABLED
    perfDataSet.cpuTimer.End();
    CopyPerfTimeStamp(debugName, perfDataSet);
#endif // RENDER_PERF_ENABLED
}

void RenderBackendGLES::RenderCommandBindPipeline(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::BIND_PIPELINE);
    const auto& renderCmd = *static_cast<const struct RenderCommandBindPipeline*>(ref.rc);
    boundProgram_ = {};
    if (renderCmd.pipelineBindPoint == PipelineBindPoint::CORE_PIPELINE_BIND_POINT_COMPUTE) {
        PLUGIN_ASSERT(currentFrameBuffer_ == nullptr);
        BindComputePipeline(renderCmd);
    } else if (renderCmd.pipelineBindPoint == PipelineBindPoint::CORE_PIPELINE_BIND_POINT_GRAPHICS) {
        BindGraphicsPipeline(renderCmd);
    }
    descriptorUpdate_ = true;
    currentPsoHandle_ = renderCmd.psoHandle;
}

void RenderBackendGLES::BindComputePipeline(const struct RenderCommandBindPipeline& renderCmd)
{
    const auto* pso = static_cast<const ComputePipelineStateObjectGLES*>(
        managers_.psoMgr->GetComputePso(renderCmd.psoHandle, nullptr));
    boundComputePipeline_ = pso;
    boundGraphicsPipeline_ = nullptr;
    boundComputeProgram_ = nullptr;
    boundShaderProgram_ = nullptr;
    if (!boundComputePipeline_) {
        return;
    }

    // Push constants and "fliplocation" uniform (ie. uniform state) should be only updated if changed...
    const auto& pipelineData =
        static_cast<const PipelineStateObjectPlatformDataGL&>(boundComputePipeline_->GetPlatformData());
    if (!pipelineData.computeShader) {
        return;
    }
    boundComputeProgram_ = pipelineData.computeShader;
    const auto& sd = static_cast<const GpuComputeProgramPlatformDataGL&>(pipelineData.computeShader->GetPlatformData());
    const uint32_t program = sd.program;
#if (RENDER_PERF_ENABLED == 1)
    if (device_.BoundProgram() != program) {
        ++perfCounters_.bindProgram;
    }
#endif
    device_.UseProgram(program);

    if (sd.flipLocation != Gles::INVALID_LOCATION) {
        const float flip = (renderingToDefaultFbo_) ? (-1.f) : (1.f);
        glProgramUniform1fv(program, sd.flipLocation, 1, &flip);
    }
}

void RenderBackendGLES::BindGraphicsPipeline(const struct RenderCommandBindPipeline& renderCmd)
{
    const auto* pso = static_cast<const GraphicsPipelineStateObjectGLES*>(
        managers_.psoMgr->GetGraphicsPso(renderCmd.psoHandle, activeRenderPass_.renderPassDesc,
            activeRenderPass_.subpasses, activeRenderPass_.subpassStartIndex, 0, nullptr, nullptr));
    boundComputePipeline_ = nullptr;
    boundGraphicsPipeline_ = pso;
    boundComputeProgram_ = nullptr;
    boundShaderProgram_ = nullptr;
    if (!boundGraphicsPipeline_ || !currentFrameBuffer_) {
        return;
    }

    const auto& pipelineData = static_cast<const PipelineStateObjectPlatformDataGL&>(pso->GetPlatformData());
    dynamicStateFlags_ = pipelineData.dynamicStateFlags;
    DoGraphicsState(pipelineData.graphicsState);
    // NOTE: Deprecate (default viewport/scissor should be set from default targets at some point)
    if (!IS_BIT(dynamicStateFlags_, CORE_DYNAMIC_STATE_VIEWPORT)) {
        SetViewport(ViewportDesc { 0.0f, 0.0f, static_cast<float>(renderArea_.extentWidth),
            static_cast<float>(renderArea_.extentHeight), 0.0f, 1.0f });
    }
    if (!IS_BIT(dynamicStateFlags_, CORE_DYNAMIC_STATE_SCISSOR)) {
        SetScissor(ScissorDesc { 0, 0, renderArea_.extentWidth, renderArea_.extentHeight });
    }
    const GpuShaderProgramGLES* shader = pipelineData.graphicsShader;
    if (!shader) {
        return;
    }
    boundShaderProgram_ = shader;
    const auto& sd = static_cast<const GpuShaderProgramPlatformDataGL&>(shader->GetPlatformData());
    // Push constants and "fliplocation" uniform (ie. uniform state) should be only updated if changed...
    if (!scissorEnabled_) {
        scissorEnabled_ = true;
        glEnable(GL_SCISSOR_TEST); // Always enabled
    }
    uint32_t program = sd.program;
#if (RENDER_PERF_ENABLED == 1)
    if (device_.BoundProgram() != program) {
        ++perfCounters_.bindProgram;
    }
#endif
    device_.UseProgram(program);
    device_.BindVertexArray(pipelineData.vao);
    vertexBufferUpdate_ = true;
    indexBufferUpdate_ = true;

    if (sd.flipLocation != Gles::INVALID_LOCATION) {
        const float flip = (renderingToDefaultFbo_) ? (-1.f) : (1.f);
        glProgramUniform1fv(program, sd.flipLocation, 1, &flip);
    }
}

void RenderBackendGLES::RenderCommandDraw(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::DRAW);
    const auto& renderCmd = *static_cast<struct RenderCommandDraw*>(ref.rc);
    if (!boundGraphicsPipeline_) {
        return;
    }
    PLUGIN_ASSERT(boundComputePipeline_ == nullptr);
    BindResources();
    if (vertexBufferUpdate_ && boundShaderProgram_) {
        vertexBufferUpdate_ = false;
        const auto& pipelineData =
            static_cast<const PipelineStateObjectPlatformDataGL&>(boundGraphicsPipeline_->GetPlatformData());
        const auto& sd = static_cast<const GpuShaderProgramPlatformDataGL&>(boundShaderProgram_->GetPlatformData());
        BindVertexInputs(pipelineData.vertexInputDeclaration, array_view<const int32_t>(sd.inputs, countof(sd.inputs)));
    }
    if (indexBufferUpdate_) {
        indexBufferUpdate_ = false;
        device_.BindElementBuffer(boundIndexBuffer_.id);
    }
    const auto type = GetPrimFromTopology(topology_);
    const auto instanceCount = static_cast<GLsizei>(renderCmd.instanceCount);
    // firstInstance is not supported yet, need to set the SPIRV_Cross generated uniform
    // "SPIRV_Cross_BaseInstance" to renderCmd.firstInstance;
    if (renderCmd.indexCount) {
        uintptr_t offsetp = boundIndexBuffer_.offset;
        GLenum indexType = GL_UNSIGNED_SHORT;
        switch (boundIndexBuffer_.type) {
            case CORE_INDEX_TYPE_UINT16:
                offsetp += renderCmd.firstIndex * sizeof(uint16_t);
                indexType = GL_UNSIGNED_SHORT;
                break;
            case CORE_INDEX_TYPE_UINT32:
                offsetp += renderCmd.firstIndex * sizeof(uint32_t);
                indexType = GL_UNSIGNED_INT;
                break;
            default:
                PLUGIN_ASSERT_MSG(false, "Invalid indexbuffer type");
                break;
        }
        const auto indexCount = static_cast<const GLsizei>(renderCmd.indexCount);
        const auto vertexOffset = static_cast<const GLsizei>(renderCmd.vertexOffset);
        const void* offset = reinterpret_cast<const void*>(offsetp);
        if (renderCmd.instanceCount > 1) {
            if (vertexOffset) {
                glDrawElementsInstancedBaseVertex(type, indexCount, indexType, offset, instanceCount, vertexOffset);
            } else {
                glDrawElementsInstanced(type, indexCount, indexType, offset, instanceCount);
            }
        } else {
            if (vertexOffset) {
                glDrawElementsBaseVertex(type, indexCount, indexType, offset, vertexOffset);
            } else {
                glDrawElements(type, indexCount, indexType, offset);
            }
        }
#if (RENDER_PERF_ENABLED == 1)
        ++perfCounters_.drawCount;
        perfCounters_.instanceCount += renderCmd.instanceCount;
        perfCounters_.triangleCount += renderCmd.indexCount * renderCmd.instanceCount;
#endif
    } else {
        const auto firstVertex = static_cast<const GLsizei>(renderCmd.firstVertex);
        const auto vertexCount = static_cast<const GLsizei>(renderCmd.vertexCount);
        if (renderCmd.instanceCount > 1) {
            glDrawArraysInstanced(type, firstVertex, vertexCount, instanceCount);
        } else {
            glDrawArrays(type, firstVertex, vertexCount);
        }
#if (RENDER_PERF_ENABLED == 1)
        ++perfCounters_.drawCount;
        perfCounters_.instanceCount += renderCmd.instanceCount;
        perfCounters_.triangleCount += (renderCmd.vertexCount * 3) * renderCmd.instanceCount; // 3: vertex dimension
#endif
    }
}

void RenderBackendGLES::RenderCommandDrawIndirect(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::DRAW_INDIRECT);
    const auto& renderCmd = *static_cast<const struct RenderCommandDrawIndirect*>(ref.rc);
    if (!boundGraphicsPipeline_) {
        return;
    }
    PLUGIN_ASSERT(boundComputePipeline_ == nullptr);
    if (vertexBufferUpdate_ && boundShaderProgram_) {
        vertexBufferUpdate_ = false;
        const auto& pipelineData =
            static_cast<const PipelineStateObjectPlatformDataGL&>(boundGraphicsPipeline_->GetPlatformData());
        const auto& sd = static_cast<const GpuShaderProgramPlatformDataGL&>(boundShaderProgram_->GetPlatformData());
        BindVertexInputs(pipelineData.vertexInputDeclaration, array_view<const int32_t>(sd.inputs, countof(sd.inputs)));
    }
    if (indexBufferUpdate_) {
        indexBufferUpdate_ = false;
        device_.BindElementBuffer(boundIndexBuffer_.id);
    }
    BindResources();
    if (const GpuBufferGLES* gpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferGLES>(renderCmd.argsHandle); gpuBuffer) {
        const auto& plat = gpuBuffer->GetPlatformData();
        device_.BindBuffer(GL_DRAW_INDIRECT_BUFFER, plat.buffer);
        const auto type = GetPrimFromTopology(topology_);
        auto offset = static_cast<GLintptr>(renderCmd.offset);
        if (renderCmd.drawType == DrawType::DRAW_INDEXED_INDIRECT) {
            GLenum indexType = GL_UNSIGNED_SHORT;
            switch (boundIndexBuffer_.type) {
                case CORE_INDEX_TYPE_UINT16:
                    indexType = GL_UNSIGNED_SHORT;
                    break;
                case CORE_INDEX_TYPE_UINT32:
                    indexType = GL_UNSIGNED_INT;
                    break;
                default:
                    PLUGIN_ASSERT_MSG(false, "Invalid indexbuffer type");
                    break;
            }
            for (uint32_t i = 0; i < renderCmd.drawCount; ++i) {
                glDrawElementsIndirect(type, indexType, reinterpret_cast<const void*>(offset));
                offset += renderCmd.stride;
            }
        } else {
            for (uint32_t i = 0; i < renderCmd.drawCount; ++i) {
                glDrawArraysIndirect(type, reinterpret_cast<const void*>(offset));
                offset += renderCmd.stride;
            }
        }
#if (RENDER_PERF_ENABLED == 1)
        perfCounters_.drawIndirectCount += renderCmd.drawCount;
#endif
    }
}

void RenderBackendGLES::RenderCommandDispatch(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::DISPATCH);
    const auto& renderCmd = *static_cast<const struct RenderCommandDispatch*>(ref.rc);
    if (!boundComputePipeline_) {
        return;
    }
    PLUGIN_ASSERT(boundGraphicsPipeline_ == nullptr);
    BindResources();
    glDispatchCompute(renderCmd.groupCountX, renderCmd.groupCountY, renderCmd.groupCountZ);
#if (RENDER_PERF_ENABLED == 1)
    ++perfCounters_.dispatchCount;
#endif
}

void RenderBackendGLES::RenderCommandDispatchIndirect(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::DISPATCH_INDIRECT);
    const auto& renderCmd = *static_cast<const struct RenderCommandDispatchIndirect*>(ref.rc);
    if (!boundComputePipeline_) {
        return;
    }
    PLUGIN_ASSERT(boundGraphicsPipeline_ == nullptr);
    BindResources();
    if (const GpuBufferGLES* gpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferGLES>(renderCmd.argsHandle); gpuBuffer) {
        const auto& plat = gpuBuffer->GetPlatformData();
        device_.BindBuffer(GL_DISPATCH_INDIRECT_BUFFER, plat.buffer);
        glDispatchComputeIndirect(static_cast<GLintptr>(renderCmd.offset));
#if (RENDER_PERF_ENABLED == 1)
        ++perfCounters_.dispatchIndirectCount;
#endif
    }
}

void RenderBackendGLES::ClearScissorInit(const RenderPassDesc::RenderArea& aArea)
{
    resetScissor_ = false;   // need to reset scissor state after clear?
    clearScissorSet_ = true; // need to setup clear scissors before clear?
    clearScissor_ = aArea;   // area to be cleared
    if (scissorPrimed_) {    // have scissors been set yet?
        if ((clearScissor_.offsetX == scissorBox_.offsetX) && (clearScissor_.offsetY == scissorBox_.offsetY) &&
            (clearScissor_.extentWidth == scissorBox_.extentWidth) &&
            (clearScissor_.extentHeight == scissorBox_.extentHeight)) {
            // Current scissors match clearscissor area, so no need to set it again.
            clearScissorSet_ = false;
        }
    }
}

void RenderBackendGLES::ClearScissorSet()
{
    if (clearScissorSet_) {       // do we need to set clear scissors.
        clearScissorSet_ = false; // clear scissors have been set now.
        resetScissor_ = true;     // we are modifying scissors, so remember to reset them afterwards.
        glScissor(static_cast<GLint>(clearScissor_.offsetX), static_cast<GLint>(clearScissor_.offsetY),
            static_cast<GLsizei>(clearScissor_.extentWidth), static_cast<GLsizei>(clearScissor_.extentHeight));
    }
}

void RenderBackendGLES::ClearScissorReset()
{
    if (resetScissor_) { // need to reset correct scissors?
        if (!scissorPrimed_) {
            // scissors have not been set yet, so use clearbox as current cache state (and don't change scissor
            // setting)
            scissorPrimed_ = true;
            glScissor(static_cast<GLint>(clearScissor_.offsetX), static_cast<GLint>(clearScissor_.offsetY),
                static_cast<GLsizei>(clearScissor_.extentWidth), static_cast<GLsizei>(clearScissor_.extentHeight));
            scissorBox_.offsetX = clearScissor_.offsetX;
            scissorBox_.offsetY = clearScissor_.offsetY;
            scissorBox_.extentHeight = clearScissor_.extentHeight;
            scissorBox_.extentWidth = clearScissor_.extentWidth;
        } else {
            // Restore scissor box to cached state. (update scissors when needed, since clearBox != scissorBox)
            glScissor(static_cast<GLint>(scissorBox_.offsetX), static_cast<GLint>(scissorBox_.offsetY),
                static_cast<GLsizei>(scissorBox_.extentWidth), static_cast<GLsizei>(scissorBox_.extentHeight));
        }
    }
}

void RenderBackendGLES::HandleColorAttachments(const array_view<const RenderPassDesc::AttachmentDesc*> colorAttachments)
{
    constexpr ColorComponentFlags clearAll = CORE_COLOR_COMPONENT_R_BIT | CORE_COLOR_COMPONENT_G_BIT |
                                             CORE_COLOR_COMPONENT_B_BIT | CORE_COLOR_COMPONENT_A_BIT;
    const auto& cBlend = cacheState_.colorBlendState;
    for (uint32_t idx = 0; idx < colorAttachments.size(); ++idx) {
        if (colorAttachments[idx] == nullptr) {
            continue;
        }
        const auto& ref = *(colorAttachments[idx]);
        if (ref.loadOp == AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR) {
            const auto& cBlendState = cBlend.colorAttachments[idx];
            if (clearAll != cBlendState.colorWriteMask) {
                glColorMaski(idx, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            }
            ClearScissorSet();
            // glClearBufferfv only for float formats?
            // glClearBufferiv & glClearbufferuv only for integer formats?
            glClearBufferfv(GL_COLOR, static_cast<GLint>(idx), ref.clearValue.color.float32);
            if (clearAll != cBlendState.colorWriteMask) {
                // NOTE: We might not need to restore here.. (we need to peek in to the command list to find out...)
                glColorMaski(idx, IS_BIT_GL(cBlendState.colorWriteMask, CORE_COLOR_COMPONENT_R_BIT),
                    IS_BIT_GL(cBlendState.colorWriteMask, CORE_COLOR_COMPONENT_G_BIT),
                    IS_BIT_GL(cBlendState.colorWriteMask, CORE_COLOR_COMPONENT_B_BIT),
                    IS_BIT_GL(cBlendState.colorWriteMask, CORE_COLOR_COMPONENT_A_BIT));
            }
        }
    }
}

void RenderBackendGLES::HandleDepthAttachment(const RenderPassDesc::AttachmentDesc& depthAttachment)
{
    const GLuint allBits = 0xFFFFFFFFu;
    const auto& ref = depthAttachment;
    const bool clearDepth = (ref.loadOp == AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR);
    const bool clearStencil = (ref.stencilLoadOp == AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR);
    // Change state if needed.
    if ((clearDepth) && (!cacheState_.depthStencilState.enableDepthWrite)) {
        glDepthMask(GL_TRUE);
    }
    if (clearStencil) {
        if (cacheState_.depthStencilState.frontStencilOpState.writeMask != allBits) {
            glStencilMaskSeparate(GL_FRONT, allBits);
        }
        if (cacheState_.depthStencilState.backStencilOpState.writeMask != allBits) {
            glStencilMaskSeparate(GL_BACK, allBits);
        }
    }
    if (clearDepth || clearStencil) {
        // Set the scissors for clear..
        ClearScissorSet();
    }
    // Do clears.
    if (clearDepth && clearStencil) {
        glClearBufferfi(GL_DEPTH_STENCIL, 0, ref.clearValue.depthStencil.depth,
            static_cast<GLint>(ref.clearValue.depthStencil.stencil));
    } else if (clearDepth) {
        glClearBufferfv(GL_DEPTH, 0, &ref.clearValue.depthStencil.depth);
    } else if (clearStencil) {
        glClearBufferiv(GL_STENCIL, 0, reinterpret_cast<const GLint*>(&ref.clearValue.depthStencil.stencil));
    }

    // Restore cached state, if we touched the state.
    if ((clearDepth) && (!cacheState_.depthStencilState.enableDepthWrite)) {
        // NOTE: We might not need to restore here.. (we need to peek in to the command list to find out...)
        glDepthMask(GL_FALSE);
    }
    if (clearStencil) {
        // NOTE: We might not need to restore here.. (we need to peek in to the command list to find out...)
        if (cacheState_.depthStencilState.frontStencilOpState.writeMask != allBits) {
            glStencilMaskSeparate(GL_FRONT, cacheState_.depthStencilState.frontStencilOpState.writeMask);
        }
        if (cacheState_.depthStencilState.backStencilOpState.writeMask != allBits) {
            glStencilMaskSeparate(GL_BACK, cacheState_.depthStencilState.backStencilOpState.writeMask);
        }
    }
}

void RenderBackendGLES::DoSubPass(uint32_t subPass)
{
    if (currentFrameBuffer_ == nullptr) {
        // Completely invalid state in backend.
        return;
    }
    const auto& rpd = activeRenderPass_.renderPassDesc;
    const auto& sb = activeRenderPass_.subpasses[subPass];

    // If there's no FBO activate the with swapchain handle so that drawing happens to the correct surface.
    if (!currentFrameBuffer_->fbos[subPass].fbo && (sb.colorAttachmentCount == 1U)) {
        auto color = rpd.attachmentHandles[sb.colorAttachmentIndices[0]];
        device_.Activate(color);
    }
    device_.BindFrameBuffer(currentFrameBuffer_->fbos[subPass].fbo);
    ClearScissorInit(renderArea_);
    if (cacheState_.rasterizationState.enableRasterizerDiscard) { // Rasterizer discard affects glClearBuffer*
        SetState(GL_RASTERIZER_DISCARD, GL_FALSE);
    }
    {
        // NOTE: clear is not yet optimal. depth, stencil and color should be cleared using ONE glClear call if
        // possible. (ie. all buffers at once)
        renderingToDefaultFbo_ = false;
        if (sb.colorAttachmentCount > 0) {
            // collect color attachment infos..
            const RenderPassDesc::AttachmentDesc*
                colorAttachments[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT];
            for (uint32_t ci = 0; ci < sb.colorAttachmentCount; ci++) {
                uint32_t index = sb.colorAttachmentIndices[ci];
                if (resolveToBackbuffer_[index]) {
                    // NOTE: this could fail with multiple color attachments....
                    renderingToDefaultFbo_ = true;
                }
                if (!attachmentCleared_[index]) {
                    attachmentCleared_[index] = true;
                    colorAttachments[ci] = &rpd.attachments[index];
                } else {
                    colorAttachments[ci] = nullptr;
                }
            }
            HandleColorAttachments(array_view(colorAttachments, sb.colorAttachmentCount));
        }
        if (sb.depthAttachmentCount) {
            if (!attachmentCleared_[sb.depthAttachmentIndex]) {
                attachmentCleared_[sb.depthAttachmentIndex] = true;
                HandleDepthAttachment(rpd.attachments[sb.depthAttachmentIndex]);
            }
        }
    }
    if (cacheState_.rasterizationState.enableRasterizerDiscard) { // Rasterizer discard affects glClearBuffer*
        // NOTE: We might not need to restore here.. (we need to peek in to the command list to find out...)
        SetState(GL_RASTERIZER_DISCARD, GL_TRUE);
    }
    ClearScissorReset();

    if (viewportPending_) {
        viewportPending_ = false;
        // Handle top-left / bottom-left origin conversion
        auto y = static_cast<GLint>(viewport_.y);
        const auto h = static_cast<GLsizei>(viewport_.height);
        if (renderingToDefaultFbo_) {
            const auto fh = static_cast<GLint>(currentFrameBuffer_->height);
            y = fh - (y + h);
        }
        glViewport(static_cast<GLint>(viewport_.x), y, static_cast<GLsizei>(viewport_.width), h);
    }
}

void RenderBackendGLES::ScanPasses(const RenderPassDesc& rpd)
{
    for (uint32_t sub = 0; sub < rpd.subpassCount; sub++) {
        const auto& currentSubPass = activeRenderPass_.subpasses[sub];
        for (uint32_t ci = 0; ci < currentSubPass.resolveAttachmentCount; ci++) {
            const uint32_t resolveTo = currentSubPass.resolveAttachmentIndices[ci];
            if (!attachmentImage_[resolveTo]) {
                PLUGIN_LOG_ONCE_E(to_string(resolveTo), "Missing attachment %u", resolveTo);
                continue;
            }
            if (attachmentFirstUse_[resolveTo] == 0xFFFFFFFF) {
                attachmentFirstUse_[resolveTo] = sub;
            }
            attachmentLastUse_[resolveTo] = sub;
            const auto& p = static_cast<const GpuImagePlatformDataGL&>(attachmentImage_[resolveTo]->GetPlatformData());
            if ((p.image == 0) && (p.renderBuffer == 0)) {
                // mark the "resolveFrom" (ie. the colorattachment) as "backbuffer-like", since we resolve to
                // backbuffer...
                uint32_t resolveFrom = currentSubPass.colorAttachmentIndices[ci];
                resolveToBackbuffer_[resolveFrom] = true;
            }
        }
        for (uint32_t ci = 0; ci < currentSubPass.inputAttachmentCount; ci++) {
            uint32_t index = currentSubPass.inputAttachmentIndices[ci];
            if (attachmentFirstUse_[index] == 0xFFFFFFFF) {
                attachmentFirstUse_[index] = sub;
            }
            attachmentLastUse_[index] = sub;
        }
        for (uint32_t ci = 0; ci < currentSubPass.colorAttachmentCount; ci++) {
            uint32_t index = currentSubPass.colorAttachmentIndices[ci];
            if (attachmentFirstUse_[index] == 0xFFFFFFFF) {
                attachmentFirstUse_[index] = sub;
            }
            attachmentLastUse_[index] = sub;
            if (attachmentImage_[index]) {
                const auto& p = static_cast<const GpuImagePlatformDataGL&>(attachmentImage_[index]->GetPlatformData());
                if ((p.image == 0) && (p.renderBuffer == 0)) {
                    resolveToBackbuffer_[index] = true;
                }
            }
        }
        if (currentSubPass.depthAttachmentCount > 0) {
            uint32_t index = currentSubPass.depthAttachmentIndex;
            if (attachmentFirstUse_[index] == 0xFFFFFFFF) {
                attachmentFirstUse_[index] = sub;
            }
            attachmentLastUse_[index] = sub;
        }
    }
}

void RenderBackendGLES::RenderCommandBeginRenderPass(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::BEGIN_RENDER_PASS);
    const auto& renderCmd = *static_cast<const struct RenderCommandBeginRenderPass*>(ref.rc);
    switch (renderCmd.beginType) {
        case RenderPassBeginType::RENDER_PASS_BEGIN: {
            ++inRenderpass_;
            PLUGIN_ASSERT_MSG(inRenderpass_ == 1, "RenderBackendGLES beginrenderpass mInRenderpass %u", inRenderpass_);
            activeRenderPass_ = renderCmd; // Store this because we need it later (in NextRenderPass)

            const auto& rpd = activeRenderPass_.renderPassDesc;
            renderArea_ = rpd.renderArea; // can subpasses have different render areas?
            auto& cpm = *(static_cast<NodeContextPoolManagerGLES*>(managers_.poolMgr));
            if (multisampledRenderToTexture_) {
                cpm.FilterRenderPass(activeRenderPass_);
            }
            currentFrameBuffer_ = cpm.GetFramebuffer(cpm.GetFramebufferHandle(activeRenderPass_));
            if (!currentFrameBuffer_) {
                // Completely invalid state in backend.
                commandListValid_ = false;
                --inRenderpass_;
                return;
            }
            PLUGIN_ASSERT_MSG(
                activeRenderPass_.subpassStartIndex == 0, "activeRenderPass_.subpassStartIndex != 0 not handled!");
            currentSubPass_ = 0;
            // find first and last use, clear clearflags. (this could be cached in the lowlewel classes)
            for (uint32_t i = 0; i < rpd.attachmentCount; i++) {
                attachmentCleared_[i] = false;
                attachmentFirstUse_[i] = 0xFFFFFFFF;
                attachmentLastUse_[i] = 0;
                resolveToBackbuffer_[i] = false;
                attachmentImage_[i] =
                    static_cast<const GpuImageGLES*>(gpuResourceMgr_.GetImage(rpd.attachmentHandles[i]));
            }
            ScanPasses(rpd);
            DoSubPass(0);
#if (RENDER_PERF_ENABLED == 1)
            ++perfCounters_.renderPassCount;
#endif
        } break;

        case RenderPassBeginType::RENDER_PASS_SUBPASS_BEGIN: {
            currentSubPass_ = renderCmd.subpassStartIndex;
            PLUGIN_ASSERT(currentSubPass_ < activeRenderPass_.renderPassDesc.subpassCount);
            if (!currentFrameBuffer_) {
                // Completely invalid state in backend.
                commandListValid_ = false;
                return;
            }
            DoSubPass(activeRenderPass_.subpassStartIndex);
        } break;

        default:
            break;
    }
}

void RenderBackendGLES::RenderCommandNextSubpass(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::NEXT_SUBPASS);
    const auto& renderCmd = *static_cast<const struct RenderCommandNextSubpass*>(ref.rc);
    PLUGIN_UNUSED(renderCmd);
    PLUGIN_ASSERT(renderCmd.subpassContents == SubpassContents::CORE_SUBPASS_CONTENTS_INLINE);
    ++currentSubPass_;
    PLUGIN_ASSERT(currentSubPass_ < activeRenderPass_.renderPassDesc.subpassCount);
    DoSubPass(currentSubPass_);
}

int32_t RenderBackendGLES::InvalidateDepthStencil(
    array_view<uint32_t> invalidateAttachment, const RenderPassDesc& rpd, const RenderPassSubpassDesc& currentSubPass)
{
    int32_t depthCount = 0;
    if (currentSubPass.depthAttachmentCount == 0) {
        return depthCount; // early out
    }
    const uint32_t index = currentSubPass.depthAttachmentIndex;
    if (attachmentLastUse_[index] != currentSubPass_) {
        return depthCount; // early out
    }
    // is last use of the attachment
    const auto& image = attachmentImage_[index];
    const auto& dplat = static_cast<const GpuImagePlatformDataGL&>(image->GetPlatformData());
    // NOTE: we expect the depth to be in FBO in this case even if there would be a depth target in render pass
    if ((dplat.image || dplat.renderBuffer) && (!renderingToDefaultFbo_)) {
        bool depth = false;
        bool stencil = false;
        if (rpd.attachments[index].storeOp == CORE_ATTACHMENT_STORE_OP_DONT_CARE) {
            if ((dplat.format == GL_DEPTH_COMPONENT) || (dplat.format == GL_DEPTH_STENCIL)) {
                depth = true;
            }
        }
        if (rpd.attachments[index].stencilStoreOp == CORE_ATTACHMENT_STORE_OP_DONT_CARE) {
            if ((dplat.format == GL_STENCIL) || (dplat.format == GL_DEPTH_STENCIL)) {
                stencil = true;
            }
        }
        if (depth && stencil) {
            invalidateAttachment[0] = GL_DEPTH_STENCIL_ATTACHMENT;
            depthCount++;
        } else if (stencil) {
            invalidateAttachment[0] = GL_STENCIL_ATTACHMENT;
            depthCount++;
        } else if (depth) {
            invalidateAttachment[0] = GL_DEPTH_ATTACHMENT;
            depthCount++;
        }
    }
    return depthCount;
}

int32_t RenderBackendGLES::InvalidateColor(
    array_view<uint32_t> invalidateAttachment, const RenderPassDesc& rpd, const RenderPassSubpassDesc& currentSubPass)
{
    int32_t colorCount = 0;
    // see which parts of the fbo can be invalidated...
    // collect color attachment infos..
    for (uint32_t ci = 0; ci < currentSubPass.colorAttachmentCount; ci++) {
        const uint32_t index = currentSubPass.colorAttachmentIndices[ci];
        if (attachmentLastUse_[index] == currentSubPass_) { // is last use of the attachment
            if (const auto* image = attachmentImage_[index]) {
                const auto& dplat = static_cast<const GpuImagePlatformDataGL&>(image->GetPlatformData());
                if (dplat.image || dplat.renderBuffer) {
                    if (rpd.attachments[index].storeOp == CORE_ATTACHMENT_STORE_OP_DONT_CARE) {
                        invalidateAttachment[static_cast<size_t>(colorCount)] = GL_COLOR_ATTACHMENT0 + ci;
                        colorCount++;
                    }
                }
            }
        }
    }
    return colorCount;
}

uint32_t RenderBackendGLES::ResolveMSAA(const RenderPassDesc& rpd, const RenderPassSubpassDesc& currentSubPass)
{
    const GLbitfield mask = ((currentSubPass.resolveAttachmentCount > 0u) ? GL_COLOR_BUFFER_BIT : 0u) |
                            ((currentSubPass.depthResolveAttachmentCount > 0u) ? GL_DEPTH_BUFFER_BIT : 0u);
    if (!mask) {
        return GL_FRAMEBUFFER;
    }

    if (scissorEnabled_) {
        glDisable(GL_SCISSOR_TEST);
        scissorEnabled_ = false;
    }

    // Resolve MSAA buffers.
    // NOTE: ARM recommends NOT to use glBlitFramebuffer here
    if (!currentSubPass.viewMask) {
        device_.BindReadFrameBuffer(currentFrameBuffer_->fbos[currentSubPass_].fbo);
        device_.BindWriteFrameBuffer(currentFrameBuffer_->fbos[currentSubPass_].resolve);

        glBlitFramebuffer(0, 0, static_cast<GLint>(currentFrameBuffer_->width),
            static_cast<GLint>(currentFrameBuffer_->height), 0, 0, static_cast<GLint>(currentFrameBuffer_->width),
            static_cast<GLint>(currentFrameBuffer_->height), mask, GL_NEAREST);
    } else {
        // Layers need to be resolved one by one. Create temporary FBOs and go through the layers.
        GLuint frameBuffers[2U]; // 2: buffer size
        glGenFramebuffers(2, frameBuffers); // 2: buffer size
        device_.BindReadFrameBuffer(frameBuffers[0U]);
        device_.BindWriteFrameBuffer(frameBuffers[1U]);

        const auto& srcImage =
            gpuResourceMgr_.GetImage(rpd.attachmentHandles[currentSubPass.colorAttachmentIndices[0U]]);
        if (srcImage == nullptr) {
            return GL_FRAMEBUFFER;
        }
        const auto& srcPlat = static_cast<const GpuImagePlatformDataGL&>(srcImage->GetBasePlatformData());
        const auto& dstImage =
            gpuResourceMgr_.GetImage(rpd.attachmentHandles[currentSubPass.resolveAttachmentIndices[0U]]);
        if (dstImage == nullptr) {
            return GL_FRAMEBUFFER;
        }
        const auto& dstPlat = static_cast<const GpuImagePlatformDataGL&>(dstImage->GetBasePlatformData());
        auto viewMask = currentSubPass.viewMask;
        auto layer = 0;
        while (viewMask) {
            glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, srcPlat.image, 0, layer);
            glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dstPlat.image, 0, layer);

            glBlitFramebuffer(0, 0, static_cast<GLint>(currentFrameBuffer_->width),
                static_cast<GLint>(currentFrameBuffer_->height), 0, 0, static_cast<GLint>(currentFrameBuffer_->width),
                static_cast<GLint>(currentFrameBuffer_->height), mask, GL_NEAREST);
            viewMask >>= 1U;
            ++layer;
        }
        glDeleteFramebuffers(2, frameBuffers); // 2: buffer size

        // invalidation exepcts to find the actual FBOs
        device_.BindReadFrameBuffer(currentFrameBuffer_->fbos[currentSubPass_].fbo);
        device_.BindWriteFrameBuffer(currentFrameBuffer_->fbos[currentSubPass_].resolve);
    }
    return GL_READ_FRAMEBUFFER;
}

void RenderBackendGLES::RenderCommandEndRenderPass(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::END_RENDER_PASS);
    const auto& renderCmd = *static_cast<const struct RenderCommandEndRenderPass*>(ref.rc);
    if (renderCmd.endType == RenderPassEndType::END_RENDER_PASS) {
        PLUGIN_ASSERT_MSG(inRenderpass_ == 1, "RenderBackendGLES endrenderpass mInRenderpass %u", inRenderpass_);
        inRenderpass_--;
    }
    if (currentFrameBuffer_ == nullptr) {
        // Completely invalid state in backend.
        return;
    }
    const auto& rpd = activeRenderPass_.renderPassDesc;
    const auto& currentSubPass = activeRenderPass_.subpasses[currentSubPass_];

    // Resolve MSAA
    const uint32_t fbType = ResolveMSAA(rpd, currentSubPass);

    // Finally invalidate color and depth..
    GLenum invalidate[PipelineStateConstants::MAX_COLOR_ATTACHMENT_COUNT + 1] = {};
    int32_t invalidateCount = InvalidateColor(invalidate, rpd, currentSubPass);
    invalidateCount += InvalidateDepthStencil(
        array_view(invalidate + invalidateCount, countof(invalidate) - invalidateCount), rpd, currentSubPass);

    // NOTE: all attachments should be the same size AND mCurrentFrameBuffer->width/height should match that!
    Invalidate(fbType, invalidateCount, invalidate, rpd, *currentFrameBuffer_);

    if (inRenderpass_ == 0) {
        currentFrameBuffer_ = nullptr;
    }
}

void RenderBackendGLES::RenderCommandBindVertexBuffers(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::BIND_VERTEX_BUFFERS);
    const auto& renderCmd = *static_cast<const struct RenderCommandBindVertexBuffers*>(ref.rc);
    PLUGIN_ASSERT(renderCmd.vertexBufferCount > 0);
    PLUGIN_ASSERT(renderCmd.vertexBufferCount <= PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT);
    if (!boundGraphicsPipeline_ || !boundShaderProgram_) {
        return;
    }
    vertexAttribBinds_ = renderCmd.vertexBufferCount;
    for (size_t i = 0; i < renderCmd.vertexBufferCount; i++) {
        const auto& currVb = renderCmd.vertexBuffers[i];
        if (const auto* gpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferGLES>(currVb.bufferHandle); gpuBuffer) {
            const auto& plat = gpuBuffer->GetPlatformData();
            uintptr_t offset = currVb.bufferOffset;
            offset += plat.currentByteOffset;
            vertexAttribBindSlots_[i].id = plat.buffer;
            vertexAttribBindSlots_[i].offset = static_cast<intptr_t>(offset);
        } else {
            vertexAttribBindSlots_[i].id = 0;
            vertexAttribBindSlots_[i].offset = 0;
        }
    }
    vertexBufferUpdate_ = true;
}

void RenderBackendGLES::RenderCommandBindIndexBuffer(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::BIND_INDEX_BUFFER);
    const auto& renderCmd = *static_cast<const struct RenderCommandBindIndexBuffer*>(ref.rc);
    if (const GpuBufferGLES* gpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferGLES>(renderCmd.indexBuffer.bufferHandle);
        gpuBuffer) {
        const auto& plat = gpuBuffer->GetPlatformData();
        boundIndexBuffer_.offset = renderCmd.indexBuffer.bufferOffset;
        boundIndexBuffer_.offset += plat.currentByteOffset;
        boundIndexBuffer_.type = renderCmd.indexBuffer.indexType;
        boundIndexBuffer_.id = plat.buffer;
    }
    indexBufferUpdate_ = true;
}

void RenderBackendGLES::RenderCommandBlitImage(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::BLIT_IMAGE);
    const auto& renderCmd = *static_cast<const struct RenderCommandBlitImage*>(ref.rc);
    const auto* srcImage = gpuResourceMgr_.GetImage<GpuImageGLES>(renderCmd.srcHandle);
    const auto* dstImage = gpuResourceMgr_.GetImage<GpuImageGLES>(renderCmd.dstHandle);
    if ((srcImage == nullptr) || (dstImage == nullptr)) {
        return;
    }
    const auto& srcDesc = srcImage->GetDesc();
    const auto& srcPlat = srcImage->GetPlatformData();
    const auto& dstDesc = dstImage->GetDesc();
    const auto& dstPlat = dstImage->GetPlatformData();
    const auto& srcRect = renderCmd.imageBlit.srcOffsets;
    const auto& dstRect = renderCmd.imageBlit.dstOffsets;
    const auto& src = renderCmd.imageBlit.srcSubresource;
    const auto& dst = renderCmd.imageBlit.dstSubresource;
    const auto srcMipLevel = static_cast<GLint>(src.mipLevel);
    const auto dstMipLevel = static_cast<GLint>(dst.mipLevel);
    const auto srcSampleCount = static_cast<uint32_t>(srcDesc.sampleCountFlags);
    const auto dstSampleCount = static_cast<uint32_t>(dstDesc.sampleCountFlags);
    PLUGIN_ASSERT_MSG(src.layerCount == dst.layerCount, "Source and Destination layercounts do not match!");
    PLUGIN_ASSERT_MSG(inRenderpass_ == 0, "RenderCommandBlitImage while inRenderPass");
    glDisable(GL_SCISSOR_TEST);
    scissorEnabled_ = false;
    // NOTE: LAYERS! (texture arrays)
    device_.BindReadFrameBuffer(blitImageSourceFbo_);
    device_.BindWriteFrameBuffer(blitImageDestinationFbo_);
    for (uint32_t layer = 0; layer < src.layerCount; layer++) {
        const GLenum srcType = GetTarget(srcPlat.type, layer, srcSampleCount);
        const GLenum dstType = GetTarget(dstPlat.type, layer, dstSampleCount);
        // glFramebufferTextureLayer for array textures....
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, srcType, srcPlat.image, srcMipLevel);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dstType, dstPlat.image, dstMipLevel);
        DoBlit(renderCmd.filter, { src.mipLevel, srcRect[0], srcRect[1], srcDesc.height },
            { dst.mipLevel, dstRect[0], dstRect[1], dstDesc.height });
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, srcType, 0, 0);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dstType, 0, 0);
    }
}

void RenderBackendGLES::RenderCommandCopyBuffer(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::COPY_BUFFER);
    const auto& renderCmd = *static_cast<const struct RenderCommandCopyBuffer*>(ref.rc);
    const auto* srcGpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferGLES>(renderCmd.srcHandle);
    const auto* dstGpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferGLES>(renderCmd.dstHandle);
    if (srcGpuBuffer && dstGpuBuffer) {
        const auto& srcData = srcGpuBuffer->GetPlatformData();
        const auto& dstData = dstGpuBuffer->GetPlatformData();
        const auto oldBindR = device_.BoundBuffer(GL_COPY_READ_BUFFER);
        const auto oldBindW = device_.BoundBuffer(GL_COPY_WRITE_BUFFER);
        device_.BindBuffer(GL_COPY_READ_BUFFER, srcData.buffer);
        device_.BindBuffer(GL_COPY_WRITE_BUFFER, dstData.buffer);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
            static_cast<GLintptr>(renderCmd.bufferCopy.srcOffset),
            static_cast<GLintptr>(renderCmd.bufferCopy.dstOffset), static_cast<GLsizeiptr>(renderCmd.bufferCopy.size));
        device_.BindBuffer(GL_COPY_READ_BUFFER, oldBindR);
        device_.BindBuffer(GL_COPY_WRITE_BUFFER, oldBindW);
    }
}

void RenderBackendGLES::BufferToImageCopy(const struct RenderCommandCopyBufferImage& renderCmd)
{
#if (RENDER_HAS_GLES_BACKEND == 1) & defined(_WIN32)
    // use the workaround only for gles backend on windows. (pvr simulator bug)
    constexpr const bool usePixelUnpackBuffer = false;
#else
    // expect this to work, and the nvidia bug to be fixed.
    constexpr const bool usePixelUnpackBuffer = true;
#endif
    auto* srcGpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferGLES>(renderCmd.srcHandle);
    auto* dstGpuImage = gpuResourceMgr_.GetImage<GpuImageGLES>(renderCmd.dstHandle);
    if ((srcGpuBuffer == nullptr) || (dstGpuImage == nullptr)) {
        return;
    }
    const auto info = SetupBlit<usePixelUnpackBuffer>(device_, renderCmd.bufferImageCopy, *srcGpuBuffer, *dstGpuImage);
    if (info.iPlat.type == GL_TEXTURE_CUBE_MAP) {
        BlitCube(device_, info);
    } else if (info.iPlat.type == GL_TEXTURE_2D) {
        Blit2D(device_, info);
    } else if (info.iPlat.type == GL_TEXTURE_2D_ARRAY) {
        BlitArray(device_, info);
    } else if (info.iPlat.type == GL_TEXTURE_3D) {
        Blit3D(device_, info);
#if RENDER_HAS_GLES_BACKEND
    } else if (info.iPlat.type == GL_TEXTURE_EXTERNAL_OES) {
        PLUGIN_LOG_E("Tried to copy to GL_TEXTURE_EXTERNAL_OES. Ignored!");
#endif
    } else {
        PLUGIN_ASSERT_MSG(false, "RenderCommandCopyBufferImage unhandled type");
    }
    FinishBlit<usePixelUnpackBuffer>(device_, *srcGpuBuffer);
}

void RenderBackendGLES::ImageToBufferCopy(const struct RenderCommandCopyBufferImage& renderCmd)
{
    const auto& bc = renderCmd.bufferImageCopy;
    const auto* srcGpuImage = static_cast<GpuImageGLES*>(gpuResourceMgr_.GetImage(renderCmd.srcHandle));
    const auto* dstGpuBuffer = static_cast<GpuBufferGLES*>(gpuResourceMgr_.GetBuffer(renderCmd.dstHandle));
    PLUGIN_ASSERT(srcGpuImage);
    PLUGIN_ASSERT(dstGpuBuffer);
    if ((srcGpuImage == nullptr) || (dstGpuBuffer == nullptr)) {
        return;
    }
    const auto& iPlat = static_cast<const GpuImagePlatformDataGL&>(srcGpuImage->GetPlatformData());
    const auto& bPlat = static_cast<const GpuBufferPlatformDataGL&>(dstGpuBuffer->GetPlatformData());
    if ((iPlat.type != GL_TEXTURE_CUBE_MAP) && (iPlat.type != GL_TEXTURE_2D)) {
        PLUGIN_LOG_E("Unsupported texture type in ImageToBufferCopy %x", iPlat.type);
        return;
    }
    device_.BindReadFrameBuffer(blitImageSourceFbo_);
    PLUGIN_ASSERT(bc.imageSubresource.layerCount == 1);
    GLenum type = GL_TEXTURE_2D;
    if (iPlat.type == GL_TEXTURE_CUBE_MAP) {
        type = GetCubeMapTarget(iPlat.type, bc.imageSubresource.baseArrayLayer);
    }
    // glFramebufferTextureLayer for array textures....
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, type, static_cast<GLuint>(iPlat.image),
        static_cast<GLint>(bc.imageSubresource.mipLevel));
    const Math::UVec2 sPos { bc.imageOffset.width, bc.imageOffset.height };
    const Math::UVec2 sExt { bc.imageExtent.width, bc.imageExtent.height };
    device_.BindBuffer(GL_PIXEL_PACK_BUFFER, bPlat.buffer);
    glPixelStorei(GL_PACK_ROW_LENGTH, static_cast<GLint>(bc.bufferRowLength));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    uintptr_t dstOffset = bc.bufferOffset + bPlat.currentByteOffset;
    glReadnPixels(static_cast<GLint>(sPos.x), static_cast<GLint>(sPos.y), static_cast<GLsizei>(sExt.x),
        static_cast<GLsizei>(sExt.y), iPlat.format, static_cast<GLenum>(iPlat.dataType),
        static_cast<GLsizei>(bPlat.alignedByteSize), reinterpret_cast<void*>(dstOffset));
    device_.BindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, type, 0, 0);
}

void RenderBackendGLES::RenderCommandCopyBufferImage(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::COPY_BUFFER_IMAGE);
    const auto& renderCmd = *static_cast<const struct RenderCommandCopyBufferImage*>(ref.rc);
    PLUGIN_ASSERT(inRenderpass_ == 0); // this command should never run during renderpass..
    if (renderCmd.copyType == RenderCommandCopyBufferImage::CopyType::BUFFER_TO_IMAGE) {
        BufferToImageCopy(renderCmd);
    } else if (renderCmd.copyType == RenderCommandCopyBufferImage::CopyType::IMAGE_TO_BUFFER) {
        ImageToBufferCopy(renderCmd);
    }
}

void RenderBackendGLES::RenderCommandCopyImage(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::COPY_IMAGE);
    const auto& renderCmd = *static_cast<const struct RenderCommandCopyImage*>(ref.rc);
    PLUGIN_ASSERT(inRenderpass_ == 0); // this command should never run during renderpass..
    const auto* srcGpuImage = gpuResourceMgr_.GetImage<GpuImageGLES>(renderCmd.srcHandle);
    const auto* dstGpuImage = gpuResourceMgr_.GetImage<GpuImageGLES>(renderCmd.dstHandle);
    if ((srcGpuImage == nullptr) || (dstGpuImage == nullptr)) {
        return;
    }
    const auto& srcDesc = srcGpuImage->GetDesc();
    const auto& dstDesc = dstGpuImage->GetDesc();
#if RENDER_VALIDATION_ENABLED
    ValidateCopyImage(renderCmd.imageCopy, srcDesc, dstDesc);
#endif
    const auto srcMipLevel =
        static_cast<GLint>(Math::min(renderCmd.imageCopy.srcSubresource.mipLevel, srcDesc.mipCount - 1));
    const auto dstMipLevel =
        static_cast<GLint>(Math::min(renderCmd.imageCopy.dstSubresource.mipLevel, dstDesc.mipCount - 1));

    auto sOffset = renderCmd.imageCopy.srcOffset;
    auto dOffset = renderCmd.imageCopy.dstOffset;
    auto size = renderCmd.imageCopy.extent;

    // clamp negative offsets to zero and adjust extent and other offset accordingly
    ClampOffset(sOffset, dOffset, size);
    ClampOffset(dOffset, sOffset, size);

    // clamp size to fit src and dst
    ClampSize(sOffset, srcDesc, size);
    ClampSize(dOffset, dstDesc, size);

    const auto& srcPlatData = srcGpuImage->GetPlatformData();
    const auto& dstPlatData = dstGpuImage->GetPlatformData();
    glCopyImageSubData(srcPlatData.image, srcPlatData.type, srcMipLevel, sOffset.x, sOffset.y, sOffset.z,
        dstPlatData.image, dstPlatData.type, dstMipLevel, dOffset.x, dOffset.y, dOffset.z,
        static_cast<GLsizei>(size.width), static_cast<GLsizei>(size.height), static_cast<GLsizei>(size.depth));
}

void RenderBackendGLES::RenderCommandBarrierPoint(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::BARRIER_POINT);
    const auto& renderCmd = *static_cast<const struct RenderCommandBarrierPoint*>(ref.rc);
    const auto& rbList = *managers_.rbList;
    // NOTE: proper flagging of barriers.
    const RenderBarrierList::BarrierPointBarriers* barrierPointBarriers =
        rbList.GetBarrierPointBarriers(renderCmd.barrierPointIndex);
    if (!barrierPointBarriers) {
        return; // early out
    }
    const uint32_t barrierListCount = barrierPointBarriers->barrierListCount;
    const auto* nextBarrierList = barrierPointBarriers->firstBarrierList;
    GLbitfield barriers = 0;
    GLbitfield barriersByRegion = 0;
    for (uint32_t barrierListIndex = 0; barrierListIndex < barrierListCount; ++barrierListIndex) {
        if (nextBarrierList == nullptr) {
            // cannot be null, just a safety
            PLUGIN_ASSERT(false);
            return;
        }
        const auto& barrierListRef = *nextBarrierList;
        nextBarrierList = barrierListRef.nextBarrierPointBarrierList; // advance to next
        const uint32_t barrierCount = barrierListRef.count;

        for (uint32_t barrierIdx = 0; barrierIdx < barrierCount; ++barrierIdx) {
            const auto& barrier = barrierListRef.commandBarriers[barrierIdx];

            // check if written by previous shader as an attachment or storage/ image buffer
            if (barrier.src.accessFlags & (CORE_ACCESS_SHADER_WRITE_BIT | CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                              CORE_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) {
                const auto resourceHandle = barrier.resourceHandle;
                const auto handleType = RenderHandleUtil::GetHandleType(resourceHandle);

                // barrier by region is between fragment shaders and supports a subset of barriers.
                if ((barrier.src.pipelineStageFlags & CORE_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) &&
                    (barrier.dst.pipelineStageFlags & CORE_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)) {
                    barriersByRegion |= CommonBarrierBits(barrier.dst.accessFlags, handleType);
                } else {
                    // check the barriers shared with ByRegion
                    barriers |= CommonBarrierBits(barrier.dst.accessFlags, handleType);

                    // the rest are invalid for ByRegion
                    if (barrier.dst.accessFlags & CORE_ACCESS_INDIRECT_COMMAND_READ_BIT) {
                        barriers |= GL_COMMAND_BARRIER_BIT;
                    }
                    if (barrier.dst.accessFlags & CORE_ACCESS_INDEX_READ_BIT) {
                        barriers |= GL_ELEMENT_ARRAY_BARRIER_BIT;
                    }
                    if (barrier.dst.accessFlags & CORE_ACCESS_VERTEX_ATTRIBUTE_READ_BIT) {
                        barriers |= GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
                    }
                    // which are the correct accessFlags?
                    // GL_PIXEL_BUFFER_BARRIER_BIT:
                    // - buffer objects via the GL_PIXEL_PACK_BUFFER and GL_PIXEL_UNPACK_BUFFER bindings (via
                    // glReadPixels, glTexSubImage1D, etc.)
                    // GL_TEXTURE_UPDATE_BARRIER_BIT:
                    // - texture via glTex(Sub)Image*, glCopyTex(Sub)Image*, glCompressedTex(Sub)Image*, and
                    // reads via glGetTexImage GL_BUFFER_UPDATE_BARRIER_BIT:
                    // - glBufferSubData, glCopyBufferSubData, or glGetBufferSubData, or to buffer object memory
                    // mapped
                    //  by glMapBuffer or glMapBufferRange
                    // These two are cover all memory access, CORE_ACCESS_MEMORY_READ_BIT,
                    // CORE_ACCESS_MEMORY_WRITE_BIT?
                    if (barrier.dst.accessFlags & (CORE_ACCESS_TRANSFER_READ_BIT | CORE_ACCESS_TRANSFER_WRITE_BIT |
                                                      CORE_ACCESS_HOST_READ_BIT | CORE_ACCESS_HOST_WRITE_BIT)) {
                        if (handleType == RenderHandleType::GPU_IMAGE) {
                            barriers |= GL_TEXTURE_UPDATE_BARRIER_BIT;
                        } else if (handleType == RenderHandleType::GPU_BUFFER) {
                            barriers |= GL_BUFFER_UPDATE_BARRIER_BIT | GL_PIXEL_BUFFER_BARRIER_BIT;
                        }
                    }
                    // GL_TRANSFORM_FEEDBACK_BARRIER_BIT is not used at the moment
                }
            }
        }
    }
    if (barriers) {
        glMemoryBarrier(barriers);
    }
    if (barriersByRegion) {
        // only for fragment-fragment
        glMemoryBarrierByRegion(barriersByRegion);
    }
}

void RenderBackendGLES::UpdateGlobalDescriptorSets()
{
    auto& descriptorSetMgr = static_cast<DescriptorSetManagerGles&>(device_.GetDescriptorSetManager());

    // Update global descset if needed
    const auto& allDescSets = descriptorSetMgr.GetUpdateDescriptorSetHandles();
    if (allDescSets.empty()) {
        return;
    }
    for (const auto& descHandle : allDescSets) {
        if (RenderHandleUtil::GetHandleType(descHandle) != RenderHandleType::DESCRIPTOR_SET) {
            continue;
        }
        descriptorSetMgr.UpdateDescriptorSetGpuHandle(descHandle);
    }
#if RENDER_HAS_GLES_BACKEND
    oesBindingsChanged_ = true;
    oesBinds_.clear();
#endif
}

void RenderBackendGLES::UpdateCommandListDescriptorSets(
    const RenderCommandList& renderCommandList, NodeContextDescriptorSetManager& ncdsm)
{
    const auto& allDescSets = renderCommandList.GetUpdateDescriptorSetHandles();
    if (allDescSets.empty()) {
        return;
    }
    for (const auto& descHandle : allDescSets) {
        if (RenderHandleUtil::GetHandleType(descHandle) != RenderHandleType::DESCRIPTOR_SET) {
            continue;
        }
        ncdsm.UpdateDescriptorSetGpuHandle(descHandle);
    }
#if RENDER_HAS_GLES_BACKEND
    oesBindingsChanged_ = true;
    oesBinds_.clear();
#endif
}

void RenderBackendGLES::RenderCommandBindDescriptorSets(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::BIND_DESCRIPTOR_SETS);
    if (!boundComputePipeline_ && !boundGraphicsPipeline_) {
        return;
    }
    const auto& renderCmd = *static_cast<const struct RenderCommandBindDescriptorSets*>(ref.rc);
    PLUGIN_ASSERT_MSG(renderCmd.psoHandle == currentPsoHandle_, "psoHandle mismatch");

    const auto lastSet = renderCmd.firstSet + renderCmd.setCount;
    if ((renderCmd.firstSet >= PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) ||
        (lastSet > PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT)) {
        return;
    }
    std::copy(renderCmd.descriptorSetHandles + renderCmd.firstSet, renderCmd.descriptorSetHandles + lastSet,
        descriptorSetHandles_ + renderCmd.firstSet);
    auto* dst = descriptorSetDynamicOffsets_ + renderCmd.firstSet;
    for (const auto &src : array_view(renderCmd.descriptorSetDynamicOffsets + renderCmd.firstSet,
                                      renderCmd.descriptorSetDynamicOffsets + lastSet)) {
        dst->dynamicOffsetCount = src.dynamicOffsetCount;
        std::copy(src.dynamicOffsets, src.dynamicOffsets + src.dynamicOffsetCount, dst->dynamicOffsets);
        ++dst;
    }
    firstSet_ = static_cast<uint16_t>(renderCmd.firstSet);
    setCount_ = static_cast<uint16_t>(renderCmd.setCount);
    descriptorUpdate_ = true;

#if RENDER_HAS_GLES_BACKEND
    oesBinds_.clear();
    oesBindingsChanged_ = true;
    const auto& ncdsm = *static_cast<NodeContextDescriptorSetManagerGles*>(managers_.descriptorSetMgr);
    for (uint32_t set = firstSet_; set < setCount_; ++set) {
        const auto& descHandle = descriptorSetHandles_[set];
        if (!ncdsm.HasPlatformConversionBindings(descHandle)) {
            continue;
        }
        const auto& resources = ncdsm.GetResources(descHandle);
        for (uint32_t binding = 0U, count = resources.size(); binding < count; ++binding) {
            auto& bind = resources[binding];
            if ((bind.descriptorType == CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ||
                (bind.descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE) ||
                (bind.descriptorType == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)) {
                if (bind.resources[0].image.mode & Gles::EXTERNAL_BIT) {
                    oesBinds_.push_back(OES_Bind { set, binding });
                }
            }
        }
    }
#endif
}

void RenderBackendGLES::SetPushConstant(uint32_t program, const Gles::PushConstantReflection& pc, const void* data)
{
    const auto location = static_cast<GLint>(pc.location);
    // the consts list has been filtered and cleared of unused uniforms.
    PLUGIN_ASSERT(location != Gles::INVALID_LOCATION);
    GLint count = Math::max(static_cast<GLint>(pc.arraySize), 1);
    switch (pc.type) {
        case GL_UNSIGNED_INT: {
            glProgramUniform1uiv(program, location, count, static_cast<const GLuint*>(data));
            break;
        }
        case GL_FLOAT: {
            glProgramUniform1fv(program, location, count, static_cast<const GLfloat*>(data));
            break;
        }
        case GL_FLOAT_VEC2: {
            glProgramUniform2fv(program, location, count, static_cast<const GLfloat*>(data));
            break;
        }
        case GL_FLOAT_VEC4: {
            glProgramUniform4fv(program, location, count, static_cast<const GLfloat*>(data));
            break;
        }
        case GL_FLOAT_MAT4: {
            glProgramUniformMatrix4fv(program, location, count, false, static_cast<const GLfloat*>(data));
            break;
        }
        case GL_UNSIGNED_INT_VEC4: {
            glProgramUniform4uiv(program, location, count, static_cast<const GLuint*>(data));
            break;
        }
        default:
            PLUGIN_ASSERT_MSG(false, "Unhandled pushconstant variable type");
    }
}

void RenderBackendGLES::SetPushConstants(uint32_t program, const array_view<Gles::PushConstantReflection>& consts)
{
    if (boundProgram_.setPushConstants) {
        boundProgram_.setPushConstants = false;
        const auto& renderCmd = boundProgram_.pushConstants;
        PLUGIN_ASSERT_MSG(renderCmd.psoHandle == currentPsoHandle_, "psoHandle mismatch");
        PLUGIN_ASSERT_MSG(renderCmd.pushConstant.byteSize > 0, "PushConstant byteSize is zero!");
        PLUGIN_ASSERT_MSG(renderCmd.data, "PushConstant data is nullptr!");
        if ((renderCmd.data == nullptr) || (renderCmd.pushConstant.byteSize == 0))
            return;
        // ASSERT: expecting data is valid
        // NOTE: handle rest of the types
        for (const auto& pc : consts) {
            const size_t offs = pc.offset;
            if ((offs + pc.size) > renderCmd.pushConstant.byteSize) {
                PLUGIN_LOG_E(
                    "pushConstant data invalid (data for %s is missing [offset:%zu size:%zu] byteSize of data:%u)",
                    pc.name.c_str(), pc.offset, pc.size, renderCmd.pushConstant.byteSize);
                continue;
            }
            /*
            NOTE: handle the strides....
            consts[i].array_stride;
            consts[i].matrix_stride; */
            SetPushConstant(program, pc, &renderCmd.data[offs]);
        }
    }
}

void RenderBackendGLES::RenderCommandPushConstant(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::PUSH_CONSTANT);
    if (!boundComputeProgram_ && !boundShaderProgram_) {
        return;
    }
    const auto& renderCmd = *static_cast<const struct RenderCommandPushConstant*>(ref.rc);
    if (renderCmd.pushConstant.byteSize > 0) {
        PLUGIN_ASSERT(renderCmd.data);
        PLUGIN_ASSERT_MSG(renderCmd.psoHandle == currentPsoHandle_, "psoHandle mismatch");
        boundProgram_.setPushConstants = true;
        boundProgram_.pushConstants = renderCmd;
        if (boundComputeProgram_) {
            const auto& sd =
                static_cast<const GpuComputeProgramPlatformDataGL&>(boundComputeProgram_->GetPlatformData());
            SetPushConstants(sd.program, sd.pushConstants);
        } else {
            const auto& sd = static_cast<const GpuShaderProgramPlatformDataGL&>(boundShaderProgram_->GetPlatformData());
            SetPushConstants(sd.program, sd.pushConstants);
        }
    }
}

void RenderBackendGLES::RenderCommandClearColorImage(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::CLEAR_COLOR_IMAGE);
#if RENDER_HAS_GLES_BACKEND
#if (RENDER_VALIDATION_ENABLED == 1)
    PLUGIN_LOG_ONCE_E("RenderBackendGLES::RenderCommandClearColorImage",
        "Render command clear color image not support with GLES. One should implement higher level path for "
        "clearing.");
#endif
#else
    const auto& renderCmd = *static_cast<const struct RenderCommandClearColorImage*>(ref.rc);

    const GpuImageGLES* imagePtr = gpuResourceMgr_.GetImage<GpuImageGLES>(renderCmd.handle);
    if (imagePtr) {
        const GpuImagePlatformDataGL& platImage = imagePtr->GetPlatformData();
        // NOTE: mip levels and array layers should be handled separately
        for (const auto& subresRef : renderCmd.ranges) {
            glClearTexImage(platImage.image,     // texture
                (int32_t)subresRef.baseMipLevel, // level
                platImage.format,                // format
                platImage.dataType,              // type
                &renderCmd.color);               // data
        }
    }
#endif
}

// dynamic states
void RenderBackendGLES::RenderCommandDynamicStateViewport(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::DYNAMIC_STATE_VIEWPORT);
    const auto& renderCmd = *static_cast<const struct RenderCommandDynamicStateViewport*>(ref.rc);
    const ViewportDesc& vd = renderCmd.viewportDesc;
    SetViewport(vd);
}

void RenderBackendGLES::RenderCommandDynamicStateScissor(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::DYNAMIC_STATE_SCISSOR);
    const auto& renderCmd = *static_cast<const struct RenderCommandDynamicStateScissor*>(ref.rc);
    const ScissorDesc& sd = renderCmd.scissorDesc;
    SetScissor(sd);
}

void RenderBackendGLES::RenderCommandDynamicStateLineWidth(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::DYNAMIC_STATE_LINE_WIDTH);
    const auto& renderCmd = *static_cast<const struct RenderCommandDynamicStateLineWidth*>(ref.rc);
    if (renderCmd.lineWidth != cacheState_.rasterizationState.lineWidth) {
        cacheState_.rasterizationState.lineWidth = renderCmd.lineWidth;
        glLineWidth(renderCmd.lineWidth);
    }
}

void RenderBackendGLES::RenderCommandDynamicStateDepthBias(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::DYNAMIC_STATE_DEPTH_BIAS);
    PLUGIN_ASSERT_MSG(false, "RenderCommandDynamicStateDepthBias not implemented");
}

void RenderBackendGLES::RenderCommandDynamicStateBlendConstants(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::DYNAMIC_STATE_BLEND_CONSTANTS);
    PLUGIN_ASSERT_MSG(false, "RenderCommandDynamicStateBlendConstants not implemented");
}

void RenderBackendGLES::RenderCommandDynamicStateDepthBounds(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::DYNAMIC_STATE_DEPTH_BOUNDS);
    PLUGIN_ASSERT_MSG(false, "RenderCommandDynamicStateDepthBounds not implemented");
}

void RenderBackendGLES::SetStencilState(const uint32_t frontFlags, const GraphicsState::StencilOpState& front,
    const uint32_t backFlags, const GraphicsState::StencilOpState& back)
{
    auto& cFront = cacheState_.depthStencilState.frontStencilOpState;
    auto& cBack = cacheState_.depthStencilState.backStencilOpState;
    const uint32_t FUNCMASK =
        (StencilSetFlags::SETCOMPAREOP | StencilSetFlags::SETCOMPAREMASK | StencilSetFlags::SETREFERENCE);
    if (frontFlags & StencilSetFlags::SETWRITEMASK) {
        cFront.writeMask = front.writeMask;
        glStencilMaskSeparate(GL_FRONT, cFront.writeMask);
    }
    if (frontFlags & FUNCMASK) {
        SetStencilCompareOp(cFront, front);
        glStencilFuncSeparate(
            GL_FRONT, GetCompareOp(cFront.compareOp), static_cast<GLint>(cFront.reference), cFront.compareMask);
    }
    if (frontFlags & StencilSetFlags::SETOP) {
        SetStencilOp(cFront, front);
        glStencilOpSeparate(
            GL_FRONT, GetStencilOp(cFront.failOp), GetStencilOp(cFront.depthFailOp), GetStencilOp(cFront.passOp));
    }
    if (backFlags & StencilSetFlags::SETWRITEMASK) {
        cBack.writeMask = back.writeMask;
        glStencilMaskSeparate(GL_BACK, cBack.writeMask);
    }
    if (backFlags & FUNCMASK) {
        SetStencilCompareOp(cBack, back);
        glStencilFuncSeparate(
            GL_BACK, GetCompareOp(cBack.compareOp), static_cast<GLint>(cBack.reference), cBack.compareMask);
    }
    if (backFlags & StencilSetFlags::SETOP) {
        SetStencilOp(cBack, back);
        glStencilOpSeparate(
            GL_FRONT, GetStencilOp(cBack.failOp), GetStencilOp(cBack.depthFailOp), GetStencilOp(cBack.passOp));
    }
}

void RenderBackendGLES::RenderCommandDynamicStateStencil(const RenderCommandWithType& ref)
{
    PLUGIN_ASSERT(ref.type == RenderCommandType::DYNAMIC_STATE_STENCIL);
    const auto& renderCmd = *static_cast<const struct RenderCommandDynamicStateStencil*>(ref.rc);
    auto& cFront = cacheState_.depthStencilState.frontStencilOpState;
    auto& cBack = cacheState_.depthStencilState.backStencilOpState;
    uint32_t setFront = 0;
    uint32_t setBack = 0;
    if (renderCmd.faceMask & StencilFaceFlagBits::CORE_STENCIL_FACE_FRONT_BIT) {
        if (renderCmd.dynamicState == StencilDynamicState::COMPARE_MASK) {
            if (renderCmd.mask != cFront.compareMask) {
                cFront.compareMask = renderCmd.mask;
                setFront |= StencilSetFlags::SETCOMPAREMASK;
            }
        } else if (renderCmd.dynamicState == StencilDynamicState::WRITE_MASK) {
            if (renderCmd.mask != cFront.writeMask) {
                cFront.writeMask = renderCmd.mask;
                setFront |= StencilSetFlags::SETWRITEMASK;
            }
        } else if (renderCmd.dynamicState == StencilDynamicState::REFERENCE) {
            if (renderCmd.mask != cFront.reference) {
                cFront.reference = renderCmd.mask;
                setFront |= StencilSetFlags::SETREFERENCE;
            }
        }
    }
    if (renderCmd.faceMask & StencilFaceFlagBits::CORE_STENCIL_FACE_BACK_BIT) {
        if (renderCmd.dynamicState == StencilDynamicState::COMPARE_MASK) {
            if (renderCmd.mask != cBack.compareMask) {
                cBack.compareMask = renderCmd.mask;
                setBack |= StencilSetFlags::SETCOMPAREMASK;
            }
        } else if (renderCmd.dynamicState == StencilDynamicState::WRITE_MASK) {
            if (renderCmd.mask != cBack.writeMask) {
                cBack.writeMask = renderCmd.mask;
                setBack |= StencilSetFlags::SETWRITEMASK;
            }
        } else if (renderCmd.dynamicState == StencilDynamicState::REFERENCE) {
            if (renderCmd.mask != cBack.reference) {
                cBack.reference = renderCmd.mask;
                setBack |= StencilSetFlags::SETREFERENCE;
            }
        }
    }
    SetStencilState(setFront, cFront, setBack, cBack);
}

void RenderBackendGLES::RenderCommandFragmentShadingRate(const RenderCommandWithType& renderCmd)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    PLUGIN_LOG_ONCE_I("gles_RenderCommandFragmentShadingRate",
        "RENDER_VALIDATION: Fragment shading rate not available with GL(ES) backend.");
#endif
}

void RenderBackendGLES::RenderCommandExecuteBackendFramePosition(const RenderCommandWithType& renderCmd)
{
    PLUGIN_ASSERT_MSG(false, "RenderCommandExecuteBackendFramePosition not implemented");
}

void RenderBackendGLES::RenderCommandWriteTimestamp(const RenderCommandWithType& renderCmd)
{
    PLUGIN_ASSERT_MSG(false, "RenderCommandWriteTimestamp not implemented");
}

void RenderBackendGLES::BindVertexInputs(
    const VertexInputDeclarationData& decldata, const array_view<const int32_t>& vertexInputs)
{
    // update bindings for the VAO.
    // process with attribute descriptions to only bind the needed vertex buffers
    // NOTE: that there are or might be extra bindings in the decldata.bindingDescriptions,
    // but we only bind the ones needed for the shader
    const uint32_t minBinding = Math::min(vertexAttribBinds_, decldata.attributeDescriptionCount);
    for (uint32_t i = 0; i < minBinding; ++i) {
        const auto& attributeRef = decldata.attributeDescriptions[i];
        const uint32_t location = attributeRef.location;
        const uint32_t binding = attributeRef.binding;
        // NOTE: we need to bind all the buffers to the correct bindings.
        // shader optimized check (vertexInputs, some locations are not in use)
        if ((location != ~0u) && (binding != ~0u) && (vertexInputs[location] != Gles::INVALID_LOCATION)) {
            const auto& slot = vertexAttribBindSlots_[binding];
            const auto& bindingRef = decldata.bindingDescriptions[binding];
            PLUGIN_ASSERT(bindingRef.binding == binding);
            // buffer bound to slot, and it's used by the shader.
            device_.BindVertexBuffer(binding, slot.id, slot.offset, static_cast<intptr_t>(bindingRef.stride));
            /*
            core/vulkan
            bindingRef.vertexInputRate =  CORE_VERTEX_INPUT_RATE_VERTEX (0)  attribute index advances per vertex
            bindingRef.vertexInputRate =  CORE_VERTEX_INPUT_RATE_INSTANCE (1)  attribute index advances per instance

            gl/gles
            If divisor is  0, the attributes using the buffer bound to bindingindex advance once per vertex.
            If divisor is >0, the attributes advance once per divisor instances of the set(s) of vertices being
            rendered.

            so we can directly pass the inputRate as VertexBindingDivisor. (ie. advance once per instance)
            ie. enum happens to match and can simply cast.
            */
            static_assert(CORE_VERTEX_INPUT_RATE_VERTEX == 0 && CORE_VERTEX_INPUT_RATE_INSTANCE == 1);
            device_.VertexBindingDivisor(binding, static_cast<uint32_t>(bindingRef.vertexInputRate));
        }
    }
}

void RenderBackendGLES::BindSampler(const array_view<const Gles::Bind::Resource> resources, const Binder& binder,
    const array_view<const Slice> descriptorIndex, const array_view<const uint8_t> ids)
{
    const auto end = Math::min(resources.size(), descriptorIndex.size());
    for (uint32_t index = 0; index < end; ++index) {
        const auto& idRange = descriptorIndex[index];
        if ((size_t(idRange.index) + idRange.count) > ids.size()) {
            continue;
        }
        const auto samplerId = resources[index].sampler.samplerId;
        for (const auto& id : array_view(ids.data() + idRange.index, idRange.count)) {
            const auto textureUnit = index + id;
#if (RENDER_PERF_ENABLED == 1)
            if (device_.BoundSampler(textureUnit) != samplerId) {
                ++perfCounters_.bindSampler;
            }
#endif
            device_.BindSampler(textureUnit, samplerId);
        }
    }
}

void RenderBackendGLES::BindTexture(array_view<const Gles::Bind::Resource> resources, const Binder& binder,
    BASE_NS::array_view<const Slice> descriptorIndex, BASE_NS::array_view<const uint8_t> ids,
    DescriptorType descriptorType)
{
    const auto end = Math::min(resources.size(), descriptorIndex.size());
    for (uint32_t index = 0; index < end; ++index) {
        const auto& idRange = descriptorIndex[index];
        if ((size_t(idRange.index) + idRange.count) > ids.size()) {
            continue;
        }
        const auto& imgType = resources[index].image;
        if (!imgType.image) {
            continue;
        }
        auto& plat = imgType.image->GetPlatformData();
        for (const auto& id : array_view(ids.data() + idRange.index, idRange.count)) {
            const auto textureUnit = index + id;
            uint32_t samplerId = UINT32_MAX;
            if (descriptorType == CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                samplerId = resources[index].sampler.samplerId;
            } else if (descriptorType == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
                samplerId = 0U;
            }
            if (samplerId != UINT32_MAX) {
#if (RENDER_PERF_ENABLED == 1)
                if (device_.BoundSampler(textureUnit) != samplerId) {
                    ++perfCounters_.bindSampler;
                }
#endif
                device_.BindSampler(textureUnit, samplerId);
            }
            const auto baseLevel =
                (imgType.mipLevel != PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS) ? imgType.mipLevel : 0U;
            if (descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
                device_.BindImageTexture(
                    textureUnit, plat.image, baseLevel, false, 0, imgType.mode & 0xFFFF, plat.internalFormat);
            } else {
#if (RENDER_PERF_ENABLED == 1)
                if (device_.BoundTexture(textureUnit, plat.type) != plat.image) {
                    ++perfCounters_.bindTexture;
                }
#endif
                device_.BindTexture(textureUnit, plat.type, plat.image);
                // NOTE: the last setting is active, can not have different miplevels bound from single
                // resource.
                // Check and update (if needed) the forced miplevel.
                if (plat.mipLevel != imgType.mipLevel) {
                    // NOTE: we are actually modifying the texture object bound above
                    const_cast<GpuImagePlatformDataGL&>(plat).mipLevel = imgType.mipLevel;
                    // either force the defined mip level or use defaults.
                    const auto maxLevel = static_cast<GLint>(
                        (plat.mipLevel != PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS) ? plat.mipLevel : 1000U);
                    glTexParameteri(plat.type, GL_TEXTURE_BASE_LEVEL, static_cast<GLint>(baseLevel));
                    glTexParameteri(plat.type, GL_TEXTURE_MAX_LEVEL, maxLevel);
                }
            }
        }
    }
}

void RenderBackendGLES::BindBuffer(array_view<const Gles::Bind::Resource> resources, const Binder& binder,
    BASE_NS::array_view<const Slice> descriptorIndex, BASE_NS::array_view<const uint8_t> ids,
    DescriptorType descriptorType)
{
    uint32_t target = 0U;
    if ((descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ||
        (descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)) {
        target = GL_UNIFORM_BUFFER;
    } else if ((descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
               (descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
        target = GL_SHADER_STORAGE_BUFFER;
    }
    const auto end = Math::min(resources.size(), descriptorIndex.size());
    for (uint32_t index = 0; index < end; index++) {
        const auto& idRange = descriptorIndex[index];
        if ((size_t(idRange.index) + idRange.count) > ids.size()) {
            continue;
        }
        const auto& obj = resources[index];
        uint32_t dynOffset = 0U;
        if (auto& currentOffsets = descriptorSetDynamicOffsets_[binder.set]; currentOffsets.dynamicOffsetCount) {
            auto& currentIndex = dynamicOffsetIndices_[binder.bind];
            if (currentIndex < currentOffsets.dynamicOffsetCount) {
                dynOffset = currentOffsets.dynamicOffsets[currentIndex];
            } else {
                PLUGIN_LOG_E("outofoffsets");
            }
        }

        for (const auto& id : array_view(ids.data() + idRange.index, idRange.count)) {
            const auto binding = index + id;
#if (RENDER_PERF_ENABLED == 1)
            if (device_.BoundBuffer(target) != obj.buffer.bufferId) {
                ++perfCounters_.bindBuffer;
            }
#endif
            device_.BindBufferRange(
                target, binding, obj.buffer.bufferId, obj.buffer.offset + dynOffset, obj.buffer.size);
        }
    }
}

void RenderBackendGLES::BindResources()
{
    if (!descriptorUpdate_) {
        return;
    }
    descriptorUpdate_ = false;
    const ResourcesView* shaderBindings = nullptr;
    if (boundComputeProgram_) {
        shaderBindings =
            &static_cast<const GpuComputeProgramPlatformDataGL&>(boundComputeProgram_->GetPlatformData()).resourcesView;
    } else if (boundShaderProgram_) {
#if RENDER_HAS_GLES_BACKEND
        if (oesBindingsChanged_) {
            oesBindingsChanged_ = false;

            // ask for a compatible program from the boundGraphicsPipeline_
            auto shader = boundGraphicsPipeline_->GetOESProgram(oesBinds_);
            if (!shader) {
                return;
            }
            if (boundShaderProgram_ != shader) {
                boundShaderProgram_ = shader;
                const auto& sd = static_cast<const GpuShaderProgramPlatformDataGL&>(shader->GetPlatformData());
                // Push constants and "fliplocation" uniform (ie. uniform state) should be only updated if changed...
                const uint32_t program = sd.program;
#if (RENDER_PERF_ENABLED == 1)
                if (device_.BoundProgram() != program) {
                    ++perfCounters_.bindProgram;
                }
#endif
                device_.UseProgram(program);
                if (sd.flipLocation != Gles::INVALID_LOCATION) {
                    const float flip = (renderingToDefaultFbo_) ? (-1.f) : (1.f);
                    glProgramUniform1fv(program, sd.flipLocation, 1, &flip);
                }
            }
        }
#endif
        shaderBindings =
            &static_cast<const GpuShaderProgramPlatformDataGL&>(boundShaderProgram_->GetPlatformData()).resourcesView;
    }
    if (!shaderBindings) {
        return;
    }

    const auto& ncdsm = *static_cast<NodeContextDescriptorSetManagerGles*>(managers_.descriptorSetMgr);
    uint32_t currentSet = UINT32_MAX;
    array_view<const Gles::Bind> descriptorSetResources;
    // with some bookkeeping might be possible to go through only descriptor sets based on previous
    // RenderCommandBindDescriptorSets instead of all sets.
    for (auto& binder : (shaderBindings->resourceList)) {
        // binders are in set - binding order. when set changes get the resources for the current set and gather dynamic
        // offsets
        if (binder.set != currentSet) {
            currentSet = binder.set;
            descriptorSetResources = ncdsm.GetResources(descriptorSetHandles_[binder.set]);

            // descriptorSetDynamicOffsets_ are only for dynamic buffers. figure out which index should be used for
            // which binding.
            dynamicOffsetIndices_.resize(descriptorSetResources.size());
            uint32_t index = 0U;
            std::transform(descriptorSetResources.cbegin(), descriptorSetResources.cend(),
                dynamicOffsetIndices_.begin(), [&index](const Gles::Bind& bind) {
                    if ((bind.descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
                        (bind.descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
                        return index++;
                    }
                    return 0U;
                });
        }
        if (binder.bind >= descriptorSetResources.size()) {
            PLUGIN_LOG_W(
                "Desctiptor count mismatch pipeline %u, binding %zu", binder.bind, descriptorSetResources.size());
            continue;
        }
        auto& curRes = descriptorSetResources[binder.bind];
#if RENDER_VALIDATION_ENABLED
        if (binder.descriptors.count != curRes.resources.size()) {
            PLUGIN_LOG_W(
                "Desctiptor size mismatch pipeline %u, binding %zu", binder.descriptors.count, curRes.resources.size());
        }

        auto descriptorType = curRes.descriptorType;
        if (descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            descriptorType = CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        } else if (descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            descriptorType = CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        }
        if (binder.type != descriptorType) {
            PLUGIN_LOG_W("Desctiptor TYPE mismatch pipeline %x, binding %x", binder.type, descriptorType);
        }
#endif
        auto descriptorIndex =
            array_view(shaderBindings->descriptorIndexIds.data() + binder.descriptors.index, binder.descriptors.count);
        switch (curRes.descriptorType) {
            case CORE_DESCRIPTOR_TYPE_SAMPLER: {
                BindSampler(curRes.resources, binder, descriptorIndex, shaderBindings->ids);
                break;
            }

            case CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            case CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
                BindTexture(curRes.resources, binder, descriptorIndex, shaderBindings->ids, curRes.descriptorType);
                break;
            }
            case CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
                BindBuffer(curRes.resources, binder, descriptorIndex, shaderBindings->ids, curRes.descriptorType);
                break;
            }
            case CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            case CORE_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            case CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE:
            case CORE_DESCRIPTOR_TYPE_MAX_ENUM:
                break;
        }
    }
}

void RenderBackendGLES::RenderCommandBeginDebugMarker(const RenderCommandWithType& ref)
{
#if (RENDER_DEBUG_MARKERS_ENABLED == 1)
    const auto& renderCmd = *static_cast<const struct RenderCommandBeginDebugMarker*>(ref.rc);
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, (const GLchar*)renderCmd.name.data());
#endif
}

void RenderBackendGLES::RenderCommandEndDebugMarker(const RenderCommandWithType&)
{
#if (RENDER_DEBUG_MARKERS_ENABLED == 1)
    glPopDebugGroup();
#endif
}

#if (RENDER_PERF_ENABLED == 1)
void RenderBackendGLES::StartFrameTimers(const RenderCommandFrameData& renderCommandFrameData)
{
    framePerfCounters_ = {};
    for (const auto& renderCommandContext : renderCommandFrameData.renderCommandContexts) {
        const string_view& debugName = renderCommandContext.debugName;
        if (timers_.count(debugName) == 0) { // new timers
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
            PerfDataSet& perfDataSet = timers_[debugName];
            constexpr GpuQueryDesc desc { QueryType::CORE_QUERY_TYPE_TIMESTAMP, 0 };
            perfDataSet.gpuHandle = gpuQueryMgr_->Create(debugName, CreateGpuQueryGLES(device_, desc));
            perfDataSet.counter = 0u;
#else
            timers_.insert({ debugName, {} });
#endif
        }
    }
}

void RenderBackendGLES::EndFrameTimers()
{
    int64_t fullGpuTime = 0;
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
    // already in micros
    fullGpuTime = fullGpuCounter_;
    fullGpuCounter_ = 0;
#endif
    if (CORE_NS::IPerformanceDataManagerFactory* globalPerfData =
            CORE_NS::GetInstance<CORE_NS::IPerformanceDataManagerFactory>(CORE_NS::UID_PERFORMANCE_FACTORY);
        globalPerfData) {
        CORE_NS::IPerformanceDataManager* perfData = globalPerfData->Get("RENDER");
        perfData->UpdateData("RenderBackend", "Full_Cpu", commonCpuTimers_.full.GetMicroseconds());
        perfData->UpdateData("RenderBackend", "Acquire_Cpu", commonCpuTimers_.acquire.GetMicroseconds());
        perfData->UpdateData("RenderBackend", "Execute_Cpu", commonCpuTimers_.execute.GetMicroseconds());
        perfData->UpdateData("RenderBackend", "Submit_Cpu", commonCpuTimers_.submit.GetMicroseconds());
        perfData->UpdateData("RenderBackend", "Present_Cpu", commonCpuTimers_.present.GetMicroseconds());
        perfData->UpdateData("RenderBackend", "Full_Gpu", fullGpuTime);

        CORE_PROFILER_PLOT("Full_Cpu", static_cast<int64_t>(commonCpuTimers_.full.GetMicroseconds()));
        CORE_PROFILER_PLOT("Acquire_Cpu", static_cast<int64_t>(commonCpuTimers_.acquire.GetMicroseconds()));
        CORE_PROFILER_PLOT("Execute_Cpu", static_cast<int64_t>(commonCpuTimers_.execute.GetMicroseconds()));
        CORE_PROFILER_PLOT("Submit_Cpu", static_cast<int64_t>(commonCpuTimers_.submit.GetMicroseconds()));
        CORE_PROFILER_PLOT("Present_Cpu", static_cast<int64_t>(commonCpuTimers_.present.GetMicroseconds()));
        CORE_PROFILER_PLOT("Full_Gpu", static_cast<int64_t>(fullGpuTime));
    }

    CORE_PROFILER_PLOT("Instance count", static_cast<int64_t>(framePerfCounters_.instanceCount));
    CORE_PROFILER_PLOT("Triangle count", static_cast<int64_t>(framePerfCounters_.triangleCount));
    CORE_PROFILER_PLOT("Draw count", static_cast<int64_t>(framePerfCounters_.drawCount));
    CORE_PROFILER_PLOT("Draw Indirect count", static_cast<int64_t>(framePerfCounters_.drawIndirectCount));
    CORE_PROFILER_PLOT("Dispatch count", static_cast<int64_t>(framePerfCounters_.dispatchCount));
    CORE_PROFILER_PLOT("Dispatch Indirect count", static_cast<int64_t>(framePerfCounters_.dispatchIndirectCount));
    CORE_PROFILER_PLOT("RenderPass count", static_cast<int64_t>(framePerfCounters_.renderPassCount));
    CORE_PROFILER_PLOT("Bind program count", static_cast<int64_t>(framePerfCounters_.bindProgram));
    CORE_PROFILER_PLOT("Bind sampler count", static_cast<int64_t>(framePerfCounters_.bindSampler));
    CORE_PROFILER_PLOT("Bind texture count", static_cast<int64_t>(framePerfCounters_.bindTexture));
    CORE_PROFILER_PLOT("Bind buffer count", static_cast<int64_t>(framePerfCounters_.bindBuffer));
}

void RenderBackendGLES::CopyPerfTimeStamp(const string_view name, PerfDataSet& perfDataSet)
{
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
    int64_t gpuMicroSeconds = 0;
    if (validGpuQueries_) {
        GpuQuery* gpuQuery = gpuQueryMgr_->Get(perfDataSet.gpuHandle);
        PLUGIN_ASSERT(gpuQuery);

        gpuQuery->NextQueryIndex();

        const auto& platData = static_cast<const GpuQueryPlatformDataGLES&>(gpuQuery->GetPlatformData());
        PLUGIN_ASSERT(platData.queryObject);

        GLint disjointOccurred = 0;
#ifdef GL_GPU_DISJOINT_EXT
        // Clear disjoint error
        glGetIntegerv(GL_GPU_DISJOINT_EXT, &disjointOccurred);
#endif
        if (!disjointOccurred && (++perfDataSet.counter) > device_.GetCommandBufferingCount()) {
            GLuint64 gpuNanoSeconds = 0U;
#ifdef GL_GPU_DISJOINT_EXT
            glGetQueryObjectui64vEXT(platData.queryObject, GL_QUERY_RESULT, &gpuNanoSeconds);
#else
            glGetQueryObjectui64v(platData.queryObject, GL_QUERY_RESULT, &gpuNanoSeconds);
#endif
            static uint64_t NANOSECONDS_TO_MICROSECONDS = 1000;
            gpuMicroSeconds = static_cast<int64_t>(gpuNanoSeconds / NANOSECONDS_TO_MICROSECONDS);
            if (gpuMicroSeconds > UINT32_MAX) {
                gpuMicroSeconds = 0;
            }
            fullGpuCounter_ += gpuMicroSeconds;
        } else if (disjointOccurred) {
            PLUGIN_LOG_V("GL_GPU_DISJOINT_EXT disjoint occurred.");
        }
    }
#endif
    const int64_t cpuMicroSeconds = perfDataSet.cpuTimer.GetMicroseconds();

    if (CORE_NS::IPerformanceDataManagerFactory* globalPerfData =
            CORE_NS::GetInstance<CORE_NS::IPerformanceDataManagerFactory>(CORE_NS::UID_PERFORMANCE_FACTORY);
        globalPerfData) {
        CORE_NS::IPerformanceDataManager* perfData = globalPerfData->Get("RenderNode");

        perfData->UpdateData(name, "Backend_Cpu", cpuMicroSeconds);
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
        perfData->UpdateData(name, "Backend_Gpu", gpuMicroSeconds);
#endif
        perfData->UpdateData(name, "Backend_Count_Triangle", perfCounters_.triangleCount,
            CORE_NS::IPerformanceDataManager::PerformanceTimingData::DataType::COUNT);
        perfData->UpdateData(name, "Backend_Count_InstanceCount", perfCounters_.instanceCount,
            CORE_NS::IPerformanceDataManager::PerformanceTimingData::DataType::COUNT);
        perfData->UpdateData(name, "Backend_Count_Draw", perfCounters_.drawCount,
            CORE_NS::IPerformanceDataManager::PerformanceTimingData::DataType::COUNT);
        perfData->UpdateData(name, "Backend_Count_DrawIndirect", perfCounters_.drawIndirectCount,
            CORE_NS::IPerformanceDataManager::PerformanceTimingData::DataType::COUNT);
        perfData->UpdateData(name, "Backend_Count_Dispatch", perfCounters_.dispatchCount,
            CORE_NS::IPerformanceDataManager::PerformanceTimingData::DataType::COUNT);
        perfData->UpdateData(name, "Backend_Count_DispatchIndirect", perfCounters_.dispatchIndirectCount,
            CORE_NS::IPerformanceDataManager::PerformanceTimingData::DataType::COUNT);
        perfData->UpdateData(name, "Backend_Count_RenderPass", perfCounters_.renderPassCount,
            CORE_NS::IPerformanceDataManager::PerformanceTimingData::DataType::COUNT);
        perfData->UpdateData(name, "Backend_Count_BindProgram", perfCounters_.bindProgram,
            CORE_NS::IPerformanceDataManager::PerformanceTimingData::DataType::COUNT);
        perfData->UpdateData(name, "Backend_Count_BindSample", perfCounters_.bindSampler,
            CORE_NS::IPerformanceDataManager::PerformanceTimingData::DataType::COUNT);
        perfData->UpdateData(name, "Backend_Count_BindTexture", perfCounters_.bindTexture,
            CORE_NS::IPerformanceDataManager::PerformanceTimingData::DataType::COUNT);
        perfData->UpdateData(name, "Backend_Count_BindBuffer", perfCounters_.bindBuffer,
            CORE_NS::IPerformanceDataManager::PerformanceTimingData::DataType::COUNT);
        framePerfCounters_.drawCount += perfCounters_.drawCount;
        framePerfCounters_.drawIndirectCount += perfCounters_.drawIndirectCount;
        framePerfCounters_.dispatchCount += perfCounters_.dispatchCount;
        framePerfCounters_.dispatchIndirectCount += perfCounters_.dispatchIndirectCount;
        framePerfCounters_.renderPassCount += perfCounters_.renderPassCount;
        framePerfCounters_.bindProgram += perfCounters_.bindProgram;
        framePerfCounters_.bindSampler += perfCounters_.bindSampler;
        framePerfCounters_.bindTexture += perfCounters_.bindTexture;
        framePerfCounters_.bindBuffer += perfCounters_.bindBuffer;
        framePerfCounters_.triangleCount += perfCounters_.triangleCount;
        framePerfCounters_.instanceCount += perfCounters_.instanceCount;
    }
}
#endif

void RenderBackendGLES::PrimeDepthStencilState(const GraphicsState& graphicsState)
{
    auto& cDepth = cacheState_.depthStencilState;
    cDepth = graphicsState.depthStencilState;
    // CORE_DYNAMIC_STATE_DEPTH_BOUNDS NOT SUPPORTED ON GLES. (and not implemented on GL either)
    SetState(GL_DEPTH_TEST, cDepth.enableDepthTest);
    SetState(GL_STENCIL_TEST, cDepth.enableStencilTest);
    glDepthFunc(GetCompareOp(cDepth.depthCompareOp));
    glDepthMask((cDepth.enableDepthWrite ? static_cast<GLboolean>(GL_TRUE) : static_cast<GLboolean>(GL_FALSE)));
    const uint32_t updateAllFlags =
        (StencilSetFlags::SETOP | StencilSetFlags::SETCOMPAREMASK | StencilSetFlags::SETCOMPAREOP |
            StencilSetFlags::SETREFERENCE | StencilSetFlags::SETWRITEMASK);
    SetStencilState(updateAllFlags, cDepth.frontStencilOpState, updateAllFlags, cDepth.backStencilOpState);
}

void RenderBackendGLES::PrimeBlendState(const GraphicsState& graphicsState)
{
    auto& cBlend = cacheState_.colorBlendState;
    cBlend = graphicsState.colorBlendState;
    glBlendColor(cBlend.colorBlendConstants[Gles::RED_INDEX], cBlend.colorBlendConstants[Gles::GREEN_INDEX],
        cBlend.colorBlendConstants[Gles::BLUE_INDEX], cBlend.colorBlendConstants[Gles::ALPHA_INDEX]);
    GLuint maxColorAttachments;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, (GLint*)&maxColorAttachments);
    maxColorAttachments = BASE_NS::Math::min(PipelineStateConstants::MAX_COLOR_ATTACHMENT_COUNT, maxColorAttachments);
    for (GLuint i = 0; i < maxColorAttachments; i++) {
        const auto& cBlendState = cBlend.colorAttachments[i];
        glColorMaski(i, IS_BIT_GL(cBlendState.colorWriteMask, CORE_COLOR_COMPONENT_R_BIT),
            IS_BIT_GL(cBlendState.colorWriteMask, CORE_COLOR_COMPONENT_G_BIT),
            IS_BIT_GL(cBlendState.colorWriteMask, CORE_COLOR_COMPONENT_B_BIT),
            IS_BIT_GL(cBlendState.colorWriteMask, CORE_COLOR_COMPONENT_A_BIT));
        if (cBlendState.enableBlend) {
            glEnablei(GL_BLEND, i);
        } else {
            glDisablei(GL_BLEND, i);
        }
        glBlendFuncSeparatei(i, GetBlendFactor(cBlendState.srcColorBlendFactor),
            GetBlendFactor(cBlendState.dstColorBlendFactor), GetBlendFactor(cBlendState.srcAlphaBlendFactor),
            GetBlendFactor(cBlendState.dstAlphaBlendFactor));
        glBlendEquationSeparatei(i, GetBlendOp(cBlendState.colorBlendOp), GetBlendOp(cBlendState.alphaBlendOp));
    }
    // logicops are unsupported on GLES
}

void RenderBackendGLES::PrimeCache(const GraphicsState& graphicsState) // Forces the graphics state..
{
    if (cachePrimed_) {
        return;
    }
    cachePrimed_ = true;
    /// GRAPHICSSTATE     inputAssembly
    const auto& ia = graphicsState.inputAssembly;
    auto& cia = cacheState_.inputAssembly;
    cia.enablePrimitiveRestart = ia.enablePrimitiveRestart;
    SetState(GL_PRIMITIVE_RESTART_FIXED_INDEX, ia.enablePrimitiveRestart);
    topology_ = ia.primitiveTopology;
    /// GRAPHICSSTATE     rasterizationState
    const auto& rs = graphicsState.rasterizationState;
    auto& crs = cacheState_.rasterizationState;
    // save, since we need to hack for non fill modes etc ! (possibly need shader help for lines...)
    polygonMode_ = rs.polygonMode;
    // GL_DEPTH_CLAMP,rs.enableDepthClamp NOT SUPPORTED    CHECK GLES 3.2
    crs.enableRasterizerDiscard = rs.enableRasterizerDiscard;
    SetState(GL_RASTERIZER_DISCARD, rs.enableRasterizerDiscard);
    crs.enableDepthBias = rs.enableDepthBias;
    SetState(GL_POLYGON_OFFSET_FILL, rs.enableDepthBias);
    crs.depthBiasConstantFactor = rs.depthBiasConstantFactor;
    crs.depthBiasSlopeFactor = rs.depthBiasSlopeFactor;
    glPolygonOffset(rs.depthBiasSlopeFactor, rs.depthBiasConstantFactor);
    // depthBiasClamp NOT SUPPORTED! CHECK GLES 3.2
    // If cull mode Flags change...
    crs.cullModeFlags = rs.cullModeFlags;
    SetCullMode(crs);
    crs.frontFace = rs.frontFace;
    SetFrontFace(crs);
    crs.lineWidth = rs.lineWidth;
    glLineWidth(rs.lineWidth);
    PrimeDepthStencilState(graphicsState);
    PrimeBlendState(graphicsState);
}

void RenderBackendGLES::UpdateDepthState(const GraphicsState& graphicsState)
{
    const auto& depth = graphicsState.depthStencilState;
    auto& cDepth = cacheState_.depthStencilState;
    if (depth.enableDepthTest != cDepth.enableDepthTest) {
        cDepth.enableDepthTest = depth.enableDepthTest;
        SetState(GL_DEPTH_TEST, depth.enableDepthTest);
    }
    if (depth.depthCompareOp != cDepth.depthCompareOp) {
        cDepth.depthCompareOp = depth.depthCompareOp;
        glDepthFunc(GetCompareOp(depth.depthCompareOp));
    }
    if (depth.enableDepthWrite != cDepth.enableDepthWrite) {
        cDepth.enableDepthWrite = depth.enableDepthWrite;
        glDepthMask((depth.enableDepthWrite == GL_TRUE));
    }
    if (!IS_BIT(dynamicStateFlags_, CORE_DYNAMIC_STATE_DEPTH_BOUNDS)) {
        // CORE_DYNAMIC_STATE_DEPTH_BOUNDS not supported on GLES.
    }
}

void RenderBackendGLES::UpdateStencilState(const GraphicsState& graphicsState)
{
    const auto& depth = graphicsState.depthStencilState;
    auto& cDepth = cacheState_.depthStencilState;
    if (depth.enableStencilTest != cDepth.enableStencilTest) {
        cDepth.enableStencilTest = depth.enableStencilTest;
        SetState(GL_STENCIL_TEST, depth.enableStencilTest);
    }
    uint32_t setFront = 0;
    uint32_t setBack = 0;
    if (!IS_BIT(dynamicStateFlags_, CORE_DYNAMIC_STATE_STENCIL_REFERENCE)) {
        if (cDepth.frontStencilOpState.reference != depth.frontStencilOpState.reference) {
            setFront |= StencilSetFlags::SETREFERENCE;
        }
        if (cDepth.backStencilOpState.reference != depth.backStencilOpState.reference) {
            setBack |= StencilSetFlags::SETREFERENCE;
        }
    }
    if (!IS_BIT(dynamicStateFlags_, CORE_DYNAMIC_STATE_STENCIL_COMPARE_MASK)) {
        if (cDepth.frontStencilOpState.compareMask != depth.frontStencilOpState.compareMask) {
            setFront |= StencilSetFlags::SETCOMPAREMASK;
        }
        if (cDepth.backStencilOpState.compareMask != depth.backStencilOpState.compareMask) {
            setBack |= StencilSetFlags::SETCOMPAREMASK;
        }
    }
    if (!IS_BIT(dynamicStateFlags_, CORE_DYNAMIC_STATE_STENCIL_WRITE_MASK)) {
        if (cDepth.frontStencilOpState.writeMask != depth.frontStencilOpState.writeMask) {
            setFront |= StencilSetFlags::SETWRITEMASK;
        }
        if (cDepth.backStencilOpState.writeMask != depth.backStencilOpState.writeMask) {
            setBack |= StencilSetFlags::SETWRITEMASK;
        }
    }
    if (cDepth.frontStencilOpState.compareOp != depth.frontStencilOpState.compareOp) {
        setFront |= StencilSetFlags::SETCOMPAREOP;
    }
    if (cDepth.backStencilOpState.compareOp != depth.backStencilOpState.compareOp) {
        setBack |= StencilSetFlags::SETCOMPAREOP;
    }
    if (!CompareStencilOp(cDepth.frontStencilOpState, depth.frontStencilOpState)) {
        setFront |= StencilSetFlags::SETOP;
    }
    if (!CompareStencilOp(cDepth.backStencilOpState, depth.backStencilOpState)) {
        setBack |= StencilSetFlags::SETOP;
    }
    SetStencilState(setFront, depth.frontStencilOpState, setBack, depth.backStencilOpState);
}

void RenderBackendGLES::UpdateDepthStencilState(const GraphicsState& graphicsState)
{
    UpdateDepthState(graphicsState);
    UpdateStencilState(graphicsState);
}

void RenderBackendGLES::UpdateBlendState(const GraphicsState& graphicsState)
{
    const auto& blend = graphicsState.colorBlendState;
    auto& cBlend = cacheState_.colorBlendState;
    for (GLuint i = 0; i < blend.colorAttachmentCount; i++) {
        const auto& blendState = blend.colorAttachments[i];
        auto& cBlendState = cBlend.colorAttachments[i];
        if (blendState.colorWriteMask != cBlendState.colorWriteMask) {
            cBlendState.colorWriteMask = blendState.colorWriteMask;
            glColorMaski(i, IS_BIT_GL(cBlendState.colorWriteMask, CORE_COLOR_COMPONENT_R_BIT),
                IS_BIT_GL(cBlendState.colorWriteMask, CORE_COLOR_COMPONENT_G_BIT),
                IS_BIT_GL(cBlendState.colorWriteMask, CORE_COLOR_COMPONENT_B_BIT),
                IS_BIT_GL(cBlendState.colorWriteMask, CORE_COLOR_COMPONENT_A_BIT));
        }

        // Check if blend state has changed
        bool factorsChanged = false;
        bool opsChanged = false;

        if (blendState.enableBlend) {
            factorsChanged = !CompareBlendFactors(cBlendState, blendState);
            opsChanged = !CompareBlendOps(cBlendState, blendState);
        }

        if (blendState.enableBlend == cBlendState.enableBlend && !factorsChanged && !opsChanged) {
            continue;
        }
        cBlendState.enableBlend = blendState.enableBlend;
        if (blendState.enableBlend) {
            glEnablei(GL_BLEND, i);
            if (factorsChanged) {
                SetBlendFactors(cBlendState, blendState);
                glBlendFuncSeparatei(i, GetBlendFactor(cBlendState.srcColorBlendFactor),
                    GetBlendFactor(cBlendState.dstColorBlendFactor), GetBlendFactor(cBlendState.srcAlphaBlendFactor),
                    GetBlendFactor(cBlendState.dstAlphaBlendFactor));
            }
            if (opsChanged) {
                SetBlendOps(cBlendState, blendState);
                glBlendEquationSeparatei(i, GetBlendOp(cBlendState.colorBlendOp), GetBlendOp(cBlendState.alphaBlendOp));
            }
        } else {
            glDisablei(GL_BLEND, i);
        }
    }
    if (!IS_BIT(dynamicStateFlags_, CORE_DYNAMIC_STATE_BLEND_CONSTANTS)) {
        if (!Compare(cBlend.colorBlendConstants, blend.colorBlendConstants)) {
            Set(cBlend.colorBlendConstants, blend.colorBlendConstants);
            glBlendColor(blend.colorBlendConstants[Gles::RED_INDEX], blend.colorBlendConstants[Gles::GREEN_INDEX],
                blend.colorBlendConstants[Gles::BLUE_INDEX], blend.colorBlendConstants[Gles::ALPHA_INDEX]);
        }
    }
    // logicOps in blend not supported on GLES
}

void RenderBackendGLES::UpdateRasterizationState(const GraphicsState& graphicsState)
{
    const auto& rs = graphicsState.rasterizationState;
    auto& crs = cacheState_.rasterizationState;
    // save, since we need to hack for non fill modes etc ! (possibly need shader help for lines...)
    polygonMode_ = rs.polygonMode;
#if RENDER_HAS_GL_BACKEND
    if (rs.polygonMode != crs.polygonMode) {
        crs.polygonMode = rs.polygonMode;
        SetPolygonMode(rs);
    }
#endif
    if (rs.enableDepthClamp != crs.enableDepthClamp) {
        crs.enableDepthClamp = rs.enableDepthClamp;
        // NOT SUPPORTED    (needs an extension)
    }
    if (rs.enableRasterizerDiscard != crs.enableRasterizerDiscard) {
        crs.enableRasterizerDiscard = rs.enableRasterizerDiscard;
        SetState(GL_RASTERIZER_DISCARD, rs.enableRasterizerDiscard);
    }
    if (rs.enableDepthBias != crs.enableDepthBias) {
        crs.enableDepthBias = rs.enableDepthBias;
        SetState(GL_POLYGON_OFFSET_FILL, rs.enableDepthBias);
    }
    if (!IS_BIT(dynamicStateFlags_, CORE_DYNAMIC_STATE_DEPTH_BIAS)) {
        if ((rs.depthBiasConstantFactor != crs.depthBiasConstantFactor) ||
            (rs.depthBiasSlopeFactor != crs.depthBiasSlopeFactor)) {
            crs.depthBiasConstantFactor = rs.depthBiasConstantFactor;
            crs.depthBiasSlopeFactor = rs.depthBiasSlopeFactor;
            glPolygonOffset(rs.depthBiasSlopeFactor, rs.depthBiasConstantFactor);
        }
        // depthBiasClamp NOT SUPPORTED    (needs an extension)
    }
    // If cull mode Flags change...
    if (rs.cullModeFlags != crs.cullModeFlags) {
        crs.cullModeFlags = rs.cullModeFlags;
        SetCullMode(crs);
    }
    auto frontFace = rs.frontFace;
    if (!renderingToDefaultFbo_) {
        // Flip winding for default fbo.
        if (frontFace == FrontFace::CORE_FRONT_FACE_COUNTER_CLOCKWISE) {
            frontFace = FrontFace::CORE_FRONT_FACE_CLOCKWISE;
        } else if (frontFace == FrontFace::CORE_FRONT_FACE_CLOCKWISE) {
            frontFace = FrontFace::CORE_FRONT_FACE_COUNTER_CLOCKWISE;
        }
    }
    if (frontFace != crs.frontFace) {
        crs.frontFace = frontFace;
        SetFrontFace(crs);
    }
    if (!IS_BIT(dynamicStateFlags_, CORE_DYNAMIC_STATE_LINE_WIDTH)) {
        if (rs.lineWidth != crs.lineWidth) {
            crs.lineWidth = rs.lineWidth;
            glLineWidth(rs.lineWidth);
        }
    }
}

void RenderBackendGLES::DoGraphicsState(const GraphicsState& graphicsState)
{
    /// GRAPHICSSTATE     inputAssembly
    const auto& ia = graphicsState.inputAssembly;
    if (ia.enablePrimitiveRestart != graphicsState.inputAssembly.enablePrimitiveRestart) {
        auto& cia = cacheState_.inputAssembly;
        cia.enablePrimitiveRestart = ia.enablePrimitiveRestart;
        SetState(GL_PRIMITIVE_RESTART_FIXED_INDEX, ia.enablePrimitiveRestart);
    }
    topology_ = ia.primitiveTopology;
    UpdateRasterizationState(graphicsState);
    UpdateDepthStencilState(graphicsState);
    UpdateBlendState(graphicsState);
}

void RenderBackendGLES::SetViewport(const ViewportDesc& vd)
{
    const bool viewportPrimed = BASE_NS::exchange(viewportPrimed_, true);
    const bool updateV = (!viewportPrimed) || ((vd.x != viewport_.x) || (vd.y != viewport_.y) ||
                                                  (vd.width != viewport_.width) || (vd.height != viewport_.height));
    const bool updateD =
        (!viewportPrimed) || ((vd.minDepth != viewport_.minDepth) || (vd.maxDepth != viewport_.maxDepth));

    if (updateV) {
        viewport_.x = vd.x;
        viewport_.y = vd.y;
        viewport_.width = vd.width;
        viewport_.height = vd.height;
        // Handle top-left / bottom-left origin conversion
        auto y = static_cast<GLint>(vd.y);
        const auto h = static_cast<GLsizei>(vd.height);
        if (renderingToDefaultFbo_) {
            if (currentFrameBuffer_) {
                const auto fh = static_cast<GLint>(currentFrameBuffer_->height);
                y = fh - (y + h);
                glViewport(static_cast<GLint>(vd.x), y, static_cast<GLsizei>(vd.width), h);
            } else {
                viewportPending_ = true;
            }
        } else {
            glViewport(static_cast<GLint>(vd.x), y, static_cast<GLsizei>(vd.width), h);
        }
    }
    if (updateD) {
        viewport_.minDepth = vd.minDepth;
        viewport_.maxDepth = vd.maxDepth;
        glDepthRangef(vd.minDepth, vd.maxDepth);
    }
}

void RenderBackendGLES::SetScissor(const ScissorDesc& sd)
{
    // NOTE: scissordesc is in floats?!?
    const bool scissorPrimed = BASE_NS::exchange(scissorPrimed_, true);
    const bool updateS =
        (!scissorPrimed) ||
        ((sd.offsetX != scissorBox_.offsetX) || (sd.offsetY != scissorBox_.offsetY) ||
            (sd.extentWidth != scissorBox_.extentWidth) || (sd.extentHeight != scissorBox_.extentHeight));
    if (updateS) {
        scissorBox_ = sd;
        // Handle top-left / bottom-left origin conversion
        auto y = static_cast<GLint>(sd.offsetY);
        const auto h = static_cast<GLsizei>(sd.extentHeight);
        if (renderingToDefaultFbo_) {
            const auto fh = static_cast<GLint>(currentFrameBuffer_->height);
            y = fh - (y + h);
        }
        glScissor(static_cast<GLint>(sd.offsetX), y, static_cast<GLsizei>(sd.extentWidth), h);
    }
}
RENDER_END_NAMESPACE()
