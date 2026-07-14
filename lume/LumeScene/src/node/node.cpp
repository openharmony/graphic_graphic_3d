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

#include "node.h"

#include <algorithm>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/scene_utils.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_application_context.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/resource/intf_exported_node.h>
#include <scene/interface/resource/types.h>

#include <base/math/matrix_util.h>

#include <meta/api/object_name.h>
#include <meta/base/interface_utils.h>
#include <meta/interface/resource/intf_owned_resource_groups.h>

#include "../component/node_component.h"
#include "../converting_value.h"
#include "../core/ecs.h"
#include "../resource/util.h"

SCENE_BEGIN_NAMESPACE()

namespace {
struct NodeFlagsConverter {
    using SourceType = SCENE_NS::NodeFlags;
    using TargetType = uint32_t;

    SourceType ConvertToSource(META_NS::IAny&, const TargetType& v) const
    {
        return static_cast<SourceType>(v);
    }
    TargetType ConvertToTarget(const SourceType& v) const
    {
        return static_cast<TargetType>(v);
    }
};
}  // namespace

bool Node::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (p && p->GetName() == "NodeFlags") {
        if (auto att = GetSelf<META_NS::IAttach>()) {
            META_NS::Property<uint32_t> ep{GetComponentProperty(att.get(), "NodeComponent", "NodeFlags")};
            if (ep) {
                return PushForwardingValueInstance(ep,
                    interface_cast<META_NS::IStackProperty>(p),
                    META_NS::IValue::Ptr(new ConvertingValue<NodeFlagsConverter>(ep)));
            }
        }
        return false;
    }
    return Super::InitDynamicProperty(p, path);
}

bool Node::Build(const META_NS::IMetadata::Ptr& d)
{
    return Super::Build(d);
}

void Node::Destroy()
{
    if (startableHandler_) {
        startableHandler_->StopAll(GetAttachmentContainer(false));
        startableHandler_.reset();
    }
    Super::Destroy();
}

IScene::Ptr Node::GetScene() const
{
    if (auto s = GetInternalScene()) {
        return s->GetScene();
    }
    return nullptr;
}

Future<INode::Ptr> Node::GetParent(NodeTag) const
{
    if (auto s = GetInternalScene()) {
        return s->AddTaskOrRunDirectly([s, myEnt = GetEntity()] {
            auto ent = s->GetEcsContext().GetParent(myEnt);
            return CORE_NS::EntityUtil::IsValid(ent) ? s->FindNode(ent, {}) : nullptr;
        });
    }
    return {};
}

bool Node::SetEcsObject(const IEcsObject::Ptr& obj)
{
    if (Super::SetEcsObject(obj)) {
        if (auto scene = GetInternalScene()) {
            if (scene->GetOptions().enableStartables) {
                if (!startableHandler_) {
                    startableHandler_ = BASE_NS::make_unique<StartableHandler>(GetSelf<INode>());
                }
                if (startableHandler_) {
                    startableHandler_->StartAll(StartableHandler::StartType::DEFERRED);
                }
            }
        }
        return true;
    }
    return false;
}

bool Node::Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext)
{
    if (Super::Attach(attachment, dataContext)) {
        if (startableHandler_) {
            startableHandler_->Start(attachment, StartableHandler::StartType::DEFERRED);
        }
        return true;
    }
    return false;
}
bool Node::Detach(const IObject::Ptr& attachment)
{
    if (startableHandler_) {
        startableHandler_->Stop(attachment);
    }
    return Super::Detach(attachment);
}

Future<BASE_NS::vector<INode::Ptr>> Node::GetChildren() const
{
    if (auto s = GetInternalScene()) {
        return s->AddTaskOrRunDirectly([this, s, weak = BASE_NS::weak_ptr{GetSelf()}] {
            return weak.lock() ? s->GetChildren(object_) : BASE_NS::vector<INode::Ptr>{};
        });
    }
    return {};
}

