/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "scene_adapter/surface_stream.h"

#include <vector>

#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/render_handle_component.h>

#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_class_register.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl31.h>

#include <ibuffer_consumer_listener.h>
#include <iconsumer_surface.h>

#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#if RENDER_HAS_GLES_BACKEND || RENDER_HAS_GL_BACKEND
#include <render/gles/intf_device_gles.h>
#endif
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#if RENDER_HAS_VULKAN_BACKEND
#include <render/vulkan/intf_device_vk.h>
#endif

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_application_context.h>
#include <scene/interface/intf_image.h>
#include <surface_buffer.h>
#include <surface_utils.h>
#include <sync_fence.h>

#include <window.h>

namespace OHOS::Render3D {

namespace {
static constexpr uint32_t WAIT_FENCE_TIME = 5000;
static constexpr uint32_t MAX_BUFFER_SIZE = 3;
static constexpr uint32_t MAX_IMAGE_EXTENT = 32768U;
static constexpr uint32_t SURFACE_QUEUE_SIZE = 5;

// RAII guard to ensure the acquired surface buffer is released when not cached.
struct ReleaseGuard {
    OHOS::sptr<OHOS::IConsumerSurface> surface = nullptr;
    OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
    bool shouldRelease = true;
    ~ReleaseGuard()
    {
        if (shouldRelease && surface && buffer) {
            surface->ReleaseBuffer(buffer, OHOS::SyncFence::INVALID_FENCE);
        }
    }
};

OH_NativeBuffer_ColorSpace ConvertColorGamutToColorSpace(OHOS::GraphicColorGamut colorGamut)
{
    switch (colorGamut) {
        case OHOS::GRAPHIC_COLOR_GAMUT_STANDARD_BT601:
            return OH_COLORSPACE_BT601_EBU_FULL;
        case OHOS::GRAPHIC_COLOR_GAMUT_STANDARD_BT709:
            return OH_COLORSPACE_BT709_FULL;
        case OHOS::GRAPHIC_COLOR_GAMUT_SRGB:
            return OH_COLORSPACE_SRGB_FULL;
        case OHOS::GRAPHIC_COLOR_GAMUT_ADOBE_RGB:
            return OH_COLORSPACE_ADOBERGB_FULL;
        case OHOS::GRAPHIC_COLOR_GAMUT_DISPLAY_P3:
            return OH_COLORSPACE_P3_FULL;
        case OHOS::GRAPHIC_COLOR_GAMUT_BT2100_PQ:
            return OH_COLORSPACE_BT2020_PQ_FULL;
        case OHOS::GRAPHIC_COLOR_GAMUT_BT2100_HLG:
            return OH_COLORSPACE_BT2020_HLG_FULL;
        case OHOS::GRAPHIC_COLOR_GAMUT_BT2020:
        case OHOS::GRAPHIC_COLOR_GAMUT_DISPLAY_BT2020:
            return OH_COLORSPACE_DISPLAY_BT2020_SRGB;
        default:
            return OH_COLORSPACE_SRGB_FULL;
    }
}
}  // namespace

bool SurfaceStream::Build(const META_NS::IMetadata::Ptr& metadata)
{
    if (metadata == nullptr) {
        CORE_LOG_E("SurfaceStream Build metadata is nullptr");
        return false;
    }

    if (!Super::Build(metadata)) {
        CORE_LOG_E("SurfaceStream Build failed");
        return false;
    }

    auto context = SCENE_NS::GetInterfaceBuildArg<SCENE_NS::IRenderContext>(metadata, "RenderContext");
    if (context == nullptr) {
        auto app = SCENE_NS::GetDefaultApplicationContext();
        if (app == nullptr) {
            CORE_LOG_E("get default application context failed");
            return false;
        }
        context = app->GetRenderContext();
        if (context == nullptr) {
            CORE_LOG_E("get default render context failed");
            return false;
        }
    }

    engineQueue_ = context->GetRenderQueue();
    std::lock_guard<std::mutex> lock(renderContextMutex_);
    renderContext_ = context->GetRenderer();
    if (renderContext_) {
        backendType_ = renderContext_->GetDevice().GetBackendType();
    }
    return true;
}

bool SurfaceStream::AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext)
{
    {
        std::lock_guard<std::mutex> lock(renderResourceMutex_);
        if (auto existing = renderResource_.lock()) {
            CORE_LOG_E("render resource already attached.");
            return false;
        }

        if (target == nullptr) {
            CORE_LOG_E("target is null");
            return false;
        }

        auto bitmap = interface_cast<SCENE_NS::IBitmap>(target);
        if (bitmap == nullptr) {
            CORE_LOG_E("Incorrect type bitmap is null");
            return false;
        }
        renderResource_ = interface_pointer_cast<SCENE_NS::IRenderResource>(target);
        if (auto attached = renderResource_.lock(); !attached) {
            CORE_LOG_E("render resource attach failed");
            return false;
        }
    }

    if (!Init()) {
        CORE_LOG_E("surface stream Init failed");
        // Roll back the render resource binding on init failure to keep state consistent.
        std::lock_guard<std::mutex> lock(renderResourceMutex_);
        renderResource_ = nullptr;
        return false;
    }

    return true;
}

