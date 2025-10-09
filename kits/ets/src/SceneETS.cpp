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

#include "SceneETS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/property/property_events.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_mesh_resource.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_render_context.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_text.h>
#include <scene/interface/resource/intf_render_resource_manager.h>

#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>

#include "3d_widget_adapter_log.h"
#include "scene_adapter/scene_adapter.h"

// LEGACY COMPATIBILITY start
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_ecs_object_access.h>

#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>

namespace {
// fix names to match "ye olde" implementation
// the bug that unnamed nodes stops hierarchy creation also still exists, works around that issue too.
void Fixnames(SCENE_NS::IScene::Ptr scene)
{
    struct rr {
        uint32_t id_ = 1;
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
            fix();
        }
        void scan()
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
        void recurse(CORE_NS::Entity id)
        {
            CORE3D_NS::NameComponent c = nm->Get(id);
            if (c.name.empty()) {
                // create a name for unnamed node.
                c.name = "Unnamed Node ";
                c.name += BASE_NS::to_string(id_++);
                nm->Set(id, c);
            }
            for (auto c : tree[id]) {
                recurse(c);
            }
        }
        void fix()
        {
            scan();
            for (auto i : roots) {
                id_ = 1;
                // force root node name to match legacy by default.
                for (auto c : tree[i]) {
                    recurse(c);
                }
            }
        }
    } r(scene);
}

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;

}  // namespace
// LEGACY COMPATIBILITY end

namespace OHOS::Render3D {
SceneETS::SceneETS()
{
    resources_ = RenderContextETS::GetInstance().GetResources();
}

SceneETS::SceneETS(SCENE_NS::IScene::Ptr scene, std::shared_ptr<OHOS::Render3D::ISceneAdapter> sceneAdapter)
    : scene_(scene), sceneAdapter_(sceneAdapter)
{
    if (!scene_ || !sceneAdapter_) {
        WIDGET_LOGE("invalid scene or sceneAdapter");
        return;
    }

    SCENE_NS::IEnvironment::Ptr environment;
    if (auto rc = scene_->RenderConfiguration()->GetValue()) {
        environment = rc->Environment()->GetValue();
    }
    if (environment) {
        environmentETS_ = std::make_shared<EnvironmentETS>(environment, scene_, "DefaultEnv");
    } else {
        WIDGET_LOGE("no environment in scene");
    }
    resources_ = RenderContextETS::GetInstance().GetResources();
}

SceneETS::~SceneETS()
{
    Destroy();
}

void SceneETS::AddScene(META_NS::IObjectRegistry *obr, SCENE_NS::IScene::Ptr scene)
{
    if (!obr) {
        return;
    }
    auto params = interface_pointer_cast<META_NS::IMetadata>(obr->GetDefaultObjectContext());
    if (!params) {
        return;
    }
    auto duh = params->GetArrayProperty<IntfWeakPtr>("Scenes");
    if (!duh) {
        return;
    }
    duh->AddValue(interface_pointer_cast<CORE_NS::IInterface>(scene));
}

SCENE_NS::ISceneManager::Ptr SceneETS::CreateSceneManager(BASE_NS::string_view uri)
{
    auto &objRegistry = META_NS::GetObjectRegistry();
    auto objContext = interface_pointer_cast<META_NS::IMetadata>(objRegistry.GetDefaultObjectContext());

    if (uri.ends_with(".scene2")) {
        const auto renderContext = SCENE_NS::GetBuildArg<SCENE_NS::IRenderContext::Ptr>(objContext, "RenderContext");
        if (!renderContext || !renderContext->GetRenderer()) {
            WIDGET_LOGE("Unable to configure file resource manager for loading scene files: render context missing");
            return {};
        }
        auto &fileManager = renderContext->GetRenderer()->GetEngine().GetFileManager();
        fileManager.RegisterPath("project", ExtractPathToProject(uri), true);
    }

    return objRegistry.Create<SCENE_NS::ISceneManager>(SCENE_NS::ClassId::SceneManager, objContext);
}

BASE_NS::string_view SceneETS::ExtractPathToProject(BASE_NS::string_view uri)
{
    // Assume the scene file is in a folder that is at the root of the project.
    // ExtractPathToProject("schema://path/to/PROJECT/assets/default.scene2") == "schema://path/to/PROJECT"
    const auto secondToLastSlashPos = uri.find_last_of('/', uri.find_last_of('/') - 1);
    return uri.substr(0, secondToLastSlashPos);
}

bool SceneETS::Load(std::string uri)
{
    if (uri.empty()) {
        uri = "scene://empty";
    }
    for (;;) {
        auto t = uri.find_first_of('\\');
        if (t == std::string::npos) {
            break;
        }
        uri[t] = '/';
    }
    auto engineThreadTask = [this](SCENE_NS::IScene::Ptr scene) mutable {
        if (!scene || !scene->RenderConfiguration()->GetValue()) {
            return;
        }

        // Make sure there's a valid root node
        scene->GetInternalScene()->GetEcsContext().CreateUnnamedRootNode();

        // LEGACY COMPATIBILITY start
        Fixnames(scene);
        // LEGACY COMPATIBILITY end
        auto &obr = META_NS::GetObjectRegistry();
        AddScene(&obr, scene);
        scene_ = scene;

        auto curenv = GetEnvironment();
        if (!curenv) {
            // setup default env
            auto res = CreateEnvironment("DefaultEnv", "");
            if (res) {
                res.value->SetBackgroundType(
                    EnvironmentETS::EnvironmentBackgroundType::BACKGROUND_IMAGE);  // image.. but with null.
                SetEnvironment(res.value);
            }
        }

        // ai assistant: FORWARD/R16G16B16A16/MSAA     pre-loaded camera
        // hair simulation: FORWARD/R8G8B8A8_SRGB/MSAA create camera
        // soft body: FORWARD/R16G16B16A16/MSAA        create camera
        for (auto &&c : scene->GetCameras().GetResult()) {
            c->RenderingPipeline()->SetValue(SCENE_NS::CameraPipeline::FORWARD);
            c->ColorTargetCustomization()->SetValue({SCENE_NS::ColorFormat{BASE_NS::BASE_FORMAT_R16G16B16A16_SFLOAT}});
        }
#ifdef __SCENE_ADAPTER__
        // set SceneAdapter
        auto sceneAdapter = std::make_shared<OHOS::Render3D::SceneAdapter>();
        sceneAdapter->SetSceneObj(interface_pointer_cast<META_NS::IObject>(scene));
        sceneAdapter_ = sceneAdapter;
#endif
    };
    auto sceneManager = CreateSceneManager(uri.c_str());
    if (!sceneManager) {
        CORE_LOG_E("Creating scene manager failed");
        return false;
    }
    auto params = interface_pointer_cast<META_NS::IMetadata>(META_NS::GetObjectRegistry().GetDefaultObjectContext());
    auto engineQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);

