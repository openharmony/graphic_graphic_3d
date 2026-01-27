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

#include "internal_scene.h"

#include <algorithm>
#include <chrono>
#include <inttypes.h>
#include <mutex>
#include <scene/ext/intf_converting_value.h>
#include <scene/ext/intf_create_entity.h>
#include <scene/ext/scene_debug_info.h>
#include <scene/ext/scene_utils.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_text.h>

#include <3d/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>

#include <meta/api/engine/util.h>
#include <meta/interface/intf_startable.h>

#include "../component/generic_component.h"
#include "../node/startable_handler.h"
#include "../resource/ecs_animation.h"

SCENE_BEGIN_NAMESPACE()

InternalScene::InternalScene(const IScene::Ptr& scene, IRenderContext::Ptr context, SceneOptions opts)
    : scene_(scene), options_(BASE_NS::move(opts))
{
    if (auto getter = interface_cast<IApplicationContextProvider>(context)) {
        context_ = getter->GetApplicationContext();
    }
    if (context_.expired()) {
        renderContext_ = context;
    }

    graphicsContext3D_ = CORE_NS::CreateInstance<CORE3D_NS::IGraphicsContext>(
        *context->GetRenderer()->GetInterface<CORE_NS::IClassFactory>(), CORE3D_NS::UID_GRAPHICS_CONTEXT);
    graphicsContext3D_->Init();
}

InternalScene::~InternalScene() {}

bool InternalScene::Initialize()
{
    ecs_.reset(new Ecs);
    if (!ecs_->Initialize(self_.lock(), options_)) {
        CORE_LOG_E("failed to initialize ecs");
        return false;
    }
    return true;
}

SceneOptions InternalScene::GetOptions() const
{
    return options_;
}

void InternalScene::Uninitialize()
{
    CORE_LOG_D("InternalScene::Uninitialize");
    {
        std::unique_lock lock { mutex_ };
        syncs_.clear();
    }
    nodes_.clear();
    animations_.clear();
    componentFactories_.clear();
    renderingCameras_.clear();

    if (ecs_) {
        ecs_->Uninitialize();
    }

    if (const auto rctx = GetRenderContextPtr()) {
        // Do a "empty render" to flush out the gpu resources instantly.
        for (size_t i = 0; i < deferedRenderCount_; ++i) {
            rctx->GetRenderer().RenderFrame({});
        }
    }
    context_.reset();
    renderContext_.reset();
}

static CORE_NS::Entity CreateDefaultEntity(CORE_NS::IEcs& ecs)
{
    CORE_NS::IEntityManager& em = ecs.GetEntityManager();
    return em.Create();
}

INode::Ptr InternalScene::CreateNode(CORE3D_NS::ISceneNode* parent, BASE_NS::string_view name, META_NS::ObjectId id)
{
    auto& r = META_NS::GetObjectRegistry();

    if (!id.IsValid()) {
        id = ClassId::Node;
    }

    auto node = r.Create<INode>(id);
    if (!node) {
        CORE_LOG_E(
            "Failed to create node object [name=%s, id=%s]", BASE_NS::string(name).c_str(), id.ToString().c_str());
        return nullptr;
    }
    CORE_NS::Entity ent;
    if (auto ce = interface_pointer_cast<ICreateEntity>(node)) {
        ent = ce->CreateEntity(self_.lock());
    } else {
        ent = CreateDefaultEntity(*ecs_->ecs);
    }
    if (!CORE_NS::EntityUtil::IsValid(ent)) {
        CORE_LOG_E("Failed to create entity [name=%s, id=%s]", BASE_NS::string(name).c_str(), id.ToString().c_str());
        return nullptr;
    }
    ecs_->AddDefaultComponents(ent);
    ecs_->SetNodeName(ent, name); // remove when reworking names for scene objects
    if (!ConstructNodeImpl(ent, node)) {
        ecs_->RemoveEntity(ent);
        return nullptr;
    }
    ecs_->SetNodeParentAndName(ent, name, parent);
    return node;
}

INode::Ptr InternalScene::CreateNode(const INode::ConstPtr& parent, BASE_NS::string_view name, META_NS::ObjectId id)
{
    CORE3D_NS::ISceneNode* snode {};
    if (auto access = interface_cast<IEcsObjectAccess>(parent)) {
        if (auto ecso = access->GetEcsObject()) {
            auto entity = ecso->GetEntity();
            if (CORE_NS::EntityUtil::IsValid(entity)) {
                snode = ecs_->GetNode(entity);
            }
        }
    }
    if (snode) {
        return CreateNode(snode, name, id);
    }
    CORE_LOG_W("Invalid parent for CreateNode (%s)", BASE_NS::string(name).c_str());
    return nullptr;
}

INode::Ptr InternalScene::CreateNode(BASE_NS::string_view path, META_NS::ObjectId id)
{
    auto parent = ecs_->FindNodeParent(path);
    if (!parent) {
        CORE_LOG_E("No parent for node [path=%s]", BASE_NS::string(path).c_str());
        return nullptr;
    }
    auto res = CreateNode(parent, EntityName(path), id);
    if (!res) {
        CORE_LOG_E("Failed to create node [path=%s, id=%s]", BASE_NS::string(path).c_str(), id.ToString().c_str());
    }
    return res;
}

