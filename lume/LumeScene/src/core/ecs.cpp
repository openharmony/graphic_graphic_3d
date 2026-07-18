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
#include <cinttypes>
#include <scene/ext/scene_utils.h>
#include <scene/ext/util.h>
#include <scene/interface/resource/types.h>

#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/implementation_uids.h>
#include <3d/util/intf_scene_util.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/resources/intf_resource.h>
#include <render/datastore/intf_render_data_store.h>

#include "../resource/util.h"
#include "ecs_object.h"
#include "internal_scene.h"

#define SCENE_CHECK_ECS_THREAD 0

#if defined(SCENE_CHECK_ECS_THREAD) && SCENE_CHECK_ECS_THREAD == 1
#define SCENE_ASSERT_THREAD(thread) CORE_ASSERT((thread) == std::this_thread::get_id())
#else
#define SCENE_ASSERT_THREAD(thread) CORE_UNUSED(thread)
#endif

SCENE_BEGIN_NAMESPACE()

template <typename Manager>
Manager* Ecs::GetCoreManager()
{
    auto m = CORE_NS::GetManager<Manager>(*ecs);
    if (m) {
        components_[m->GetName()] = m;
    } else {
        CORE_LOG_W("Could not find manager '%s'", BASE_NS::string(GetName(m)).c_str());
    }
    return m;
}

void Ecs::InitializeComponentManagers()
{
    animationComponentManager = GetCoreManager<CORE3D_NS::IAnimationComponentManager>();
    animationStateComponentManager = GetCoreManager<CORE3D_NS::IAnimationStateComponentManager>();
    cameraComponentManager = GetCoreManager<CORE3D_NS::ICameraComponentManager>();
    envComponentManager = GetCoreManager<CORE3D_NS::IEnvironmentComponentManager>();
    layerComponentManager = GetCoreManager<CORE3D_NS::ILayerComponentManager>();
    lightComponentManager = GetCoreManager<CORE3D_NS::ILightComponentManager>();
    lightProbeComponentManager = GetCoreManager<CORE3D_NS::ILightProbeGroupComponentManager>();
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
    morphComponentManager = GetCoreManager<CORE3D_NS::IMorphComponentManager>();
}

bool Ecs::Initialize(const BASE_NS::shared_ptr<IInternalScene>& scene, const SceneOptions& opts)
{
    using namespace CORE_NS;
    thread_ = std::this_thread::get_id();
    scene_ = scene;
    auto& context = scene->GetRenderContext();
    auto& engine = context.GetEngine();

    ecs = engine.CreateEcs();
    ecs->SetRenderMode(CORE_NS::IEcs::RENDER_ALWAYS);

    if (!opts.systemGraphUri.empty()) {
        auto* factory = GetInstance<ISystemGraphLoaderFactory>(UID_SYSTEM_GRAPH_LOADER);
        if (factory) {
            auto systemGraphLoader = factory->Create(engine.GetFileManager());
            if (systemGraphLoader->Load(opts.systemGraphUri, *ecs).success) {
                CORE_LOG_D("Using custom system graph: %s", opts.systemGraphUri.c_str());
            }
        }
    }

    ecs->Initialize();
    InitializeComponentManagers();

    if (animationComponentManager) {
        animationQuery.reset(new CORE_NS::ComponentQuery());
        animationQuery->SetEcsListenersEnabled(true);
        const ComponentQuery::Operation operations[] = {{*nodeComponentManager, ComponentQuery::Operation::OPTIONAL},
            {*nameComponentManager, ComponentQuery::Operation::OPTIONAL}};
        animationQuery->SetupQuery(*animationComponentManager, operations);
    }

    if (meshComponentManager) {
        meshQuery.reset(new CORE_NS::ComponentQuery());
        meshQuery->SetEcsListenersEnabled(true);
        const ComponentQuery::Operation operations[] = {{*nodeComponentManager, ComponentQuery::Operation::OPTIONAL},
            {*nameComponentManager, ComponentQuery::Operation::OPTIONAL}};
        meshQuery->SetupQuery(*meshComponentManager, operations);
    }

    if (materialComponentManager) {
        materialQuery.reset(new CORE_NS::ComponentQuery());
        materialQuery->SetEcsListenersEnabled(true);
        const ComponentQuery::Operation operations[] = {{*nodeComponentManager, ComponentQuery::Operation::OPTIONAL},
            {*nameComponentManager, ComponentQuery::Operation::OPTIONAL},
            {*uriComponentManager, ComponentQuery::Operation::OPTIONAL}};
        materialQuery->SetupQuery(*materialComponentManager, operations);
    }
    nodeSystem = GetSystem<CORE3D_NS::INodeSystem>(*ecs);
    if (auto* classRegister = context.GetInterface<IClassRegister>()) {
        picking = GetInstance<CORE3D_NS::IPicking>(*classRegister, CORE3D_NS::UID_PICKING);
    }

    entityOwnerComponentManager =
        static_cast<IEntityOwnerComponentManager*>(ecs->CreateComponentManager(ENTITY_OWNER_COMPONENT_TYPE_INFO));

    if (!entityOwnerComponentManager || !nodeSystem || !picking) {
        return false;
    }

    components_[entityOwnerComponentManager->GetName()] = entityOwnerComponentManager;

    resourceComponentManager =
        static_cast<IResourceComponentManager*>(ecs->CreateComponentManager(RESOURCE_COMPONENT_TYPE_INFO));
    if (!resourceComponentManager) {
        return false;
    }
    components_[resourceComponentManager->GetName()] = resourceComponentManager;

    if (!EcsListener::Initialize(*this)) {
        CORE_LOG_E("failed to initialize ecs listener");
        return false;
    }

    return true;
}

