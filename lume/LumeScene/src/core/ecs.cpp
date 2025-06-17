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
#include <random>
#include <scene/ext/util.h>

#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/implementation_uids.h>
#include <3d/util/intf_scene_util.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <render/datastore/intf_render_data_store.h>

#include "ecs_object.h"
#include "internal_scene.h"

SCENE_BEGIN_NAMESPACE()

template<typename Manager>
Manager* Ecs::GetCoreManager()
{
    auto m = CORE_NS::GetManager<Manager>(*ecs);
    if (m) {
        components_[m->GetName()] = m;
    }
    return m;
}

bool Ecs::Initialize(const BASE_NS::shared_ptr<IInternalScene>& scene, const SceneOptions& opts)
{
    using namespace CORE_NS;
    scene_ = scene;
    auto& context = scene->GetRenderContext();
    auto& engine = context.GetEngine();

    ecs = engine.CreateEcs();
    ecs->SetRenderMode(CORE_NS::IEcs::RENDER_ALWAYS);

    if (!opts.systemGraphUri.empty()) {
        auto* factory = GetInstance<ISystemGraphLoaderFactory>(UID_SYSTEM_GRAPH_LOADER);
        auto systemGraphLoader = factory->Create(engine.GetFileManager());
        if (systemGraphLoader->Load(opts.systemGraphUri, *ecs).success) {
            CORE_LOG_D("Using custom system graph: %s", opts.systemGraphUri.c_str());
        }
    }

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
    morphComponentManager = GetCoreManager<CORE3D_NS::IMorphComponentManager>();

    if (animationComponentManager) {
        animationQuery.reset(new CORE_NS::ComponentQuery());
        animationQuery->SetEcsListenersEnabled(true);
        const ComponentQuery::Operation operations[] = { { *nodeComponentManager, ComponentQuery::Operation::OPTIONAL },
            { *nameComponentManager, ComponentQuery::Operation::OPTIONAL } };
        animationQuery->SetupQuery(*animationComponentManager, operations);
    }

    if (meshComponentManager) {
        meshQuery.reset(new CORE_NS::ComponentQuery());
        meshQuery->SetEcsListenersEnabled(true);
        const ComponentQuery::Operation operations[] = { { *nodeComponentManager, ComponentQuery::Operation::OPTIONAL },
            { *nameComponentManager, ComponentQuery::Operation::OPTIONAL } };
        meshQuery->SetupQuery(*meshComponentManager, operations);
    }

    if (materialComponentManager) {
        materialQuery.reset(new CORE_NS::ComponentQuery());
        materialQuery->SetEcsListenersEnabled(true);
        const ComponentQuery::Operation operations[] = { { *nodeComponentManager, ComponentQuery::Operation::OPTIONAL },
            { *nameComponentManager, ComponentQuery::Operation::OPTIONAL },
            { *uriComponentManager, ComponentQuery::Operation::OPTIONAL } };
        materialQuery->SetupQuery(*materialComponentManager, operations);
    }
    nodeSystem = GetSystem<CORE3D_NS::INodeSystem>(*ecs);
    picking = GetInstance<CORE3D_NS::IPicking>(*context.GetInterface<IClassRegister>(), CORE3D_NS::UID_PICKING);

    entityOwnerComponentManager =
        static_cast<IEntityOwnerComponentManager*>(ecs->CreateComponentManager(ENTITY_OWNER_COMPONENT_TYPE_INFO));

    if (!entityOwnerComponentManager || !nodeSystem || !picking) {
        return false;
    }

    components_[entityOwnerComponentManager->GetName()] = entityOwnerComponentManager;

    if (!EcsListener::Initialize(*this)) {
        CORE_LOG_E("failed to initialize ecs listener");
        return false;
    }

    return true;
}

void Ecs::Uninitialize()
{
    EcsListener::Uninitialize();
    nodeSystem->RemoveListener(*this);
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
    morphComponentManager = {};
    meshQuery.reset();
    materialQuery.reset();
    animationQuery.reset();
    rootEntity_ = {};
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

CORE3D_NS::ISceneNode* Ecs::FindNode(BASE_NS::string_view path)
{
    if (path.empty() || path == "/") {
        return GetNode(rootEntity_);
    }
    CORE3D_NS::ISceneNode* node = &nodeSystem->GetRootNode();
    BASE_NS::string_view p = FirstSegment(path);
    while (node && !path.empty()) {
        node = node->GetChild(p);
        path.remove_prefix(p.size() + 1);
        p = FirstSegment(path);
    }
    return node;
}

CORE3D_NS::ISceneNode* Ecs::FindNodeParent(BASE_NS::string_view path)
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
    return rootEntity_;
}