META_NS::IObject::Ptr InternalScene::FindResource(CORE_NS::Entity ent) const
{
    if (auto c = GetContext()) {
        if (auto r = c->GetResources()) {
            if (auto v = ecs_->resourceComponentManager->Read(ent)) {
                return interface_pointer_cast<META_NS::IObject>(r->GetResource(v->resourceId, scene_.lock()));
            }
        }
    }
    return nullptr;
}

META_NS::IObject::Ptr InternalScene::CreatePlainObject(
    META_NS::ObjectId id, CORE_NS::Entity ent, const CORE_NS::ResourceId& rid) const
{
    auto& r = META_NS::GetObjectRegistry();
    auto md = CreateRenderContextArg(GetContext());
    if (md) {
        md->AddProperty(META_NS::ConstructProperty<IInternalScene::Ptr>("Scene", self_.lock()));
    }
    auto object = r.Create<META_NS::IObject>(id, md);
    if (!object) {
        CORE_LOG_E("Failed to create scene object [id=%s]", id.ToString().c_str());
        return nullptr;
    }
    if (auto acc = interface_cast<IEcsObjectAccess>(object)) {
        if (!CORE_NS::EntityUtil::IsValid(ent)) {
            if (auto ce = interface_pointer_cast<ICreateEntity>(object)) {
                ent = ce->CreateEntity(self_.lock());
            } else {
                ent = CreateDefaultEntity(*ecs_->ecs);
            }
            if (!CORE_NS::EntityUtil::IsValid(ent)) {
                CORE_LOG_E("Failed to create entity [id=%s]", id.ToString().c_str());
                return nullptr;
            }
            if (rid.IsValid()) {
                ecs_->resourceComponentManager->Create(ent);
                if (auto v = ecs_->resourceComponentManager->Write(ent)) {
                    v->resourceId = rid;
                }
            }
        }
        auto eobj = ecs_->GetEcsObject(ent);
        if (!acc->SetEcsObject(eobj)) {
            return nullptr;
        }
    }
    return object;
}

META_NS::IObject::Ptr InternalScene::CreateObject(META_NS::ObjectId id, CORE_NS::Entity ent) const
{
    if (CORE_NS::EntityUtil::IsValid(ent)) {
        if (auto res = FindResource(ent)) {
            return res;
        }
    }
    return CreatePlainObject(id, ent, {});
}

META_NS::IObject::Ptr InternalScene::CreateObject(META_NS::ObjectId id, const CORE_NS::ResourceId& rid) const
{
    return CreatePlainObject(id, ecs_->resourceComponentManager->GetEntityWithResource(rid), rid);
}

INode::Ptr InternalScene::ConstructNodeImpl(CORE_NS::Entity ent, INode::Ptr node) const
{
    auto acc = interface_cast<IEcsObjectAccess>(node);
    if (!acc) {
        return nullptr;
    }
    auto& r = META_NS::GetObjectRegistry();

    auto eobj = ecs_->GetEcsObject(ent);
    if (!eobj) {
        return nullptr;
    }
    AttachComponents(node, eobj, ent);

    if (!acc->SetEcsObject(eobj)) {
        return nullptr;
    }

    nodes_[ent] = node;
    return node;
}

META_NS::ObjectId InternalScene::DeducePrimaryNodeType(CORE_NS::Entity ent) const
{
    if (ecs_->cameraComponentManager->HasComponent(ent)) {
        return ClassId::CameraNode;
    }
    if (ecs_->lightComponentManager->HasComponent(ent)) {
        return ClassId::LightNode;
    }
    if (ecs_->textComponentManager && ecs_->textComponentManager->HasComponent(ent)) {
        return ClassId::TextNode;
    }
    if (ecs_->renderMeshComponentManager->HasComponent(ent)) {
        return ClassId::MeshNode;
    }
    return ClassId::Node;
}

INode::Ptr InternalScene::ConstructNode(CORE_NS::Entity ent, META_NS::ObjectId id) const
{
    auto& r = META_NS::GetObjectRegistry();
    if (!id.IsValid()) {
        id = DeducePrimaryNodeType(ent);
    }

    auto node = r.Create<INode>(id);
    if (!node) {
        CORE_LOG_E("Failed to create node object [id=%s]", id.ToString().c_str());
        return nullptr;
    }

    // make sure imported nodes have our default set of components
    ecs_->AddDefaultComponents(ent);

    return ConstructNodeImpl(ent, node);
}

IComponent::Ptr InternalScene::CreateEcsComponent(const INode::Ptr& node, BASE_NS::string_view componentName)
{
    // First check we already have the component
    auto attach = interface_cast<META_NS::IAttach>(node);
    if (!attach) {
        return {};
    }
    if (auto cont = attach->GetAttachmentContainer(true)) {
        if (auto existing = cont->FindAny<IComponent>(componentName, META_NS::TraversalType::NO_HIERARCHY)) {
            return existing;
        }
    }
    // Then find a component manager with a matching name
    IEcsObject::Ptr ecso;
    if (auto acc = interface_cast<IEcsObjectAccess>(node)) {
        ecso = acc->GetEcsObject();
    }
    if (ecs_ && ecso) {
        if (auto ecs = ecs_->GetNativeEcs()) {
            for (auto&& manager : ecs->GetComponentManagers()) {
                if (manager->GetName() == componentName) {
                    // Also create the Ecs component if not there already
                    if (auto component = CreateComponent(manager, ecso, true)) {
                        // We don't call component->PopulateAllProperties() here, i.e. if the component was newly
                        // created its properties will not be populated
                        attach->Attach(component);
                        return component;
                    }
                }
            }
        }
    }
    return {};
}

