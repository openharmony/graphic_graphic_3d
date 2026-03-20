/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef OHOS_RENDER_3D_FG_MODULE_H
#define OHOS_RENDER_3D_FG_MODULE_H

#include <core/intf_engine.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/engine_info.h>
#include <core/implementation_uids.h>
#include <core/io/intf_file_manager.h>
#include <core/namespace.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/intf_property_handle.h>

#include <scene/base/namespace.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_material.h>

#include <3d/intf_graphics_context.h>

#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <string>
#include <string_view>
#include "scene_adapter/fg_component.h"

#define FG_BEGIN_NAMESPACE() namespace FG {
#define FG_END_NAMESPACE() } //FG

FG_BEGIN_NAMESPACE()
constexpr BASE_NS::string_view RENDER_DATA_STORE_FG_NAME { "RenderDataStoreFG" };
static constexpr BASE_NS::Uid UID_FG_PLUGIN { "f62d40d0-2d52-4e7c-9f11-34d1f1978f59" };
static constexpr BASE_NS::Uid UID_FG_COMPONENT_MANAGER { "d8dc6863-7347-432e-8503-06a373c0ef8f" };
struct FGData {
    bool enable_ = false;
    bool withSR_ = false;
    BASE_NS::string name_ {"fg_name_1"};
    float rate_ = 1.0;
    FGDetailConfiguration::FGQualityType quality_ = FGDetailConfiguration::FGQualityType::QUALITY_TYPE_NORMAL;
    FGDetailConfiguration::FGAlgorithm algorithm_ = FGDetailConfiguration::FGAlgorithm::FG_HYDRA;
    int width_ = 0;
    int height_ = 0;
};

class FGModule {
public:
    FGModule();

    void Init(BASE_NS::shared_ptr<SCENE_NS::IInternalScene> scene,
        BASE_NS::shared_ptr<SCENE_NS::IRenderContext> renderContext,
        BASE_NS::shared_ptr<CORE_NS::IEngine> engine,
        BASE_NS::refcnt_ptr<CORE_NS::IEcs> ecs);
    
    void Update(BASE_NS::shared_ptr<SCENE_NS::IScene> scene);

    static const FGData InitConfig();
    static const FGData GetConfig();
    static bool EnableFG();
    static bool IsEnable();
    static bool SetWindowSize(const int& width, const int& height);
    static void AttachComponent();

    static RENDER_NS::RenderHandleReference CreateGpuResource(
        BASE_NS::shared_ptr<RENDER_NS::IRenderContext> rc, float width, float height);
    
    static void CreateGpuImages(
        BASE_NS::shared_ptr<RENDER_NS::IRenderContext> rc, float width, float height,
        const FGData& fg,
        RENDER_NS::RenderHandleReference& FGColorOutputHandle_,
        RENDER_NS::RenderHandleReference& FGPredictOutputHandle_,
        RENDER_NS::RenderHandleReference& FGDepthOutputHandle_);

private:
    static inline FGData fg_;
    static BASE_NS::refcnt_ptr<CORE_NS::IEcs> ecs_;
};
FG_END_NAMESPACE()
#endif // OHOS_RENDER_3D_FG_MODULE_H