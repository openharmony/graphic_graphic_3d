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

#ifndef GEOMETRY_DEFINITION_PLANE_ETS_H
#define GEOMETRY_DEFINITION_PLANE_ETS_H

#include <scene/interface/intf_create_mesh.h>

#include "geometry_definition/GeometryDefinition.h"

namespace OHOS::Render3D::Geometry {

class PlaneETS : public GeometryDefinition {
public:
    explicit PlaneETS(const BASE_NS::Math::Vec2 &size);
    ~PlaneETS() override = default;
    GeometryType GetType() override;
    virtual SCENE_NS::IMesh::Ptr CreateMesh(
        const SCENE_NS::ICreateMesh::Ptr &creator, const SCENE_NS::MeshConfig &config) const override;

private:
    BASE_NS::Math::Vec2 size_{};
};

}  // namespace OHOS::Render3D::Geometry
#endif  // GEOMETRY_DEFINITION_PLANE_ETS_H