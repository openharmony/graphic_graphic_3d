/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef GEOMETRY_DEFINITION_CUSTOM_ETS_H
#define GEOMETRY_DEFINITION_CUSTOM_ETS_H

#include <scene/interface/intf_create_mesh.h>

#include <base/math/vector.h>

#include "geometry_definition/GeometryDefinition.h"

namespace OHOS::Render3D::Geometry {

class CustomETS : public GeometryDefinition {
public:
    enum PrimitiveTopology {
        TRIANGLE_LIST = 0,
        TRIANGLE_STRIP = 1,
    };

    explicit CustomETS(const PrimitiveTopology topology, const BASE_NS::vector<BASE_NS::Math::Vec3> &vertices,
        const BASE_NS::vector<uint32_t> &indices, const BASE_NS::vector<BASE_NS::Math::Vec3> &normals,
        const BASE_NS::vector<BASE_NS::Math::Vec2> &uvs, const BASE_NS::vector<BASE_NS::Color> &colors);

    ~CustomETS() override = default;
    GeometryType GetType() override;
    virtual SCENE_NS::IMesh::Ptr CreateMesh(
        const SCENE_NS::ICreateMesh::Ptr &creator, const SCENE_NS::MeshConfig &config) const override;

private:
    // Note: This converts to the internal type. The underlying integer values don't match.
    SCENE_NS::PrimitiveTopology GetTopology() const;

    PrimitiveTopology topology_{PrimitiveTopology::TRIANGLE_LIST};
    BASE_NS::vector<BASE_NS::Math::Vec3> vertices_;
    BASE_NS::vector<uint32_t> indices_;
    BASE_NS::vector<BASE_NS::Math::Vec3> normals_;
    BASE_NS::vector<BASE_NS::Math::Vec2> uvs_;
    BASE_NS::vector<BASE_NS::Color> colors_;
};

}  // namespace OHOS::Render3D::Geometry
#endif  // GEOMETRY_DEFINITION_CUSTOM_ETS_H