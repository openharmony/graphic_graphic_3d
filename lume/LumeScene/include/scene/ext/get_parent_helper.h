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

#ifndef SCENE_EXT_GET_PARENT_HELPER_H
#define SCENE_EXT_GET_PARENT_HELPER_H

#include <scene/base/namespace.h>
#include <scene/interface/intf_node.h>

#include <meta/interface/intf_containable.h>

SCENE_BEGIN_NAMESPACE()

struct ContainableTag {};
class NodeContaineable : public META_NS::IContainable {
    virtual META_NS::IObject::Ptr GetParent(ContainableTag) const = 0;
    META_NS::IObject::Ptr GetParent() const override
    {
        return GetParent(ContainableTag {});
    }
};

struct NodeTag {};
class NodeInterface : public INode {
    virtual Future<INode::Ptr> GetParent(NodeTag) const = 0;
    Future<INode::Ptr> GetParent() const override
    {
        return GetParent(NodeTag {});
    }
};

SCENE_END_NAMESPACE()

#endif