IComponent::Ptr InternalScene::CreateComponent(
    CORE_NS::IComponentManager* m, const IEcsObject::Ptr& ecsObject, bool createEcsComponent) const
{
    if (!m) {
        return {};
    }
    auto& r = META_NS::GetObjectRegistry();
    IComponent::Ptr comp;
    ;
    if (createEcsComponent) {
        if (auto entity = ecsObject->GetEntity(); CORE_NS::EntityUtil::IsValid(entity)) {
            if (!m->HasComponent(entity)) {
                m->Create(entity);
            }
        }
    }
    if (auto fac = FindComponentFactory(m->GetUid())) {
        comp = fac->CreateComponent(ecsObject);
    } else {
        auto md = r.Create<META_NS::IMetadata>(META_NS::ClassId::Object);
        md->AddProperty(META_NS::ConstructProperty<BASE_NS::string>("Component", BASE_NS::string(m->GetName())));
        comp = r.Create<IComponent>(ClassId::GenericComponent, md);
        if (auto acc = interface_cast<IEcsObjectAccess>(comp)) {
            if (!acc->SetEcsObject(ecsObject)) {
                return nullptr;
            }
        }
    }
    return comp;
}

void InternalScene::AttachComponents(
    const INode::Ptr& node, const IEcsObject::Ptr& ecsObject, CORE_NS::Entity ent) const
{
    auto& r = META_NS::GetObjectRegistry();
    auto att = interface_cast<META_NS::IAttach>(node);
    if (!att) {
        return;
    }

    auto attachments = att->GetAttachmentContainer(true);

    BASE_NS::vector<CORE_NS::IComponentManager*> managers;
    ecs_->ecs->GetComponents(ent, managers);
    for (auto m : managers) {
        if (!attachments->FindByName(m->GetName())) {
            if (auto comp = CreateComponent(m, ecsObject, false)) {
                att->Attach(comp);
            } else {
                CORE_LOG_E("Failed to construct component for '%s'", BASE_NS::string(m->GetName()).c_str());
            }
        }
    }
}

INode::Ptr InternalScene::FindNode(CORE_NS::Entity ent, META_NS::ObjectId id) const
{
    if (ecs_->IsNodeEntity(ent)) {
        auto it = nodes_.find(ent);
        if (it != nodes_.end()) {
            return it->second;
        }
        return ConstructNode(ent, id);
    }

    CORE_LOG_W("Could not find entity: %" PRIu64, ent.id);
    return nullptr;
}

INode::Ptr InternalScene::FindNode(BASE_NS::string_view path, META_NS::ObjectId id) const
{
    auto npath = NormalisePath(path);
    auto n = ecs_->FindNode(npath);
    if (n) {
        auto it = nodes_.find(n->GetEntity());
        if (it != nodes_.end()) {
            return it->second;
        }
        return ConstructNode(n->GetEntity(), id);
    }

    CORE_LOG_W("Could not find node: %s", BASE_NS::string(npath).c_str());
    return nullptr;
}

BASE_NS::vector<INode::Ptr> InternalScene::FindNodes(CORE_NS::Entity root, BASE_NS::string_view name, size_t maxCount,
    META_NS::ObjectId id, META_NS::TraversalType traversalType) const
{
    BASE_NS::vector<INode::Ptr> nodes;
    // Return true if maxCount reached
    auto addNode = [&](CORE_NS::Entity entity) -> bool {
        auto it = nodes_.find(entity);
        if (auto node = it == nodes_.end() ? ConstructNode(entity, id) : it->second; node) {
            nodes.emplace_back(BASE_NS::move(node));
        }
        return maxCount && nodes.size() == maxCount;
    };

    if (ecs_) {
        if (const auto node = ecs_->GetNode(root)) {
            if (node->GetName() == name) {
                if (addNode(root)) {
                    return nodes;
                }
            }
            if (traversalType == META_NS::TraversalType::BREADTH_FIRST_ORDER) {
                const auto children = node->GetChildren();
                BASE_NS::vector<CORE3D_NS::ISceneNode*> queue = { children.begin(), children.end() };
                while (!queue.empty()) {
                    const auto child = queue.front();
                    queue.erase(queue.begin());
                    if (child) {
                        if (child->GetName() == name) {
                            if (addNode(child->GetEntity())) {
                                return nodes;
                            }
                        }
                        if (const auto cc = child->GetChildren(); !children.empty()) {
                            queue.insert(queue.end(), cc.begin(), cc.end());
                        }
                    }
                }
            } else {
                // depth-first
                for (auto&& child : node->GetChildren()) {
                    if (child) {
                        if (auto found =
                                FindNodes(child->GetEntity(), name, maxCount - nodes.size(), id, traversalType);
                            !found.empty()) {
                            nodes.insert(nodes.end(), found.begin(), found.end());
                        }
                    }
                    if (maxCount && nodes.size() >= maxCount) {
                        break;
                    }
                }
            }
        }
    }
    return nodes;
}

