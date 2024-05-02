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
#include "hierarchy_controller.h"

#include <meta/api/future.h>
#include <meta/api/make_callback.h>
#include <meta/api/task.h>
#include <meta/interface/intf_content.h>
#include <meta/interface/intf_task_queue_registry.h>

using namespace META_NS;

SCENE_BEGIN_NAMESPACE()

bool NodeHierarchyController::Build(const IMetadata::Ptr& data)
{
    observer_ = GetObjectRegistry().Create<IObjectHierarchyObserver>(META_NS::ClassId::ObjectHierarchyObserver);
    CORE_ASSERT(observer_);

    // Do not serialize the observer.
    META_NS::SetObjectFlags(observer_, META_NS::ObjectFlagBits::SERIALIZE, false);

    observer_->OnHierarchyChanged()->AddHandler(
        MakeCallback<IOnHierarchyChanged>(this, &NodeHierarchyController::HierarchyChanged));

    return true;
}

void NodeHierarchyController::Destroy()
{
    SetTarget({}, {});
    observer_.reset();
}

void NodeHierarchyController::SetTarget(const IObject::Ptr& hierarchyRoot, HierarchyChangeModeValue mode)
{
    if (!observer_) {
        return;
    }
    target_ = hierarchyRoot;

    if (!hierarchyRoot) {
        DetachAll();
    }

    observer_->SetTarget(hierarchyRoot, mode);

    if (hierarchyRoot) {
        AttachAll();
    }
}

IObject::Ptr NodeHierarchyController::GetTarget() const
{
    return observer_->GetTarget();
}

BASE_NS::vector<IObject::Ptr> NodeHierarchyController::GetAllObserved() const
{
    return observer_->GetAllObserved();
}

bool NodeHierarchyController::AttachAll()
{
    if (const auto root = target_.lock()) {
        return RunOperation({ NodeOperation::ATTACH, target_ });
    }
    return false;
}

bool NodeHierarchyController::DetachAll()
{
    if (auto root = target_.lock()) {
        return RunOperation({ NodeOperation::DETACH, target_ });
    }
    return false;
}

void NodeHierarchyController::HierarchyChanged(const HierarchyChangedInfo& info)
{
    if (info.change == HierarchyChangeType::ADDED) {
        RunOperation({ NodeOperation::ATTACH, info.object });
    } else if (info.change == HierarchyChangeType::REMOVING) {
        RunOperation({ NodeOperation::DETACH, info.object });
    }
}

template<class T, class Callback>
void IterateChildren(const BASE_NS::vector<T>& children, bool reverse, Callback&& callback)
{
    if (reverse) {
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            callback(*it);
        }
    } else {
        for (auto&& child : children) {
            callback(child);
        }
    }
}

template<class Callback>
void IterateHierarchy(const IObject::Ptr& root, bool reverse, Callback&& callback)
{
    if (const auto container = interface_cast<IContainer>(root)) {
        IterateChildren(container->GetAll(), reverse, callback);
    }
    if (const auto content = interface_cast<IContent>(root)) {
        if (auto object = GetValue(content->Content())) {
            callback(object);
        }
    }
}

BASE_NS::vector<INode::Ptr> NodeHierarchyController::GetAllNodes() const
{
    BASE_NS::vector<INode::Ptr> nodes;
    auto add = [&nodes](const INode::Ptr& node) { nodes.push_back(node); };
    if (const auto root = target_.lock()) {
        IterateShared(
            root, [&add](const IObject::Ptr& object) { return true; }, TraversalType::DEPTH_FIRST_POST_ORDER);
    }
    return nodes;
}

void NodeHierarchyController::AttachHierarchy(const IObject::Ptr& root)
{
    if (!root) {
        return;
    }

    AttachNode(interface_cast<INode>(root));
    IterateHierarchy(root, false, [this](const IObject::Ptr& object) { AttachHierarchy(object); });
}

void NodeHierarchyController::AttachNode(INode* const node)
{
    if (node) {
        const auto state = node->GetAttachedState();
        if (state == NodeState::DETACHED) {
            node->AttachToHierarchy();
        }
    }
}

void NodeHierarchyController::DetachHierarchy(const IObject::Ptr& root)
{
    if (!root) {
        return;
    }

    IterateHierarchy(root, true, [this](const IObject::Ptr& object) { DetachHierarchy(object); });
    DetachNode(interface_cast<INode>(root));
}

void NodeHierarchyController::DetachNode(INode* const node)
{
    if (node) {
        const auto state = node->GetAttachedState();
        if (state == NodeState::ATTACHED) {
            node->DetachFromHierarchy();
        }
    }
}

bool NodeHierarchyController::RunOperation(NodeOperation operation)
{
    auto object = operation.root_.lock();
    if (!object) {
        return false;
    }

    // This may potentially end up calling Attach/DetachHierarchy several times
    // for the same subtrees, but we will accept that. Attach/Detach will only
    // be called once since the functions check for current state.
    if (auto root = operation.root_.lock()) {
        if (operation.operation_ == NodeOperation::ATTACH) {
            AttachHierarchy(root);
        } else if (operation.operation_ == NodeOperation::DETACH) {
            DetachHierarchy(root);
        }
    }

    return true;
}

void RegisterNodeHierarchyController()
{
    META_NS::GetObjectRegistry().RegisterObjectType<NodeHierarchyController>();
}
void UnregisterNodeHierarchyController()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<NodeHierarchyController>();
}

SCENE_END_NAMESPACE()