    sceneManager->CreateScene(uri.c_str())
        .Then(BASE_NS::move(engineThreadTask), engineQ)
        .Wait();

    return true;
}

std::vector<std::shared_ptr<AnimationETS>> SceneETS::GetAnimations()
{
    if (animations_) {
        return animations_.value();
    }
    ExecSyncTask([this]() {
        nativeAnimations_ = scene_->GetAnimations().GetResult();
        return META_NS::IAny::Ptr {};
    });

    std::vector<std::shared_ptr<AnimationETS>> animationETSlist;
    for (const auto &animationRef : nativeAnimations_) {
        // use make_unique instead in the future.
        animationETSlist.emplace_back(std::make_shared<AnimationETS>(animationRef, scene_));
    }
    animations_ = std::move(animationETSlist);
    return animations_.value();
}

std::shared_ptr<NodeETS> SceneETS::GetNodeByPath(const std::string &path)
{
    std::shared_ptr<NodeETS> rootNode = GetRoot().value;
    if (!rootNode) {
        return nullptr;
    }
    std::string realPath = path;
    const std::string rootName = rootNode->GetName();
    if (realPath.empty() || (realPath == std::string_view("/")) || (realPath == rootName)) {
        // empty or '/' or "exact rootnodename". so return root
        return rootNode;
    }

    // remove the "root nodes name", if given (make sure it also matches though..)
    size_t pos = 0;
    if (realPath[0] != '/') {
        pos = realPath.find('/', 0);
        std::string_view step = realPath.substr(0, pos);
        if (!step.empty() && (step != rootName)) {
            // root not matching
            return nullptr;
        }
    }
    if (pos != std::string_view::npos) {
        realPath = realPath.substr(pos + 1);
    }

    if (realPath.empty()) {
        // after removing the root node name
        // nothing left in path, so return root.
        return rootNode;
    }
    return rootNode->GetNodeByPath(realPath);
}

