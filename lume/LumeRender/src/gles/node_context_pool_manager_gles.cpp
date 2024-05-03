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

#include "node_context_pool_manager_gles.h"

#include <algorithm>
#include <numeric>

#include <render/namespace.h>

#include "device/gpu_resource_manager.h"
#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "gles/gpu_image_gles.h"
#include "gles/swapchain_gles.h"
#include "nodecontext/render_command_list.h" // RenderPassBeginInfo...
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr const bool VERBOSE_LOGGING = false;
constexpr const bool HASH_LAYOUTS = false;
constexpr const uint32_t EMPTY_ATTACHMENT = ~0u;

struct BindImage {
    uint32_t layer;
    uint32_t mipLevel;
    const GpuImageGLES* image;
};

struct FboHash {
    uint64_t hash;
    GLuint fbo;
};

uint32_t HighestBit(uint32_t value)
{
    uint32_t count = 0;
    while (value) {
        ++count;
        value >>= 1U;
    }
    return count;
}

void UpdateBindImages(const RenderCommandBeginRenderPass& beginRenderPass, array_view<BindImage> images,
    GpuResourceManager& gpuResourceMgr_)
{
    const auto& renderPassDesc = beginRenderPass.renderPassDesc;
    for (uint32_t idx = 0; idx < renderPassDesc.attachmentCount; ++idx) {
        images[idx].layer = renderPassDesc.attachments[idx].layer;
        images[idx].mipLevel = renderPassDesc.attachments[idx].mipLevel;
        images[idx].image = gpuResourceMgr_.GetImage<GpuImageGLES>(renderPassDesc.attachmentHandles[idx]);
    }
}

uint64_t HashRPD(const RenderCommandBeginRenderPass& beginRenderPass, GpuResourceManager& gpuResourceMgr_)
{
    const auto& renderPassDesc = beginRenderPass.renderPassDesc;
    uint64_t rpHash = 0;
    // hash engine gpu handle
    {
        for (uint32_t idx = 0; idx < renderPassDesc.attachmentCount; ++idx) {
            HashCombine(rpHash, renderPassDesc.attachments[idx].layer);
            HashCombine(rpHash, renderPassDesc.attachments[idx].mipLevel);
            const EngineResourceHandle gpuHandle = gpuResourceMgr_.GetGpuHandle(renderPassDesc.attachmentHandles[idx]);
            HashCombine(rpHash, gpuHandle.id);
        }
    }

    // hash input and output layouts (ignored since, they do not contribute in GL/GLES at all, they are a artifact
    // of vulkan)
    if constexpr (HASH_LAYOUTS) {
        const RenderPassImageLayouts& renderPassImageLayouts = beginRenderPass.imageLayouts;
        for (uint32_t idx = 0; idx < renderPassDesc.attachmentCount; ++idx) {
            HashCombine(rpHash, renderPassImageLayouts.attachmentInitialLayouts[idx],
                renderPassImageLayouts.attachmentFinalLayouts[idx]);
        }
    }

    // hash subpasses
    PLUGIN_ASSERT(renderPassDesc.subpassCount <= beginRenderPass.subpasses.size());
    for (const RenderPassSubpassDesc& subpass : beginRenderPass.subpasses) {
        HashRange(
            rpHash, subpass.inputAttachmentIndices, subpass.inputAttachmentIndices + subpass.inputAttachmentCount);
        HashRange(
            rpHash, subpass.colorAttachmentIndices, subpass.colorAttachmentIndices + subpass.colorAttachmentCount);
        if (subpass.depthAttachmentCount) {
            HashCombine(rpHash, (uint64_t)subpass.depthAttachmentIndex);
        }
        HashCombine(rpHash, (uint64_t)subpass.viewMask);
    }
    return rpHash;
}

bool VerifyFBO()
{
    GLenum status;
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        // failed!
#if (RENDER_VALIDATION_ENABLED == 1)
        switch (status) {
            case GL_FRAMEBUFFER_UNDEFINED:
                // is returned if target is the default framebuffer, but the default framebuffer does not exist.
                PLUGIN_LOG_E("Framebuffer undefined");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                // is returned if any of the framebuffer attachment points are framebuffer incomplete.
                PLUGIN_LOG_E("Framebuffer incomplete attachment");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                // is returned if the framebuffer does not haveat least one image attached to it.
                PLUGIN_LOG_E("Framebuffer incomplete missing attachment");
                break;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                // is returned if depth and stencil attachments, if present, are not the same renderbuffer, or
                // if the combination of internal formats of the attached images violates an
                // implementation-dependent set of restrictions.
                PLUGIN_LOG_E("Framebuffer unsupported");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                // is returned if the value of GL_RENDERBUFFER_SAMPLES  is not the same for all attached
                // renderbuffers or, if the attached images are a mix of renderbuffers and textures, the value
                // of GL_RENDERBUFFER_SAMPLES is not zero.
                PLUGIN_LOG_E("Framebuffer incomplete multisample");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
                // is returned if any framebuffer attachment is layered, and any populated attachment is not
                // layered, or if all populated color attachments are not from textures of the same target.
                PLUGIN_LOG_E("Framebuffer incomplete layer targets");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_VIEW_TARGETS_OVR:
                PLUGIN_LOG_E("Framebuffer incomplete view targets");
                break;
            default: {
                PLUGIN_LOG_E("Framebuffer other error: %x", status);
                break;
            }
        }
#endif
        return false;
    }
    return true;
}

