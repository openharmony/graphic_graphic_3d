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

#include "GeometryETS.h"

namespace OHOS::Render3D {
std::shared_ptr<GeometryETS> GeometryETS::FromJS(
    const SCENE_NS::INode::Ptr meshNode, const std::string &name, const std::string &uri)
{
    auto geometryETS = std::make_shared<GeometryETS>(meshNode);
    geometryETS->SetName(name);
    geometryETS->SetUri(uri);
    return geometryETS;
}

GeometryETS::GeometryETS(const SCENE_NS::INode::Ptr meshNode)
    : NodeETS(NodeETS::NodeType::GEOMETRY, meshNode), meshNode_(meshNode)
{}

GeometryETS::~GeometryETS()
{
    meshNode_.reset();
}

std::shared_ptr<MeshETS> GeometryETS::GetMesh()
{
    if (auto ma = interface_pointer_cast<SCENE_NS::IMeshAccess>(meshNode_)) {
        auto mesh = ma->GetMesh().GetResult();
        return std::make_shared<MeshETS>(mesh);
    }
    return nullptr;
}

std::shared_ptr<MorpherETS> GeometryETS::GetMorpher()
{
    if (auto ma = interface_pointer_cast<SCENE_NS::IMorphAccess>(meshNode_)) {
        if (auto morpher = META_NS::GetValue(ma->Morpher())) {
            return std::make_shared<MorpherETS>(morpher);
        } else {
            return nullptr;
        }
    }
    return nullptr;
}
}  // namespace OHOS::Render3D