InvokeReturn<std::shared_ptr<NodeETS>> SceneETS::CreateNode(const std::string &path)
{
    if (!scene_) {
        return InvokeReturn<std::shared_ptr<NodeETS>>(nullptr, "Invalid scene");
    }
    if (auto node = scene_->CreateNode(path.c_str(), SCENE_NS::ClassId::Node).GetResult()) {
        return InvokeReturn(std::make_shared<NodeETS>(NodeETS::NodeType::NODE, node));
    } else {
        return InvokeReturn<std::shared_ptr<NodeETS>>(
            nullptr, "Node creation failed. Is the given node path unique and valid?");
    }
}

InvokeReturn<std::shared_ptr<NodeETS>> SceneETS::GetRoot()
{
    if (!scene_) {
        return InvokeReturn<std::shared_ptr<NodeETS>>(nullptr, "Invalid scene");
    }
    return InvokeReturn(std::make_shared<NodeETS>(NodeETS::NodeType::NODE, scene_->GetRootNode().GetResult()));
}

InvokeReturn<std::shared_ptr<GeometryETS>> SceneETS::CreateGeometry(
    const std::string &path, const std::shared_ptr<MeshResourceETS> &mr)
{
    if (!scene_) {
        return InvokeReturn<std::shared_ptr<GeometryETS>>(nullptr, "Invalid scene");
    }
    auto meshNode = scene_->CreateNode(path.c_str(), SCENE_NS::ClassId::MeshNode).GetResult();
    if (auto access = interface_pointer_cast<SCENE_NS::IMeshAccess>(meshNode)) {
        const auto mesh = mr->CreateMesh(scene_);
        access->SetMesh(mesh).GetResult();
        return InvokeReturn<std::shared_ptr<GeometryETS>>(GeometryETS::FromJS(meshNode, mr->GetName(), mr->GetUri()));
    } else {
        return InvokeReturn<std::shared_ptr<GeometryETS>>(
            nullptr, "Geometry node creation failed. Is the given node path unique and valid?");
    }
}

InvokeReturn<std::shared_ptr<CameraETS>> SceneETS::CreateCamera(const std::string &path, uint32_t pipeline)
{
    // renderPipeline is (at the moment of writing) an undocumented param. Check the API docs and usage.
    // Remove this, if it has been added to the API. Else if it's not used anywhere, remove the implementation.

    if (!scene_) {
        return InvokeReturn<std::shared_ptr<CameraETS>>(nullptr, "Invalid scene");
    }
    // Don't create the camera asynchronously. There's a race condition, and we need to deactivate it immediately.
    // Otherwise we get tons of render validation issues.
    const auto camera = scene_->CreateNode<SCENE_NS::ICamera>(path.c_str(), SCENE_NS::ClassId::CameraNode).GetResult();
    if (!camera) {
        return InvokeReturn<std::shared_ptr<CameraETS>>(nullptr, "Create camera error");
    }
    camera->SetActive(false);

    camera->ColorTargetCustomization()->SetValue({SCENE_NS::ColorFormat{BASE_NS::BASE_FORMAT_R16G16B16A16_SFLOAT}});
    camera->RenderingPipeline()->SetValue(SCENE_NS::CameraPipeline(pipeline));
    return InvokeReturn(std::make_shared<CameraETS>(camera));
}

InvokeReturn<std::shared_ptr<LightETS>> SceneETS::CreateLight(
    const std::string &name, const std::string &path, LightETS::LightType lightType)
{
    if (!scene_) {
        return InvokeReturn<std::shared_ptr<LightETS>>(nullptr, "Invalid scene");
    }

    if (auto nativeLight = interface_pointer_cast<SCENE_NS::ILight>(
            scene_->CreateNode(path.c_str(), SCENE_NS::ClassId::LightNode).GetResult())) {
        return InvokeReturn(std::make_shared<LightETS>(nativeLight, lightType, name, path));
    } else {
        return InvokeReturn<std::shared_ptr<LightETS>>(nullptr, "Light creation failed");
    }
}

