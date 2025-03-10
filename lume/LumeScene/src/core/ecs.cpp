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

#include "ecs.h"

#include <algorithm>
#include <scene/ext/util.h>

#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/implementation_uids.h>
#include <3d/util/intf_scene_util.h>
#include <core/ecs/intf_system_graph_loader.h>

#include "internal_scene.h"

SCENE_BEGIN_NAMESPACE()

static bool InitPreprocesor(const CORE_NS::IEcs& ecs, RENDER_NS::IRenderContext& context)
{
    auto renderPreprocessorSystem = CORE_NS::GetSystem<CORE3D_NS::IRenderPreprocessorSystem>(ecs);

    BASE_NS::string dataStorePrefix = "renderDataStore: " + BASE_NS::string(BASE_NS::to_string(intptr_t(&ecs)));

    auto& renderDataStoreManager = context.GetRenderDataStoreManager();

    auto sceneDataStore = dataStorePrefix + "RenderDataStoreDefaultScene";
    auto cameraDataStore = dataStorePrefix + "RenderDataStoreDefaultCamera";
    auto lightDataStore = dataStorePrefix + "RenderDataStoreDefaultLight";
    auto materialDataStore = dataStorePrefix + "RenderDataStoreDefaultMaterial";
    auto morphDataStore = dataStorePrefix + "RenderDataStoreMorph";

    if (renderPreprocessorSystem) {
        CORE3D_NS::IRenderPreprocessorSystem::Properties props;
        props.dataStorePrefix = dataStorePrefix;
        props.dataStoreScene = sceneDataStore;
        props.dataStoreCamera = cameraDataStore;
        props.dataStoreLight = lightDataStore;
        props.dataStoreMaterial = materialDataStore;
        props.dataStoreMorph = morphDataStore;

        *CORE_NS::ScopedHandle<CORE3D_NS::IRenderPreprocessorSystem::Properties>(
            renderPreprocessorSystem->GetProperties()) = props;
    }
    return renderPreprocessorSystem != nullptr;
}

template<typename Manager>
Manager* Ecs::GetCoreManager()
{
    auto m = CORE_NS::GetManager<Manager>(*ecs);
    if (m) {
        components_[m->GetName()] = m;
    }
    return m;
}

bool Ecs::Initialize(const BASE_NS::shared_ptr<IInternalScene>& scene)
{
    using namespace CORE_NS;
    if (!scene) {
        CORE_LOG_E("invalid scene");
        return false;
    }
    scene_ = scene;
    auto& context = scene->GetRenderContext();
    auto& engine = context.GetEngine();

    ecs = engine.CreateEcs();
    ecs->SetRenderMode(CORE_NS::IEcs::RENDER_ALWAYS);

    auto* factory = GetInstance<ISystemGraphLoaderFactory>(UID_SYSTEM_GRAPH_LOADER);
    auto systemGraphLoader = factory->Create(engine.GetFileManager());
    // Use default graph.
    systemGraphLoader->Load(scene->GetSystemGraphUri().c_str(), *ecs);

    InitPreprocesor(*ecs, context);
    ecs->Initialize();

    animationComponentManager = GetCoreManager<CORE3D_NS::IAnimationComponentManager>();
    animationStateComponentManager = GetCoreManager<CORE3D_NS::IAnimationStateComponentManager>();
    cameraComponentManager = GetCoreManager<CORE3D_NS::ICameraComponentManager>();
    envComponentManager = GetCoreManager<CORE3D_NS::IEnvironmentComponentManager>();
    layerComponentManager = GetCoreManager<CORE3D_NS::ILayerComponentManager>();
    lightComponentManager = GetCoreManager<CORE3D_NS::ILightComponentManager>();
    materialComponentManager = GetCoreManager<CORE3D_NS::IMaterialComponentManager>();
    meshComponentManager = GetCoreManager<CORE3D_NS::IMeshComponentManager>();
    nameComponentManager = GetCoreManager<CORE3D_NS::INameComponentManager>();
    nodeComponentManager = GetCoreManager<CORE3D_NS::INodeComponentManager>();
    renderMeshComponentManager = GetCoreManager<CORE3D_NS::IRenderMeshComponentManager>();
    rhComponentManager = GetCoreManager<CORE3D_NS::IRenderHandleComponentManager>();
    transformComponentManager = GetCoreManager<CORE3D_NS::ITransformComponentManager>();
    uriComponentManager = GetCoreManager<CORE3D_NS::IUriComponentManager>();
    renderConfigComponentManager = GetCoreManager<CORE3D_NS::IRenderConfigurationComponentManager>();
    postProcessComponentManager = GetCoreManager<CORE3D_NS::IPostProcessComponentManager>();
    localMatrixComponentManager = GetCoreManager<CORE3D_NS::ILocalMatrixComponentManager>();
    worldMatrixComponentManager = GetCoreManager<CORE3D_NS::IWorldMatrixComponentManager>();
    textComponentManager = GetCoreManager<TEXT3D_NS::ITextComponentManager>();
    if (animationComponentManager) {
        animationQuery.reset(new CORE_NS::ComponentQuery());
        animationQuery->SetEcsListenersEnabled(true);
        const ComponentQuery::Operation operations[] = {
            {
                *nodeComponentManager, ComponentQuery::Operation::OPTIONAL
            },
            {
                *nameComponentManager, ComponentQuery::Operation::OPTIONAL
            }
        };
        animationQuery->SetupQuery(*animationComponentManager, operations);
    }

    if (meshComponentManager) {
        meshQuery.reset(new CORE_NS::ComponentQuery());
        meshQuery->SetEcsListenersEnabled(true);
        const ComponentQuery::Operation operations[] = {
            {
                *nodeComponentManager, ComponentQuery::Operation::OPTIONAL
            },
            {
                *nameComponentManager, ComponentQuery::Operation::OPTIONAL
            }
        };
        meshQuery->SetupQuery(*meshComponentManager, operations);
    }

    if (materialComponentManager) {
        materialQuery.reset(new CORE_NS::ComponentQuery());
        materialQuery->SetEcsListenersEnabled(true);
        const ComponentQuery::Operation operations[] = {
            {
                *nodeComponentManager, ComponentQuery::Operation::OPTIONAL
            },
            {
                *nameComponentManager, ComponentQuery::Operation::OPTIONAL
            },
            {
                *uriComponentManager, ComponentQuery::Operation::OPTIONAL
            }
        };
        materialQuery->SetupQuery(*materialComponentManager, operations);
    }
    nodeSystem = GetSystem<CORE3D_NS::INodeSystem>(*ecs);
    picking = GetInstance<CORE3D_NS::IPicking>(*context.GetInterface<IClassRegister>(), CORE3D_NS::UID_PICKING);

#ifdef __PHYSICS_MODULE__
    const auto& rngGroup = scene->GetCustomRngGroupUri();
    if (!(rngGroup.lwrp.empty() && rngGroup.lwrpMsaa.empty() &&
        rngGroup.hdrp.empty() && rngGroup.hdrpMsaa.empty())) {
        scene->SetCustomRngGroup();
    }
#endif
    if (!EcsListener::Initialize(*this)) {
        CORE_LOG_E("failed to initialize ecs listener");
        return false;
    }

    return true;
}

