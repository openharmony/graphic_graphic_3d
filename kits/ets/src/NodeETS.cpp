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

#include "NodeETS.h"

#include <scene/interface/intf_layer.h>
#include <scene/interface/intf_light.h>

#include <3d/ecs/components/layer_component.h>

#include "CameraETS.h"
#include "GeometryETS.h"
#include "LightETS.h"

namespace OHOS::Render3D {
std::shared_ptr<NodeETS> NodeETS::FromNative(const SCENE_NS::INode::Ptr &node)
{
    if (auto obj = interface_cast<META_NS::IObject>(node)) {
        BASE_NS::string name{obj->GetClassName()};
        if (name == "MeshNode") {
            return std::make_shared<GeometryETS>(node);
        } else if (name == "CameraNode") {
            return std::make_shared<CameraETS>(interface_pointer_cast<SCENE_NS::ICamera>(node));
        } else if (name == "LightNode") {
            return std::make_shared<LightETS>(interface_pointer_cast<SCENE_NS::ILight>(node));
        } else if (name == "TextNode") {
            // should return TextETS
            CORE_LOG_E("TextETS not implemented yet!");
            return std::make_shared<NodeETS>(NodeType::TEXT, node);
        }
    }
    return std::make_shared<NodeETS>(NodeType::NODE, node);
}

NodeETS::NodeETS(const NodeType &type, const SCENE_NS::INode::Ptr &node)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::NODE), type_(type), node_(node)
{}

NodeETS::NodeETS(const SCENE_NS::INode::Ptr &node)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::NODE), node_(node)
{}

NodeETS::~NodeETS()
{
    posProxy_.reset();
    sclProxy_.reset();
    rotProxy_.reset();
    node_.reset();
}

META_NS::IObject::Ptr NodeETS::GetNativeObj() const
{
    auto node = node_.lock();
    if (!node) {
        CORE_LOG_E("empty node_!");
        return {};
    }
    return interface_pointer_cast<META_NS::IObject>(node);
}

NodeETS::NodeType NodeETS::GetNodeType()
{
    return type_;
}

std::string NodeETS::GetName() const
{
    std::string nodeName = SceneResourceETS::GetName();
    if (nodeName.empty()) {
        if (auto node = node_.lock()) {
            if (auto parent = node->GetParent().GetResult(); !parent) {
                nodeName = "rootNode_";
            }
        }
    }
    return nodeName;
}

std::shared_ptr<Vec3Proxy> NodeETS::GetPosition()
{
    auto node = node_.lock();
    if (!node) {
        return nullptr;
    }
    if (!posProxy_) {
        posProxy_ = std::make_shared<Vec3Proxy>(node->Position());
    }
    return posProxy_;
}

void NodeETS::SetPosition(const BASE_NS::Math::Vec3 &position)
{
    auto node = node_.lock();
    if (!node) {
        return;
    }
    if (!posProxy_) {
        posProxy_ = std::make_shared<Vec3Proxy>(node->Position());
    }
    posProxy_->SetValue(position);
}

std::shared_ptr<Vec3Proxy> NodeETS::GetScale()
{
    auto node = node_.lock();
    if (!node) {
        return nullptr;
    }
    if (!sclProxy_) {
        sclProxy_ = std::make_shared<Vec3Proxy>(node->Scale());
    }
    return sclProxy_;
}

void NodeETS::SetScale(const BASE_NS::Math::Vec3 &scale)
{
    auto node = node_.lock();
    if (!node) {
        return;
    }
    if (!sclProxy_) {
        sclProxy_ = std::make_shared<Vec3Proxy>(node->Scale());
    }
    sclProxy_->SetValue(scale);
}

std::shared_ptr<QuatProxy> NodeETS::GetRotation()
{
    auto node = node_.lock();
    if (!node) {
        return {};
    }
    if (!rotProxy_) {
        rotProxy_ = std::make_shared<QuatProxy>(node->Rotation());
    }
    return rotProxy_;
}

void NodeETS::SetRotation(const BASE_NS::Math::Quat &rotation)
{
    auto node = node_.lock();
    if (!node) {
        return;
    }
    if (!rotProxy_) {
        rotProxy_ = std::make_shared<QuatProxy>(node->Rotation());
    }
    rotProxy_->SetValue(rotation);
}

std::string NodeETS::GetPath()
{
    auto node = node_.lock();
    if (!node) {
        return "";
    }
    auto parent = node->GetParent().GetResult();
    if (!parent) {
        // parentless nodes are "root", and root nodes path is ofcourse empty.
        return "";
    }
    BASE_NS::string rootName = "";
    if (auto scene = node->GetScene()) {
        if (auto root = scene->GetRootNode().GetResult()) {
            rootName = root->GetName();
        }
    }
    BASE_NS::string path = node->GetPath().GetResult();
    // remove node name from path.
    path = path.substr(0, path.find_last_of('/') + 1);
    // make sure root node name is there.. (hack)

    // see if root name is in the path.
    auto pos = path.find(rootName);
    if (pos == BASE_NS::string::npos) {
        // rootname missing from path.
        path.insert(1, rootName.c_str());
    }
    return path.c_str();
}