BASE_NS::vector<INode::Ptr> InternalScene::FindNamedNodes(BASE_NS::string_view name, size_t maxCount,
    const INode::Ptr& root, META_NS::ObjectId id, META_NS::TraversalType traversalType) const
{
    CORE_NS::Entity entity;
    if (auto access = interface_cast<IEcsObjectAccess>(root)) {
        if (auto ecso = access->GetEcsObject()) {
            entity = ecso->GetEntity();
        }
    }
    if (!CORE_NS::EntityUtil::IsValid(entity)) {
        if (auto n = ecs_->FindNode("")) {
            entity = n->GetEntity();
        }
    }
    if (CORE_NS::EntityUtil::IsValid(entity)) {
        return FindNodes(entity, name, maxCount, id, traversalType);
    }
    return {};
}

INode::Ptr InternalScene::GetRootNode() const
{
    return FindNode("", {});
}

BASE_NS::string InternalScene::GetUniqueName(BASE_NS::string_view name, CORE_NS::Entity parent) const
{
    CORE3D_NS::ISceneNode* node = ecs_ ? ecs_->GetNode(parent) : nullptr;
    BASE_NS::vector<BASE_NS::string> names;
    BASE_NS::vector<BASE_NS::string_view> ns;
    if (node) {
        const auto children = node->GetChildren();
        names.reserve(children.size());
        ns.reserve(children.size());
        for (auto&& child : children) {
            names.emplace_back(BASE_NS::move(child->GetName()));
        }
    }
    // name strings are now in a fixed position in names-vector, initialize the vector of string_views
    for (auto&& name : names) {
        ns.emplace_back(name);
    }
    return META_NS::GetObjectUtil().GetUniqueName(name, ns);
}

INode::Ptr InternalScene::ReleaseCached(NodesType::iterator it)
{
    auto node = BASE_NS::move(it->second);
    nodes_.erase(it);
    if (auto i = interface_cast<INodeNotify>(node)) {
        if (i->IsListening()) {
            ListenNodeChanges(false);
        }
    }
    return node;
}

uint32_t InternalScene::ReleaseChildNodes(const IEcsObject::Ptr& eobj)
{
    uint32_t res {};
    if (auto n = ecs_->GetNode(eobj->GetEntity())) {
        for (auto&& c : n->GetChildren()) {
            auto it = nodes_.find(c->GetEntity());
            if (it != nodes_.end()) {
                auto nn = it->second;
                res += ReleaseNode(BASE_NS::move(nn), true);
            }
        }
    }
    return res;
}

static bool HasNonInternalAttachments(const INode::Ptr& node)
{
    if (auto i = interface_cast<META_NS::IAttach>(node)) {
        for (auto&& v : i->GetAttachments()) {
            if (!interface_cast<META_NS::IEvent>(v) && !interface_cast<META_NS::IFunction>(v) &&
                !interface_cast<META_NS::IProperty>(v) && !interface_cast<IComponent>(v)) {
                return true;
            }
        }
    }
    return false;
}

uint32_t InternalScene::ReleaseNode(INode::Ptr&& node, bool recursive)
{
    if (!IsSameScene(node)) {
        return 0;
    }
    uint32_t res{};
    if (node) {
        IEcsObject::Ptr eobj;
        if (auto acc = interface_cast<IEcsObjectAccess>(node)) {
            eobj = acc->GetEcsObject();
        }
        node.reset();
        if (eobj) {
            auto it = nodes_.find(eobj->GetEntity());
            if (it != nodes_.end()) {
                // are we the only owner?
                if (it->second.use_count() == 1) {
                    // do we have non-component attachments
                    if (!HasNonInternalAttachments(it->second)) {
                        node = ReleaseCached(it);
                    }
                }
            }
            if (recursive) {
                res += ReleaseChildNodes(eobj);
            }
            if (node) {
                ecs_->RemoveEcsObject(eobj);
                ++res;
            }
        }
    }
    return res;
}

static IExternalNode::Ptr GetExternalNodeAttachment(const INode::Ptr& node)
{
    if (auto att = interface_cast<META_NS::IAttach>(node)) {
        auto ext = att->GetAttachments<IExternalNode>();
        if (!ext.empty()) {
            return ext.front();
        }
    }
    return nullptr;
}

static BASE_NS::vector<CORE_NS::Entity> GetAddedEntities(const INode::Ptr& node)
{
    auto p = GetExternalNodeAttachment(node);
    return p ? p->GetAddedEntities() : BASE_NS::vector<CORE_NS::Entity> {};
}

bool InternalScene::RemoveEntity(
    const CORE_NS::IResourceManager::Ptr& resources, CORE_NS::Entity entity, bool removeFromIndex)
{
    if (!(ecs_ && CORE_NS::EntityUtil::IsValid(entity))) {
        return false;
    }
    animations_.erase(entity);
    if (resources) {
        if (auto v = ecs_->resourceComponentManager->Read(entity)) {
            if (removeFromIndex) {
                resources->RemoveResource(v->resourceId);
            } else {
                resources->PurgeResource(v->resourceId);
            }
        }
    }
    return ecs_->RemoveEntity(entity);
}

bool InternalScene::RemoveObject(META_NS::IObject::Ptr&& object, bool removeFromIndex)
{
    if (auto node = interface_pointer_cast<INode>(object)) {
        object.reset();
        return RemoveNode(BASE_NS::move(node), removeFromIndex);
    }
    bool res = false;
    if (auto entity = GetEcsObjectEntity(interface_cast<IEcsObjectAccess>(object));
        CORE_NS::EntityUtil::IsValid(entity)) {
        res = RemoveEntity(GetResourceManager(this), entity, removeFromIndex);
    }
    return res;
}