InvokeReturn<std::shared_ptr<MaterialETS>> SceneETS::CreateMaterial(const std::string &name, const std::string &uri,
                                                                    const MaterialETS::MaterialType &materialType)
{
    if (!scene_) {
        return InvokeReturn<std::shared_ptr<MaterialETS>>(nullptr, "Invalid scene");
    }
    if (auto mat = scene_->CreateObject<SCENE_NS::IMaterial>(SCENE_NS::ClassId::Material).GetResult()) {
        if (materialType == MaterialETS::MaterialType::SHADER) {
            META_NS::SetValue(mat->Type(), SCENE_NS::MaterialType::CUSTOM);
        }
        return InvokeReturn(std::make_shared<MaterialETS>(mat, name, uri));
    } else {
        return InvokeReturn<std::shared_ptr<MaterialETS>>(nullptr, "Material creation failed");
    }
}

InvokeReturn<std::shared_ptr<EnvironmentETS>> SceneETS::CreateEnvironment(
    const std::string &name, const std::string &uri)
{
    if (!scene_) {
        return InvokeReturn<std::shared_ptr<EnvironmentETS>>(nullptr, "Invalid scene");
    }
    if (auto nativeEnv = interface_pointer_cast<SCENE_NS::IEnvironment>(
            scene_->CreateObject(SCENE_NS::ClassId::Environment).GetResult())) {
        return InvokeReturn(std::make_shared<EnvironmentETS>(nativeEnv, scene_, name, uri));
    } else {
        return InvokeReturn<std::shared_ptr<EnvironmentETS>>(nullptr, "Environment creation failed");
    }
}

InvokeReturn<std::shared_ptr<EnvironmentETS>> SceneETS::GetEnvironment()
{
    if (!scene_) {
        return InvokeReturn<std::shared_ptr<EnvironmentETS>>(nullptr, "Invalid scene");
    }
    SCENE_NS::IEnvironment::Ptr environment;
    if (auto rc = scene_->RenderConfiguration()->GetValue()) {
        environment = rc->Environment()->GetValue();
    }
    if (environment) {
        if (environmentETS_ &&
            environmentETS_->GetNativeObj() == interface_pointer_cast<META_NS::IObject>(environment)) {
            return InvokeReturn(environmentETS_);
        }
        CORE_LOG_E("no environmentETS_, do not expect go here");
        environmentETS_ = std::make_shared<EnvironmentETS>(environment, scene_);
        return InvokeReturn(environmentETS_);
    }
    return InvokeReturn<std::shared_ptr<EnvironmentETS>>(nullptr, "no environment in rendercontext");
}

void SceneETS::SetEnvironment(const std::shared_ptr<EnvironmentETS> environmentETS)
{
    if (!scene_) {
        CORE_LOG_E("empty scene object");
        return;
    }
    if (!environmentETS) {
        CORE_LOG_E("empty environmentETS");
        return;
    }
    if (environmentETS == environmentETS_) {
        return;  // setting the exactly the same environment. do nothing.
    }
    environmentETS_ = environmentETS;
    auto environment = interface_pointer_cast<SCENE_NS::IEnvironment>(environmentETS_->GetNativeObj());

    if (auto rc = scene_->RenderConfiguration()->GetValue()) {
        rc->Environment()->SetValue(environment);
    }
}

bool SceneETS::RenderFrame(RenderParameters renderParam)
{
    if (!scene_) {
        CORE_LOG_E("empty scene object");
        return false;
    }
    bool alwaysRender = renderParam.valid_ ? true : renderParam.alwaysRender_;
    if (alwaysRender != currentAlwaysRender_) {
        currentAlwaysRender_ = alwaysRender;
        if (alwaysRender) {
            scene_->SetRenderMode(SCENE_NS::RenderMode::ALWAYS);
        } else {
            scene_->SetRenderMode(SCENE_NS::RenderMode::IF_DIRTY);
        }
    }

    if (auto sceneAdapter = std::static_pointer_cast<OHOS::Render3D::SceneAdapter>(sceneAdapter_)) {
        sceneAdapter->SetNeedsRepaint(false);
        sceneAdapter->RenderFrame(false);
        return true;
    } else {
        return false;
    }
}