std::shared_ptr<NodeETS> NodeETS::GetParent()
{
    auto node = node_.lock();
    if (!node) {
        return nullptr;
    }
    if (auto parent = node->GetParent().GetResult()) {
        return FromNative(parent);
    } else {
        return nullptr;
    }
}

std::shared_ptr<NodeETS> NodeETS::GetNodeByPath(const std::string &path)
{
    auto node = node_.lock();
    if (!node) {
        return nullptr;
    }
    BASE_NS::string childPath = node->GetPath().GetResult() + "/" + BASE_NS::string(path.c_str());
    auto scene = node->GetScene();
    if (!scene) {
        return nullptr;
    }
    const auto child = scene->FindNode(childPath).GetResult();
    if (child) {
        return FromNative(child);
    } else {
        return nullptr;
    }
}

// napi_value NodeETS::GetComponent(NapiApi::FunctionContext<BASE_NS::string> &ctx);

std::shared_ptr<NodeETS> NodeETS::GetChildContainer()
{
    return shared_from_this();
}

bool NodeETS::GetVisible()
{
    if (auto node = node_.lock()) {
        return node->Enabled()->GetValue();
    }
    return false;
}

void NodeETS::SetVisible(const bool visible)
{
    if (auto node = node_.lock()) {
        node->Enabled()->SetValue(visible);
    }
}

std::shared_ptr<NodeETS> NodeETS::GetLayerMask()
{
    return shared_from_this();
}

bool NodeETS::GetLayerMaskEnabled(const uint32_t bit)
{
    bool enabled = true;
    if (auto node = node_.lock()) {
        uint64_t mask = 1ull << bit;
        if (auto comp = SCENE_NS::GetComponent<SCENE_NS::ILayer>(node)) {
            enabled = comp->LayerMask()->GetValue() & mask;
        }
    }
    return enabled;
}

void NodeETS::SetLayerMaskEnabled(const uint32_t bit, const bool enabled)
{
    if (auto node = node_.lock()) {
        if (!SCENE_NS::GetComponent<SCENE_NS::ILayer>(node)) {
            SCENE_NS::AddComponent<CORE3D_NS::ILayerComponentManager>(node);
        }
        if (auto layer = SCENE_NS::GetComponent<SCENE_NS::ILayer>(node)) {
            uint64_t mask = 1ull << bit;
            if (enabled) {
                layer->LayerMask()->SetValue(layer->LayerMask()->GetValue() | mask);
            } else {
                layer->LayerMask()->SetValue(layer->LayerMask()->GetValue() & ~mask);
            }
        }
    }
}

size_t NodeETS::GetCount()
{
    size_t count = 0;
    if (auto node = node_.lock()) {
        count = node->GetChildren().GetResult().size();
    }
    return count;
}

std::shared_ptr<NodeETS> NodeETS::GetChild(const uint32_t index)
{
    if (auto node = node_.lock()) {
        auto children = node->GetChildren().GetResult();
        if (index < children.size()) {
            const auto child = children[index];
            return FromNative(child);
        }
    }
    return nullptr;
}

void NodeETS::ClearChildren()
{
    BASE_NS::vector<SCENE_NS::INode::Ptr> removedNodes;
    if (auto parent = node_.lock()) {
        for (auto node : parent->GetChildren().GetResult()) {
            parent->RemoveChild(node).GetResult();
            node->Enabled()->SetValue(false);
            removedNodes.emplace_back(BASE_NS::move(node));
        }
    }
}

void NodeETS::InsertChildAfter(const std::shared_ptr<NodeETS> &childNode, const std::shared_ptr<NodeETS> &siblingNode)
{
    if (!childNode) {
        return;
    }
    auto node = node_.lock();
    if (!node) {
        return;
    }
    SCENE_NS::INode::Ptr child = childNode->GetInternalNode();
    SCENE_NS::INode::Ptr sibling = (siblingNode == nullptr ? nullptr : siblingNode->GetInternalNode());
    if (auto parent = node_.lock()) {
        size_t index = 0;
        if (sibling) {
            auto data = parent->GetChildren().GetResult();
            for (auto d : data) {
                index++;
                if (d == sibling) {
                    break;
                }
            }
        }
        parent->AddChild(child, index).GetResult();
        child->Enabled()->SetValue(true);
    }
}

void NodeETS::AppendChild(const std::shared_ptr<NodeETS> &childNode)
{
    if (!childNode) {
        return;
    }
    if (auto parent = node_.lock()) {
        SCENE_NS::INode::Ptr child = childNode->GetInternalNode();
        parent->AddChild(child);
        child->Enabled()->SetValue(true);
    }
}

void NodeETS::RemoveChild(const std::shared_ptr<NodeETS> &childNode)
{
    if (!childNode) {
        return;
    }
    if (auto parent = node_.lock()) {
        SCENE_NS::INode::Ptr child = childNode->GetInternalNode();
        parent->RemoveChild(child).GetResult();
        child->Enabled()->SetValue(false);
    }
}

bool operator==(const SCENE_NS::INode::Ptr node, const std::shared_ptr<NodeETS> nodeETS)
{
    if (!node || !nodeETS) {
        return false;
    }
    return node == nodeETS->GetInternalNode();
}
}  // namespace OHOS::Render3D