bool HasStencil(const GpuImageGLES* image)
{
    const GpuImagePlatformDataGL& dplat = static_cast<const GpuImagePlatformDataGL&>(image->GetPlatformData());
    return (dplat.format == GL_STENCIL_INDEX) || (dplat.format == GL_DEPTH_STENCIL);
}

bool HasDepth(const GpuImageGLES* image)
{
    const GpuImagePlatformDataGL& dplat = static_cast<const GpuImagePlatformDataGL&>(image->GetPlatformData());
    return (dplat.format == GL_DEPTH_COMPONENT) || (dplat.format == GL_DEPTH_STENCIL);
}

#if RENDER_GL_FLIP_Y_SWAPCHAIN == 0
bool IsDefaultAttachment(array_view<const BindImage> images, const RenderPassSubpassDesc& sb)
{
    // Valid backbuffer configurations are.
    // 1. color only
    // 2. color + depth (stencil).
    // It is not allowed to mix custom render targets and backbuffer!
    if (sb.colorAttachmentCount == 1) {
        // okay, looks good. one color...
        if (const auto* color = images[sb.colorAttachmentIndices[0]].image) {
            const auto& plat = static_cast<const GpuImagePlatformDataGL&>(color->GetPlatformData());
            if ((plat.image == 0) &&                // not texture
                (plat.renderBuffer == 0))           // not renderbuffer
            {                                       // Colorattachment is backbuffer
                if (sb.depthAttachmentCount == 1) { // and has one depth.
                    const auto depth = images[sb.depthAttachmentIndex].image;
                    const auto& dPlat = static_cast<const GpuImagePlatformDataGL&>(depth->GetPlatformData());
                    // NOTE: CORE_DEFAULT_BACKBUFFER_DEPTH is not used legacy way anymore
                    if ((dPlat.image == 0) && (dPlat.renderBuffer == 0)) { // depth attachment is backbuffer depth.
                        return true;
                    } else {
                        // Invalid configuration. (this should be caught earlier already)
#if (RENDER_VALIDATION_ENABLED == 1)
                        PLUGIN_LOG_ONCE_I("backbuffer_depth_gles_mixing" + to_string(plat.image),
                            "RENDER_VALIDATION: Depth target dropped due to using swapchain buffer (depth)");
#endif
                        return true;
                    }
                } else {
                    return true;
                }
            }
        }
    }
    return false;
}

bool IsDefaultResolve(array_view<const BindImage> images, const RenderPassSubpassDesc& sb)
{
    if (sb.resolveAttachmentCount == 1 && sb.depthResolveAttachmentCount == 0) {
        // looks good, one color
        if (sb.resolveAttachmentIndices[0U] < static_cast<uint32_t>(images.size())) {
            if (const GpuImageGLES* color = images[sb.resolveAttachmentIndices[0]].image; color) {
                const GpuImagePlatformDataGL& plat =
                    static_cast<const GpuImagePlatformDataGL&>(color->GetPlatformData());
                if ((plat.image == 0) && (plat.renderBuffer == 0)) {
                    return true;
                }
            }
        }
    }
    if (sb.depthResolveAttachmentCount == 1) {
        if (sb.depthResolveAttachmentIndex < static_cast<uint32_t>(images.size())) {
            // looks good, one depth
            if (const GpuImageGLES* depth = images[sb.depthResolveAttachmentIndex].image; depth) {
                const GpuImagePlatformDataGL& plat =
                    static_cast<const GpuImagePlatformDataGL&>(depth->GetPlatformData());
                if ((plat.image == 0) && (plat.renderBuffer == 0)) {
                    return true;
                }
            }
        }
    }
    return false;
}
#endif

void DeleteFbos(DeviceGLES& device, LowlevelFramebufferGL& ref)
{
    for (uint32_t i = 0; i < ref.fbos.size(); i++) {
        GLuint f, r;
        f = ref.fbos[i].fbo;
        r = ref.fbos[i].resolve;
        if (f != 0) {
            device.DeleteFrameBuffer(f);
        }
        if (r != 0) {
            device.DeleteFrameBuffer(r);
        }
        // the same fbos can be used multiple render passes, so clean those references too.
        for (uint32_t j = 0; j < ref.fbos.size(); j++) {
            if (f == ref.fbos[j].fbo) {
                ref.fbos[j].fbo = 0;
            }
            if (r == ref.fbos[j].resolve) {
                ref.fbos[j].resolve = 0;
            }
        }
    }
    ref.fbos.clear();
}