Future<bool> Node::RemoveChild(const INode::Ptr& child)
{
    if (auto s = GetInternalScene()) {
        return s->AddTaskOrRunDirectly([this, s, child, weak = BASE_NS::weak_ptr{GetSelf()}] {
            return weak.lock() ? s->RemoveChild(object_, child) : false;
        });
    }
    return {};
}
Future<bool> Node::AddChild(const INode::Ptr& child, size_t index)
{
    if (auto s = GetInternalScene()) {
        return s->AddTaskOrRunDirectly([this, s, child, index, weak = BASE_NS::weak_ptr{GetSelf()}] {
            return weak.lock() ? s->AddChild(object_, child, index) : false;
        });
    }
    return {};
}
Future<INode::Ptr> Node::Clone(BASE_NS::string_view nodeName, const INode::Ptr& parent)
{
    if (parent && parent->GetScene() != GetScene()) {
        // Require same scene for node import
        CORE_LOG_E("Cannot clone a Node to a different Scene.");
        return {};
    }
    if (auto s = GetInternalScene()) {
        return s->AddTaskOrRunDirectly(
            [this, s, parent, weak = BASE_NS::weak_ptr{GetSelf()}, name = BASE_NS::string(nodeName)]() mutable {
                auto self = weak.lock();
                if (!self) {
                    return INode::Ptr{};
                }
                s->SyncProperties();

                IEcsObject::Ptr parentObj = nullptr;
                if (auto i = interface_cast<IEcsObjectAccess>(parent)) {
                    parentObj = i->GetEcsObject();
                }
                if (name.empty()) {
                    auto p = parent ? parent : GetParent().GetResult();
                    if (p) {
                        name = p->GetUniqueChildName(GetName()).GetResult();
                    }
                }
                auto res = CloneAsChild(*object_, parentObj);
                auto node = CORE_NS::EntityUtil::IsValid(res.entity) ? s->FindNode(res.entity, {}) : nullptr;
                if (auto named = interface_cast<META_NS::INamed>(node)) {
                    named->Name()->SetValue(name);
                    SyncPropertyDirect(s, named->Name()).GetResult();
                }
                return node;
            });
    }
    return {};
}
BASE_NS::string Node::GetName() const
{
    return NamedSceneObject::GetName();
}
Future<BASE_NS::string> Node::GetPath() const
{
    if (auto s = GetInternalScene()) {
        return s->AddTaskOrRunDirectly([this, weak = BASE_NS::weak_ptr{GetSelf()}] {
            return weak.lock() ? object_->GetPath() : BASE_NS::string{};
        });
    }
    return {};
}

BASE_NS::Math::Mat4X4 Node::GetTransformMatrix() const
{
    return BASE_NS::Math::Trs(
        META_ACCESS_PROPERTY_VALUE(Position), META_ACCESS_PROPERTY_VALUE(Rotation), META_ACCESS_PROPERTY_VALUE(Scale));
}

static CORE_NS::IResourceManager::Ptr GetResources(INode& node)
{
    if (auto s = node.GetScene()) {
        if (auto is = s->GetInternalScene()) {
            if (auto c = is->GetContext()) {
                return c->GetResources();
            }
        }
    }
    return nullptr;
}

static IExternalNode::Ptr AddExternalNodeAttachment(INode& node, BASE_NS::vector<CORE_NS::Entity> entities,
    const CORE_NS::ResourceIdContext& id, META_NS::IResourceGroupHandle::Ptr handle)
{
    IExternalNode::Ptr ext = META_NS::GetObjectRegistry().Create<IExternalNode>(ClassId::ExternalNode);
    if (ext) {
        if (auto att = interface_cast<META_NS::IAttach>(&node)) {
            ext->SetResourceId(id);
            ext->SetAddedEntities(BASE_NS::move(entities));
            ext->SetSubresourceGroup(BASE_NS::move(handle));
            att->Attach(ext);
        }
    }
    return ext;
}

Future<INode::Ptr> Node::ImportChild(const INode::ConstPtr& node)
{
    if (!node) {
        return {};
    }
    if (node->GetScene() == GetScene()) {
        CORE_LOG_W("Child already belongs to the target scene.");
        return {};
    }
    IEcsObject::Ptr eobj;
    if (auto i = interface_cast<IEcsObjectAccess>(node)) {
        eobj = i->GetEcsObject();
    }
    if (eobj) {
        if (auto s = GetInternalScene()) {
            return s->AddTaskOrRunDirectly([this, s, eobj, node, weak = BASE_NS::weak_ptr{GetSelf()}] {
                auto self = weak.lock();
                if (!self) {
                    return INode::Ptr{};
                }
                auto res = CopyExternalAsChild(*object_, *eobj);
                auto n = CORE_NS::EntityUtil::IsValid(res.entity) ? s->FindNode(res.entity, {}) : nullptr;
                if (n && s->GetOptions().createResources) {
                    if (auto resource = interface_pointer_cast<CORE_NS::IResource>(node->GetScene())) {
                        AddExternalNodeAttachment(*n,
                            BASE_NS::move(res.newEntities),
                            {resource->GetResourceId(), resource->GetContext()},
                            {});
                    }
                }
                return n;
            });
        }
    }
    return {};
}

