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
}

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

    BASE_NS::shared_ptr<SCENE_NS::IRenderContext> context = nullptr;
    context = SCENE_NS::GetInterfaceBuildArg<SCENE_NS::IRenderContext>(metadata, "RenderContext");
    if (context) {
        engineQueue_ = context->GetRenderQueue();
        renderContext_ = context->GetRenderer();
        if (renderContext_) {
            backendType_ = renderContext_->GetDevice().GetBackendType();
        }
        return true;
    }

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
    engineQueue_ = context->GetRenderQueue();
    renderContext_ = context->GetRenderer();
    if (renderContext_) {
        backendType_ = renderContext_->GetDevice().GetBackendType();
    }
    return true;
}

bool SurfaceStream::AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext)
{
    if (auto sp = renderResource_.lock()) {
        CORE_LOG_E("render resource already attached.");
        return false;
    }

    auto bitmap = interface_cast<SCENE_NS::IBitmap>(target);
    renderResource_ = interface_pointer_cast<SCENE_NS::IRenderResource>(target);
    if (bitmap == nullptr) {
        CORE_LOG_E("Incorrect type bitmap is null");
        return false;
    }

    Init();
    return true;
}

bool SurfaceStream::DetachFrom(const META_NS::IAttach::Ptr& target)
{
    if (interface_pointer_cast<IAttach>(renderResource_) != interface_pointer_cast<IAttach>(target)) {
        CORE_LOG_E("Invalid input parameter, incorrect type");
        return false;
    }

    Deinit();
    renderResource_ = nullptr;
    return true;
}

void SurfaceStream::Init()
{
    consumerSurface_ = OHOS::IConsumerSurface::Create("IumeSurfaceConsumer");
    if (consumerSurface_ == nullptr) {
        CORE_LOG_E("create surface consumer failed");
        return;
    }
    auto producer = consumerSurface_->GetProducer();
    if (producer == nullptr) {
        CORE_LOG_E("create surface producer failed");
        return;
    }
    producerSurface_ = OHOS::Surface::CreateSurfaceAsProducer(producer);
    auto utils = OHOS::SurfaceUtils::GetInstance();
    if (producerSurface_ == nullptr || utils == nullptr) {
        CORE_LOG_E("get producer surface or utils failed");
        return;
    }
    producerSurface_->SetQueueSize(queueSize_);
    surfaceId_ = producerSurface_->GetUniqueId();
    utils->Add(surfaceId_, producerSurface_);
    consumerSurface_->RegisterConsumerListener(this);
}

void SurfaceStream::Deinit()
{
    if (consumerSurface_) {
        consumerSurface_->UnregisterConsumerListener();
    }

    auto utils = OHOS::SurfaceUtils::GetInstance();
    if (utils) {
        utils->Remove(surfaceId_);
    }
    surfaceId_ = 0;

    producerSurface_ = nullptr;
    consumerSurface_ = nullptr;
}

