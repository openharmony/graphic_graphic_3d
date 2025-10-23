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

#ifndef OHOS_RENDER_3D_SCENE_ADAPTER_H
#define OHOS_RENDER_3D_SCENE_ADAPTER_H

#include "intf_scene_adapter.h"
#include "intf_surface_buffer_info.h"
#include <meta/interface/intf_object.h>

#include <base/containers/array_view.h>
#include <base/containers/shared_ptr.h>

#include <core/intf_engine.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/engine_info.h>
#include <core/implementation_uids.h>
#include <core/io/intf_file_manager.h>
#include <core/namespace.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/intf_property_handle.h>

#include <3d/ecs/systems/intf_node_system.h>

#include <meta/interface/intf_meta_object_lib.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/base/interface_macros.h>
#include <meta/api/make_callback.h>
#include <meta/ext/object.h>

#include <scene/base/namespace.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_material.h>

#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/implementation_uids.h>
#include <render/gles/intf_device_gles.h>
#include <render/intf_renderer.h>
#include <render/intf_render_context.h>
#include <render/util/intf_render_frame_util.h>

#include <ohos/platform_data.h>

namespace OHOS::Render3D {

class TextureLayer;

class SceneAdapter : public ISceneAdapter {
public:
    SceneAdapter();
    bool LoadPluginsAndInit() override; // it should be static function
    std::shared_ptr<TextureLayer> CreateTextureLayer() override;
    void OnWindowChange(const WindowChangeInfo& windowChangeInfo) override;
    void RenderFrame(bool needsSyncPaint = false) override;
    void Deinit() override;
    void SetNeedsRepaint(bool needsRepaint);
    bool NeedsRepaint() override;
    void AcquireImage(const SurfaceBufferInfo &bufferInfo) override;
    virtual void SetSceneObj(META_NS::IObject::Ptr pt);
    static void ShutdownPluginRegistry();
    static void DeinitRenderThread();
    ~SceneAdapter() override;
    static bool IsEngineInitSuccessful()
    {
        return engineInitSuccessful_;
    }

protected:
    static bool LoadEngineLib();
    static bool LoadPlugins(const CORE_NS::PlatformCreateInfo& platformCreateInfo);
    static bool InitEngine(CORE_NS::PlatformCreateInfo platformCreateInfo);
    void AttachSwapchain(META_NS::IObject::Ptr camera);
    void RenderFunction();
    void CreateRenderFunction();
    void UpdateSurfaceBuffer();
    void InitEnvironmentResource(const uint32_t bufferSize);
    int32_t CreateFenceFD(const RENDER_NS::IRenderFrameUtil::SignalData &signalData, RENDER_NS::IDevice &device);
    void PropSync();

    META_NS::IObject::Ptr sceneWidgetObj_;

    RENDER_NS::RenderHandleReference swapchainHandle_;
    RENDER_NS::RenderHandleReference camTransBufferHandle_;
    RENDER_NS::RenderHandleReference bufferFenceHandle_;
    BASE_NS::refcnt_ptr<RENDER_NS::IRenderDataStoreDefaultStaging> renderDataStoreDefaultStaging_;
    CORE_NS::EntityReference camImageEF_;

    std::shared_ptr<TextureLayer> textureLayer_;
    SCENE_NS::IRenderTarget::Ptr bitmap_;
    uint32_t key_ = 0;
    bool needsRepaint_ = true;
    bool receiveBuffer_ = false;
    bool initCamRNG_ = false;
    META_NS::ITaskQueueTask::Ptr singleFrameAsync_;
    META_NS::ITaskQueueWaitableTask::Ptr singleFrameSync_;
    META_NS::ITaskQueueWaitableTask::Ptr propSyncSync_;

    SurfaceBufferInfo sfBufferInfo_;

    bool onWindowChanged_ = false; // todo engine thread check
    static bool engineInitSuccessful_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_SCENE_ADAPTER_H
