/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_3D_SCENE_ETS_H
#define OHOS_3D_SCENE_ETS_H

#include <optional>

#include <base/containers/unordered_map.h>
#include <meta/api/threading/mutex.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_object.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/resource/intf_render_resource_manager.h>

#include "scene_adapter/intf_scene_adapter.h"

#include "AnimationETS.h"
#include "CameraETS.h"
#include "EnvironmentETS.h"
#include "GeometryETS.h"
#include "LightETS.h"
#include "MaterialETS.h"
#include "MeshResourceETS.h"
#include "NodeETS.h"
#include "SceneComponentETS.h"
#include "RenderContextETS.h"
#include "Vec4Proxy.h"
#include "Utils.h"

namespace OHOS::Render3D {
class SceneETS {
public:
    struct RenderParameters {
        bool valid_ = false;
        bool alwaysRender_ = true;
    };

public:
    SceneETS();
    SceneETS(SCENE_NS::IScene::Ptr scene, std::shared_ptr<OHOS::Render3D::ISceneAdapter> sceneAdapter);
    ~SceneETS();
    bool Load(std::string uri);
    std::shared_ptr<OHOS::Render3D::ISceneAdapter> *GetSceneAdapter()
    {
        return &sceneAdapter_;
    }
    InvokeReturn<std::shared_ptr<NodeETS>> CreateNode(const std::string &path);
    InvokeReturn<std::shared_ptr<NodeETS>> GetRoot();
    InvokeReturn<std::shared_ptr<GeometryETS>> CreateGeometry(
        const std::string &path, const std::shared_ptr<MeshResourceETS> &mr);
    InvokeReturn<std::shared_ptr<CameraETS>> CreateCamera(
        const std::string &path, uint32_t pipeline = uint32_t(SCENE_NS::CameraPipeline::LIGHT_FORWARD));
    InvokeReturn<std::shared_ptr<EnvironmentETS>> CreateEnvironment(const std::string &name, const std::string &uri);
    InvokeReturn<std::shared_ptr<LightETS>> CreateLight(
        const std::string &name, const std::string &path, LightETS::LightType lightType);
    InvokeReturn<std::shared_ptr<MaterialETS>> CreateMaterial(
        const std::string &name, const std::string &uri, const MaterialETS::MaterialType &materialType);

    InvokeReturn<std::shared_ptr<EnvironmentETS>> GetEnvironment();
    void SetEnvironment(const std::shared_ptr<EnvironmentETS> environmentETS);

    std::shared_ptr<NodeETS> ImportNode(const std::string &name, std::shared_ptr<NodeETS> node,
        std::shared_ptr<NodeETS> parent);
    std::shared_ptr<NodeETS> ImportScene(const std::string &name, std::shared_ptr<SceneETS> scene,
        std::shared_ptr<NodeETS> parent);

    bool RenderFrame(RenderParameters renderParam);

    std::vector<std::shared_ptr<AnimationETS>> GetAnimations();
    std::shared_ptr<NodeETS> GetNodeByPath(const std::string &path);
    InvokeReturn<std::shared_ptr<SceneComponentETS>> CreateComponent(std::shared_ptr<NodeETS> node,
        const std::string &name);
    InvokeReturn<std::shared_ptr<SceneComponentETS>> GetComponent(std::shared_ptr<NodeETS> node,
        const std::string &name);

    void Destroy();

    SCENE_NS::IScene::Ptr GetNativeScene()
    {
        return scene_;
    }

private:
    static void AddScene(META_NS::IObjectRegistry *obr, SCENE_NS::IScene::Ptr scene);
    static void FlushScenes();
    // Based on the file extension, supply the scene manager a file resource manager to handle loading.
    static SCENE_NS::ISceneManager::Ptr CreateSceneManager(BASE_NS::string_view uri);
    static BASE_NS::string_view ExtractPathToProject(BASE_NS::string_view uri);

private:
    void ResetAnimations();
    std::shared_ptr<OHOS::Render3D::ISceneAdapter> sceneAdapter_ = nullptr;
    SCENE_NS::IScene::Ptr scene_{nullptr};

    std::shared_ptr<EnvironmentETS> environmentETS_{nullptr};
    std::shared_ptr<RenderResourcesETS> resources_;
    std::optional<std::vector<std::shared_ptr<AnimationETS>>> animations_;

    bool disposed_ = false;
    bool currentAlwaysRender_ = true;
};

}  // namespace OHOS::Render3D
#endif  // OHOS_3D_SCENE_ETS_H