bool InternalScene::RemoveNode(INode::Ptr&& node, bool removeFromIndex)
{
    if (auto acc = interface_cast<IEcsObjectAccess>(node)) {
        BASE_NS::vector<CORE_NS::Entity> added;
        if (auto ext = GetExternalNodeAttachment(node)) {
            auto sg = ext->GetSubresourceGroup();
            if (!sg.empty()) {
                auto bundle = GetResourceGroups();
                bundle.RemoveHandle(sg);
                SetResourceGroups(BASE_NS::move(bundle));
            }
            added = ext->GetAddedEntities();
        }
        const auto decents = ecs_->GetNodeDescendants(GetEcsObjectEntity(acc));
        for (auto&& ent : decents) {
            if (auto it = nodes_.find(ent); it != nodes_.end()) {
                ReleaseCached(it);
            }
            ecs_->RemoveEntity(ent);
            for (auto it = added.begin(); it != added.end(); ++it) {
                if (*it == ent) {
                    added.erase(it);
                    break;
                }
            }
        }
        const auto resources = GetResourceManager(this);
        for (auto it = added.begin(); it != added.end(); ++it) {
            RemoveEntity(resources, *it, removeFromIndex);
        }
        return true;
    }
    return false;
}

bool InternalScene::RemoveNamedChild(const INode::Ptr& node, BASE_NS::string_view name)
{
    if (!ecs_) {
        return false;
    }
    auto parentEntity = GetEcsObjectEntity(interface_cast<IEcsObjectAccess>(node));
    CORE3D_NS::ISceneNode* sn = ecs_->GetNode(parentEntity);
    if (!sn) {
        return false;
    }
    CORE_NS::Entity childEntity;
    for (auto&& child : sn->GetChildren()) {
        if (child && child->GetName() == name) {
            childEntity = child->GetEntity();
            break;
        }
    }
    if (CORE_NS::EntityUtil::IsValid(childEntity)) {
        if (auto n = nodes_.find(childEntity); n != nodes_.end()) {
            // We already have a wrapper for the node
            auto remove = n->second;
            return RemoveNode(BASE_NS::move(remove), true);
        }
        // No wrapper, remove only from ECS
        const auto decents = ecs_->GetNodeDescendants(childEntity);
        const auto resources = GetResourceManager(this);
        for (auto&& ent : decents) {
            if (auto it = nodes_.find(ent); it != nodes_.end()) {
                ReleaseCached(it);
            }
            RemoveEntity(resources, ent, true);
        }
        return RemoveEntity(resources, childEntity, true);
    }
    return false;
}

/// Returns a list of instantiated child nodes of root (including root) which implement INodeNotify
BASE_NS::vector<INodeNotify::Ptr> InternalScene::GetNotifiableNodesFromHierarchy(CORE_NS::Entity root)
{
    BASE_NS::vector<INodeNotify::Ptr> notify;
    auto findNode = [this](CORE_NS::Entity entity) {
        auto n = nodes_.find(entity);
        return n != nodes_.end() ? interface_pointer_cast<INodeNotify>(n->second) : nullptr;
    };
    // Add root to the list
    if (auto n = findNode(root)) {
        notify.emplace_back(BASE_NS::move(n));
    }
    // Add descendants to the list
    auto descendants = ecs_->GetNodeDescendants(root);
    notify.reserve(descendants.size() + 1);
    for (auto&& d : descendants) {
        if (auto n = findNode(d)) {
            notify.emplace_back(BASE_NS::move(n));
        }
    }
    return notify;
}

void InternalScene::SetNodeActive(const INode::Ptr& child, bool active)
{
    if (auto ecsobj = interface_pointer_cast<IEcsObjectAccess>(child)) {
        SetEntityActive(ecsobj->GetEcsObject(), active);
    }
    auto ext = GetExternalNodeAttachment(child);
    if (ext) {
        for (auto&& v : ext->GetAddedEntities()) {
            ecs_->ecs->GetEntityManager().SetActive(v, active);
        }
        if (active) {
            ext->Activate();
        } else {
            ext->Deactivate();
        }
    }
}

void InternalScene::SetEntityActive(const BASE_NS::shared_ptr<IEcsObject>& child, bool active)
{
    if (!child || !ecs_) {
        return;
    }
    const auto entity = child->GetEntity();
    if (!active) {
        for (auto&& node : GetNotifiableNodesFromHierarchy(entity)) {
            node->OnNodeActiveStateChanged(INodeNotify::NodeActiteStateInfo::DEACTIVATING);
        }
    }
    ecs_->SetNodesActive(entity, active);
    if (active) {
        for (auto&& node : GetNotifiableNodesFromHierarchy(entity)) {
            node->OnNodeActiveStateChanged(INodeNotify::NodeActiteStateInfo::ACTIVATED);
        }
    }
}