static META_NS::IResourceGroupHandle::Ptr GetSubGroup(const IInternalScene::Ptr& s, const BASE_NS::string& group)
{
    META_NS::IResourceGroupHandle::Ptr handle;
    if (auto c = s->GetContext()) {
        if (auto i = interface_pointer_cast<META_NS::IOwnedResourceGroups>(c->GetResources())) {
            handle = i->GetGroupHandle(group, s->GetScene());
        }
    }
    return handle;
}

BASE_NS::string HandleName(const INode::Ptr& node, IInternalScene& scene, BASE_NS::string_view name,
    INodeImport::ImportOptions::NamingBehavior behavior)
{
    BASE_NS::string renamed(name);
    switch (behavior) {
        case INodeImport::ImportOptions::NamingBehavior::ENFORCE_UNIQUE_NAME:
            renamed = scene.GetUniqueName(name, GetEcsObjectEntity(interface_cast<IEcsObjectAccess>(node)));
            break;
        case Scene::INodeImport::ImportOptions::NamingBehavior::REMOVE_EXISTING:
            if (!name.empty() && scene.RemoveNamedChild(node, renamed)) {
                CORE_LOG_W("Removed existing duplicate node when importing '%s'", renamed.c_str());
            }
            break;
        case INodeImport::ImportOptions::NamingBehavior::NAME_AS_IS:
            [[fallthrough]];
        default:
            break;
    }
    return renamed;
}

Future<INode::Ptr> Node::ImportChildScene(const IScene::ConstPtr& scene, const ImportOptions& options)
{
    if (scene == GetScene()) {
        CORE_LOG_E("Cannot import scene into itself.");
        return {};
    }
    auto s = GetInternalScene();
    if (!s || !scene) {
        return {};
    }
    BASE_NS::string name(options.nodeName);
    BASE_NS::string resourceGroup(options.resourceGroup);
    auto namingBehavior = options.namingBehavior;
    auto shareResources = options.shareResources;

    return s->AddTaskOrRunDirectly([this,
                                       s,
                                       scene,
                                       name,
                                       resourceGroup,
                                       namingBehavior,
                                       shareResources,
                                       weak = BASE_NS::weak_ptr{GetSelf()}]() mutable {
        auto self = weak.lock();
        if (!self) {
            return INode::Ptr{};
        }
        name = HandleName(GetSelf<INode>(), *s, name, namingBehavior);
        auto res = CopyExternalAsChild(*object_, *scene, resourceGroup, shareResources);
        auto node = CORE_NS::EntityUtil::IsValid(res.entity) ? s->FindNode(res.entity, {}) : nullptr;
        if (auto named = interface_cast<META_NS::INamed>(node)) {
            named->Name()->SetValue(name);
            SyncPropertyDirect(s, named->Name()).GetResult();
        }
        if (node && s->GetOptions().createResources) {
            auto handle = GetSubGroup(s, res.resourceGroup);
            if (auto resource = interface_pointer_cast<CORE_NS::IResource>(scene)) {
                if (auto ext = AddExternalNodeAttachment(*node,
                        BASE_NS::move(res.newEntities),
                        {resource->GetResourceId(), resource->GetContext()},
                        handle)) {
                    ext->AddAssociatedResources(res.associatedResources);
                }
            }
        }
        return node;
    });
}