void BindToFbo(
    GLenum attachType, const BindImage& image, uint32_t& width, uint32_t& height, uint32_t views, bool isStarted)
{
    const GpuImagePlatformDataGL& plat = static_cast<const GpuImagePlatformDataGL&>(image.image->GetPlatformData());
    const GpuImageDesc& desc = image.image->GetDesc();
    if (isStarted) {
#if (RENDER_VALIDATION_ENABLED == 1)
        if (width != desc.width) {
            PLUGIN_LOG_W("Attachment is not the same size (%d) as other attachments (%u)", desc.width, width);
        }
        if (height != desc.height) {
            PLUGIN_LOG_W("Attachment is not the same size (%d) as other attachments (%u)", desc.height, height);
        }
#endif
        width = Math::min(width, desc.width);
        height = Math::min(height, desc.height);
    } else {
        width = desc.width;
        height = desc.height;
    }
    const bool isSrc = (desc.usageFlags & CORE_IMAGE_USAGE_TRANSFER_SRC_BIT);
    const bool isDst = (desc.usageFlags & CORE_IMAGE_USAGE_TRANSFER_DST_BIT);
    const bool isSample = (desc.usageFlags & CORE_IMAGE_USAGE_SAMPLED_BIT);
    const bool isStorage = (desc.usageFlags & CORE_IMAGE_USAGE_STORAGE_BIT);
    // could check for bool isColor = (desc.usageFlags & CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    // could check for isDepth = (desc.usageFlags & CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    // could check for bool isTrans = (desc.usageFlags & CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
    const bool isInput = (desc.usageFlags & CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
    PLUGIN_UNUSED(isSrc);
    PLUGIN_UNUSED(isDst);
    PLUGIN_UNUSED(isSample);
    PLUGIN_UNUSED(isStorage);
    PLUGIN_UNUSED(isInput);
    if (plat.renderBuffer) {
        PLUGIN_ASSERT((!isSrc) && (!isDst) && (!isSample) && (!isStorage) && (!isInput));
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachType, GL_RENDERBUFFER, plat.renderBuffer);
    } else {
        if ((plat.type == GL_TEXTURE_2D_ARRAY) || (plat.type == GL_TEXTURE_2D_MULTISAMPLE_ARRAY)) {
            if (views) {
                glFramebufferTextureMultiviewOVR(
                    GL_FRAMEBUFFER, attachType, plat.image, (GLint)image.mipLevel, (GLint)image.layer, (GLsizei)views);
            } else {
                glFramebufferTextureLayer(
                    GL_FRAMEBUFFER, attachType, plat.image, (GLint)image.mipLevel, (GLint)image.layer);
            }
        } else {
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachType, plat.type, plat.image, (GLint)image.mipLevel);
        }
    }
}

void BindToFboMultisampled(GLenum attachType, const BindImage& image, const BindImage& resolveImage, uint32_t& width,
    uint32_t& height, uint32_t views, bool isStarted, bool multisampledRenderToTexture)
{
    const GpuImagePlatformDataGL& plat =
        static_cast<const GpuImagePlatformDataGL&>(resolveImage.image->GetPlatformData());
    const GpuImageDesc& desc = image.image->GetDesc();
    if (isStarted) {
#if (RENDER_VALIDATION_ENABLED == 1)
        if (width != desc.width) {
            PLUGIN_LOG_W("Attachment is not the same size (%d) as other attachments (%u)", desc.width, width);
        }
        if (height != desc.height) {
            PLUGIN_LOG_W("Attachment is not the same size (%d) as other attachments (%u)", desc.height, height);
        }
#endif
        width = Math::min(width, desc.width);
        height = Math::min(height, desc.height);
    } else {
        width = desc.width;
        height = desc.height;
    }
    const bool isSrc = (desc.usageFlags & CORE_IMAGE_USAGE_TRANSFER_SRC_BIT);
    const bool isDst = (desc.usageFlags & CORE_IMAGE_USAGE_TRANSFER_DST_BIT);
    const bool isSample = (desc.usageFlags & CORE_IMAGE_USAGE_SAMPLED_BIT);
    const bool isStorage = (desc.usageFlags & CORE_IMAGE_USAGE_STORAGE_BIT);
    // could check for bool isColor = (desc.usageFlags & CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    // could check for isDepth = (desc.usageFlags & CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    const bool isTrans = (desc.usageFlags & CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
    const bool isInput = (desc.usageFlags & CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
    PLUGIN_UNUSED(isSrc);
    PLUGIN_UNUSED(isDst);
    PLUGIN_UNUSED(isSample);
    PLUGIN_UNUSED(isStorage);
    PLUGIN_UNUSED(isTrans);
    PLUGIN_UNUSED(isInput);
    if (plat.renderBuffer) {
        PLUGIN_ASSERT((!isSrc) && (!isDst) && (!isSample) && (!isStorage) && (!isInput));
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachType, GL_RENDERBUFFER, plat.renderBuffer);
    } else if ((plat.type == GL_TEXTURE_2D_ARRAY) || (plat.type == GL_TEXTURE_2D_MULTISAMPLE_ARRAY)) {
#if RENDER_HAS_GLES_BACKEND
        if (views && multisampledRenderToTexture && isTrans &&
            ((plat.type == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) || (desc.sampleCountFlags & ~CORE_SAMPLE_COUNT_1_BIT))) {
            const auto samples = (desc.sampleCountFlags & CORE_SAMPLE_COUNT_8_BIT)
                                     ? 8
                                     : ((desc.sampleCountFlags & CORE_SAMPLE_COUNT_4_BIT) ? 4 : 2);
            glFramebufferTextureMultisampleMultiviewOVR(
                GL_FRAMEBUFFER, attachType, plat.image, image.mipLevel, samples, image.layer, views);
        } else {
#endif
            if (views) {
                glFramebufferTextureMultiviewOVR(
                    GL_FRAMEBUFFER, attachType, plat.image, (GLint)image.mipLevel, (GLint)image.layer, (GLsizei)views);
            } else {
                glFramebufferTextureLayer(
                    GL_FRAMEBUFFER, attachType, plat.image, (GLint)image.mipLevel, (GLint)image.layer);
            }
#if RENDER_HAS_GLES_BACKEND
        }
    } else if (multisampledRenderToTexture && isTrans &&
               ((plat.type == GL_TEXTURE_2D_MULTISAMPLE) || (desc.sampleCountFlags & ~CORE_SAMPLE_COUNT_1_BIT))) {
        const auto samples = (desc.sampleCountFlags & CORE_SAMPLE_COUNT_8_BIT)
                                 ? 8
                                 : ((desc.sampleCountFlags & CORE_SAMPLE_COUNT_4_BIT) ? 4 : 2);
        glFramebufferTexture2DMultisampleEXT(
            GL_FRAMEBUFFER, attachType, plat.type, plat.image, (GLint)image.mipLevel, samples);
#endif
    } else {
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachType, plat.type, plat.image, (GLint)image.mipLevel);
    }
}

