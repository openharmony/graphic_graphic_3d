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

#include "scene_adapter/intf_mrt_depth_adapter.h"
#include "scene_adapter/scene_adapter.h"
#include <dlfcn.h>
#include <atomic>
#include <memory>
#include <string_view>
#include <string>
#include <sys/syscall.h>
#include <cinttypes>

#include <base/containers/array_view.h>
#include <base/containers/shared_ptr.h>

#include <core/intf_engine.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/engine_info.h>
#include <core/namespace.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/intf_property_handle.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES3/gl32.h>

#include <meta/interface/intf_meta_object_lib.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/base/interface_macros.h>
#include <meta/api/make_callback.h>
#include <meta/ext/object.h>

#include <png/implementation_uids.h>
#include <scene/base/namespace.h>
#include <scene/interface/intf_scene.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/uri_component.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_material.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_camera.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/interface/intf_application_context.h>

#include <scene/ext/intf_ecs_object_access.h>
#include "ability.h"

#include "data_ability_helper.h"
#include <sys/resource.h>
#include "napi_base_context.h"

#include <render/implementation_uids.h>

#include "3d_widget_adapter_log.h"
#include "widget_trace.h"

#include <cam_preview/namespace.h>
#include <cam_preview/implementation_uids.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/loaders/intf_scene_loader.h>
#include <3d/ecs/components/transform_component.h>