std::shared_ptr<NodeETS> SceneETS::ImportNode(const std::string &name, std::shared_ptr<NodeETS> node,
    std::shared_ptr<NodeETS> parent)
{
    if (!scene_ || !node) {
        CORE_LOG_E("empty scene/node object");
        return nullptr;
    }
    SCENE_NS::INode::Ptr nodeObj = node->GetInternalNode();
    SCENE_NS::INode::Ptr parentObj;
    if (parent) {
        parentObj = parent->GetInternalNode();
    } else {
        parentObj = scene_->GetRootNode().GetResult();
    }

    if (auto import = interface_cast<SCENE_NS::INodeImport>(parentObj)) {
        auto importedNode = import->ImportChild(nodeObj).GetResult();
        if (!name.empty()) {
            if (auto named = interface_cast<META_NS::INamed>(importedNode)) {
                named->Name()->SetValue(name.c_str());
            }
        }
        ResetAnimations();
        return std::make_shared<NodeETS>(importedNode);
    }
    return nullptr;
}

std::shared_ptr<NodeETS> SceneETS::ImportScene(const std::string &name, std::shared_ptr<SceneETS> scene,
    std::shared_ptr<NodeETS> parent)
{
    if (!scene_ || !scene) {
        CORE_LOG_E("empty scene/importedScene object");
        return nullptr;
    }
    SCENE_NS::IScene::Ptr extScene = scene->GetNativeScene();
    SCENE_NS::INode::Ptr parentObj;
    if (parent) {
        parentObj = parent->GetInternalNode();
    } else {
        parentObj = scene_->GetRootNode().GetResult();
    }

    if (auto import = interface_cast<SCENE_NS::INodeImport>(parentObj)) {
        auto importedNode = import->ImportChildScene(extScene, name.c_str()).GetResult();
        ResetAnimations();
        return std::make_shared<NodeETS>(importedNode);
    }
    return nullptr;
}

InvokeReturn<std::shared_ptr<SceneComponentETS>> SceneETS::CreateComponent(std::shared_ptr<NodeETS> node,
    const std::string &name)
{
    if (!scene_ || !node || name.empty()) {
        return InvokeReturn<std::shared_ptr<SceneComponentETS>>(nullptr, "Invalid scene or input");
    }

    auto nodeObj = node->GetInternalNode();
    if (auto comp = scene_->CreateComponent(nodeObj, name.c_str()).GetResult()) {
        return InvokeReturn(std::make_shared<SceneComponentETS>(comp, name));
    }
    return InvokeReturn<std::shared_ptr<SceneComponentETS>>(nullptr, "CreateComponent failed");
}

InvokeReturn<std::shared_ptr<SceneComponentETS>> SceneETS::GetComponent(std::shared_ptr<NodeETS> node,
    const std::string &name)
{
    if (!scene_ || !node || name.empty()) {
        return InvokeReturn<std::shared_ptr<SceneComponentETS>>(nullptr, "Invalid scene or input");
    }

    auto nodeObj = node->GetInternalNode();
    if (auto attach = interface_pointer_cast<META_NS::IAttach>(nodeObj)) {
        if (auto cont = attach->GetAttachmentContainer(false)) {
            if (auto comp = cont->FindByName<SCENE_NS::IComponent>(name.c_str())) {
                return InvokeReturn(std::make_shared<SceneComponentETS>(comp, name));
            }
        }
    }
    return InvokeReturn<std::shared_ptr<SceneComponentETS>>(nullptr, "GetComponent failed");
}

void SceneETS::Destroy()
{
    if (disposed_) {
        return;
    }
    disposed_ = true;

    // what should be released ???
#ifdef __SCENE_ADAPTER__
    if (auto sceneAdapter = std::static_pointer_cast<OHOS::Render3D::SceneAdapter>(sceneAdapter_)) {
        sceneAdapter->Deinit();
    }
    sceneAdapter_.reset();
#endif
    environmentETS_.reset();
    scene_.reset();
    resources_.reset();
    ResetAnimations();
}

void SceneETS::ResetAnimations()
{
    if (animations_) {
        animations_.value().clear();
    }
    animations_ = std::nullopt;
    nativeAnimations_.clear();
}
}  // namespace OHOS::Render3D