bool SurfaceStream::DetachFrom(const META_NS::IAttach::Ptr& target)
{
    if (target == nullptr) {
        CORE_LOG_E("detach target is null");
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(renderResourceMutex_);
        if (interface_pointer_cast<IAttach>(renderResource_) != interface_pointer_cast<IAttach>(target)) {
            CORE_LOG_E("Invalid input parameter, incorrect type");
            return false;
        }
        renderResource_ = nullptr;
    }

    Deinit();
    return true;
}

bool SurfaceStream::Init()
{
    std::lock_guard<std::mutex> lock(consumerSurfaceMutex_);
    consumerSurface_ = OHOS::IConsumerSurface::Create("LumeSurfaceConsumer");
    if (consumerSurface_ == nullptr) {
        CORE_LOG_E("create surface consumer failed");
        return false;
    }
    auto producer = consumerSurface_->GetProducer();
    if (producer == nullptr) {
        CORE_LOG_E("create surface producer failed");
        // Release the consumer surface created above to avoid resource leak.
        consumerSurface_ = nullptr;
        return false;
    }
    producerSurface_ = OHOS::Surface::CreateSurfaceAsProducer(producer);
    auto utils = OHOS::SurfaceUtils::GetInstance();
    if (producerSurface_ == nullptr || utils == nullptr) {
        CORE_LOG_E("get producer surface or utils failed");
        // Release partially created resources to avoid leak.
        producerSurface_ = nullptr;
        consumerSurface_ = nullptr;
        return false;
    }
    producerSurface_->SetQueueSize(SURFACE_QUEUE_SIZE);
    surfaceId_.store(producerSurface_->GetUniqueId(), std::memory_order_relaxed);
    utils->Add(surfaceId_.load(std::memory_order_relaxed), producerSurface_);
    consumerSurface_->RegisterConsumerListener(this);

    return true;
}

void SurfaceStream::Deinit()
{
    std::lock_guard<std::mutex> lock(consumerSurfaceMutex_);
    if (consumerSurface_) {
        consumerSurface_->UnregisterConsumerListener();
    }

    auto utils = OHOS::SurfaceUtils::GetInstance();
    if (utils) {
        utils->Remove(surfaceId_.load(std::memory_order_relaxed));
    }
    surfaceId_.store(0, std::memory_order_relaxed);

    producerSurface_ = nullptr;
    consumerSurface_ = nullptr;
}

void SurfaceStream::OnBufferAvailable()
{
    META_NS::AddFutureTaskOrRunDirectly(engineQueue_, [this]() { ProcessBufferAvailable(); }).Wait();
}