Future<INode::Ptr> Node::ImportChildScene(BASE_NS::string_view uri, BASE_NS::string_view nodeName)
{
    auto s = GetInternalScene();
    if (!s) {
        return {};
    }
    return s->AddTaskOrRunDirectly(
        [this, s, weak = BASE_NS::weak_ptr{GetSelf()}, uri = BASE_NS::string(uri), name = BASE_NS::string(nodeName)] {
            auto self = weak.lock();
            if (!self) {
                return INode::Ptr{};
            }
            if (auto provider = interface_cast<IApplicationContextProvider>(s)) {
                if (auto appContext = provider->GetApplicationContext()) {
                    if (auto manager = appContext->GetSceneManager()) {
                        auto scene = manager->CreateScene(uri, s->GetOptions()).GetResult();
                        INodeImport::ImportOptions options;
                        options.nodeName = name;
                        options.namingBehavior = INodeImport::ImportOptions::NamingBehavior::NAME_AS_IS;
                        return ImportChildScene(scene, options).GetResult();
                    }
                }
            }
            return INode::Ptr{};
        });
}

static void AddNodeTemplateExternalNode(const IInternalScene::Ptr& s, INodeImportResult& nodeRes,
    const META_NS::IObjectTemplate::ConstPtr& templ, const BASE_NS::string& group)
{
    auto res = interface_pointer_cast<CORE_NS::IResource>(templ);
    if (!res) {
        return;
    }
    auto handle = GetSubGroup(s, group);
    BASE_NS::vector<CORE_NS::Entity> entities;
    if (auto resuMan = static_cast<IResourceComponentManager*>(s->GetEcsContext().FindComponent<ResourceComponent>())) {
        for (auto&& r : nodeRes.GetNewResourcesImported()) {
            auto ent = resuMan->GetEntity(r.id);
            if (CORE_NS::EntityUtil::IsValid(ent)) {
                entities.push_back(ent);
            }
        }
        auto groupEnts = resuMan->GetEntities({CORE_NS::MatchingResourceId{group}});
        entities.insert(entities.begin(), groupEnts.begin(), groupEnts.end());
    }
    if (auto ext = AddExternalNodeAttachment(
            *nodeRes.GetNode(), entities, {res->GetResourceId(), res->GetContext()}, handle)) {
        ext->AddAssociatedResources(nodeRes.GetAssociatedResources());
    }
}

Future<INode::Ptr> Node::ImportTemplate(
    const META_NS::IObjectTemplate::ConstPtr& templ, BASE_NS::string_view resourceGroup)
{
    auto s = GetInternalScene();
    if (!templ || !s) {
        return {};
    }
    return s->AddTaskOrRunDirectly(
        [s, templ, weak = BASE_NS::weak_ptr{GetSelf()}, rg = BASE_NS::string(resourceGroup)] {
            auto self = weak.lock();
            if (!self) {
                return INode::Ptr{};
            }
            BASE_NS::string group = rg;
            if (group.empty()) {
                if (auto res = interface_pointer_cast<CORE_NS::IResource>(templ)) {
                    group = res->GetResourceId().name;
                } else {
                    group = "Node Template";
                }
            }

            CORE_LOG_D("template import group: %s", group.c_str());
            INodeImportResult::Ptr nodeRes;

            auto ntcontext = META_NS::GetObjectRegistry().Create<INodeTemplateContext>(ClassId::NodeTemplateContext);
            if (!ntcontext) {
                CORE_LOG_W("Invalid state");
                return INode::Ptr{};
            }
            ntcontext->SetTargetNode(interface_pointer_cast<INode>(self));
            ntcontext->SetGroup(group);

            nodeRes = interface_pointer_cast<INodeImportResult>(templ->Instantiate(ntcontext));

            bool isValid = nodeRes && nodeRes->GetNode();
            if (isValid && s->GetOptions().createResources) {
                AddNodeTemplateExternalNode(s, *nodeRes, templ, group);
            }
            return isValid ? nodeRes->GetNode() : nullptr;
        });
}

template<typename Func>
static META_NS::IterationResult IterateImpl(const BASE_NS::vector<INode::Ptr>& cont, const Func& func)
{
    for (auto&& child : cont) {
        if (auto obj = interface_pointer_cast<META_NS::IObject>(child)) {
            auto res = func->Invoke(obj);
            if (!res.Continue()) {
                return res;
            }
        }
    }
    return META_NS::IterationResult::CONTINUE;
}

