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

#ifndef GEOMETRY_ETS_H
#define GEOMETRY_ETS_H

#include <meta/interface/intf_object.h>

#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_mesh_resource.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_node.h>

#include "MeshETS.h"
#include "MorpherETS.h"
#include "NodeETS.h"

class GeometryETS : public NodeETS {
public:
    static std::shared_ptr<GeometryETS> FromJS(const SCENE_NS::INode::Ptr meshNode, const std::string &name, const std::string &uri = "");
    GeometryETS(const SCENE_NS::INode::Ptr meshNode);
    ~GeometryETS();

    std::shared_ptr<MeshETS> GetMesh();
    std::shared_ptr<MorpherETS> GetMorpher();

private:
    SCENE_NS::INode::WeakPtr meshNode_{nullptr};
};
#endif