void SurfaceStream::ProcessBufferAvailable()
{
    OHOS::sptr<OHOS::IConsumerSurface> localConsumerSurface = nullptr;
    {
        std::lock_guard<std::mutex> lock(consumerSurfaceMutex_);
        if (consumerSurface_ == nullptr) {
            CORE_LOG_E("invalid consumer surface");
            return;
        }
        localConsumerSurface = consumerSurface_;
    }

    OHOS::sptr<OHOS::SurfaceBuffer> acquireSurfaceBuffer = nullptr;
    OHOS::sptr<OHOS::SyncFence> acquireFence = OHOS::SyncFence::INVALID_FENCE;
    int64_t timestamp = 0;
    OHOS::Rect damage = {0, 0, 0, 0};
    auto success = localConsumerSurface->AcquireBuffer(acquireSurfaceBuffer, acquireFence, timestamp, damage);
    if ((success != OHOS::SURFACE_ERROR_OK) || acquireSurfaceBuffer == nullptr ||
        acquireFence == nullptr || !acquireFence->IsValid()) {
        CORE_LOG_E("consumer surface acquire surface buffer failed: %d", success);
        return;
    }

    // RAII guard ensures the acquired buffer is released on any early return
    // (e.g. native buffer conversion failure) or exception before it is cached.
    ReleaseGuard guard { localConsumerSurface, acquireSurfaceBuffer };

    int32_t fenceStatus = acquireFence->Wait(WAIT_FENCE_TIME);
    if (fenceStatus != 0) {
        CORE_LOG_E("acquire fence wait failed or timed out, status: %d", fenceStatus);
        return;
    }

    OH_NativeBuffer* nativeBuffer = acquireSurfaceBuffer->SurfaceBufferToNativeBuffer();
    if (nativeBuffer == nullptr) {
        CORE_LOG_E("surface buffer to native buffer failed");
        return;
    }
    uint32_t width = static_cast<uint32_t>(acquireSurfaceBuffer->GetSurfaceBufferWidth());
    uint32_t height = static_cast<uint32_t>(acquireSurfaceBuffer->GetSurfaceBufferHeight());
    width_.store(width, std::memory_order_relaxed);
    height_.store(height, std::memory_order_relaxed);

    UpdateView(nativeBuffer, width, height, acquireSurfaceBuffer->GetSurfaceBufferColorGamut());

    {
        std::lock_guard<std::mutex> lock(surfaceBufferCacheMutex_);
        if (surfaceBufferCache_.size() >= MAX_BUFFER_SIZE) {
            auto oldSurfaceBuffer = surfaceBufferCache_.front();
            surfaceBufferCache_.pop();
            if (oldSurfaceBuffer) {
                localConsumerSurface->ReleaseBuffer(oldSurfaceBuffer, OHOS::SyncFence::INVALID_FENCE);
            }
        }
        surfaceBufferCache_.push(acquireSurfaceBuffer);
        // Only cancel the release guard after the buffer is safely cached, so
        // that a push failure still releases the buffer via the guard.
        guard.shouldRelease = false;
    }
}