// --- IIterable
META_NS::IterationResult Node::Iterate(const META_NS::IterationParameters& params)
{
    auto f = params.function.GetInterface<META_NS::IIterableCallable<META_NS::IObject::Ptr>>();
    if (!f) {
        CORE_LOG_W("Incompatible function with Iterate");
        return META_NS::IterationResult::FAILED;
    }
    return IterateImpl(GetChildren().GetResult(), f);
}
META_NS::IterationResult Node::Iterate(const META_NS::IterationParameters& params) const
{
    auto f = params.function.GetInterface<META_NS::IIterableConstCallable<META_NS::IObject::Ptr>>();
    if (!f) {
        CORE_LOG_W("Incompatible function with Iterate");
        return META_NS::IterationResult::FAILED;
    }
    return IterateImpl(GetChildren().GetResult(), f);
}
// --- IIterable

// --- IContainer
BASE_NS::vector<META_NS::IObject::Ptr> Node::GetAll() const
{
    return META_NS::PtrArrayCast<IObject>(GetChildren().GetResult());
}
META_NS::IObject::Ptr Node::GetAt(SizeType index) const
{
    auto children = GetChildren().GetResult();
    return index < children.size() ? interface_pointer_cast<IObject>(children[index]) : nullptr;
}
META_NS::IContainer::SizeType Node::GetSize() const
{
    return GetChildren().GetResult().size();
}
static bool MatchCriteria(const META_NS::IContainer::FindOptions& options, const META_NS::IObject::Ptr& object)
{
    return object && (options.name.empty() || object->GetName() == options.name) &&
           META_NS::CheckInterfaces(object, options.uids, options.strict);
}
BASE_NS::vector<META_NS::IObject::Ptr> Node::FindAll(const FindOptions& options) const
{
    BASE_NS::vector<IObject::Ptr> res;
    META_NS::Internal::ConstIterate(
        this,
        [&](const IObject::Ptr& obj) {
            if (MatchCriteria(options, obj)) {
                res.push_back(obj);
            }
            return true;
        },
        META_NS::IterateStrategy{options.behavior, META_NS::LockType::SHARED_LOCK});
    return res;
}
META_NS::IObject::Ptr Node::FindAny(const FindOptions& options) const
{
    IObject::Ptr res;
    META_NS::Internal::ConstIterate(
        this,
        [&](const IObject::Ptr& obj) {
            if (MatchCriteria(options, obj)) {
                res = obj;
            }
            return !res;
        },
        META_NS::IterateStrategy{options.behavior, META_NS::LockType::SHARED_LOCK});
    return res;
}
META_NS::IObject::Ptr Node::FindByName(BASE_NS::string_view name) const
{
    return FindAny({BASE_NS::string(name), META_NS::TraversalType::NO_HIERARCHY});
}
bool Node::Add(const META_NS::IObject::Ptr& object)
{
    if (auto n = interface_pointer_cast<INode>(object)) {
        return AddChild(n).GetResult();
    }
    return false;
}
bool Node::Insert(SizeType index, const META_NS::IObject::Ptr& object)
{
    if (auto n = interface_pointer_cast<INode>(object)) {
        return AddChild(n, index).GetResult();
    }
    return false;
}
bool Node::Remove(SizeType index)
{
    if (auto s = GetInternalScene()) {
        return s->RunDirectlyOrInTask([&] {
            auto obj = GetAt(index);
            return obj && Remove(obj);
        });
    }
    return false;
}
bool Node::Remove(const META_NS::IObject::Ptr& child)
{
    if (auto n = interface_pointer_cast<INode>(child)) {
        return RemoveChild(n).GetResult();
    }
    return false;
}
bool Node::Move(SizeType fromIndex, SizeType toIndex)
{
    if (auto s = GetInternalScene()) {
        return s->RunDirectlyOrInTask([&] {
            auto obj = GetAt(fromIndex);
            return Move(obj, toIndex);
        });
    }
    return false;
}
bool Node::Move(const META_NS::IObject::Ptr& child, SizeType toIndex)
{
    if (auto s = GetInternalScene()) {
        return s->RunDirectlyOrInTask([&] { return child && Remove(child) && Insert(toIndex, child); });
    }
    return false;
}
bool Node::Replace(const META_NS::IObject::Ptr& child, const META_NS::IObject::Ptr& replaceWith, bool addAlways)
{
    if (auto s = GetInternalScene()) {
        return s->RunDirectlyOrInTask([&] {
            auto vec = GetAll();
            size_t index = 0;
            for (; index < vec.size(); ++index) {
                if (vec[index] == child) {
                    break;
                }
            }
            if (index == vec.size()) {
                if (addAlways) {
                    Add(replaceWith);
                    return true;
                }
                return false;
            }
            return Remove(index) && Insert(index, replaceWith);
        });
    }
    return false;
}
void Node::RemoveAll()
{
    if (auto s = GetInternalScene()) {
        s->RunDirectlyOrInTask([&] {
            for (auto&& c : GetAll()) {
                Remove(c);
            }
        });
    }
}
bool Node::IsAncestorOf(const META_NS::IObject::ConstPtr& object) const
{
    if (auto s = GetInternalScene()) {
        return s->RunDirectlyOrInTask([&] {
            auto node = interface_pointer_cast<INode>(object);
            auto self = GetSelf<INode>();
            do {
                if (node == self) {
                    return true;
                }
                if (node) {
                    node = node->GetParent().GetResult();
                }
            } while (node);
            return false;
        });
    }
    return false;
}