void Ecs::Uninitialize()
{
    thread_ = {};
    EcsListener::Uninitialize();
    nodeSystem->RemoveListener(*this);
    animationComponentManager = {};
    cameraComponentManager = {};
    envComponentManager = {};
    layerComponentManager = {};
    lightComponentManager = {};
    lightProbeComponentManager = {};
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
    externalPicking_.reset();
    components_.clear();
    if (ecs) {
        ecs->Uninitialize();
        ecs.reset();
    }
}

bool Ecs::IsDefaultComponent(BASE_NS::string_view n) const
{
    auto hasName = [&](auto* component) { return component && component->GetName() == n; };
    return hasName(nameComponentManager) || hasName(nodeComponentManager) || hasName(transformComponentManager) ||
           hasName(localMatrixComponentManager) || hasName(worldMatrixComponentManager) ||
           hasName(layerComponentManager) || hasName(lightProbeComponentManager);
}

CORE_NS::IComponentManager* Ecs::FindComponent(BASE_NS::string_view name) const
{
    SCENE_ASSERT_THREAD(thread_);
    auto it = components_.find(name);
    return it != components_.end() ? it->second : nullptr;
}

CORE_NS::IComponentManager* Ecs::FindComponent(BASE_NS::string_view name, CORE_NS::Entity entity) const
{
    SCENE_ASSERT_THREAD(thread_);
    if (auto m = FindComponent(name)) {
        return m;
    }
    // Fallback: search the entity's actual components for custom/plugin components
    // not registered in the EcsContext.
    if (ecs && CORE_NS::EntityUtil::IsValid(entity)) {
        BASE_NS::vector<CORE_NS::IComponentManager*> managers;
        ecs->GetComponents(entity, managers);
        for (auto* m : managers) {
            if (m->GetName() == name) {
                return m;
            }
        }
    }
    return nullptr;
}

CORE3D_NS::ISceneNode* Ecs::FindNode(BASE_NS::string_view path)
{
    SCENE_ASSERT_THREAD(thread_);
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
    SCENE_ASSERT_THREAD(thread_);
    return FindNode(NormalisePath(ParentPath(path)));
}

BASE_NS::string Ecs::GetPath(const CORE3D_NS::ISceneNode* node) const
{
    SCENE_ASSERT_THREAD(thread_);
    BASE_NS::string path;
    while (node && node->GetParent()) {
        path = "/" + node->GetName() + path;
        node = node->GetParent();
    }
    return path;
}

BASE_NS::string Ecs::GetPath(CORE_NS::Entity ent) const
{
    SCENE_ASSERT_THREAD(thread_);
    return GetPath(nodeSystem->GetNode(ent));
}

bool Ecs::IsNodeEntity(CORE_NS::Entity ent) const
{
    SCENE_ASSERT_THREAD(thread_);
    return nodeSystem->GetNode(ent) != nullptr;
}

CORE_NS::Entity Ecs::GetRootEntity() const
{
    SCENE_ASSERT_THREAD(thread_);
    return rootEntity_;
}

