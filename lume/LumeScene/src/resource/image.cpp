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

#include "image.h"

#include <mutex>

#include <3d/render/default_material_constants.h>
#include <render/device/intf_gpu_resource_manager.h>

#if RENDER_HAS_VULKAN_BACKEND
#include <render/vulkan/intf_device_vk.h>
#endif
#if RENDER_HAS_GLES_BACKEND || RENDER_HAS_GL_BACKEND
#include <render/gles/intf_device_gles.h>
#endif

SCENE_BEGIN_NAMESPACE()

bool Image::SetRenderHandle(RENDER_NS::RenderHandleReference handle)
{
    if (handle.GetHandleType() != RENDER_NS::RenderHandleType::GPU_IMAGE) {
        CORE_LOG_E("Image requires GPU_IMAGE handle");
        return false;
    }
    IRenderContext::Ptr context;
    {
        std::unique_lock lock { mutex_ };
        if (handle_.GetHandle() == handle.GetHandle()) {
            return true;
        }
        handle_ = handle;
        context = context_.lock();
        if (!context) {
            return false;
        }
    }

    RENDER_NS::GpuImageDesc desc;
    context->RunDirectlyOrInTask([&] {
        auto& resources = context->GetRenderer()->GetDevice().GetGpuResourceManager();
        desc = resources.GetImageDescriptor(handle);
    });

    // todo: notify in right queue
    Size()->SetValue({ desc.width, desc.height });
    if (auto ev = EventOnResourceChanged(META_NS::MetadataQuery::EXISTING)) {
        META_NS::Invoke<META_NS::IOnChanged>(ev);
    }
    return true;
}

bool ExternalImage::SetExternalBuffer(const ExternalBufferInfo& buffer)
{
    auto context = context_.lock();
    return context ? context->AddTaskOrRunDirectly([=]() { return SetBufferInternal(context, buffer); }).GetResult()
                   : false;
}

bool ExternalImage::SetBufferInternal(const IRenderContext::Ptr& context, const ExternalBufferInfo& buffer)
{
#if defined(__ANDROID__) || defined(__OHOS__)
    if (!context) {
        return false;
    }

    auto& resources = context->GetRenderer()->GetDevice().GetGpuResourceManager();
    RENDER_NS::GpuImageDesc gpuImageDesc { RENDER_NS::ImageType::CORE_IMAGE_TYPE_2D,
        RENDER_NS::ImageViewType::CORE_IMAGE_VIEW_TYPE_2D, BASE_NS::Format::BASE_FORMAT_UNDEFINED,
        RENDER_NS::ImageTiling::CORE_IMAGE_TILING_LINEAR, RENDER_NS::ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT,
        RENDER_NS::MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        0, // ImageCreateFlags
        0, // EngineImageCreationFlags
        buffer.size.x, buffer.size.y, 1, 1, 1, RENDER_NS::SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT, {} };

    const RENDER_NS::DeviceBackendType backendType = context->GetRenderer()->GetDevice().GetBackendType();
    RENDER_NS::RenderHandleReference handle;
#if RENDER_HAS_VULKAN_BACKEND
    if (backendType == RENDER_NS::DeviceBackendType::VULKAN) {
        const RENDER_NS::ImageDescVk plat { {}, VK_NULL_HANDLE, VK_NULL_HANDLE, buffer.buffer };
        handle = resources.CreateView("", gpuImageDesc, plat);
    }
#endif
#if RENDER_HAS_GLES_BACKEND
    if (backendType == RENDER_NS::DeviceBackendType::OPENGLES) {
        const RENDER_NS::ImageDescGLES plat { {}, {}, {}, {}, {}, {}, {}, {}, buffer.buffer };
        handle = resources.CreateView("", gpuImageDesc, plat);
    }
#endif
#if RENDER_HAS_GL_BACKEND
    // GL does not support external buffer so create default white texture
    if (backendType == RENDER_NS::DeviceBackendType::OPENGL) {
        handle =
            resources.GetImageHandle(CORE3D_NS::DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_BASE_COLOR);
    }
#endif
    return SetRenderHandle(handle);
#else
    // Not Android or OHOS so hardware buffers not implemented
    return false;
#endif
}

SCENE_END_NAMESPACE()
