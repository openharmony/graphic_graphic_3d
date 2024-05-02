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

#ifndef HIERARCHY_CONTROLLER_H
#define HIERARCHY_CONTROLLER_H

#include <scene_plugin/api/node.h>
#include <scene_plugin/interface/intf_hierarchy_controller.h>
#include <shared_mutex>

#include <meta/ext/attachment/attachment.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_hierarchy_observer.h>
#include <meta/interface/intf_task_queue.h>

SCENE_BEGIN_NAMESPACE()

class NodeHierarchyController
    : public META_NS::MetaObjectFwd<NodeHierarchyController, SCENE_NS::ClassId::NodeHierarchyController,
          META_NS::ClassId::MetaObject, INodeHierarchyController, META_NS::IObjectHierarchyObserver> {
public: // ILifeCycle
    bool Build(const META_NS::IMetadata::Ptr& data) override;
    void Destroy() override;

public: // INodeHierarchyController
    bool AttachAll() override;
    bool DetachAll() override;
    BASE_NS::vector<INode::Ptr> GetAllNodes() const override;

public: // IObjectHierarchyObserver
    void SetTarget(const META_NS::IObject::Ptr& root, META_NS::HierarchyChangeModeValue mode) override;
    META_NS::IObject::Ptr GetTarget() const override;
    BASE_NS::vector<IObject::Ptr> GetAllObserved() const override;
    META_FORWARD_EVENT(META_NS::IOnHierarchyChanged, OnHierarchyChanged, observer_->OnHierarchyChanged())

private:
    void AttachHierarchy(const IObject::Ptr& root);
    void DetachHierarchy(const IObject::Ptr& root);
    void AttachNode(INode* const startable);
    void DetachNode(INode* const startable);
    void HierarchyChanged(const META_NS::HierarchyChangedInfo& info);

    META_NS::IObject::WeakPtr target_;
    META_NS::IObjectHierarchyObserver::Ptr observer_;

private: // Task queue handling
    struct NodeOperation {
        enum Operation {
            /** Run AttachHierarchy() for root_ */
            ATTACH,
            /** Run DetachHierarchy() for root_ */
            DETACH,
        };

        Operation operation_;
        META_NS::IObject::WeakPtr root_;
    };

    bool RunOperation(NodeOperation operation);
};

SCENE_END_NAMESPACE()

#endif // HIERARCHY_CONTROLLER_H
