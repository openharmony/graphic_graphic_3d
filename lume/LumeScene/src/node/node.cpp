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

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_application_context.h>
#include <scene/interface/intf_scene_manager.h>

#include <meta/api/object_name.h>
#include <meta/base/interface_utils.h>

#include "../core/ecs.h"

SCENE_BEGIN_NAMESPACE()

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
        return s->AddTask([=] {
            auto ent = s->GetEcsContext().GetParent(GetEntity());
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
        return s->AddTask([=] { return s->GetChildren(object_); });
    }
    return {};
}

Future<bool> Node::RemoveChild(const INode::Ptr& child)
{
    if (auto s = GetInternalScene()) {
        if (auto ecsobj = interface_pointer_cast<IEcsObjectAccess>(child)) {
            return s->AddTask([=] { return s->RemoveChild(object_, ecsobj->GetEcsObject()); });
        }
    }
    return {};
}
Future<bool> Node::AddChild(const INode::Ptr& child, size_t index)
{
    if (auto s = GetInternalScene()) {
        return s->AddTask([=] { return s->AddChild(object_, child, index); });
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
        return s->AddTask([=] { return object_->GetPath(); });
    }
    return {};
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
            return s->AddTask([=] {
                auto ent = CopyExternalAsChild(*object_, *eobj);
                return CORE_NS::EntityUtil::IsValid(ent) ? s->FindNode(ent, {}) : nullptr;
            });
        }
    }
    return {};
}

Future<INode::Ptr> Node::ImportChildScene(const IScene::ConstPtr& scene, BASE_NS::string_view nodeName)
{
    if (auto s = GetInternalScene(); s && scene) {
        if (scene != GetScene()) {
            return s->AddTask([=, name = BASE_NS::string(nodeName)] {
                auto ent = CopyExternalAsChild(*object_, *scene);
                auto node = CORE_NS::EntityUtil::IsValid(ent) ? s->FindNode(ent, {}) : nullptr;
                if (auto named = interface_cast<META_NS::INamed>(node)) {
                    named->Name()->SetValue(name);
                    SyncProperty(s, named->Name());
                }
                return node;
            });
        }
        CORE_LOG_E("Cannot import a scene into itself.");
    }
    return {};
}

Future<INode::Ptr> Node::ImportChildScene(BASE_NS::string_view uri, BASE_NS::string_view nodeName)
{
    if (auto s = GetInternalScene()) {
        return s->AddTask([=, uri = BASE_NS::string(uri), name = BASE_NS::string(nodeName)] {
            if (auto provider = interface_cast<IApplicationContextProvider>(s)) {
                if (auto appContext = provider->GetApplicationContext()) {
                    if (auto manager = appContext->GetSceneManager()) {
                        auto scene = manager->CreateScene(uri, s->GetOptions()).GetResult();
                        return ImportChildScene(scene, name).GetResult();
                    }
                }
            }
            return INode::Ptr {};
        });
    }
    return {};
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
        META_NS::IterateStrategy { options.behavior, META_NS::LockType::SHARED_LOCK });
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
        META_NS::IterateStrategy { options.behavior, META_NS::LockType::SHARED_LOCK });
    return res;
}
META_NS::IObject::Ptr Node::FindByName(BASE_NS::string_view name) const
{
    return FindAny({ BASE_NS::string(name), META_NS::TraversalType::NO_HIERARCHY });
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
        return s
            ->AddTask([&] {
                auto obj = GetAt(index);
                return obj && Remove(obj);
            })
            .GetResult();
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
    if (auto s = object_->GetScene()) {
        return s
            ->AddTask([&] {
                auto obj = GetAt(fromIndex);
                return Move(obj, toIndex);
            })
            .GetResult();
    }
    return false;
}
bool Node::Move(const META_NS::IObject::Ptr& child, SizeType toIndex)
{
    if (auto s = object_->GetScene()) {
        return s->AddTask([&] { return child && Remove(child) && Insert(toIndex, child); }).GetResult();
    }
    return false;
}
bool Node::Replace(const META_NS::IObject::Ptr& child, const META_NS::IObject::Ptr& replaceWith, bool addAlways)
{
    if (auto s = object_->GetScene()) {
        return s
            ->AddTask([&] {
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
            })
            .GetResult();
    }
    return false;
}
void Node::RemoveAll()
{
    if (auto s = object_->GetScene()) {
        s->AddTask([&] {
             for (auto&& c : GetAll()) {
                 Remove(c);
             }
         }).Wait();
    }
}
bool Node::IsAncestorOf(const META_NS::IObject::ConstPtr& object) const
{
    if (auto s = object_->GetScene()) {
        return s
            ->AddTask([&] {
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
            })
            .GetResult();
    }
    return false;
}

META_NS::IObject::Ptr Node::GetParent(ContainableTag) const
{
    return interface_pointer_cast<META_NS::IObject>(GetParent(NodeTag {}).GetResult());
}

// --- IContainer

void Node::OnMetadataConstructed(const META_NS::StaticMetadata& m, CORE_NS::IInterface& i)
{
    if (BASE_NS::string_view("OnContainerChanged") == m.name) {
        if (auto s = object_->GetScene()) {
            s->AddTask([&] { s->ListenNodeChanges(true); }).Wait();
        }
    }
}

void Node::OnChildChanged(META_NS::ContainerChangeType type, const INode::Ptr& child, size_t index)
{
    if (auto event = GetEvent("OnContainerChanged", META_NS::MetadataQuery::EXISTING)) {
        if (auto s = object_->GetScene()) {
            META_NS::ChildChangedInfo info { type, interface_pointer_cast<META_NS::IObject>(child),
                GetSelf<META_NS::IContainer>() };
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
        return s->AddTask([=, ent = GetEntity()] {
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

SCENE_END_NAMESPACE()
