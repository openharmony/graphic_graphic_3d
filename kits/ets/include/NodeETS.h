/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef NODE_ETS_H
#define NODE_ETS_H

#include <memory>

#include <meta/interface/intf_object.h>
#include <scene/ext/component_util.h>
#include <scene/interface/component_util.h>
#include <scene/interface/intf_node.h>

#include "SceneResourceETS.h"
#include "Vec3Proxy.h"
#include "QuatProxy.h"

namespace OHOS::Render3D {
class NodeETS : public SceneResourceETS, public std::enable_shared_from_this<NodeETS> {
    friend bool operator==(const SCENE_NS::INode::Ptr node, const std::shared_ptr<NodeETS> nodeETS);
    friend class CameraETS;

public:
    static std::shared_ptr<NodeETS> FromNative(const SCENE_NS::INode::Ptr &node);
    enum NodeType { NODE = 1, GEOMETRY = 2, CAMERA = 3, LIGHT = 4, TEXT = 5 };

    NodeETS(const NodeType &type, const SCENE_NS::INode::Ptr &node);
    NodeETS(const SCENE_NS::INode::Ptr &node);
    virtual ~NodeETS();

    META_NS::IObject::Ptr GetNativeObj() const override;

    NodeType GetNodeType();

    std::string GetName() const override;

    std::shared_ptr<Vec3Proxy> GetPosition();
    void SetPosition(const BASE_NS::Math::Vec3 &position);

    std::shared_ptr<Vec3Proxy> GetScale();
    void SetScale(const BASE_NS::Math::Vec3 &scale);

    std::shared_ptr<QuatProxy> GetRotation();
    void SetRotation(const BASE_NS::Math::Quat &rotation);

    std::string GetPath();
    std::shared_ptr<NodeETS> GetParent();

    std::shared_ptr<NodeETS> GetNodeByPath(const std::string &path);
    // napi_value GetComponent(NapiApi::FunctionContext<BASE_NS::string> &ctx);

    std::shared_ptr<NodeETS> GetChildContainer();  // returns a container object.

    bool GetVisible();
    void SetVisible(const bool visible);

    std::shared_ptr<NodeETS> GetLayerMask();

    bool GetLayerMaskEnabled(const uint32_t bit);
    void SetLayerMaskEnabled(const uint32_t bit, const bool enabled);

    size_t GetCount();
    std::shared_ptr<NodeETS> GetChild(const uint32_t index);

    void ClearChildren();
    void InsertChildAfter(const std::shared_ptr<NodeETS> &childNode, const std::shared_ptr<NodeETS> &siblindNode);
    void AppendChild(const std::shared_ptr<NodeETS> &childNode);
    void RemoveChild(const std::shared_ptr<NodeETS> &childNode);

protected:
    SCENE_NS::INode::Ptr GetInternalNode()
    {
        return node_.lock();
    }

private:
    NodeType type_;
    SCENE_NS::INode::WeakPtr node_{nullptr};
    std::shared_ptr<Vec3Proxy> posProxy_{nullptr};
    std::shared_ptr<Vec3Proxy> sclProxy_{nullptr};
    std::shared_ptr<QuatProxy> rotProxy_{nullptr};
};
}  // namespace OHOS::Render3D
#endif  // NODE_ETS_H