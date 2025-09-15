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

#ifndef OHOS_3D_NODE_IMPL_H
#define OHOS_3D_NODE_IMPL_H

#include "SceneNodes.proj.hpp"
#include "SceneNodes.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

#include "SceneNodes.Transfer.proj.hpp"
#include "SceneNodes.Transfer.impl.hpp"
#include "BaseObjectJS.h"
#include <napi_api.h>
#include "interop_js/arkts_interop_js_api.h"
#include "interop_js/arkts_esvalue.h"
#include "napi/native_api.h"
#include "CheckNapiEnv.h"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include "SceneResourceImpl.h"
#include "NodeETS.h"

namespace OHOS::Render3D::KITETS {
class LayerMaskImpl {
public:
    LayerMaskImpl(const std::shared_ptr<NodeETS> nodeETS);
    ~LayerMaskImpl();
    bool getEnabled(int32_t index);
    void setEnabled(int32_t index, bool enabled);

private:
    std::shared_ptr<NodeETS> nodeETS_;
};

class NodeContainerImpl {
public:
    NodeContainerImpl(const std::shared_ptr<NodeETS> nodeETS);
    ~NodeContainerImpl();
    void append(::SceneNodes::weak::Node item);
    void insertAfter(::SceneNodes::weak::Node item, ::SceneNodes::NodeOrNull const &sibling);
    void remove(::SceneNodes::weak::Node item);
    ::SceneNodes::VariousNodesOrNull get(int32_t index);
    void clear();
    int32_t count();

private:
    std::shared_ptr<NodeETS> nodeETS_;
};

class NodeImpl : public SceneResourceImpl {
public:
    static ::SceneNodes::VariousNodesOrNull MakeVariousNodesOrNull(const std::shared_ptr<NodeETS> &node);
    static ::SceneNodes::VariousNodes MakeVariousNodes(const std::shared_ptr<NodeETS> &node);

    explicit NodeImpl(const std::shared_ptr<NodeETS> nodeETS);
    ~NodeImpl();
    ::SceneTypes::Vec3 getPosition();
    void setPosition(::SceneTypes::weak::Vec3 pos);
    ::SceneTypes::Quaternion getRotation();
    void setRotation(::SceneTypes::weak::Quaternion rotate);
    ::SceneTypes::Vec3 getScale();
    void setScale(::SceneTypes::weak::Vec3 scale);
    bool getVisible();
    void setVisible(bool visible);
    ::SceneNodes::NodeType getNodeType();
    ::SceneNodes::LayerMask getLayerMask();
    ::taihe::string getPath();
    ::SceneNodes::VariousNodesOrNull getParent();
    ::SceneNodes::NodeContainer getNodeContainer();
    ::SceneNodes::VariousNodesOrNull getNodeByPath(::taihe::string_view path);
    void destroy();

    std::shared_ptr<NodeETS> GetInternalNode()
    {
        return nodeETS_;
    }

private:
    std::shared_ptr<NodeETS> nodeETS_;
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_NODE_IMPL_H