uint64_t HashAttachments(const RenderPassSubpassDesc& sb)
{
    // generate hash for attachments.
    uint64_t subHash = 0;
    HashRange(subHash, sb.colorAttachmentIndices, sb.colorAttachmentIndices + sb.colorAttachmentCount);
    if (sb.depthAttachmentCount) {
        HashCombine(subHash, (uint64_t)sb.depthAttachmentIndex);
    }
    return subHash;
}

GLenum BindType(const GpuImageGLES* image)
{
    GLenum bindType = GL_NONE;
    const bool depth = HasDepth(image);
    const bool stencil = HasStencil(image);
    if (depth && stencil) {
        bindType = GL_DEPTH_STENCIL_ATTACHMENT;
    } else if (depth) {
        bindType = GL_DEPTH_ATTACHMENT;
    } else if (stencil) {
        bindType = GL_STENCIL_ATTACHMENT;
    }
    return bindType;
}

uint32_t GenerateSubPassFBO(DeviceGLES& device, LowlevelFramebufferGL& framebuffer, const RenderPassSubpassDesc& sb,
    const array_view<const BindImage> images, const size_t resolveAttachmentCount,
    const array_view<const uint32_t> imageMap, bool multisampledRenderToTexture)
{
    // generate fbo for subpass (depth/color).
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
    PLUGIN_LOG_D("fbo id >: %u", fbo);
#endif
    device.BindFrameBuffer(fbo);
    GLenum drawBuffers[PipelineStateConstants::MAX_COLOR_ATTACHMENT_COUNT] = { GL_NONE };
    GLenum colorAttachmentCount = 0;
    const auto views = HighestBit(sb.viewMask);
    for (uint32_t idx = 0; idx < sb.colorAttachmentCount; ++idx) {
        const uint32_t ci = sb.colorAttachmentIndices[idx];
        const uint32_t original = (ci < imageMap.size()) ? imageMap[ci] : EMPTY_ATTACHMENT;
        if (images[ci].image) {
            drawBuffers[idx] = GL_COLOR_ATTACHMENT0 + colorAttachmentCount;
            if (original == EMPTY_ATTACHMENT) {
                BindToFbo(drawBuffers[idx], images[ci], framebuffer.width, framebuffer.height, views,
                    (colorAttachmentCount) || (resolveAttachmentCount));
            } else {
                BindToFboMultisampled(drawBuffers[idx], images[original], images[ci], framebuffer.width,
                    framebuffer.height, views, (colorAttachmentCount) || (resolveAttachmentCount),
                    multisampledRenderToTexture);
            }
            ++colorAttachmentCount;
        } else {
#if (RENDER_VALIDATION_ENABLED == 1)
            PLUGIN_LOG_E("RENDER_VALIDATION: no image for color attachment %u %u", idx, ci);
#endif
            drawBuffers[idx] = GL_NONE;
        }
    }
    glDrawBuffers((GLsizei)sb.colorAttachmentCount, drawBuffers);
    if (sb.depthAttachmentCount == 1) {
        const uint32_t di = sb.depthAttachmentIndex;
        const auto* image = images[di].image;
        uint32_t original = (di < imageMap.size()) ? imageMap[di] : EMPTY_ATTACHMENT;
        if (original == EMPTY_ATTACHMENT) {
            original = di;
        }
        if (image) {
            const GLenum bindType = BindType(image);
            PLUGIN_ASSERT_MSG(bindType != GL_NONE, "Depth attachment has no stencil or depth");
            BindToFboMultisampled(bindType, images[original], images[di], framebuffer.width, framebuffer.height, views,
                (colorAttachmentCount) || (resolveAttachmentCount), multisampledRenderToTexture);
        } else {
            PLUGIN_LOG_E("no image for depth attachment");
        }
    }
    if (!VerifyFBO()) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("RENDER_VALIDATION: Failed to create subpass FBO size [%u %u] [color:%u depth:%u resolve:%u]",
            framebuffer.width, framebuffer.height, sb.colorAttachmentCount, sb.depthAttachmentCount,
            sb.resolveAttachmentCount);
#endif
        device.BindFrameBuffer(0U);
        glDeleteFramebuffers(1, &fbo);
        fbo = 0U;
    } else if constexpr (VERBOSE_LOGGING) {
        PLUGIN_LOG_V("Created subpass FBO size [%u %u] [color:%u depth:%u resolve:%u]", framebuffer.width,
            framebuffer.height, sb.colorAttachmentCount, sb.depthAttachmentCount, sb.resolveAttachmentCount);
    }
    return fbo;
}