BASE_NS::vector<INode::Ptr> InternalScene::GetChildren(const IEcsObject::Ptr& obj) const
{
    BASE_NS::vector<INode::Ptr> res;
    if (auto node = ecs_->GetNode(obj->GetEntity())) {
        for (auto&& c : node->GetChildren()) {
            if (auto n = FindNode(c->GetEntity(), {})) {
                res.push_back(n);
            }
        }
    }
    return res;
}
bool InternalScene::RemoveChild(const BASE_NS::shared_ptr<IEcsObject>& object, const INode::Ptr& child)
{
    bool ret = false;
    if (!IsSameScene(child)) {
        return ret;
    }
    if (auto node = ecs_->GetNode(object->GetEntity())) {
        if (auto ecsobj = interface_pointer_cast<IEcsObjectAccess>(child)) {
            if (auto obj = ecsobj->GetEcsObject()) {
                if (auto childNode = ecs_->GetNode(obj->GetEntity())) {
                    ret = node->RemoveChild(*childNode);
                    if (ret) {
                        SetNodeActive(child, false);
                    }
                }
            }
        }
    }
    return ret;
}
bool InternalScene::AddChild(const BASE_NS::shared_ptr<IEcsObject>& object, const INode::Ptr& child, size_t index)
{
    bool ret = false;
    if (!IsSameScene(child)) {
        return ret;
    }
    if (ecs_->IsNodeEntity(object->GetEntity())) {
        if (auto acc = interface_cast<IEcsObjectAccess>(child)) {
            if (auto ecsobj = acc->GetEcsObject()) {
                SetNodeActive(child, true);
                if (auto node = ecs_->GetNode(object->GetEntity())) {
                    if (auto childNode = ecs_->GetNode(ecsobj->GetEntity())) {
                        if (node->InsertChild(index, *childNode)) {
                            nodes_[ecsobj->GetEntity()] = child;
                            ret = true;
                        }
                    }
                }
            }
        }
    }
    return ret;
}

bool InternalScene::IsSameScene(const INode::Ptr& node) const
{
    auto scene = scene_.lock();
    return node && scene && node->GetScene() == scene;
}

BASE_NS::shared_ptr<IScene> InternalScene::GetScene() const
{
    return scene_.lock();
}

void InternalScene::SchedulePropertyUpdate(const META_NS::IEnginePropertySync::Ptr& obj)
{
    std::unique_lock lock { mutex_ };
    syncs_[obj.get()] = obj;
}

void InternalScene::SyncProperties()
{
    UpdateSyncProperties(false);
}

bool InternalScene::UpdateSyncProperties(bool resetPending)
{
    bool pending = false;
    BASE_NS::unordered_map<void*, META_NS::IEnginePropertySync::WeakPtr> syncs;
    {
        std::unique_lock lock { mutex_ };
        syncs = BASE_NS::move(syncs_);
        pending = pendingRender_;
        if (resetPending) {
            pendingRender_ = false;
        }
    }

    // events has to be processes so that ecs is in consistent state when synchronising the properties
    ecs_->ecs->ProcessEvents();
    for (auto&& v : syncs) {
        if (auto o = v.second.lock()) {
            o->SyncProperties();
        }
    }
    return pending;
}

void InternalScene::Update(const UpdateInfo& info)
{
    using namespace std::chrono;
    bool pending;
    if (info.syncProperties) {
        pending = UpdateSyncProperties(true);
    } else {
        std::unique_lock lock { mutex_ };
        pending = pendingRender_;
        pendingRender_ = false;
    }

    // process potential modified events from property synchronisation
    ecs_->ecs->ProcessEvents();

    uint64_t currentTime {};

    if (info.clock) {
        currentTime = static_cast<uint64_t>(info.clock->GetTime().ToMicroseconds());
    } else {
        currentTime =
            static_cast<uint64_t>(duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count());
    }

    if (firstTime_ == ~0u) {
        previousFrameTime_ = firstTime_ = currentTime;
    }
    auto deltaTime = currentTime - previousFrameTime_;
    constexpr auto limitHz = duration_cast<microseconds>(duration<float, std::ratio<1, 15u>>(1)).count();
    if (deltaTime > limitHz) {
        deltaTime = limitHz; // clamp the time step to no longer than 15hz.
    }
    previousFrameTime_ = currentTime;
    const uint64_t totalTime = currentTime - firstTime_;

    bool needsRender = ecs_->ecs->Update(totalTime, deltaTime);

    // process modifications from the ecs update
    ecs_->ecs->ProcessEvents();

    if ((needsRender && mode_ != RenderMode::MANUAL) || pending) {
        auto renderHandles = graphicsContext3D_->GetRenderNodeGraphs(*ecs_->ecs);
        if (!renderHandles.empty()) {
            // The scene needs to be rendered.
            if (auto rctx = GetRenderContextPtr()) {
                RENDER_NS::IRenderer& renderer = rctx->GetRenderer();
                renderer.RenderDeferred(renderHandles);
                NotifyRenderingCameras();
            }
        }
    }
}

void InternalScene::Update(bool syncProperties)
{
    IInternalSceneCore::UpdateInfo info;
    info.syncProperties = syncProperties;
    Update(info);
}

void InternalScene::NotifyRenderingCameras()
{
    for (auto it = renderingCameras_.begin(); it != renderingCameras_.end();) {
        if (auto c = it->lock()) {
            c->NotifyRenderTargetChanged();
            ++it;
        } else {
            it = renderingCameras_.erase(it);
        }
    }
}

BASE_NS::vector<ICamera::Ptr> InternalScene::GetCameras() const
{
    BASE_NS::vector<ICamera::Ptr> ret;

    for (size_t i = 0; i != ecs_->cameraComponentManager->GetComponentCount(); ++i) {
        if (auto n = interface_pointer_cast<ICamera>(
                FindNode(ecs_->cameraComponentManager->GetEntity(i), ClassId::CameraNode))) {
            ret.push_back(n);
        }
    }

    return ret;
}