META_NS::IObject::Ptr Node::GetParent(ContainableTag) const
{
    META_NS::IObject::Ptr res;
    if (auto s = GetInternalScene()) {
        res = interface_pointer_cast<META_NS::IObject>(GetParent(NodeTag{}).GetResult());
        if (!res && GetEntity() == s->GetEcsContext().GetRootEntity()) {
            res = interface_pointer_cast<META_NS::IObject>(GetScene());
        }
    }
    return res;
}

// --- IContainer

void Node::OnMetadataConstructed(const META_NS::StaticMetadata& m, CORE_NS::IInterface& i)
{
    if (BASE_NS::string_view("OnContainerChanged") == m.name) {
        if (auto s = GetInternalScene()) {
            s->RunDirectlyOrInTask([&] { s->ListenNodeChanges(true); });
        }
    }
}

void Node::OnChildChanged(META_NS::ContainerChangeType type, const INode::Ptr& child, size_t index)
{
    if (auto event = GetEvent("OnContainerChanged", META_NS::MetadataQuery::EXISTING)) {
        if (auto s = GetInternalScene()) {
            META_NS::ChildChangedInfo info{
                type, interface_pointer_cast<META_NS::IObject>(child), GetSelf<META_NS::IContainer>()};
            if (type == META_NS::ContainerChangeType::ADDED) {
                info.to = index;
            }
            if (type == META_NS::ContainerChangeType::REMOVED) {
                info.from = index;
            }
            s->InvokeUserNotification<META_NS::IOnChildChanged>(event, info);
        }
    }
}

bool Node::IsListening() const
{
    return GetEvent("OnContainerChanged", META_NS::MetadataQuery::EXISTING) != nullptr;
}

void Node::OnNodeActiveStateChanged(NodeActiteStateInfo state)
{
    if (startableHandler_) {
        if (state == NodeActiteStateInfo::ACTIVATED) {
            startableHandler_->StartAll(StartableHandler::StartType::DEFERRED);
        } else if (state == NodeActiteStateInfo::DEACTIVATING) {
            startableHandler_->StopAll();
        }
    }
}

Future<bool> Node::IsEnabledInHierarchy() const
{
    if (auto s = GetInternalScene()) {
        return s->AddTaskOrRunDirectly([s, ent = GetEntity()] {
            auto& ecs = s->GetEcsContext();
            if (auto c =
                    static_cast<CORE3D_NS::INodeComponentManager*>(ecs.FindComponent<CORE3D_NS::NodeComponent>())) {
                if (auto v = c->Read(ent)) {
                    return v->effectivelyEnabled;
                }
            }
            return false;
        });
    }
    return {};
}

Future<BASE_NS::string> Node::GetUniqueChildName(BASE_NS::string_view name) const
{
    auto is = GetInternalScene();
    const auto n = BASE_NS::string(name);
    if (!is) {
        return META_NS::GetTaskQueueRegistry().ConstructFutureWithValue(META_NS::ConstructAny(n));
    }
    return is->AddTaskOrRunDirectly([is, n, entity = GetEntity()] { return is->GetUniqueName(n, entity); });
}

SCENE_END_NAMESPACE()