void SurfaceStream::UpdateView(
    OH_NativeBuffer* buffer, uint32_t width, uint32_t height, OHOS::GraphicColorGamut colorGamut)
{
    if (buffer == nullptr || width == 0 || height == 0 || width > MAX_IMAGE_EXTENT || height > MAX_IMAGE_EXTENT) {
        CORE_LOG_E("invalid native buffer or width(%u) or height(%u)", width, height);
        return;
    }

    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> localRenderContext = nullptr;
    RENDER_NS::DeviceBackendType localBackendType = RENDER_NS::DeviceBackendType::VULKAN;
    {
        // backendType_ is written under renderContextMutex_ in Build(),
        // so it must be read under the same lock to avoid a data race.
        std::lock_guard<std::mutex> lock(renderContextMutex_);
        if (renderContext_ == nullptr) {
            CORE_LOG_E("invalid render context");
            return;
        }
        localRenderContext = renderContext_;
        localBackendType = backendType_;
    }

    auto colorSpace = ConvertColorGamutToColorSpace(colorGamut);
    int32_t colorSpaceResult = OH_NativeBuffer_SetColorSpace(buffer, colorSpace);
    if (colorSpaceResult != 0) {
        CORE_LOG_E("OH_NativeBuffer_SetColorSpace failed: %d", colorSpaceResult);
    }
    auto& device = localRenderContext->GetDevice();
    auto& grm = device.GetGpuResourceManager();
    RENDER_NS::GpuImageDesc imageDesc{};
    imageDesc.usageFlags = RENDER_NS::ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
    imageDesc.width = width;
    imageDesc.height = height;
    RENDER_NS::RenderHandleReference handle;
    BASE_NS::string frameId = "Internal://SurfaceStream_";
    frameId += BASE_NS::to_string(surfaceId_.load(std::memory_order_relaxed)) + "_" +
        BASE_NS::to_string(frameIndex_.fetch_add(1, std::memory_order_relaxed));

    if (localBackendType == RENDER_NS::DeviceBackendType::VULKAN) {
        RENDER_NS::ImageDescVk vkImageDesc{};
        vkImageDesc.platformHwBuffer = reinterpret_cast<uintptr_t>(buffer);
        vkImageDesc.isFormatEffectivelySet = true;
        handle = grm.CreateView(frameId, imageDesc, vkImageDesc);
    } else if (localBackendType == RENDER_NS::DeviceBackendType::OPENGLES) {
        RENDER_NS::ImageDescGLES glImageDesc{};
        glImageDesc.type = GL_TEXTURE_EXTERNAL_OES;
        glImageDesc.platformHwBuffer = reinterpret_cast<uintptr_t>(buffer);
        handle = grm.CreateView(frameId, imageDesc, glImageDesc);
    } else {
        CORE_LOG_E("unsupported backend type: %d", static_cast<int>(localBackendType));
        return;
    }

    UpdateRenderResourceHandle(handle);
}

void SurfaceStream::UpdateRenderResourceHandle(const RENDER_NS::RenderHandleReference& handle)
{
    std::lock_guard<std::mutex> lock(renderResourceMutex_);
    auto bitmap = renderResource_.lock();
    if (bitmap == nullptr) {
        CORE_LOG_E("bitmap is nullptr");
        return;
    }
    if (handle) {
        bitmap->SetRenderHandle(handle);
    }
}

SurfaceStream::~SurfaceStream()
{
    // Collect cached buffers under the cache lock, then release them under the
    // consumer surface lock separately. This avoids nested locking
    // (surfaceBufferCacheMutex_ -> consumerSurfaceMutex_) whose order is the
    // reverse of ProcessBufferAvailable() (consumerSurfaceMutex_ ->
    // surfaceBufferCacheMutex_), which would otherwise risk a deadlock.
    std::vector<OHOS::sptr<OHOS::SurfaceBuffer>> pendingRelease;
    {
        std::lock_guard<std::mutex> lock(surfaceBufferCacheMutex_);
        while (!surfaceBufferCache_.empty()) {
            pendingRelease.push_back(std::move(surfaceBufferCache_.front()));
            surfaceBufferCache_.pop();
        }
    }

    {
        std::lock_guard<std::mutex> lock(consumerSurfaceMutex_);
        if (consumerSurface_) {
            for (auto& oldSurfaceBuffer : pendingRelease) {
                if (oldSurfaceBuffer != nullptr) {
                    consumerSurface_->ReleaseBuffer(oldSurfaceBuffer, OHOS::SyncFence::INVALID_FENCE);
                }
            }
        }
    }

    Deinit();

    {
        std::lock_guard<std::mutex> lock(renderResourceMutex_);
        renderResource_ = nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(renderContextMutex_);
        renderContext_ = nullptr;
    }

    engineQueue_ = nullptr;
}

}  // namespace OHOS::Render3D