CORE_NS::Entity Ecs::GetParent(CORE_NS::Entity ent) const
{
    SCENE_ASSERT_THREAD(thread_);
    if (auto n = nodeComponentManager->Read(ent)) {
        return n->parent;
    }
    return {};
}

BASE_NS::vector<CORE3D_NS::IPicking*> Ecs::GetPicking()
{
    static constexpr BASE_NS::Uid UID_EXTERNAL_PICKING{"567c693e-9f0d-4801-8255-359e141c9c86"};

    BASE_NS::vector<CORE3D_NS::IPicking*> result;
    result.push_back(picking);
    if (!externalPicking_.has_value()) {
        // Only try instantiating external picking once. If we get null then it will remain null
        if (auto scene = scene_.lock()) {
            if (auto fac = scene->GetRenderContext().GetInterface<CORE_NS::IClassRegister>()) {
                if (auto picking = static_cast<CORE3D_NS::IPicking*>(fac->GetInstance(UID_EXTERNAL_PICKING))) {
                    externalPicking_ = CORE3D_NS::IPicking::Ptr(picking);
                } else {
                    externalPicking_ = nullptr;
                }
            }
        }
    }
    if (auto ep = externalPicking_.value_or(nullptr)) {
        result.push_back(ep.get());
    }
    return result;
}

IEcsObject::Ptr Ecs::GetEcsObject(CORE_NS::Entity ent)
{
    SCENE_ASSERT_THREAD(thread_);
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
    SCENE_ASSERT_THREAD(thread_);
    DeregisterEcsObject(obj);
}

bool Ecs::RemoveEntity(CORE_NS::Entity ent)
{
    SCENE_ASSERT_THREAD(thread_);
    bool res = CORE_NS::EntityUtil::IsValid(ent);
    if (res) {
        DeregisterEcsObject(ent);
        ecs->GetEntityManager().Destroy(ent);
    }
    return res;
}

bool Ecs::CreateUnnamedRootNode()
{
    SCENE_ASSERT_THREAD(thread_);
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
    SCENE_ASSERT_THREAD(thread_);
    auto n = nodeSystem->GetNode(ent);
    if (n) {
        n->SetName(name);
    }
    return n != nullptr;
}

bool Ecs::SetNodeParentAndName(CORE_NS::Entity ent, BASE_NS::string_view name, CORE3D_NS::ISceneNode* parent)
{
    SCENE_ASSERT_THREAD(thread_);
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
    SCENE_ASSERT_THREAD(thread_);
    return nodeSystem->GetNode(ent);
}
const CORE3D_NS::ISceneNode* Ecs::GetNode(CORE_NS::Entity ent) const
{
    SCENE_ASSERT_THREAD(thread_);
    return nodeSystem->GetNode(ent);
}

CORE_NS::EntityReference Ecs::GetRenderHandleEntity(const RENDER_NS::RenderHandleReference& handle)
{
    SCENE_ASSERT_THREAD(thread_);
    CORE_NS::EntityReference ent;
    if (handle && rhComponentManager) {
        ent = CORE3D_NS::GetOrCreateEntityReference(ecs->GetEntityManager(), *rhComponentManager, handle);
    }
    return ent;
}

void Ecs::AddDefaultComponents(CORE_NS::Entity ent) const
{
    SCENE_ASSERT_THREAD(thread_);
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
    create(layerComponentManager);
}

void Ecs::ListenNodeChanges(bool enabled)
{
    SCENE_ASSERT_THREAD(thread_);
    if (enabled) {
        nodeSystem->AddListener(*this);
    } else {
        nodeSystem->RemoveListener(*this);
    }
}

