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
#include "GeometryImpl.h"
#include "CameraImpl.h"
#include "LightImpl.h"
#include "Vec3Impl.h"
#include "QuaternionImpl.h"

#include "GeometryETS.h"
#include "CameraETS.h"
#include "LightETS.h"

namespace OHOS::Render3D::KITETS {
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

::SceneNodes::VariousNodesOrNull MakeVariousNodesOrNull(const std::shared_ptr<NodeETS> &node) {
    if (!node) {
        return SceneNodes::VariousNodesOrNull::make_nValue();
    }
    NodeETS::NodeType type = node->GetNodeType();
    switch (type) {
        case NodeETS::NodeType::GEOMETRY:
            return SceneNodes::VariousNodesOrNull::make_geometry(taihe::make_holder<GeometryImpl,
                SceneNodes::Geometry>(std::static_pointer_cast<GeometryETS>(node)));
        case NodeETS::NodeType::CAMERA:
            return SceneNodes::VariousNodesOrNull::make_camera(taihe::make_holder<CameraImpl,
                SceneNodes::Camera>(std::static_pointer_cast<CameraETS>(node)));
        case NodeETS::NodeType::LIGHT:
            return SceneNodes::VariousNodesOrNull::make_light(taihe::make_holder<LightImpl,
                SceneNodes::Light>(std::static_pointer_cast<LightETS>(node)));
        default:
            return SceneNodes::VariousNodesOrNull::make_node(taihe::make_holder<NodeImpl, SceneNodes::Node>(node));
    }
}

::SceneNodes::VariousNodesOrNull NodeImpl::getParentInner()
{
    if (!nodeETS_) {
        WIDGET_LOGE("node.getParent invalid nodeETS");
        return SceneNodes::VariousNodesOrNull::make_nValue();
    }
    std::shared_ptr<NodeETS> parent = nodeETS_->GetParent();
    return MakeVariousNodesOrNull(parent);
}

::SceneNodes::Container NodeImpl::getChildren()
{
    return taihe::make_holder<ContainerImpl, ::SceneNodes::Container>(nodeETS_);
}

::SceneNodes::VariousNodesOrNull NodeImpl::getNodeByPathInner(::taihe::string_view path)
{
    if (!nodeETS_) {
        WIDGET_LOGE("node.getNodeByPath invalid nodeETS");
        return SceneNodes::VariousNodesOrNull::make_nValue();
    }
    std::shared_ptr<NodeETS> node = nodeETS_->GetNodeByPath(std::string(path));
    return MakeVariousNodesOrNull(node);
}

void NodeImpl::destroy()
{
    nodeETS_.reset();
}

::SceneNodes::Node nodeTransferStaticImpl(uintptr_t input)
{
    WIDGET_LOGI("nodeTransferStaticImpl");
    ani_object esValue = reinterpret_cast<ani_object>(input);
    void *nativePtr = nullptr;
    if (!arkts_esvalue_unwrap(taihe::get_env(), esValue, &nativePtr) || nativePtr == nullptr) {
        WIDGET_LOGE("nodeTransferStaticImpl failed during arkts_esvalue_unwrap.");
        return SceneNodes::Node({nullptr, nullptr});
    }

    TrueRootObject *tro = reinterpret_cast<TrueRootObject *>(nativePtr);
    SCENE_NS::INode::Ptr nodePtr = tro->GetNativeObject<SCENE_NS::INode>();
    if (nodePtr == nullptr) {
        WIDGET_LOGE("nodeTransferStaticImpl failed during GetNativeObject.");
        return SceneNodes::Node({nullptr, nullptr});
    }
    auto node = NodeETS::FromNative(nodePtr);
    NodeETS::NodeType type = node->GetNodeType();
    switch (type) {
        case NodeETS::NodeType::GEOMETRY:
            return taihe::make_holder<GeometryImpl, SceneNodes::Geometry>(std::static_pointer_cast<GeometryETS>(node));
        case NodeETS::NodeType::CAMERA:
            return taihe::make_holder<CameraImpl, SceneNodes::Camera>(std::static_pointer_cast<CameraETS>(node));
        case NodeETS::NodeType::LIGHT:
            return taihe::make_holder<LightImpl, SceneNodes::Light>(std::static_pointer_cast<LightETS>(node));
        default:
            return taihe::make_holder<NodeImpl, SceneNodes::Node>(node);
    }
}

uintptr_t nodeTransferDynamicImpl(::SceneNodes::Node input)
{
    WIDGET_LOGI("nodeTransferDynamicImpl");
    int64_t implRawPtr = input->GetImpl();
    NodeImpl *implPtr = reinterpret_cast<NodeImpl *>(implRawPtr);
    std::shared_ptr<NodeETS> nodeETS = implPtr->GetInternalNode();
    if (!nodeETS) {
        WIDGET_LOGE("get NodeETS failed");
        return 0;
    }

    SCENE_NS::INode::Ptr node = interface_pointer_cast<SCENE_NS::INode>(nodeETS->GetNativeObj());
    if (!node) {
        WIDGET_LOGE("can't get scene from node");
        return 0;
    }
    napi_env jsenv;
    if (!arkts_napi_scope_open(taihe::get_env(), &jsenv)) {
        WIDGET_LOGE("arkts_napi_scope_open failed");
        return 0;
    }
    if (!TransferEnvironment::check(jsenv)) {
        WIDGET_LOGE("TransferEnvironment check failed");
        // An error has occurred, ignoring the function call result.
        arkts_napi_scope_close_n(jsenv, 0, nullptr, nullptr);
        return 0;
    }
    auto sceneJs = CreateFromNativeInstance(jsenv, node->GetScene(), PtrType::STRONG, {});
    if (!sceneJs) {
        WIDGET_LOGE("create SceneJS failed.");
        // An error has occurred, ignoring the function call result.
        arkts_napi_scope_close_n(jsenv, 0, nullptr, nullptr);
        return 0;
    }
    napi_value args[] = {sceneJs.ToNapiValue(), NapiApi::Object(jsenv).ToNapiValue()};
    auto napiObj = CreateFromNativeInstance(jsenv, node, PtrType::WEAK, args);
    napi_value nodeValue = napiObj.ToNapiValue();
    ani_ref resAny;
    if (!arkts_napi_scope_close_n(jsenv, 1, &nodeValue, &resAny)) {
        WIDGET_LOGE("arkts_napi_scope_close_n failed");
        return 0;
    }
    return reinterpret_cast<uintptr_t>(resAny);
}
}  // namespace OHOS::Render3D::KITETS

using namespace OHOS::Render3D::KITETS;
// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_nodeTransferStaticImpl(nodeTransferStaticImpl);
TH_EXPORT_CPP_API_nodeTransferDynamicImpl(nodeTransferDynamicImpl);
// NOLINTEND