struct ResolvePair {
    uint32_t resolveAttachmentCount;
    uint32_t resolveFbo;
};

ResolvePair GenerateResolveFBO(DeviceGLES& device, LowlevelFramebufferGL& framebuffer, const RenderPassSubpassDesc& sb,
    array_view<const BindImage> images)
{
    // generate fbos for resolve attachments if needed.
    if ((sb.resolveAttachmentCount == 0) && (sb.depthResolveAttachmentCount == 0)) {
        return { 0, 0 };
    }
#if RENDER_GL_FLIP_Y_SWAPCHAIN == 0
    // currently resolving to backbuffer AND other attachments at the same time is not possible.
    if (IsDefaultResolve(images, sb)) {
        // resolving from custom render target to default fbo.
        const auto* color = images[sb.colorAttachmentIndices[0]].image;
        if (color) {
            const auto& desc = color->GetDesc();
            framebuffer.width = desc.width;
            framebuffer.height = desc.height;
        }
        return { 1, 0 };
    }
#endif
    // all subpasses with resolve will get a special resolve fbo.. (expecting that no more than one
    // subpass resolves to a single attachment. if more than one subpass resolves to a single
    // attachment we have extra fbos.)
    ResolvePair rp { 0, 0 };
    glGenFramebuffers(1, &rp.resolveFbo);
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
    PLUGIN_LOG_D("fbo id >: %u", rp.resolveFbo);
#endif
    const auto views = HighestBit(sb.viewMask);
    device.BindFrameBuffer(rp.resolveFbo);
    GLenum drawBuffers[PipelineStateConstants::MAX_COLOR_ATTACHMENT_COUNT] = { GL_NONE };
    for (uint32_t idx = 0; idx < sb.resolveAttachmentCount; ++idx) {
        const uint32_t ci = sb.resolveAttachmentIndices[idx];
        const auto* image = images[ci].image;
        if (image) {
            drawBuffers[idx] = GL_COLOR_ATTACHMENT0 + rp.resolveAttachmentCount;
            BindToFbo(drawBuffers[idx], images[ci], framebuffer.width, framebuffer.height, views,
                (rp.resolveAttachmentCount > 0));
            ++rp.resolveAttachmentCount;
        } else {
            PLUGIN_LOG_E("no image for resolve attachment %u %u", idx, ci);
            drawBuffers[idx] = GL_NONE;
        }
    }
    glDrawBuffers((GLsizei)sb.resolveAttachmentCount, drawBuffers);
    for (uint32_t idx = 0; idx < sb.depthResolveAttachmentCount; ++idx) {
        const uint32_t ci = sb.depthResolveAttachmentIndex;
        const auto* image = images[ci].image;
        if (image) {
            BindToFbo(BindType(image), images[ci], framebuffer.width, framebuffer.height, views,
                (rp.resolveAttachmentCount > 0));
        } else {
            PLUGIN_LOG_E("no image for depth resolve attachment %u %u", idx, ci);
        }
    }
    return rp;
}

