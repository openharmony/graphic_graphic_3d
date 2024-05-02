/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_RENDER_3D_SCENE_ADAPTER_H
#define OHOS_RENDER_3D_SCENE_ADAPTER_H

#include "intf_scene_adapter.h"
#include <meta/interface/intf_object.h>

#include <base/containers/array_view.h>

#include <core/intf_engine.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/engine_info.h>
#include <core/implementation_uids.h>
#include <core/io/intf_file_manager.h>
#include <core/namespace.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/intf_property_handle.h>

#include <meta/interface/intf_meta_object_lib.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/base/shared_ptr.h>
#include <meta/base/interface_macros.h>
#include <meta/api/make_callback.h>
#include <meta/ext/object.h>

#include <scene_plugin/namespace.h>
#include <scene_plugin/interface/intf_scene.h>
#include <scene_plugin/interface/intf_ecs_scene.h>
#include <scene_plugin/interface/intf_mesh.h>
#include <scene_plugin/interface/intf_material.h>
#include <scene_plugin/api/scene_uid.h>

#include <render/implementation_uids.h>
#include <render/gles/intf_device_gles.h>
#include <render/intf_renderer.h>
#include <render/intf_render_context.h>

#include <ohos/platform_data.h>

namespace OHOS::Render3D {

class TextureLayer;

class SceneAdapter : public ISceneAdapter {
public:
    SceneAdapter();
    bool LoadPluginsAndInit() override;
    std::shared_ptr<TextureLayer> CreateTextureLayer() override;
    void OnWindowChange(const WindowChangeInfo& windowChangeInfo) override;
    void RenderFrame() override;
    void Deinit() override {};
    virtual void SetSceneObj(META_NS::IObject::Ptr pt);

    ~SceneAdapter() override;

private:
    bool LoadEngineLib();
    bool LoadPlugins(const CORE_NS::PlatformCreateInfo& platformCreateInfo);
    bool InitEngine(CORE_NS::PlatformCreateInfo platformCreateInfo);
    void AttachSwapchain(META_NS::IObject::Ptr camera, RENDER_NS::RenderHandleReference swapchain);
    void RenderFunction();
    void InitRenderThread();
    void DeinitRenderThread();

    META_NS::IObject::Ptr sceneWidgetObj_;
    RENDER_NS::RenderHandleReference swapchainHandle_;

    std::shared_ptr<TextureLayer> textureLayer_;
    HapInfo hapInfo_;
    SCENE_NS::IBitmap::Ptr bitmap_;
    uint32_t key_;
    META_NS::ITaskQueue::Token renderTask {};
    bool useAsyncRender = true;
    META_NS::ITaskQueueTask::Ptr singleFrameAsync;
    META_NS::ITaskQueueWaitableTask::Ptr singleFrameSync;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_SCENE_ADAPTER_H
