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

#include "geometry_definition/CustomETS.h"

namespace GeometryDefinition {

CustomETS::CustomETS(const PrimitiveTopology topology, const BASE_NS::vector<BASE_NS::Math::Vec3> &vertices,
    const BASE_NS::vector<uint32_t> &indices, const BASE_NS::vector<BASE_NS::Math::Vec3> &normals,
    const BASE_NS::vector<BASE_NS::Math::Vec2> &uvs, const BASE_NS::vector<BASE_NS::Color> &colors)
    : GeometryDefinition(), topology_(topology), vertices_(vertices), indices_(indices), normals_(normals), uvs_(uvs),
      colors_(colors)
{}

GeometryType CustomETS::GetType()
{
    return GeometryType::CUSTOM;
}

SCENE_NS::IMesh::Ptr CustomETS::CreateMesh(
    const SCENE_NS::ICreateMesh::Ptr &creator, const SCENE_NS::MeshConfig &config) const
{
    const auto meshData = SCENE_NS::CustomMeshData{GetTopology(), vertices_, indices_, normals_, uvs_, colors_};
    return creator->Create(config, meshData).GetResult();
}

SCENE_NS::PrimitiveTopology CustomETS::GetTopology() const
{
    if (topology_ == PrimitiveTopology::TRIANGLE_LIST) {
        return SCENE_NS::PrimitiveTopology::TRIANGLE_LIST;
    } else {
        return SCENE_NS::PrimitiveTopology::TRIANGLE_STRIP;
    }
}
}  // namespace GeometryDefinition