LowlevelFramebufferGL::SubPassPair ProcessSubPass(DeviceGLES& device, LowlevelFramebufferGL& framebuffer,
    vector<FboHash>& fboMap, array_view<const BindImage> images, const array_view<const uint32_t> imageMap,
    const RenderPassSubpassDesc& sb, bool multisampledRenderToTexture)
{
#if RENDER_GL_FLIP_Y_SWAPCHAIN == 0
    if (IsDefaultAttachment(images, sb)) {
        // This subpass uses backbuffer!
        const auto* color = images[sb.colorAttachmentIndices[0]].image;
        if (color) {
            const auto& desc = color->GetDesc();
            framebuffer.width = desc.width;
            framebuffer.height = desc.height;
        }
        // NOTE: it is technically possible to resolve from backbuffer to a custom render target.
        // but we do not support it now.
        if (sb.resolveAttachmentCount != 0) {
            PLUGIN_LOG_E("No resolving from default framebuffer");
        }
        return { 0, 0 };
    }
#endif
    // This subpass uses custom render targets.
    PLUGIN_ASSERT((sb.colorAttachmentCount + sb.depthAttachmentCount) <
                  (PipelineStateConstants::MAX_COLOR_ATTACHMENT_COUNT + 1)); // +1 for depth
    uint32_t fbo = 0;
    const auto resolveResult = GenerateResolveFBO(device, framebuffer, sb, images);
    const uint64_t subHash = HashAttachments(sb);
    if (const auto it = std::find_if(
            fboMap.begin(), fboMap.end(), [subHash](const FboHash& fboHash) { return fboHash.hash == subHash; });
        it != fboMap.end()) {
        // matching fbo created already, re-use
        fbo = it->fbo;
    } else {
        fbo = GenerateSubPassFBO(device, framebuffer, sb, images, resolveResult.resolveAttachmentCount, imageMap,
            multisampledRenderToTexture);
        fboMap.push_back({ subHash, fbo });
    }
    return { fbo, resolveResult.resolveFbo };
}

#if RENDER_HAS_GLES_BACKEND
void MapColorAttachments(array_view<RenderPassSubpassDesc>::iterator begin,
    array_view<RenderPassSubpassDesc>::iterator pos, array_view<const BindImage> images, array_view<uint32_t> imageMap,
    array_view<RenderPassDesc::AttachmentDesc> attachments)
{
    auto resolveAttachmentIndices = array_view(pos->resolveAttachmentIndices, pos->resolveAttachmentCount);
    const auto colorAttachmentIndices = array_view(pos->colorAttachmentIndices, pos->colorAttachmentCount);
    for (auto i = 0U; i < pos->resolveAttachmentCount; ++i) {
        const auto color = colorAttachmentIndices[i];
        // if the attachment can be used as an input attachment we can't render directly to the resolve attachment.
        if (images[color].image &&
            (images[color].image->GetDesc().usageFlags & ImageUsageFlagBits::CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)) {
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto subpassIdx = static_cast<uint32_t>(pos - begin);
            const auto unique = to_string(subpassIdx) + " " + to_string(color);
            PLUGIN_LOG_ONCE_W(unique,
                "Subpass %u attachment %u might be used as input attachment, cannot render directly to "
                "texture.",
                subpassIdx, color);
#endif
            continue;
        }
        const auto resolve = exchange(resolveAttachmentIndices[i], EMPTY_ATTACHMENT);
        // map the original attachment as the resolve
        imageMap[color] = resolve;

        // update the resolve attachment's loadOp and clearValue to match the original attachment.
        attachments[resolve].loadOp = attachments[color].loadOp;
        attachments[resolve].clearValue = attachments[color].clearValue;
    }
}

void MapDepthAttachments(array_view<RenderPassSubpassDesc>::iterator begin,
    array_view<RenderPassSubpassDesc>::iterator pos, array_view<const BindImage> images, array_view<uint32_t> imageMap,
    array_view<RenderPassDesc::AttachmentDesc> attachments)
{
    if ((pos->depthResolveAttachmentCount > 0) && (pos->depthResolveModeFlagBit || pos->stencilResolveModeFlagBit)) {
        const auto depth = pos->depthAttachmentIndex;
        // if the attachment can be used as an input attachment we can't render directly to the resolve attachment.
        if (images[depth].image &&
            (images[depth].image->GetDesc().usageFlags & ImageUsageFlagBits::CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)) {
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto subpassIdx = static_cast<uint32_t>(pos - begin);
            const auto unique = to_string(subpassIdx) + " " + to_string(depth);
            PLUGIN_LOG_ONCE_W(unique,
                "Subpass %u attachment %u might be used as input attachment, cannot render directly to "
                "texture.",
                subpassIdx, depth);
#endif
            return;
        }
        const auto resolve = pos->depthResolveAttachmentIndex;
        // map the original attachment as the resolve
        imageMap[depth] = resolve;

        // update the resolve attachment's loadOp and clearValue to match the original attachment.
        attachments[resolve].loadOp = attachments[depth].loadOp;
        attachments[resolve].clearValue = attachments[depth].clearValue;
    }
}

