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

#ifndef SCENE_INTERFACE_INODEHIERARCHY_CONTROLLER_H
#define SCENE_INTERFACE_INODEHIERARCHY_CONTROLLER_H

#include <scene_plugin/interface/intf_node.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/intf_startable.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(INodeHierarchyController, "839c9854-c13f-4c57-a59d-0b712d80ae84")

/**
 * @brief The INodeHierarchyController interface is implemented by controller objects for nodes.
 * @note The default implementation is SCENE_NS::ClassId::NodeHierarchyController.
 */
class INodeHierarchyController : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, INodeHierarchyController)
public:
    /**
     * @brief Attach all nodes which are part of the target hierarchy.
     * @return True if successful, false otherwise.
     */
    virtual bool AttachAll() = 0;
    /**
     * @brief Detaches all nodes which are part of the target hierarchy.
     * @return True if successful, false otherwise.
     */
    virtual bool DetachAll() = 0;
    /**
     * @brief Returns all nodes currently being controlled.
     */
    virtual BASE_NS::vector<INode::Ptr> GetAllNodes() const = 0;
};

SCENE_END_NAMESPACE()

#endif
