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

#ifndef SCENE_INTERFACE_INODE_H
#define SCENE_INTERFACE_INODE_H

#include <scene/base/types.h>
#include <scene/interface/intf_transform.h>

SCENE_BEGIN_NAMESPACE()

class IScene;

class INode : public ITransform {
    META_INTERFACE(ITransform, INode, "b875c0d3-e974-4106-bbd3-4dae92fe0a50")
public:
    /**
     * @brief Enable node in 3D scene.
     */
    META_PROPERTY(bool, Enabled)

    /// true if the node is enabled and all its parents are enabled
    virtual Future<bool> IsEnabledInHierarchy() const = 0;

    virtual BASE_NS::string GetName() const = 0;
    virtual Future<BASE_NS::string> GetPath() const = 0;

    virtual Future<INode::Ptr> GetParent() const = 0;
    virtual Future<BASE_NS::vector<INode::Ptr>> GetChildren() const = 0;

    virtual Future<bool> AddChild(const INode::Ptr& child, size_t index = -1) = 0;
    virtual Future<bool> RemoveChild(const INode::Ptr&) = 0;

    /**
     * @brief Make a deep clone of this node
     * @param nodeName Name of the cloned node, if empty, automatically generated
     * @param parent Parent node of the cloned node, if nullptr, the parent of the original node is used
     * @return Cloned node
     */
    virtual Future<INode::Ptr> Clone(BASE_NS::string_view nodeName, const INode::Ptr& parent) = 0;
    Future<INode::Ptr> Clone(BASE_NS::string_view nodeName = {})
    {
        return Clone(nodeName, nullptr);
    }

    virtual BASE_NS::shared_ptr<IScene> GetScene() const = 0;

    /**
     * @brief Returns a unique name for a child node based on a name.
     * @param name The name to get a unique version of.
     * @note The returned name is unique while the future is being processed. It is up to the caller to make sure the
     *       Node's child hierarchy is not changed before the unique name is used by the caller.
     * @return If a child of given name does not exist, returns the name as is. If a child of the same name already
     *         exists, name is appended the next available numerical increment.
     */
    virtual Future<BASE_NS::string> GetUniqueChildName(BASE_NS::string_view name) const = 0;
};

META_REGISTER_CLASS(Node, "4e5561d1-0313-4922-b91a-816f388efbbf", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::INode)

#endif
