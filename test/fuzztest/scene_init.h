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

#ifndef OHOS_FUZZ_SCENE
#define OHOS_FUZZ_SCENE


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
#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>

#include <render/implementation_uids.h>
#include <render/gles/intf_device_gles.h>
#include <render/intf_renderer.h>
#include <render/intf_render_context.h>

#include <meta/base/shared_ptr.h>
#include <memory>
#include "scene_adapter/intf_scene_adapter.h"

namespace OHOS::FUZZ {

struct EngineInstance {
    void *libHandle_ = nullptr;
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext_;
    BASE_NS::shared_ptr<CORE_NS::IEngine> engine_;
    BASE_NS::shared_ptr<CORE3D_NS::IGraphicsContext> graphicsContext_;
    bool libsLoaded_ = false;
};

class ISceneInit : public Render3D::ISceneAdapter {
public:
    virtual bool LoadPluginsAndInit() = 0;
    virtual std::shared_ptr<Render3D::TextureLayer> CreateTextureLayer() = 0;
    virtual void OnWindowChange(const Render3D::WindowChangeInfo& windowChangeInfo) = 0;
    virtual void RenderFrame(bool needsSyncPaint = false) = 0;
    virtual EngineInstance& GetEngineInstance() = 0;
    virtual void Deinit() = 0;
    virtual bool NeedsRepaint() = 0;
    virtual ~ISceneInit() = default;
};

std::shared_ptr<ISceneInit> CreateFuzzScene();
}

#endif // OHOS_FUZZ_SCENE