BASE_NS::vector<META_NS::IAnimation::Ptr> InternalScene::GetAnimations() const
{
    BASE_NS::vector<META_NS::IAnimation::Ptr> ret;

    for (size_t i = 0; i != ecs_->animationComponentManager->GetComponentCount(); ++i) {
        auto ent = ecs_->animationComponentManager->GetEntity(i);
        if (ecs_->ecs->GetEntityManager().IsAlive(ent)) {
            if (META_NS::IAnimation::Ptr anim = FindAnimation(ent)) {
                animations_[ent] = anim;
                ret.push_back(anim);
            } else {
                CORE_LOG_W("Failed to create EcsAnimation object");
            }
        }
    }

    return ret;
}

META_NS::IAnimation::Ptr InternalScene::FindAnimation(CORE_NS::Entity ent, bool queryResource) const
{
    META_NS::IAnimation::Ptr anim;
    if (auto it = animations_.find(ent); it != animations_.end()) {
        anim = it->second.lock();
    }
    if (!anim) {
        if (queryResource) {
            anim = interface_pointer_cast<META_NS::IAnimation>(CreateObject(ClassId::EcsAnimation, ent));
        } else {
            anim = interface_pointer_cast<META_NS::IAnimation>(CreatePlainObject(ClassId::EcsAnimation, ent, {}));
        }
    }
    return anim;
}

META_NS::IAnimation::Ptr InternalScene::FindAnimation(CORE_NS::Entity ent) const
{
    return FindAnimation(ent, true);
}

META_NS::IAnimation::Ptr InternalScene::FindAnimation(const CORE_NS::ResourceId& id) const
{
    META_NS::IAnimation::Ptr anim;
    for (size_t i = 0; !anim && i != ecs_->animationComponentManager->GetComponentCount(); ++i) {
        auto ent = ecs_->animationComponentManager->GetEntity(i);
        if (ecs_->ecs->GetEntityManager().IsAlive(ent)) {
            if (auto r = ecs_->resourceComponentManager->Read(ent)) {
                if (r->resourceId == id) {
                    anim = FindAnimation(ent, false);
                }
            }
        }
    }
    return anim;
}

void InternalScene::RegisterComponent(const BASE_NS::Uid& id, const IComponentFactory::Ptr& p)
{
    componentFactories_[id] = p;
}

void InternalScene::UnregisterComponent(const BASE_NS::Uid& id)
{
    componentFactories_.erase(id);
}

BASE_NS::shared_ptr<IComponentFactory> InternalScene::FindComponentFactory(const BASE_NS::Uid& id) const
{
    auto it = componentFactories_.find(id);
    return it != componentFactories_.end() ? it->second : nullptr;
}

bool InternalScene::SyncProperty(const META_NS::IProperty::ConstPtr& p, META_NS::EngineSyncDirection dir)
{
    META_NS::IEngineValue::Ptr value = GetEngineValueFromProperty(p);
    if (!value) {
        if (auto i = META_NS::GetFirstValueFromProperty<IConvertingValue>(p)) {
            value = GetEngineValueFromProperty(i->GetTargetProperty());
        }
    }
    META_NS::InterfaceUniqueLock valueLock { value };
    return value && value->Sync(dir);
}

void InternalScene::AddRenderingCamera(const IInternalCamera::Ptr& camera)
{
    for (auto&& v : renderingCameras_) {
        if (camera == v.lock()) {
            return;
        }
    }
    renderingCameras_.push_back(camera);
}
void InternalScene::RemoveRenderingCamera(const IInternalCamera::Ptr& camera)
{
    for (auto it = renderingCameras_.begin(); it != renderingCameras_.end(); ++it) {
        if (camera == it->lock()) {
            renderingCameras_.erase(it);
            return;
        }
    }
}

bool InternalScene::SetRenderMode(RenderMode mode)
{
    mode_ = mode;
    ecs_->ecs->SetRenderMode(
        mode == RenderMode::IF_DIRTY ? CORE_NS::IEcs::RENDER_IF_DIRTY : CORE_NS::IEcs::RENDER_ALWAYS);
    return true;
}
RenderMode InternalScene::GetRenderMode() const
{
    return mode_;
}
void InternalScene::RenderFrame()
{
    std::unique_lock lock { mutex_ };
    pendingRender_ = true;
}
bool InternalScene::HasPendingRender() const
{
    std::unique_lock lock { mutex_ };
    return pendingRender_;
}

NodeHits InternalScene::MapHitResults(
    const BASE_NS::vector<CORE3D_NS::RayCastResult>& res, const RayCastOptions& options) const
{
    NodeHits result;
    CORE3D_NS::ISceneNode* n = nullptr;
    if (auto obj = interface_cast<IEcsObjectAccess>(options.node)) {
        if (auto ecs = obj->GetEcsObject()) {
            n = ecs_->GetNode(ecs->GetEntity());
        }
    }

    for (auto&& v : res) {
        NodeHit h;
        if (v.node && (!n || n->IsAncestorOf(*v.node))) {
            h.node = FindNode(v.node->GetEntity(), {});
            h.distance = v.distance;
            h.distanceToCenter = v.centerDistance;
            h.position = v.worldPosition;
            result.push_back(BASE_NS::move(h));
        }
    }
    return result;
}

