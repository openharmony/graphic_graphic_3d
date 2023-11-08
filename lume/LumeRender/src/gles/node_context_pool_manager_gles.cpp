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

#include "node_context_pool_manager_gles.h"

#include <algorithm>

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
typedef struct {
    uint32_t layer, mipLevel;
    const GpuImageGLES* image;
} BindImage;

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

uint64_t hashRPD(const RenderCommandBeginRenderPass& beginRenderPass, GpuResourceManager& gpuResourceMgr_)
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
    }
    return rpHash;
}

bool VerifyFBO()
{
    GLenum status;
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        // failed!
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
                PLUGIN_LOG_E("Framebuffer imcomplete missing attachment");
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
            default: {
                PLUGIN_LOG_E("Framebuffer other error: %x", status);
                break;
            }
        }
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
        const auto color = images[sb.colorAttachmentIndices[0]].image;
        const auto& plat = static_cast<const GpuImagePlatformDataGL&>(color->GetPlatformData());
        if ((plat.image == 0) &&                // not texture
            (plat.renderBuffer == 0))           // not renderbuffer
        {                                       // Colorattachment is backbuffer
            if (sb.depthAttachmentCount == 1) { // and has one depth.
                const auto depth = images[sb.depthAttachmentIndex].image;
                const auto& dPlat = static_cast<const GpuImagePlatformDataGL&>(depth->GetPlatformData());
                if ((dPlat.image == 0) && (dPlat.renderBuffer == 0)) { // depth attachment is backbuffer depth.
                    return true;
                } else {
                    // Invalid configuration. (this should be caught earlier already)
                    PLUGIN_LOG_E("Mixing backbuffer with custom depth is not allowed!");
                    PLUGIN_ASSERT_MSG(false, "Mixing backbuffer with custom depth is not allowed!");
                }
            } else {
                return true;
            }
        }
    }
    return false;
}
bool IsDefaultResolve(array_view<const BindImage> images, const RenderPassSubpassDesc& sb)
{
    if (sb.resolveAttachmentCount == 1 && sb.depthResolveAttachmentCount == 0) {
        // okay, looks good. one color...
        const GpuImageGLES* color = images[sb.resolveAttachmentIndices[0]].image;
        const GpuImagePlatformDataGL& plat = static_cast<const GpuImagePlatformDataGL&>(color->GetPlatformData());
        if ((plat.image == 0) && (plat.renderBuffer == 0)) {
            return true;
        }
    }
    if (sb.depthResolveAttachmentCount == 1) {
        // okay, looks good. one depth...
        const GpuImageGLES* depth = images[sb.depthResolveAttachmentIndex].image;
        const GpuImagePlatformDataGL& plat = static_cast<const GpuImagePlatformDataGL&>(depth->GetPlatformData());
        if ((plat.image == 0) && (plat.renderBuffer == 0)) {
            return true;
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

void BindToFbo(GLenum attachType, const BindImage& image, size_t& width, size_t& height, bool isStarted)
{
    const GpuImagePlatformDataGL& plat = static_cast<const GpuImagePlatformDataGL&>(image.image->GetPlatformData());
    const GpuImageDesc& desc = image.image->GetDesc();
    if (isStarted) {
        // Assert that all attachments are the same size.
        PLUGIN_ASSERT_MSG(width == desc.width, "Depth attachment is not the same size as other attachments");
        PLUGIN_ASSERT_MSG(height == desc.height, "Depth attachment is not the same size as other attachments");
    }
    width = desc.width;
    height = desc.height;
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
            glFramebufferTextureLayer(
                GL_FRAMEBUFFER, attachType, plat.image, (GLint)image.mipLevel, (GLint)image.layer);
        } else {
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachType, plat.type, plat.image, (GLint)image.mipLevel);
        }
    }
}
void BindToFboMultisampled(GLenum attachType, const BindImage& image, const BindImage& resolveImage, size_t& width,
    size_t& height, bool isStarted, bool multisampledRenderToTexture)
{
    const GpuImagePlatformDataGL& plat =
        static_cast<const GpuImagePlatformDataGL&>(resolveImage.image->GetPlatformData());
    const GpuImageDesc& desc = image.image->GetDesc();
    if (isStarted) {
        // Assert that all attachments are the same size.
        PLUGIN_ASSERT_MSG(width == desc.width, "Depth attachment is not the same size as other attachments");
        PLUGIN_ASSERT_MSG(height == desc.height, "Depth attachment is not the same size as other attachments");
    }
    width = desc.width;
    height = desc.height;
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
    } else {
        if ((plat.type == GL_TEXTURE_2D_ARRAY) || (plat.type == GL_TEXTURE_2D_MULTISAMPLE_ARRAY)) {
            glFramebufferTextureLayer(
                GL_FRAMEBUFFER, attachType, plat.image, (GLint)image.mipLevel, (GLint)image.layer);
#if RENDER_HAS_GLES_BACKEND
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
    for (uint32_t idx = 0; idx < sb.colorAttachmentCount; ++idx) {
        const uint32_t ci = sb.colorAttachmentIndices[idx];
        const uint32_t original = (ci < imageMap.size()) ? imageMap[ci] : 0xff;
        if (images[ci].image) {
            drawBuffers[idx] = GL_COLOR_ATTACHMENT0 + colorAttachmentCount;
            if (original == 0xff) {
                BindToFbo(drawBuffers[idx], images[ci], framebuffer.width, framebuffer.height,
                    (colorAttachmentCount) || (resolveAttachmentCount));
            } else {
                BindToFboMultisampled(drawBuffers[idx], images[original], images[ci], framebuffer.width,
                    framebuffer.height, (colorAttachmentCount) || (resolveAttachmentCount),
                    multisampledRenderToTexture);
            }
            ++colorAttachmentCount;
        } else {
            PLUGIN_LOG_E("no image for color attachment %u %u", idx, ci);
            drawBuffers[idx] = GL_NONE;
        }
    }
    glDrawBuffers((GLsizei)sb.colorAttachmentCount, drawBuffers);
    if (sb.depthAttachmentCount == 1) {
        const auto* image = images[sb.depthAttachmentIndex].image;
        if (image) {
            const GLenum bindType = BindType(image);
            PLUGIN_ASSERT_MSG(bindType != GL_NONE, "Depth attachment has no stencil or depth");
            BindToFboMultisampled(bindType, images[sb.depthAttachmentIndex], images[sb.depthAttachmentIndex],
                framebuffer.width, framebuffer.height, (colorAttachmentCount) || (resolveAttachmentCount),
                multisampledRenderToTexture);
        } else {
            PLUGIN_LOG_E("no image for depth attachment");
        }
    }
    if (!VerifyFBO()) {
        PLUGIN_LOG_E("Failed to create subpass FBO size [%zd %zd] [color:%d depth:%d resolve:%d]", framebuffer.width,
            framebuffer.height, sb.colorAttachmentCount, sb.depthAttachmentCount, sb.resolveAttachmentCount);
        PLUGIN_ASSERT_MSG(false, "Framebuffer creation failed!");
    } else if constexpr (VERBOSE_LOGGING) {
        PLUGIN_LOG_V("Created subpass FBO size [%zd %zd] [color:%d depth:%d resolve:%d]", framebuffer.width,
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
        const auto* swp = device.GetSwapchain();
        if (swp) {
            const auto desc = swp->GetDesc();
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
    device.BindFrameBuffer(rp.resolveFbo);
    GLenum drawBuffers[PipelineStateConstants::MAX_COLOR_ATTACHMENT_COUNT] = { GL_NONE };
    for (uint32_t idx = 0; idx < sb.resolveAttachmentCount; ++idx) {
        const uint32_t ci = sb.resolveAttachmentIndices[idx];
        const auto* image = images[ci].image;
        if (image) {
            drawBuffers[idx] = GL_COLOR_ATTACHMENT0 + rp.resolveAttachmentCount;
            BindToFbo(
                drawBuffers[idx], images[ci], framebuffer.width, framebuffer.height, (rp.resolveAttachmentCount > 0));
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
            BindToFbo(
                BindType(image), images[ci], framebuffer.width, framebuffer.height, (rp.resolveAttachmentCount > 0));
        } else {
            PLUGIN_LOG_E("no image for depth resolve attachment %u %u", idx, ci);
        }
    }
    return rp;
}

LowlevelFramebufferGL::SubPassPair ProcessSubPass(DeviceGLES& device, LowlevelFramebufferGL& framebuffer,
    std::map<uint64_t, GLuint>& fboMap, array_view<const BindImage> images, const array_view<const uint32_t> imageMap,
    const RenderPassSubpassDesc& sb, bool multisampledRenderToTexture)
{
#if RENDER_GL_FLIP_Y_SWAPCHAIN == 0
    if (IsDefaultAttachment(images, sb)) {
        // This subpass uses backbuffer!
        const auto* swp = device.GetSwapchain();
        if (swp) {
            const auto desc = swp->GetDesc();
            framebuffer.width = desc.width;
            framebuffer.height = desc.height;
        }
        // NOTE: it is technically possible to resolve from backbuffer to a custom render target.
        // but we do not support it now.
        PLUGIN_ASSERT_MSG(sb.resolveAttachmentCount == 0, "No resolving from default framebuffer");
        return { 0, 0 };
    }
#endif
    // This subpass uses custom render targets.
    PLUGIN_ASSERT((sb.colorAttachmentCount + sb.depthAttachmentCount) <
                  (PipelineStateConstants::MAX_COLOR_ATTACHMENT_COUNT + 1)); // +1 for depth
    uint32_t fbo = 0;
    const auto resolveResult = GenerateResolveFBO(device, framebuffer, sb, images);
    const uint64_t subHash = HashAttachments(sb);
    if (const auto it = fboMap.find(subHash); it != fboMap.end()) {
        // matching fbo created already, re-use
        fbo = it->second;
    } else {
        fbo = GenerateSubPassFBO(device, framebuffer, sb, images, resolveResult.resolveAttachmentCount, imageMap,
            multisampledRenderToTexture);
        fboMap[subHash] = fbo;
    }
    return { fbo, resolveResult.resolveFbo };
}

} // namespace

NodeContextPoolManagerGLES::NodeContextPoolManagerGLES(Device& device, GpuResourceManager& gpuResourceManager)
    : NodeContextPoolManager(), device_ { (DeviceGLES&)device }, gpuResourceMgr_ { gpuResourceManager }
{
    bufferingCount_ = device_.GetCommandBufferingCount();
#if RENDER_HAS_GLES_BACKEND
    if (device_.GetBackendType() == DeviceBackendType::OPENGLES) {
        multisampledRenderToTexture_ = device_.HasExtension("GL_EXT_multisampled_render_to_texture2");
    }
#endif
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
    bufferingIndex_ = 0;
    const auto maxAge = 2;
    const auto minAge = device_.GetCommandBufferingCount() + maxAge;
    const auto ageLimit = (device_.GetFrameCount() < minAge) ? 0 : (device_.GetFrameCount() - minAge);
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
    const uint64_t rpHash = hashRPD(beginRenderPass, gpuResourceMgr_);
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
        PLUGIN_LOG_V("Creating framebuffer with %d subpasses", rpd.subpassCount);
    }
    // NOTE: we currently expect that resolve, color and depth attachments are regular 2d textures (or
    // renderbuffers)
    std::map<uint64_t, GLuint> fboMap;
    std::transform(std::begin(beginRenderPass.subpasses), std::begin(beginRenderPass.subpasses) + rpd.subpassCount,
        fb.fbos.data(),
        [this, &fb, &fboMap, images = array_view<const BindImage>(images),
            imageMap = array_view<const uint32_t>(imageMap_)](const RenderPassSubpassDesc& subpass) {
            return ProcessSubPass(device_, fb, fboMap, images, imageMap, subpass, multisampledRenderToTexture_);
        });
    if constexpr (VERBOSE_LOGGING) {
        PLUGIN_LOG_V("Created framebuffer with %d subpasses at size [%zd %zd]", rpd.subpassCount, fb.width, fb.height);
    }

    uint32_t arrayIndex = 0;
    if (auto const pos = std::find_if(framebufferCache_.framebuffers.begin(), framebufferCache_.framebuffers.end(),
            [](auto const& framebuffer) { return framebuffer.fbos.empty(); });
        pos != framebufferCache_.framebuffers.end()) {
        arrayIndex = (uint32_t)std::distance(framebufferCache_.framebuffers.begin(), pos);
        *pos = move(fb);
        framebufferCache_.frameBufferFrameUseIndex[arrayIndex] = device_.GetFrameCount();
    } else {
        framebufferCache_.framebuffers.emplace_back(move(fb));
        framebufferCache_.frameBufferFrameUseIndex.emplace_back(device_.GetFrameCount());
        arrayIndex = (uint32_t)framebufferCache_.framebuffers.size() - 1;
    }
    framebufferCache_.renderPassHashToIndex[rpHash] = arrayIndex;
    return RenderHandleUtil::CreateEngineResourceHandle(RenderHandleType::UNDEFINED, arrayIndex, 0);
}

const LowlevelFramebufferGL& NodeContextPoolManagerGLES::GetFramebuffer(const EngineResourceHandle handle) const
{
    PLUGIN_ASSERT(RenderHandleUtil::IsValid(handle));
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    PLUGIN_ASSERT(index < framebufferCache_.framebuffers.size());
    return framebufferCache_.framebuffers[index];
}

void NodeContextPoolManagerGLES::FilterRenderPass(RenderCommandBeginRenderPass& beginRenderPass)
{
    imageMap_.clear();
    imageMap_.insert(imageMap_.end(), beginRenderPass.renderPassDesc.attachmentCount, 0xff);
    auto begin = beginRenderPass.subpasses.begin();
    auto pos = std::find_if(begin, beginRenderPass.subpasses.end(),
        [](const RenderPassSubpassDesc& subpass) { return (subpass.resolveAttachmentCount > 0); });
    if (pos != beginRenderPass.subpasses.end()) {
        BindImage images[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT];
        UpdateBindImages(beginRenderPass, images, gpuResourceMgr_);
        if (!IsDefaultResolve(images, *pos)) {
            const auto resolveAttachmentIndices =
                array_view(pos->resolveAttachmentIndices, pos->resolveAttachmentCount);
            const auto colorAttachmentIndices = array_view(pos->colorAttachmentIndices, pos->colorAttachmentCount);
            for (auto i = 0U; i < pos->resolveAttachmentCount; ++i) {
                const auto color = colorAttachmentIndices[i];
                const auto resolve = resolveAttachmentIndices[i];
                imageMap_[color] = resolve;
                beginRenderPass.renderPassDesc.attachments[resolve].loadOp =
                    beginRenderPass.renderPassDesc.attachments[color].loadOp;
            }
            for (auto i = begin; i != pos; ++i) {
                for (auto ci = 0U; ci < i->colorAttachmentCount; ++ci) {
                    const auto oldColor = i->colorAttachmentIndices[ci];
                    if (const auto newColor = imageMap_[oldColor]; newColor != 0xff) {
                        i->colorAttachmentIndices[ci] = newColor;
                    }
                }
            }
            for (auto ci = 0U; ci < pos->colorAttachmentCount; ++ci) {
                const auto oldColor = pos->colorAttachmentIndices[ci];
                if (const auto newColor = imageMap_[oldColor]; newColor != 0xff) {
                    pos->colorAttachmentIndices[ci] = newColor;
                }
            }
            pos->resolveAttachmentCount = 0;
            vector<uint32_t> map(imageMap_.size(), 0xff);
            for (uint32_t color = 0U, end = static_cast<uint32_t>(imageMap_.size()); color < end; ++color) {
                const auto resolve = imageMap_[color];
                if (resolve != 0xff) {
                    map[resolve] = color;
                }
            }
            imageMap_ = map;
        }
    }
}

RENDER_END_NAMESPACE()