void UpdateSubpassAttachments(array_view<RenderPassSubpassDesc>::iterator begin,
    array_view<RenderPassSubpassDesc>::iterator end, array_view<uint32_t> imageMap)
{
    // update all the attachment indices in the subpasses according to the mapping.
    for (auto i = begin; i != end; ++i) {
        for (auto ci = 0U; ci < i->colorAttachmentCount; ++ci) {
            const auto oldColor = i->colorAttachmentIndices[ci];
            if (const auto newColor = imageMap[oldColor]; newColor != EMPTY_ATTACHMENT) {
                i->colorAttachmentIndices[ci] = newColor;
            }
        }
        // check what is the last index with a valid resolve attachment
        auto resolveCount = i->resolveAttachmentCount;
        for (; resolveCount > 0u; --resolveCount) {
            if (i->resolveAttachmentIndices[resolveCount - 1] != EMPTY_ATTACHMENT) {
                break;
            }
        }
        i->resolveAttachmentCount = resolveCount;

        const auto oldDepth = i->depthAttachmentIndex;
        if (const auto newDepth = imageMap[oldDepth]; newDepth != EMPTY_ATTACHMENT) {
            i->depthAttachmentIndex = newDepth;
            i->depthResolveAttachmentCount = 0;
        }
    }
}
#endif
} // namespace

NodeContextPoolManagerGLES::NodeContextPoolManagerGLES(Device& device, GpuResourceManager& gpuResourceManager)
    : NodeContextPoolManager(), device_ { (DeviceGLES&)device }, gpuResourceMgr_ { gpuResourceManager }
{
    bufferingCount_ = device_.GetCommandBufferingCount();
#if RENDER_HAS_GLES_BACKEND
    if (device_.GetBackendType() == DeviceBackendType::OPENGLES) {
        multisampledRenderToTexture_ = device_.HasExtension("GL_EXT_multisampled_render_to_texture2");
        multiViewMultisampledRenderToTexture_ = device_.HasExtension("GL_OVR_multiview_multisampled_render_to_texture");
    }
#endif
    multiView_ = device_.HasExtension("GL_OVR_multiview2");
}

NodeContextPoolManagerGLES::~NodeContextPoolManagerGLES()
{
    if (!framebufferCache_.framebuffers.empty()) {
        PLUGIN_ASSERT(device_.IsActive());
        for (auto& ref : framebufferCache_.framebuffers) {
            DeleteFbos(device_, ref);
        }
    }
}

void NodeContextPoolManagerGLES::BeginFrame()
{
#if (RENDER_VALIDATION_ENABLED == 1)
    frameIndexFront_ = device_.GetFrameCount();
#endif
}

void NodeContextPoolManagerGLES::BeginBackendFrame()
{
    const uint64_t frameCount = device_.GetFrameCount();

#if (RENDER_VALIDATION_ENABLED == 1)
    PLUGIN_ASSERT(frameIndexBack_ != frameCount); // prevent multiple calls per frame
    frameIndexBack_ = frameCount;
    PLUGIN_ASSERT(frameIndexFront_ == frameIndexBack_);
#endif

    // not used
    bufferingIndex_ = 0;

    const auto maxAge = 2;
    const auto minAge = device_.GetCommandBufferingCount() + maxAge;
    const auto ageLimit = (frameCount < minAge) ? 0 : (frameCount - minAge);
    const size_t limit = framebufferCache_.frameBufferFrameUseIndex.size();
    for (size_t index = 0; index < limit; ++index) {
        auto const useIndex = framebufferCache_.frameBufferFrameUseIndex[index];
        auto& ref = framebufferCache_.framebuffers[index];
        if (useIndex < ageLimit && (!ref.fbos.empty())) {
            DeleteFbos(device_, ref);
            auto const pos = std::find_if(framebufferCache_.renderPassHashToIndex.begin(),
                framebufferCache_.renderPassHashToIndex.end(),
                [index](auto const& hashToIndex) { return hashToIndex.second == index; });
            if (pos != framebufferCache_.renderPassHashToIndex.end()) {
                framebufferCache_.renderPassHashToIndex.erase(pos);
            }
        }
    }
}