void SurfaceStream::OnBufferAvailable()
{
    META_NS::AddFutureTaskOrRunDirectly(engineQueue_, [this]() {
        if (consumerSurface_ == nullptr) {
            CORE_LOG_E("invalid consumer surface");
            return;
        }

        OHOS::sptr<OHOS::SurfaceBuffer> acquireSurfaceBuffer = nullptr;
        OHOS::sptr<OHOS::SyncFence> acquireFence = OHOS::SyncFence::INVALID_FENCE;
        int64_t timestamp = 0;
        OHOS::Rect damage = { 0, 0, 0, 0 };
        auto success = consumerSurface_->AcquireBuffer(acquireSurfaceBuffer, acquireFence, timestamp, damage);
        if ((success != OHOS::SURFACE_ERROR_OK) || acquireSurfaceBuffer == nullptr || !acquireFence->IsValid()) {
            CORE_LOG_E("consumer surface acquire surface buffer failed: %d", success);
            return;
        }

        acquireFence->Wait(WAIT_FENCE_TIME);

        OH_NativeBuffer* nativeBuffer = acquireSurfaceBuffer->SurfaceBufferToNativeBuffer();
        if (nativeBuffer == nullptr) {
            CORE_LOG_E("surface buffer to native buffer failed");
            consumerSurface_->ReleaseBuffer(acquireSurfaceBuffer, OHOS::SyncFence::INVALID_FENCE);
            return;
        }
        width_ = acquireSurfaceBuffer->GetSurfaceBufferWidth();
        height_ = acquireSurfaceBuffer->GetSurfaceBufferHeight();
        UpdateView(nativeBuffer, width_, height_, acquireSurfaceBuffer->GetSurfaceBufferColorGamut());

        {
            std::lock_guard<std::mutex> lock(surfaceBufferCacheMutex_);
            if (surfaceBufferCache_.size() >= MAX_BUFFER_SIZE) {
                auto oldSurfaceBuffer = surfaceBufferCache_.front();
                surfaceBufferCache_.pop();
                if (oldSurfaceBuffer) {
                    consumerSurface_->ReleaseBuffer(oldSurfaceBuffer, OHOS::SyncFence::INVALID_FENCE);
                }
            }
            surfaceBufferCache_.push(std::move(acquireSurfaceBuffer));
        }
    }).Wait();
}

void SurfaceStream::UpdateView(OH_NativeBuffer* buffer, uint32_t width, uint32_t height,
    OHOS::GraphicColorGamut colorGamut)
{
    if (renderContext_ == nullptr) {
        CORE_LOG_E("invalid render context");
        return;
    }

    auto colorSpace = ConvertColorGamutToColorSpace(colorGamut);
    OH_NativeBuffer_SetColorSpace(buffer, colorSpace);
    auto& device = renderContext_->GetDevice();
    auto& grm = device.GetGpuResourceManager();
    RENDER_NS::GpuImageDesc imageDesc {};
    imageDesc.usageFlags = RENDER_NS::ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
    imageDesc.width = width;
    imageDesc.height = height;
    imageDesc.isFormatEffectivelySet = true;
    RENDER_NS::RenderHandleReference handle;
    BASE_NS::string frameId;
    frameId = "Internal://SurfaceStream_" + BASE_NS::to_string(surfaceId_) + "_" +
        BASE_NS::to_string(frameIndex_.fetch_add(1, std::memory_order_relaxed));

    if (backendType_ == RENDER_NS::DeviceBackendType::VULKAN) {
        RENDER_NS::ImageDescVk vkImageDesc {};
        vkImageDesc.platformHwBuffer = (uintptr_t)buffer;
        handle = grm.CreateView(frameId, imageDesc, vkImageDesc);
    }
    if (backendType_ == RENDER_NS::DeviceBackendType::OPENGLES) {
        RENDER_NS::ImageDescGLES glImageDesc {};
        glImageDesc.type = GL_TEXTURE_EXTERNAL_OES;
        glImageDesc.platformHwBuffer = (uintptr_t)buffer;
        handle = grm.CreateView(frameId, imageDesc, glImageDesc);
    }

    auto bitmap = renderResource_.lock();
    if (bitmap == nullptr) {
        CORE_LOG_E("bitmap is nullptr");
        return;
    }
    bitmap->SetRenderHandle(handle);
}

SurfaceStream::~SurfaceStream()
{
    std::lock_guard<std::mutex> lock(surfaceBufferCacheMutex_);
    while (!surfaceBufferCache_.empty()) {
        auto oldSurfaceBuffer = surfaceBufferCache_.front();
        surfaceBufferCache_.pop();
        if (oldSurfaceBuffer) {
            consumerSurface_->ReleaseBuffer(oldSurfaceBuffer, OHOS::SyncFence::INVALID_FENCE);
        }
    }

    Deinit();
    renderResource_ = nullptr;
    renderContext_ = nullptr;
    engineQueue_ = nullptr;
}

}