void Ecs::OnChildChanged(const CORE3D_NS::ISceneNode& parent, CORE3D_NS::INodeSystem::SceneNodeListener::EventType type,
    const CORE3D_NS::ISceneNode& child, size_t index)
{
    SCENE_ASSERT_THREAD(thread_);
    if (auto i = interface_pointer_cast<IOnNodeChanged>(scene_)) {
        META_NS::ContainerChangeType cchange{};
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
    SCENE_ASSERT_THREAD(thread_);
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
    SCENE_ASSERT_THREAD(thread_);
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
    SCENE_ASSERT_THREAD(thread_);
    auto& entMan = ecs->GetEntityManager();

    std::set<CORE_NS::Entity> reactivate = GetDeactivatedChildren(ent);
    reactivate.insert(ent);

    for (auto&& n : reactivate) {
        entMan.SetActive(n, true);
    }
}

void Ecs::SetNodesActive(CORE_NS::Entity ent, bool enabled)
{
    SCENE_ASSERT_THREAD(thread_);
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
    SCENE_ASSERT_THREAD(thread_);
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
    SCENE_ASSERT_THREAD(thread_);
    BASE_NS::vector<CORE_NS::Entity> entities;
    GetNodeDescendants(ent, entities);
    return entities;
}

static void NotifyChildAdded(IInternalScene& scene, CORE_NS::Entity parent, CORE_NS::Entity entity)
{
    if (CORE_NS::EntityUtil::IsValid(entity)) {
        if (auto i = interface_cast<IOnNodeChanged>(&scene)) {
            size_t index(-1);
            if (auto nodeSystem = CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(*scene.GetEcsContext().GetNativeEcs())) {
                if (auto parentNode = nodeSystem->GetNode(parent)) {
                    auto children = parentNode->GetChildren();
                    for (size_t childIndex = 0; childIndex < children.size(); ++childIndex) {
                        if (children[childIndex]->GetEntity() == entity) {
                            index = childIndex;
                            break;
                        }
                    }
                }
            } else {
                CORE_LOG_W("Failed to resolve imported child index, NodeSystem not found");
            }

            i->OnChildChanged(META_NS::ContainerChangeType::ADDED, parent, entity, index);
        }
    }
}

EcsCopyResult CopyExternalAsChild(const IEcsObject& parent, const IEcsObject& extChild)
{
    EcsCopyResult res;

    // todo: set the res.importedEntities when Clone supports it

    IInternalScene::Ptr localScene = parent.GetScene();
    IInternalScene::Ptr extScene = extChild.GetScene();
    if (!localScene || !extScene) {
        return {};
    }

    auto& util = localScene->GetGraphicsContext().GetSceneUtil();

    auto cloneRes = util.Clone(*localScene->GetEcsContext().GetNativeEcs(),
        parent.GetEntity(),
        *extScene->GetEcsContext().GetNativeEcs(),
        extChild.GetEntity());

    res.entity = cloneRes.node;
    NotifyChildAdded(*localScene, parent.GetEntity(), res.entity);

    return res;
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
    return node ? node->GetEntity() : CORE_NS::Entity{};
}

static CORE_NS::ResourceId CopyResourceEntry(const CORE_NS::IResourceManager::Ptr& resources,
    const CORE_NS::ResourceInfo& oldInfo, const CORE_NS::ResourceIdContext& id,
    BASE_NS::vector<CORE_NS::ResourceIdContext>& instantiate)
{
    auto info = resources->GetResourceInfo(id);
    if (!info.id.IsValid()) {
        CORE_NS::IResourceOptions::Ptr opts;
        if (oldInfo.options) {
            opts = oldInfo.options->Clone();
            instantiate.push_back(id);
        }
        resources->AddResource(id, oldInfo.type, oldInfo.path, opts);
    } else {
        // if options data is not empty, we need instantiate the resource to apply the changes
        if (info.options) {
            instantiate.push_back(id);
        }
    }

    return id.id;
}

static BASE_NS::unordered_map<CORE_NS::Entity, CORE3D_NS::ISceneUtil::MappedEntity> GetResourceMapping(
    IInternalScene& source, IInternalScene& target, const BASE_NS::string& group)
{
    auto sourceRes = static_cast<IResourceComponentManager*>(source.GetEcsContext().FindComponent<ResourceComponent>());
    auto targetRes = static_cast<IResourceComponentManager*>(target.GetEcsContext().FindComponent<ResourceComponent>());
    auto sourceResources = source.GetContext()->GetResources();
    if (!sourceRes || !targetRes || !sourceResources) {
        return {};
    }
    auto& eman = target.GetEcsContext().GetNativeEcs()->GetEntityManager();
    BASE_NS::unordered_map<CORE_NS::Entity, CORE3D_NS::ISceneUtil::MappedEntity> map;

    auto mapEntity = [&](auto index, auto id) {
        // animations are somewhat special as they point to other entities, so we cannot re-use those
        if (sourceResources->GetResourceInfo({id, source.GetScene()}).type != ClassId::AnimationResource.Id().ToUid()) {
            auto cRes = id;
            CORE_NS::ResourceId destRId{cRes.name, group.empty() ? cRes.group : group};

            if (auto targetEnt = targetRes->GetEntity(destRId);
                CORE_NS::EntityUtil::IsValid(targetEnt) && eman.IsAlive(targetEnt)) {
                if (auto th = targetRes->Read(targetEnt)) {
                    map[sourceRes->GetEntity(index)] = {targetEnt, th->origin.empty()};
                }
            }
        }
    };

    for (size_t si = 0; si != sourceRes->GetComponentCount(); ++si) {
        if (auto h = sourceRes->Write(si); h && h->resourceId.IsValid()) {
            mapEntity(si, h->resourceId);
        }
    }
    return map;
}

static BASE_NS::string ChangeResourceGroup(const IInternalScene::Ptr& scene,
    const BASE_NS::vector<CORE_NS::Entity>& entities, const BASE_NS::string& group, const BASE_NS::string& origin,
    const IInternalScene::Ptr& source)
{
    auto resources = GetResourceManager(scene);
    if (!resources) {
        return "";
    }
    auto resuMan = static_cast<IResourceComponentManager*>(scene->GetEcsContext().FindComponent<ResourceComponent>());
    if (!resuMan) {
        return "";
    }
    CORE_LOG_D("change resource group to '%s' while importing scene", group.c_str());

    auto& eman = scene->GetEcsContext().GetNativeEcs()->GetEntityManager();
    BASE_NS::unordered_map<CORE_NS::ResourceId, CORE_NS::ResourceId> entries;
    BASE_NS::vector<CORE_NS::ResourceIdContext> instantiate;
    for (auto&& v : entities) {
        if (auto h = resuMan->Write(v)) {
            auto& entry = entries[h->resourceId];
            if (!entry.IsValid()) {
                entry = CORE_NS::ResourceId{h->resourceId.name, group};
                auto info = resources->GetResourceInfo({h->resourceId, source->GetScene()});
                CORE_NS::ResourceIdContext id{entry, scene->GetScene()};
                if (info.type == ClassId::AnimationResource.Id().ToUid()) {
                    // animations are somewhat special as they point to other entities, so we cannot re-use
                    // those
                    id = FirstUnusedEcsResourceName(scene, id, v);
                }
                if (info.id.IsValid()) {
                    entry = CopyResourceEntry(resources, info, id, instantiate);
                }
            }
            // make sure we have unique resource id
            if (auto ent = resuMan->GetEntity(entry);
                CORE_NS::EntityUtil::IsValid(ent) && ent != v && eman.IsAlive(ent)) {
                if (auto v = resuMan->Write(ent)) {
                    v->resourceId = {};
                    CORE_LOG_W("Conflicting resource ids [%s] resetting for entity %" PRIx64,
                        entry.ToString().c_str(),
                        ent.id);
                }
            }
            h->resourceId = entry;
            h->origin = origin;
        }
    }
    if (auto ext = interface_cast<META_NS::IResourceManagerExtension>(resources)) {
        for (auto&& v : instantiate) {
            if (auto r = resources->GetResource(v)) {
                ext->ReapplyOptions(r, scene->GetScene());
            }
        }
    }
    return group;
}

static bool FilterAnimationCheck(const CORE_NS::IResourceManager& resources, const CORE_NS::ResourceIdContext& id,
    BASE_NS::vector<CORE_NS::ResourceIdContext>& associated)
{
    auto info = resources.GetResourceInfo(id);
    if (info.id.IsValid()) {
        associated.push_back(id);
        if (info.type == ClassId::AnimationResource.Id().ToUid()) {
            return true;
        }
    }
    return false;
}

static BASE_NS::vector<CORE_NS::Entity> FilterAddedEntities(const IInternalScene::Ptr& scene,
    const CORE3D_NS::ISceneUtil::PartialClonedEntities& pce, BASE_NS::vector<CORE_NS::ResourceIdContext>& associated)
{
    BASE_NS::vector<CORE_NS::Entity> res;
    auto resources = GetResourceManager(scene);
    if (!resources) {
        return {};
    }
    auto resuMan = static_cast<IResourceComponentManager*>(scene->GetEcsContext().FindComponent<ResourceComponent>());
    if (!resuMan) {
        return res;
    }
    std::set<CORE_NS::Entity> handled;
    for (auto&& e : pce.newEntities) {
        handled.insert(e);
        if (!scene->GetEcsContext().IsNodeEntity(e)) {
            auto h = resuMan->Read(e);
            if (h && FilterAnimationCheck(*resources, {h->resourceId, scene->GetScene()}, associated)) {
                h = {};
            }
            if (!h) {
                res.push_back(e);
            }
        }
    }
    for (auto&& e : pce.extMapEntities) {
        if (handled.count(e)) {
            continue;
        }
        if (auto h = resuMan->Read(e)) {
            auto info = resources->GetResourceInfo({h->resourceId, scene->GetScene()});
            if (info.id.IsValid()) {
                associated.push_back({h->resourceId, scene->GetScene()});
            }
        }
    }
    return res;
}

EcsCopyResult CopyExternalAsChild(
    const IEcsObject& parent, const IScene& scene, BASE_NS::string_view rgroup, bool shareResources)
{
    EcsCopyResult res;

    IInternalScene::Ptr localScene = parent.GetScene();
    IInternalScene::Ptr extScene = scene.GetInternalScene();
    if (!localScene || !extScene) {
        return {};
    }

    auto& util = localScene->GetGraphicsContext().GetSceneUtil();
    auto origin = extScene->GetResourceGroups().PrimaryGroup();

    BASE_NS::string group;
    if (!rgroup.empty()) {
        group = rgroup;
    } else {
        group = origin.empty() ? localScene->GetResourceGroups().PrimaryGroup() : origin;
        if (!shareResources) {
            group = UniqueGroupName(localScene, group);
        }
    }

    auto mapping = GetResourceMapping(*extScene, *localScene, group);
    auto cloneRes =
        util.Clone(*localScene->GetEcsContext().GetNativeEcs(), *extScene->GetEcsContext().GetNativeEcs(), mapping);

    res.resourceGroup = ChangeResourceGroup(localScene, cloneRes.newEntities, group, origin, extScene);
    res.newEntities = FilterAddedEntities(localScene, cloneRes, res.associatedResources);
    // re-parenting can cause notifications, so it has to be after changing the resource groups
    res.entity = ReparentOldRoot(localScene, parent, cloneRes.newEntities);
    return res;
}

static void ChangeResourceNames(const IInternalScene::Ptr& scene, const BASE_NS::vector<CORE_NS::Entity>& entities)
{
    auto resources = GetResourceManager(scene);
    if (!resources) {
        return;
    }
    auto resuMan = static_cast<IResourceComponentManager*>(scene->GetEcsContext().FindComponent<ResourceComponent>());
    if (!resuMan) {
        return;
    }
    BASE_NS::unordered_map<CORE_NS::ResourceId, CORE_NS::ResourceId> entries;
    BASE_NS::vector<CORE_NS::ResourceIdContext> instantiate;

    for (auto&& v : entities) {
        if (auto h = resuMan->Write(v)) {
            auto& entry = entries[h->resourceId];
            if (!entry.IsValid()) {
                CORE_NS::ResourceIdContext oldRic{h->resourceId, scene->GetScene()};
                auto ric = UniqueResourceName(resources, oldRic);
                entry = ric.id;
                auto info = resources->GetResourceInfo(oldRic);
                if (info.id.IsValid()) {
                    entry = CopyResourceEntry(resources, info, ric, instantiate);
                }
            }
            h->resourceId = entry;
        }
    }
}

EcsCopyResult CloneAsChild(const IEcsObject& node, const IEcsObject::Ptr& parentNode)
{
    EcsCopyResult res;

    IInternalScene::Ptr scene = node.GetScene();
    if (!scene) {
        return {};
    }
    IEcsContext& ec = scene->GetEcsContext();

    CORE_NS::Entity parent;
    if (parentNode) {
        parent = parentNode->GetEntity();
    } else {
        parent = ec.GetParent(node.GetEntity());
    }

    auto& util = scene->GetGraphicsContext().GetSceneUtil();
    auto cloneRes = util.Clone(*ec.GetNativeEcs(), node.GetEntity(), parent);

    res.entity = cloneRes.node;
    res.newEntities = cloneRes.entities;
    ChangeResourceNames(scene, res.newEntities);

    NotifyChildAdded(*scene, parent, res.entity);

    return res;
}

SCENE_END_NAMESPACE()
