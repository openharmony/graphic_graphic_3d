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

#include "NodeImpl.h"
#include "Vec3Impl.h"
#include "QuaternionImpl.h"

LayerMaskImpl::LayerMaskImpl(const std::shared_ptr<NodeETS> nodeETS) : nodeETS_(nodeETS)
{}

LayerMaskImpl::~LayerMaskImpl()
{
    nodeETS_.reset();
}

bool LayerMaskImpl::getEnabled(const int32_t index)
{
    if (nodeETS_) {
        return nodeETS_->GetLayerMaskEnabled(index);
    }
    return false;
}

void LayerMaskImpl::setEnabled(int32_t index, bool enabled)
{
    if (nodeETS_) {
        nodeETS_->SetLayerMaskEnabled(index, enabled);
    }
}

ContainerImpl::ContainerImpl(const std::shared_ptr<NodeETS> nodeETS) : nodeETS_(nodeETS)
{}

ContainerImpl::~ContainerImpl()
{
    nodeETS_.reset();
}

void ContainerImpl::append(::SceneNodes::weak::Node item)
{
    TH_THROW(std::runtime_error, "append not implemented");
}

void ContainerImpl::insertAfter(::SceneNodes::weak::Node item, ::SceneNodes::NodeOrNull const &sibling)
{
    TH_THROW(std::runtime_error, "insertAfter not implemented");
}

void ContainerImpl::remove(::SceneNodes::weak::Node item)
{
    TH_THROW(std::runtime_error, "remove not implemented");
}

::SceneNodes::NodeOrNull ContainerImpl::get(int32_t index)
{
    TH_THROW(std::runtime_error, "get not implemented");
}

void ContainerImpl::clear()
{
    TH_THROW(std::runtime_error, "clear not implemented");
}

int32_t ContainerImpl::count()
{
    TH_THROW(std::runtime_error, "count not implemented");
}

NodeImpl::NodeImpl(const std::shared_ptr<NodeETS> nodeETS)
    : SceneResourceImpl(SceneResources::SceneResourceType::key_t::NODE, nodeETS), nodeETS_(nodeETS)
{}

NodeImpl::~NodeImpl()
{
    nodeETS_.reset();
}

::SceneTypes::Vec3 NodeImpl::getPosition()
{
    if (nodeETS_) {
        return taihe::make_holder<Vec3Impl, SceneTypes::Vec3>(nodeETS_->GetPosition());
    }
    return SceneTypes::Vec3({nullptr, nullptr});
}

void NodeImpl::setPosition(::SceneTypes::weak::Vec3 pos)
{
    if (nodeETS_) {
        nodeETS_->SetPosition(BASE_NS::Math::Vec3(pos->getX(), pos->getY(), pos->getZ()));
    }
}

::SceneTypes::Quaternion NodeImpl::getRotation()
{
    if (nodeETS_) {
        return taihe::make_holder<QuaternionImpl, SceneTypes::Quaternion>(nodeETS_->GetRotation());
    }
    return SceneTypes::Quaternion({nullptr, nullptr});
}

void NodeImpl::setRotation(::SceneTypes::weak::Quaternion rotate)
{
    if (nodeETS_) {
        nodeETS_->SetRotation(BASE_NS::Math::Quat(rotate->getX(), rotate->getY(), rotate->getZ(), rotate->getW()));
    }
}

::SceneTypes::Vec3 NodeImpl::getScale()
{
    if (nodeETS_) {
        return taihe::make_holder<Vec3Impl, SceneTypes::Vec3>(nodeETS_->GetScale());
    }
    return SceneTypes::Vec3({nullptr, nullptr});
}

void NodeImpl::setScale(::SceneTypes::weak::Vec3 scale)
{
    if (nodeETS_) {
        nodeETS_->SetScale(BASE_NS::Math::Vec3(scale->getX(), scale->getY(), scale->getZ()));
    }
}

bool NodeImpl::getVisible()
{
    if (nodeETS_) {
        return nodeETS_->GetVisible();
    }
    return false;
}

void NodeImpl::setVisible(const bool visible)
{
    if (nodeETS_) {
        nodeETS_->SetVisible(visible);
    }
}

::SceneNodes::NodeType NodeImpl::getNodeType()
{
    if (nodeETS_) {
        return static_cast<SceneNodes::NodeType::key_t>(static_cast<int32_t>(nodeETS_->GetNodeType()));
    }
    return SceneNodes::NodeType::key_t::NODE;
}

::SceneNodes::LayerMask NodeImpl::getLayerMask()
{
    return taihe::make_holder<LayerMaskImpl, ::SceneNodes::LayerMask>(nodeETS_);
}

::taihe::string NodeImpl::getPath()
{
    if (nodeETS_) {
        return nodeETS_->GetPath();
    }
    return "";
}

::SceneNodes::NodeOrNull NodeImpl::getParent()
{
    std::shared_ptr<NodeETS> parent = nullptr;
    if (nodeETS_) {
        parent = nodeETS_->GetParent();
    }
    if (parent) {
        return SceneNodes::NodeOrNull::make_node(taihe::make_holder<NodeImpl, SceneNodes::Node>(parent));
    } else {
        return SceneNodes::NodeOrNull::make_nValue();
    }
}

::SceneNodes::Container NodeImpl::getChildren()
{
    return taihe::make_holder<ContainerImpl, ::SceneNodes::Container>(nodeETS_);
}

::SceneNodes::NodeOrNull NodeImpl::getNodeByPath(::taihe::string_view path)
{
    WIDGET_LOGI("node.getNodeByPath");
    if (!nodeETS_) {

        WIDGET_LOGI("node.getNodeByPath invalid nodeETS");
        return SceneNodes::NodeOrNull::make_nValue();
    }
    std::shared_ptr<NodeETS> node = nodeETS_->GetNodeByPath(std::string(path));
    return SceneNodes::NodeOrNull::make_node(taihe::make_holder<NodeImpl, SceneNodes::Node>(node));
}

void NodeImpl::destroy()
{
    nodeETS_.reset();
}