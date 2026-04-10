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

#ifndef OHOS_RENDER_3D_SURFACE_STREAM_H
#define OHOS_RENDER_3D_SURFACE_STREAM_H

#include <queue>

#include <iconsumer_surface.h>
#include <meta/ext/attachment/attachment.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/resource/intf_dynamic_resource.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>
#include <scene_adapter/intf_surface_stream.h>
#include <surface_buffer.h>
#include <surface_utils.h>

namespace OHOS::Render3D {
constexpr uint32_t SURFACE_QUEUE_SIZE = 5;

class SurfaceStream final : public META_NS::IntroduceInterfaces<META_NS::AttachmentFwd, ISurfaceStream>,
    public OHOS::IBufferConsumerListenerClazz {
    META_OBJECT(SurfaceStream, ClassId::SurfaceStream, IntroduceInterfaces)
    ~SurfaceStream();
protected:
    void OnBufferAvailable() override;
private:
    bool Build(const META_NS::IMetadata::Ptr& metadata) override;

    bool AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext) override;
    bool DetachFrom(const META_NS::IAttach::Ptr& target) override;

    void Init();
    void Deinit();

    void UpdateView(OH_NativeBuffer* buffer, uint32_t width, uint32_t height, OHOS::GraphicColorGamut colorGamut);

    void SetHeight(uint32_t height) override { height_ = height; }
    uint32_t GetHeight() const override { return height_; }
    void SetWidth(uint32_t width) override { width_ = width; }
    uint32_t GetWidth() const override { return width_; }

    uint64_t GetSurfaceId() const override { return surfaceId_; }
private:
    RENDER_NS::DeviceBackendType backendType_ = RENDER_NS::DeviceBackendType::VULKAN;
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext_ = nullptr;
    META_NS::ITaskQueue::Ptr engineQueue_ = nullptr;
    SCENE_NS::IRenderResource::WeakPtr renderResource_ = nullptr;
    uint64_t surfaceId_ = 0;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t queueSize_ = SURFACE_QUEUE_SIZE;
    OHOS::sptr<OHOS::IConsumerSurface> consumerSurface_ = nullptr;
    OHOS::sptr<OHOS::Surface> producerSurface_ = nullptr;
    std::atomic<uint64_t> frameIndex_ { 0 };

    std::mutex surfaceBufferCacheMutex_;
    std::queue<OHOS::sptr<OHOS::SurfaceBuffer>> surfaceBufferCache_;
};

}
#endif // OHOS_RENDER_3D_SURFACE_STREAM_H