// dfx switch
#include <parameter.h>
#include <parameters.h>
#include "param/sys_param.h"
namespace OHOS::Render3D {
static constexpr BASE_NS::Uid ENGINE_THREAD{ "2070e705-d061-40e4-bfb7-90fad2c280af" };

static const std::string UID_MRT_PLUGIN { "a1b2c3d4-e5f6-7890-abcd-ef123456daca" };
static bool MRTDFXEnabled()
{
    // only read parameter upon restart of the process
    // avoid numerous IO load
    static bool dfxEnabled =
        std::atoi(system::GetParameter("AGP_MRT_DEBUG", "0").c_str()) == 1;
    return dfxEnabled;
}

static bool MRTDisabled()
{
    // force disable
    static bool disabled =
        std::atoi(system::GetParameter("AGP_MRT_FORCE_DISABLE", "0").c_str()) == 1;
    return disabled;
}

#define CHECK_NULL_RET_LOGE(ptr, ret)                        \
    do {                                                     \
        if (!(ptr)) {                                        \
            WIDGET_LOGE("%s is null in %s", #ptr, __func__); \
            return ret;                                      \
        }                                                    \
    } while (0)

class MrtDepthAdapter : public IMrtDepthAdapter {
public:
    MrtDepthAdapter()
    {
        WIDGET_LOGI("MrtDepthAdapter::MrtDepthAdapter()");
        sceneAdapter_ = BASE_NS::make_shared<SceneAdapter>();
        sceneAdapter_->LoadPluginsAndInit();
        engineInited_ = true;
    }

    void CreateSceneByGltfUri(std::string u) override
    {
        WIDGET_LOGI("MrtDepthAdapter::CreateSceneByGltfUri with uri: %{public}s", u.c_str());
        // wait to add implementation
        sceneInited_ = true;
    }

    bool OnWindowChange(const std::vector<WindowChangeInfo>& vWindowChangeInfo) override
    {
        // wait to add implementation
        return true;
    }

    bool RenderFrame() override
    {
        WIDGET_SCOPED_TRACE("MrtDepthAdapter::RenderFrame");

        sceneAdapter_->RenderFrame(false);
        return true;
    }

    bool CreateCamera(const CameraConfigs& p) override
    {
        CHECK_NULL_RET_LOGE(sceneAdapter_, false);
        if (MRTDFXEnabled()) {
            WIDGET_LOGI("MrtDepthAdapter::CreateCamera with config: %{public}s", p.Dump().c_str());
        }

        auto sceneObj = sceneAdapter_->GetSceneObj();
        auto scene = interface_pointer_cast<SCENE_NS::IScene>(sceneObj);
        // renderPipeline is an undocumented, deprecated param. It is in use in multiple apps, so support it for now.
        // If the documented param renderingPipeline is supplied in CameraParameters, it will take precedence.
        auto pipeline = uint32_t(SCENE_NS::CameraPipeline::LIGHT_FORWARD);
        auto deactivateCamera = [](SCENE_NS::ICamera::Ptr camera) {
            if (camera) {
                camera->SetActive(false);
            }
            return camera;
        };

        const auto engineQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
        auto camera = scene->CreateNode<SCENE_NS::ICamera>("/", SCENE_NS::ClassId::CameraNode)
                            .Then(BASE_NS::move(deactivateCamera), engineQ)
                            .GetResult();
        CHECK_NULL_RET_LOGE(camera, false);

        camera->ColorTargetCustomization()->SetValue({SCENE_NS::ColorFormat{BASE_NS::BASE_FORMAT_R16G16B16A16_SFLOAT}});
        camera->RenderingPipeline()->SetValue(SCENE_NS::CameraPipeline(pipeline));
        camera->PostProcess()->SetValue(nullptr);
        camera->NearPlane()->SetValue(p.intrinsics_.near_);
        camera->FarPlane()->SetValue(p.intrinsics_.far_);
        camera->FoV()->SetValue(p.intrinsics_.fov_);
        cameraPtr_ = camera;

        auto node = interface_pointer_cast<SCENE_NS::INode>(camera);
        node->Position()->SetValue({p.position_.x, p.position_.y, p.position_.z});
        node->Rotation()->SetValue({p.rotation_.x, p.rotation_.y, p.rotation_.z, p.rotation_.w});

        META_NS::AddFutureTaskOrRunDirectly(engineQ, [=]() {
            if (camera) {
                uint32_t flags = camera->SceneFlags()->GetValue();
                flags |= uint32_t(SCENE_NS::CameraSceneFlag::MAIN_CAMERA_BIT);
                camera->SceneFlags()->SetValue(flags);
                camera->SetActive(true);
            }
            return camera;
        }).GetResult();

        auto &clearColor = p.clearColor_;
        camera->ClearColor()->SetValue({clearColor.x, clearColor.y, clearColor.z, clearColor.w});   // RGBA

        uint32_t curBits = camera->PipelineFlags()->GetValue(); // enable camera clear
        curBits |= static_cast<uint32_t>(SCENE_NS::CameraPipelineFlag::CLEAR_COLOR_BIT);
        camera->PipelineFlags()->SetValue(curBits);
        return true;
    }

    bool SetCameraConfigs(const CameraConfigs& p) override
    {
        CHECK_NULL_RET_LOGE(cameraPtr_, false);

        cameraPtr_->NearPlane()->SetValue(p.intrinsics_.near_);
        cameraPtr_->FarPlane()->SetValue(p.intrinsics_.far_);
        cameraPtr_->FoV()->SetValue(p.intrinsics_.fov_);

        auto node = interface_pointer_cast<SCENE_NS::INode>(cameraPtr_);
        node->Position()->SetValue({p.position_.x, p.position_.y, p.position_.z});
        node->Rotation()->SetValue({p.rotation_.x, p.rotation_.y, p.rotation_.z, p.rotation_.w});

        auto &clearColor = p.clearColor_;
        cameraPtr_->ClearColor()->SetValue({clearColor.x, clearColor.y, clearColor.z, clearColor.w});   // RGBA

        if (MRTDFXEnabled()) {
            WIDGET_LOGI("OffScreenScene::SetCameraConfigs camera info: %{public}s", p.Dump().c_str());
        }
        return true;
    }
    
private:
    void SetDefaultEnvironment()
    {
        WIDGET_LOGI("OffScreenScene::SetDefaultEnvironment");

        auto sceneObj = sceneAdapter_->GetSceneObj();
        auto scene = interface_pointer_cast<SCENE_NS::IScene>(sceneObj);
        auto envObj = scene->CreateObject(SCENE_NS::ClassId::Environment).GetResult();
        auto env = interface_pointer_cast<SCENE_NS::IEnvironment>(envObj);
        env->Background()->SetValue(SCENE_NS::EnvBackgroundType::NONE);
        auto rc = scene->RenderConfiguration()->GetValue();
        if (rc) {
            rc->Environment()->SetValue(env);
        }
    }

    void AttachRootNode()
    {
        const auto engineQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
        META_NS::AddFutureTaskOrRunDirectly(engineQ, [=]() -> SCENE_NS::IScene::Ptr {
            auto sceneObj = sceneAdapter_->GetSceneObj();
            auto scene = interface_pointer_cast<SCENE_NS::IScene>(sceneObj);
            if (!scene || !scene->RenderConfiguration()->GetValue()) {
                WIDGET_LOGE("AttachRootNode create rootnode fail");
                return {};
            }
            // make sure there's a valid root node
            scene->GetInternalScene()->GetEcsContext().CreateUnnamedRootNode();
            FixName();
            return scene;
        }).GetResult();
    }

    void FixName()
    {
        auto sceneObj = sceneAdapter_->GetSceneObj();
        auto scene = interface_pointer_cast<SCENE_NS::IScene>(sceneObj);
        struct rr {
        uint32_t id = 1;
            // not actual tree, but map of entities, and their children.
            BASE_NS::unordered_map<CORE_NS::Entity, BASE_NS::vector<CORE_NS::Entity>> tree;
            BASE_NS::vector<CORE_NS::Entity> roots;
            CORE3D_NS::INodeComponentManager *cm;
            CORE3D_NS::INameComponentManager *nm;
            explicit rr(SCENE_NS::IScene::Ptr scene)
            {
                CORE_NS::IEcs::Ptr ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
                cm = CORE_NS::GetManager<CORE3D_NS::INodeComponentManager>(*ecs);
                nm = CORE_NS::GetManager<CORE3D_NS::INameComponentManager>(*ecs);
                Fix();
            }
            void Scan()
            {
                const auto count = cm->GetComponentCount();
                // collect nodes and their children.
                tree.reserve(cm->GetComponentCount());
                for (auto i = 0; i < count; i++) {
                    auto enti = cm->GetEntity(i);
                    // add node to our list. (if not yet added)
                    tree.insert({enti, {}});
                    auto parent = cm->Get(i).parent;
                    if (CORE_NS::EntityUtil::IsValid(parent)) {
                        tree[parent].push_back(enti);
                    } else {
                        // no parent, so it's a "root"
                        roots.push_back(enti);
                    }
                }
            }
            void Recurse(CORE_NS::Entity id)
            {
                CORE3D_NS::NameComponent c = nm->Get(id);
                if (c.name.empty()) {
                    // create a name for unnamed node.
                    c.name = "Unnamed Node ";
                    c.name += BASE_NS::to_string(id++);
                    nm->Set(id, c);
                }
                for (auto c : tree[id]) {
                    Recurse(c);
                }
            }
            void Fix()
            {
                Scan();
                for (auto i : roots) {
                    id = 1;
                    // force root node name to match legacy by default.
                    for (auto c : tree[i]) {
                        Recurse(c);
                    }
                }
            }
        } r(scene);
    }
    BASE_NS::shared_ptr<SceneAdapter> sceneAdapter_ = nullptr;
    BASE_NS::shared_ptr<SCENE_NS::ICamera> cameraPtr_ = nullptr;
    bool engineInited_ = false;
    bool sceneInited_ = false;
public:

    CORE_NS::IEcs::Ptr GetEcs() override
    {
        return sceneAdapter_ == nullptr ? nullptr : sceneAdapter_->GetEcs();
    }
    META_NS::IObject::Ptr GetSceneObj() override
    {
        return sceneAdapter_ == nullptr ? nullptr : sceneAdapter_->GetSceneObj();
    }
    BASE_NS::shared_ptr<SCENE_NS::ICamera> GetCamera() override
    {
        return cameraPtr_;
    }

    void Deinit(bool deinitEngine = false) override
    {
        WIDGET_LOGI("MrtDepthAdapter::Deinit %{public}s", deinitEngine ? "with engine" : " ");
        sceneAdapter_->Deinit();
        if (deinitEngine) {
            sceneAdapter_->DeinitRenderThread();
        }
        engineInited_ = false;
    }
    ~MrtDepthAdapter()
    {
        if (engineInited_) {
            this->Deinit();
        }
    }
};

BASE_NS::shared_ptr<IMrtDepthAdapter> GetMrtDepthAdapterInstance()
{
    return BASE_NS::make_shared<MrtDepthAdapter>();
}

} // namespace OHOS::Render3D