void Ecs::Uninitialize()
{
    EcsListener::Uninitialize();
    animationComponentManager = {};
    cameraComponentManager = {};
    envComponentManager = {};
    layerComponentManager = {};
    lightComponentManager = {};
    materialComponentManager = {};
    meshComponentManager = {};
    nameComponentManager = {};
    nodeComponentManager = {};
    renderMeshComponentManager = {};
    rhComponentManager = {};
    transformComponentManager = {};
    uriComponentManager = {};
    renderConfigComponentManager = {};
    meshQuery.reset();
    materialQuery.reset();
    animationQuery.reset();
    rootNode_ = {};
    nodeSystem = {};
    picking = {};
    components_.clear();
    if (ecs) {
        ecs->Uninitialize();
        ecs.reset();
    }
}

CORE_NS::IComponentManager* Ecs::FindComponent(BASE_NS::string_view name) const
{
    auto it = components_.find(name);
    return it != components_.end() ? it->second : nullptr;
}

const CORE3D_NS::ISceneNode* Ecs::FindNode(BASE_NS::string_view path) const
{
    if (path.empty() || path == "/") {
        return rootNode_;
    }
    const CORE3D_NS::ISceneNode* node = &nodeSystem->GetRootNode();
    BASE_NS::string_view p = FirstSegment(path);
    while (node && !path.empty()) {
        node = node->GetChild(p);
        path.remove_prefix(p.size() + 1);
        p = FirstSegment(path);
    }
    return node;
}

const CORE3D_NS::ISceneNode* Ecs::FindNodeParent(BASE_NS::string_view path) const
{
    return FindNode(NormalisePath(ParentPath(path)));
}

BASE_NS::string Ecs::GetPath(const CORE3D_NS::ISceneNode* node) const
{
    BASE_NS::string path;
    while (node && node->GetParent()) {
        path = "/" + node->GetName() + path;
        node = node->GetParent();
    }
    return path;
}

BASE_NS::string Ecs::GetPath(CORE_NS::Entity ent) const
{
    return GetPath(nodeSystem->GetNode(ent));
}

bool Ecs::IsNodeEntity(CORE_NS::Entity ent) const
{
    return nodeSystem->GetNode(ent) != nullptr;
}

CORE_NS::Entity Ecs::GetRootEntity() const
{
    return rootNode_ ? rootNode_->GetEntity() : CORE_NS::Entity {};
}