CORE_NS::Entity Ecs::GetParent(CORE_NS::Entity ent) const
{
    if (auto n = nodeComponentManager->Read(ent)) {
        return n->parent;
    }
    return {};
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

bool Ecs::RemoveEntity(CORE_NS::Entity ent)
{
    bool res = CORE_NS::EntityUtil::IsValid(ent);
    if (res) {
        DeregisterEcsObject(ent);
        ecs->GetEntityManager().Destroy(ent);
    }
    return res;
}

bool Ecs::CreateUnnamedRootNode()
{
    if (CORE_NS::EntityUtil::IsValid(rootEntity_)) {
        return false;
    }

    BASE_NS::vector<CORE_NS::Entity> children;
    for (auto&& c : nodeSystem->GetRootNode().GetChildren()) {
        children.push_back(c->GetEntity());
    }

    auto root = nodeSystem->CreateNode();
    rootEntity_ = root->GetEntity();
    root->SetParent(nodeSystem->GetRootNode());

    for (auto&& n : children) {
        if (auto root = GetNode(rootEntity_)) {
            if (auto child = GetNode(n)) {
                child->SetParent(*root);
            }
        }
    }

    AddDefaultComponents(rootEntity_);
    return true;
}

bool Ecs::SetNodeName(CORE_NS::Entity ent, BASE_NS::string_view name)
{
    auto n = nodeSystem->GetNode(ent);
    if (n) {
        n->SetName(name);
    }
    return n != nullptr;
}

bool Ecs::SetNodeParentAndName(CORE_NS::Entity ent, BASE_NS::string_view name, CORE3D_NS::ISceneNode* parent)
{
    auto n = nodeSystem->GetNode(ent);
    if (n) {
        n->SetName(name);
        if (parent) {
            parent->AddChild(*n);
        }
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

CORE_NS::EntityReference Ecs::GetRenderHandleEntity(const RENDER_NS::RenderHandleReference& handle)
{
    CORE_NS::EntityReference ent;
    if (handle && rhComponentManager) {
        ent = CORE3D_NS::GetOrCreateEntityReference(ecs->GetEntityManager(), *rhComponentManager, handle);
    }
    return ent;
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

void Ecs::ListenNodeChanges(bool enabled)
{
    if (enabled) {
        nodeSystem->AddListener(*this);
    } else {
        nodeSystem->RemoveListener(*this);
    }
}

void Ecs::OnChildChanged(const CORE3D_NS::ISceneNode& parent, CORE3D_NS::INodeSystem::SceneNodeListener::EventType type,
    const CORE3D_NS::ISceneNode& child, size_t index)
{
    if (auto i = interface_pointer_cast<IOnNodeChanged>(scene_)) {
        META_NS::ContainerChangeType cchange {};
        if (type == CORE3D_NS::INodeSystem::SceneNodeListener::EventType::ADDED) {
            cchange = META_NS::ContainerChangeType::ADDED;
        } else if (type == CORE3D_NS::INodeSystem::SceneNodeListener::EventType::REMOVED) {
            cchange = META_NS::ContainerChangeType::REMOVED;
        } else {
            // invalid event
            return;
        }
        i->OnChildChanged(cchange, parent.GetEntity(), child.GetEntity(), index);
    }
}

bool Ecs::CheckDeactivatedAncestry(
    CORE_NS::Entity start, CORE_NS::Entity node, std::set<CORE_NS::Entity>& entities) const
{
    if (start == node) {
        return true;
    }
    if (CORE_NS::EntityUtil::IsValid(node)) {
        if (auto n = nodeComponentManager->Read(node)) {
            auto ret = CheckDeactivatedAncestry(start, n->parent, entities);
            if (ret) {
                entities.insert(node);
            }
            return ret;
        }
    }
    return false;
}

std::set<CORE_NS::Entity> Ecs::GetDeactivatedChildren(CORE_NS::Entity ent) const
{
    std::set<CORE_NS::Entity> entities;

    if (nodeComponentManager->HasComponent(ent)) {
        auto& entMan = ecs->GetEntityManager();
        auto it = entMan.Begin(CORE_NS::IEntityManager::IteratorType::DEACTIVATED);
        auto end = entMan.End(CORE_NS::IEntityManager::IteratorType::DEACTIVATED);

        BASE_NS::vector<CORE_NS::Entity> nodes;
        for (; it && !it->Compare(end); it->Next()) {
            auto e = it->Get();
            if (nodeComponentManager->HasComponent(e)) {
                nodes.push_back(e);
            }
        }
        for (auto&& n : nodes) {
            CheckDeactivatedAncestry(ent, n, entities);
        }
    }
    return entities;
}

void Ecs::ReactivateNodes(CORE_NS::Entity ent)
{
    auto& entMan = ecs->GetEntityManager();

    std::set<CORE_NS::Entity> reactivate = GetDeactivatedChildren(ent);
    reactivate.insert(ent);

    for (auto&& n : reactivate) {
        entMan.SetActive(n, true);
    }
}

void Ecs::SetNodesActive(CORE_NS::Entity ent, bool enabled)
{
    if (!enabled) {
        auto decents = GetNodeDescendants(ent);
        for (auto&& ent : decents) {
            ecs->GetEntityManager().SetActive(ent, false);
        }
    } else {
        ReactivateNodes(ent);
    }
}

void Ecs::GetNodeDescendants(CORE_NS::Entity ent, BASE_NS::vector<CORE_NS::Entity>& entities) const
{
    auto& entMan = ecs->GetEntityManager();
    if (entMan.IsAlive(ent)) {
        if (auto n = GetNode(ent)) {
            for (auto&& c : n->GetChildren()) {
                GetNodeDescendants(c->GetEntity(), entities);
            }
        }
    } else {
        for (auto&& e : GetDeactivatedChildren(ent)) {
            entities.push_back(e);
        }
    }
    entities.push_back(ent);
}

BASE_NS::vector<CORE_NS::Entity> Ecs::GetNodeDescendants(CORE_NS::Entity ent) const
{
    BASE_NS::vector<CORE_NS::Entity> entities;
    GetNodeDescendants(ent, entities);
    return entities;
}

CORE_NS::Entity CopyExternalAsChild(const IEcsObject& parent, const IEcsObject& extChild)
{
    IInternalScene::Ptr localScene = parent.GetScene();
    IInternalScene::Ptr extScene = extChild.GetScene();
    if (!localScene || !extScene) {
        return {};
    }

    auto& util = localScene->GetGraphicsContext().GetSceneUtil();

    auto ent = util.Clone(*localScene->GetEcsContext().GetNativeEcs(), parent.GetEntity(),
        *extScene->GetEcsContext().GetNativeEcs(), extChild.GetEntity());

    if (CORE_NS::EntityUtil::IsValid(ent)) {
        if (auto i = interface_pointer_cast<IOnNodeChanged>(localScene)) {
            size_t index(-1);
            if (auto nodeSystem =
                    CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(*localScene->GetEcsContext().GetNativeEcs())) {
                if (auto parentNode = nodeSystem->GetNode(parent.GetEntity())) {
                    auto children = parentNode->GetChildren();
                    for (auto childIndex = 0; childIndex < children.size(); ++childIndex) {
                        if (children[childIndex]->GetEntity() == ent) {
                            index = childIndex;
                            break;
                        }
                    }
                }
            } else {
                CORE_LOG_W("Failed to resolve imported child index, NodeSystem not found");
            }

            i->OnChildChanged(META_NS::ContainerChangeType::ADDED, parent.GetEntity(), ent, index);
        }
    }

    return ent;
}

static CORE_NS::Entity ReparentOldRoot(
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
            parentNode->AddChild(*node);
        } else {
            CORE_LOG_W("Invalid parent when import scene");
        }
    } else {
        CORE_LOG_W("Failed to find old root when import scene");
    }
    return node ? node->GetEntity() : CORE_NS::Entity {};
}

CORE_NS::Entity CopyExternalAsChild(
    const IEcsObject& parent, const IScene& scene, BASE_NS::vector<CORE_NS::Entity>& imported)
{
    IInternalScene::Ptr localScene = parent.GetScene();
    IInternalScene::Ptr extScene = scene.GetInternalScene();
    if (!localScene || !extScene) {
        return {};
    }

    auto& util = localScene->GetGraphicsContext().GetSceneUtil();
    imported = util.Clone(*localScene->GetEcsContext().GetNativeEcs(), *extScene->GetEcsContext().GetNativeEcs());
    return ReparentOldRoot(localScene, parent, imported);
}

SCENE_END_NAMESPACE()