void AddPickResult(BASE_NS::vector<CORE3D_NS::RayCastResult>& target, BASE_NS::vector<CORE3D_NS::RayCastResult>&& vec)
{
    if (!vec.empty()) {
        bool sort = !target.empty(); // Only sort if adding to non-empty list
        target.append(vec.begin(), vec.end());
        if (sort) {
            std::sort(target.begin(), target.end(),
                [](const auto& lhs, const auto& rhs) { return (lhs.distance < rhs.distance); });
        }
    }
}

NodeHits InternalScene::CastRay(
    const BASE_NS::Math::Vec3& pos, const BASE_NS::Math::Vec3& dir, const RayCastOptions& options) const
{
    BASE_NS::vector<CORE3D_NS::RayCastResult> result;
    for (auto&& picking : ecs_->GetPicking()) {
        if (picking) {
            AddPickResult(result, BASE_NS::move(picking->RayCast(*ecs_->ecs, pos, dir, options.layerMask)));
        }
    }
    return MapHitResults(result, options);
}
NodeHits InternalScene::CastRay(
    const IEcsObject::ConstPtr& entity, const BASE_NS::Math::Vec2& pos, const RayCastOptions& options) const
{
    BASE_NS::vector<CORE3D_NS::RayCastResult> result;
    for (auto&& picking : ecs_->GetPicking()) {
        if (picking) {
            AddPickResult(result,
                BASE_NS::move(picking->RayCastFromCamera(*ecs_->ecs, entity->GetEntity(), pos, options.layerMask)));
        }
    }
    return MapHitResults(result, options);
}
BASE_NS::Math::Vec3 InternalScene::ScreenPositionToWorld(
    const IEcsObject::ConstPtr& entity, const BASE_NS::Math::Vec3& pos) const
{
    BASE_NS::Math::Vec3 result;
    if (ecs_->picking) { // Only use the default picking
        result = ecs_->picking->ScreenToWorld(*ecs_->ecs, entity->GetEntity(), pos);
    }
    return result;
}
BASE_NS::Math::Vec3 InternalScene::WorldPositionToScreen(
    const IEcsObject::ConstPtr& entity, const BASE_NS::Math::Vec3& pos) const
{
    BASE_NS::Math::Vec3 result;
    if (ecs_->picking) { // Only use the default picking
        result = ecs_->picking->WorldToScreen(*ecs_->ecs, entity->GetEntity(), pos);
    }
    return result;
}

void InternalScene::ListenNodeChanges(bool enabled)
{
    bool change = false;
    if (enabled) {
        change = !nodeListening_++;
    } else if (nodeListening_ > 0) {
        change = !--nodeListening_;
    }
    if (change) {
        ecs_->ListenNodeChanges(enabled);
    }
}

void InternalScene::OnChildChanged(
    META_NS::ContainerChangeType type, CORE_NS::Entity parent, CORE_NS::Entity childEntity, size_t index)
{
    if (auto it = nodes_.find(parent); it != nodes_.end()) {
        if (auto i = interface_cast<INodeNotify>(it->second)) {
            if (auto child = FindNode(childEntity, {})) {
                i->OnChildChanged(type, child, index);
            } else {
                CORE_LOG_W("child changed but cannot construct it?!");
            }
        }
    }
}

BASE_NS::vector<INode::Ptr> InternalScene::GetNodes() const
{
    // This could be improved to traverse the Node system, and get a list of nodes that we have a wrapper for in a given
    // traversal order
    BASE_NS::vector<INode::Ptr> nodes;
    nodes.reserve(nodes_.size());
    for (auto&& n : nodes_) {
        nodes.push_back(n.second);
    }
    return nodes;
}

void InternalScene::StartAllStartables(META_NS::IStartableController::ControlBehavior behavior)
{
    if (options_.enableStartables) {
        if (auto me = self_.lock()) {
            for (auto&& n : GetNodes()) {
                Internal::StartAllStartables(
                    me, StartableHandler::StartType::DEFERRED, interface_pointer_cast<META_NS::IObject>(n));
            }
        }
    }
}

void InternalScene::StopAllStartables(META_NS::IStartableController::ControlBehavior behavior)
{
    if (options_.enableStartables) {
        for (auto&& n : GetNodes()) {
            Internal::StopAllStartables(interface_pointer_cast<META_NS::IObject>(n));
        }
    }
}

ResourceGroupBundle InternalScene::GetResourceGroups() const
{
    std::shared_lock lock { mutex_ };
    return groups_;
}

void InternalScene::SetResourceGroups(ResourceGroupBundle b)
{
    std::shared_lock lock { mutex_ };
    groups_ = BASE_NS::move(b);
}

SceneDebugInfo InternalScene::GetDebugInfo() const
{
    SceneDebugInfo info;
    info.nodeObjectsAlive = nodes_.size();
    for (auto&& n : nodes_) {
        if (auto i = interface_cast<META_NS::INamed>(n.second)) {
            CORE_LOG_D("node: '%s'", i->Name()->GetValue().c_str());
        }
    }
    for (auto&& a : animations_) {
        if (!a.second.expired()) {
            ++info.animationObjectsAlive;
        }
    }
    return info;
}

SCENE_END_NAMESPACE()