IEcsObject::Ptr Ecs::GetEcsObject(CORE_NS::Entity ent)
{
    if (auto obj = FindEcsObject(ent)) {
        return obj;
    }

    auto& r = META_NS::GetObjectRegistry();
    auto md = r.Create<META_NS::IMetadata>(META_NS::ClassId::Object);
    md->AddProperty(META_NS::ConstructProperty<IInternalScene::Ptr>("Scene", scene_.lock()));
    md->AddProperty(META_NS::ConstructProperty<CORE_NS::Entity>("Entity", ent));
    auto obj = r.Create<IEcsObject>(ClassId::EcsObject, md);
    if (obj) {
        RegisterEcsObject(obj);
    }

    return obj;
}

void Ecs::RemoveEcsObject(const IEcsObject::ConstPtr& obj)
{
    DeregisterEcsObject(obj);
}

bool Ecs::CreateUnnamedRootNode()
{
    if (rootNode_) {
        return false;
    }
    rootNode_ = nodeSystem->CreateNode();
    for (auto&& c : nodeSystem->GetRootNode().GetChildren()) {
        if (c != rootNode_) {
            c->SetParent(*rootNode_);
        }
    }
    rootNode_->SetParent(nodeSystem->GetRootNode());
    AddDefaultComponents(rootNode_->GetEntity());
    return true;
}

bool Ecs::SetNodeParentAndName(CORE_NS::Entity ent, BASE_NS::string_view name, const CORE3D_NS::ISceneNode* parent)
{
    auto n = nodeSystem->GetNode(ent);
    if (n) {
        if (parent) {
            n->SetParent(*parent);
        }
        n->SetName(name);
    }
    return n != nullptr;
}

CORE3D_NS::ISceneNode* Ecs::GetNode(CORE_NS::Entity ent)
{
    return nodeSystem->GetNode(ent);
}
const CORE3D_NS::ISceneNode* Ecs::GetNode(CORE_NS::Entity ent) const
{
    return nodeSystem->GetNode(ent);
}
void Ecs::RemoveNode(CORE_NS::Entity ent)
{
    if (auto n = nodeSystem->GetNode(ent)) {
        n->SetParent(nodeSystem->GetRootNode());
    }
}

void Ecs::AddDefaultComponents(CORE_NS::Entity ent) const
{
    auto create = [&](auto* component) {
        if (component) {
            if (!component->HasComponent(ent)) {
                component->Create(ent);
            }
        }
    };
    create(nameComponentManager);
    create(nodeComponentManager);
    create(transformComponentManager);
    create(localMatrixComponentManager);
    create(worldMatrixComponentManager);
}

CORE_NS::EntityReference CopyExternalAsChild(const IEcsObject& parent, const IEcsObject& extChild)
{
    IInternalScene::Ptr localScene = parent.GetScene();
    IInternalScene::Ptr extScene = extChild.GetScene();
    if (!localScene || !extScene) {
        return {};
    }

    auto& util = localScene->GetGraphicsContext().GetSceneUtil();

    auto ent = util.Clone(*localScene->GetEcsContext().GetNativeEcs(), parent.GetEntity(),
        *extScene->GetEcsContext().GetNativeEcs(), extChild.GetEntity());

    return localScene->GetEcsContext().GetNativeEcs()->GetEntityManager().GetReferenceCounted(ent);
}

static CORE_NS::EntityReference ReparentOldRoot(
    IInternalScene::Ptr scene, const IEcsObject& parent, const BASE_NS::vector<CORE_NS::Entity>& entities)
{
    auto nodeSystem = CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(*scene->GetEcsContext().GetNativeEcs());
    if (!nodeSystem) {
        return {};
    }
    auto& root = nodeSystem->GetRootNode();
    CORE3D_NS::ISceneNode* node = nullptr;
    for (auto&& v : root.GetChildren()) {
        if (std::find(entities.begin(), entities.end(), v->GetEntity()) != entities.end()) {
            node = v;
            break;
        }
    }
    if (node) {
        if (auto parentNode = nodeSystem->GetNode(parent.GetEntity())) {
            node->SetParent(*parentNode);
        } else {
            CORE_LOG_W("Invalid parent when import scene");
        }
    } else {
        CORE_LOG_W("Failed to find old root when import scene");
    }
    return scene->GetEcsContext().GetNativeEcs()->GetEntityManager().GetReferenceCounted(node->GetEntity());
}

CORE_NS::EntityReference CopyExternalAsChild(const IEcsObject& parent, const IScene& scene)
{
    IInternalScene::Ptr localScene = parent.GetScene();
    IInternalScene::Ptr extScene = scene.GetInternalScene();
    if (!localScene || !extScene) {
        return {};
    }

    auto& util = localScene->GetGraphicsContext().GetSceneUtil();
    auto vec = util.Clone(*localScene->GetEcsContext().GetNativeEcs(), *extScene->GetEcsContext().GetNativeEcs());
    return ReparentOldRoot(localScene, parent, vec);
}

SCENE_END_NAMESPACE()
