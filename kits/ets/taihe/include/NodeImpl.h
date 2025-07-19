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

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include "SceneResourceImpl.h"
#include "NodeETS.h"

class LayerMaskImpl {
public:
    LayerMaskImpl(const std::shared_ptr<NodeETS> nodeETS);
    ~LayerMaskImpl();
    bool getEnabled(int32_t index);
    void setEnabled(int32_t index, bool enabled);

private:
    std::shared_ptr<NodeETS> nodeETS_;
};

class ContainerImpl {
public:
    ContainerImpl(const std::shared_ptr<NodeETS> nodeETS);
    ~ContainerImpl();
    void append(::SceneNodes::weak::Node item);
    void insertAfter(::SceneNodes::weak::Node item, ::SceneNodes::NodeOrNull const &sibling);
    void remove(::SceneNodes::weak::Node item);
    ::SceneNodes::NodeOrNull get(int32_t index);
    void clear();
    int32_t count();

private:
    std::shared_ptr<NodeETS> nodeETS_;
};

class NodeImpl : public SceneResourceImpl {
public:
    NodeImpl(const std::shared_ptr<NodeETS> nodeETS);
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
    ::SceneNodes::NodeOrNull getParent();
    ::SceneNodes::Container getChildren();
    ::SceneNodes::NodeOrNull getNodeByPath(::taihe::string_view path);
    void destroy();

    int64_t GetImpl()
    {
        return reinterpret_cast<int64_t>(this);
    }

    std::shared_ptr<NodeETS> GetInternalNode()
    {
        return nodeETS_;
    }

private:
    std::shared_ptr<NodeETS> nodeETS_;
};
#endif  // OHOS_3D_NODE_IMPL_H