EngineResourceHandle NodeContextPoolManagerGLES::GetFramebufferHandle(
    const RenderCommandBeginRenderPass& beginRenderPass)
{
    PLUGIN_ASSERT(device_.IsActive());
    const uint64_t rpHash = HashRPD(beginRenderPass, gpuResourceMgr_);
    if (const auto iter = framebufferCache_.renderPassHashToIndex.find(rpHash);
        iter != framebufferCache_.renderPassHashToIndex.cend()) {
        PLUGIN_ASSERT(iter->second < framebufferCache_.framebuffers.size());
        // store frame index for usage
        framebufferCache_.frameBufferFrameUseIndex[iter->second] = device_.GetFrameCount();
        return RenderHandleUtil::CreateEngineResourceHandle(RenderHandleType::UNDEFINED, iter->second, 0);
    }

    BindImage images[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT];
    UpdateBindImages(beginRenderPass, images, gpuResourceMgr_);

    // Create fbo for each subpass
    LowlevelFramebufferGL fb;

    const auto& rpd = beginRenderPass.renderPassDesc;
    fb.fbos.resize(rpd.subpassCount);
    if constexpr (VERBOSE_LOGGING) {
        PLUGIN_LOG_V("Creating framebuffer with %u subpasses", rpd.subpassCount);
    }
    // NOTE: we currently expect that resolve, color and depth attachments are regular 2d textures (or
    // renderbuffers)
    vector<FboHash> fboMap;
    std::transform(std::begin(beginRenderPass.subpasses), std::begin(beginRenderPass.subpasses) + rpd.subpassCount,
        fb.fbos.data(),
        [this, &fb, &fboMap, images = array_view<const BindImage>(images),
            imageMap = array_view<const uint32_t>(imageMap_)](const RenderPassSubpassDesc& subpass) {
            return ProcessSubPass(device_, fb, fboMap, images, imageMap, subpass,
                subpass.viewMask ? multiViewMultisampledRenderToTexture_ : multisampledRenderToTexture_);
        });
    if constexpr (VERBOSE_LOGGING) {
        PLUGIN_LOG_V("Created framebuffer with %u subpasses at size [%u %u]", rpd.subpassCount, fb.width, fb.height);
    }
    if (!fb.width || !fb.height) {
        return {};
    }
    uint32_t arrayIndex = 0;
    if (auto const pos = std::find_if(framebufferCache_.framebuffers.begin(), framebufferCache_.framebuffers.end(),
            [](auto const& framebuffer) { return framebuffer.fbos.empty(); });
        pos != framebufferCache_.framebuffers.end()) {
        arrayIndex = (uint32_t)std::distance(framebufferCache_.framebuffers.begin(), pos);
        *pos = move(fb);
        framebufferCache_.frameBufferFrameUseIndex[arrayIndex] = device_.GetFrameCount();
    } else {
        framebufferCache_.framebuffers.push_back(move(fb));
        framebufferCache_.frameBufferFrameUseIndex.push_back(device_.GetFrameCount());
        arrayIndex = (uint32_t)framebufferCache_.framebuffers.size() - 1;
    }
    framebufferCache_.renderPassHashToIndex[rpHash] = arrayIndex;
    return RenderHandleUtil::CreateEngineResourceHandle(RenderHandleType::UNDEFINED, arrayIndex, 0);
}

const LowlevelFramebufferGL* NodeContextPoolManagerGLES::GetFramebuffer(const EngineResourceHandle handle) const
{
    if (RenderHandleUtil::IsValid(handle)) {
        if (const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
            (index < framebufferCache_.framebuffers.size())) {
            return &framebufferCache_.framebuffers[index];
        }
    }
    return nullptr;
}

void NodeContextPoolManagerGLES::FilterRenderPass(RenderCommandBeginRenderPass& beginRenderPass)
{
#if RENDER_HAS_GLES_BACKEND
    if (!multisampledRenderToTexture_) {
        return;
    }
    if (!multiViewMultisampledRenderToTexture_ &&
        std::any_of(beginRenderPass.subpasses.cbegin(), beginRenderPass.subpasses.cend(),
            [](const RenderPassSubpassDesc& subpass) { return subpass.viewMask != 0; })) {
        return;
    }
    imageMap_.clear();
    imageMap_.insert(
        imageMap_.end(), static_cast<size_t>(beginRenderPass.renderPassDesc.attachmentCount), EMPTY_ATTACHMENT);
    auto begin = beginRenderPass.subpasses.begin();
    auto end = beginRenderPass.subpasses.end();
    auto pos = std::find_if(begin, end, [](const RenderPassSubpassDesc& subpass) {
        return (subpass.resolveAttachmentCount > 0) ||
               ((subpass.depthResolveAttachmentCount > 0) &&
                   (subpass.depthResolveModeFlagBit || subpass.stencilResolveModeFlagBit));
    });
    if (pos != end) {
        BindImage images[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT];
        UpdateBindImages(beginRenderPass, images, gpuResourceMgr_);
        if (!IsDefaultResolve(images, *pos)) {
            // build a mapping which attachments can be skipped and render directly to the resolve
            MapColorAttachments(begin, pos, images, imageMap_, beginRenderPass.renderPassDesc.attachments);
            if (device_.IsDepthResolveSupported()) {
                MapDepthAttachments(begin, pos, images, imageMap_, beginRenderPass.renderPassDesc.attachments);
            }
            UpdateSubpassAttachments(begin, pos + 1, imageMap_);

            // flip the mapping from attachment -> resolve to resolve -> attachment. this will be used when selecting
            // what images are bound as attachments at the begining of the renderpass.
            vector<uint32_t> map(imageMap_.size(), EMPTY_ATTACHMENT);
            for (uint32_t color = 0U, last = static_cast<uint32_t>(imageMap_.size()); color < last; ++color) {
                const auto resolve = imageMap_[color];
                if (resolve != EMPTY_ATTACHMENT) {
                    map[resolve] = color;
                }
            }
            imageMap_ = map;
        }
    }
#endif
}
RENDER_END_NAMESPACE()
