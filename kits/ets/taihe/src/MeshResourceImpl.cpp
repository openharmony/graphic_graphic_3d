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

#include "ANIUtils.h"
#include "MeshResourceImpl.h"
#include "geometry_definition/GeometryDefinition.h"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

namespace OHOS::Render3D::KITETS {

static constexpr int CYLINDER_MIN_SEGMENTS = 3;

BASE_NS::unique_ptr<Geometry::CustomETS> MeshResourceImpl::MakeCustomETS(
    const SceneTypes::CustomGeometry &customGeometry)
{
    const taihe::optional<SceneTypes::PrimitiveTopology> topologyOP = customGeometry->getTopology();
    Geometry::CustomETS::PrimitiveTopology topology;
    if (topologyOP.has_value()) {
        topology = static_cast<Geometry::CustomETS::PrimitiveTopology>(topologyOP.value().get_value());
    } else {
        topology = Geometry::CustomETS::PrimitiveTopology::TRIANGLE_LIST;
    }

    auto vertices = ArrayToVector<SceneTypes::Vec3, BASE_NS::Math::Vec3>(customGeometry->getVertices(),
        [](const SceneTypes::Vec3 &a) { return BASE_NS::Math::Vec3(a->getX(), a->getY(), a->getZ()); });

    auto indices = ArrayToVector<int32_t, uint32_t>(
        customGeometry->getIndices(), [](const int32_t a) { return static_cast<uint32_t>(a); });

    auto normals = ArrayToVector<SceneTypes::Vec3, BASE_NS::Math::Vec3>(customGeometry->getNormals(),
        [](const SceneTypes::Vec3 &a) { return BASE_NS::Math::Vec3(a->getX(), a->getY(), a->getZ()); });

    auto uvs = ArrayToVector<SceneTypes::Vec2, BASE_NS::Math::Vec2>(
        customGeometry->getUvs(), [](const SceneTypes::Vec2 &a) { return BASE_NS::Math::Vec2(a->getX(), a->getY()); });

    auto colors = ArrayToVector<SceneTypes::Color, BASE_NS::Color>(customGeometry->getColors(),
        [](const SceneTypes::Color &a) { return BASE_NS::Color(a->getR(), a->getG(), a->getB(), a->getA()); });
    return BASE_NS::make_unique<Geometry::CustomETS>(topology, vertices, indices, normals, uvs, colors);
}

SceneResources::MeshResource MeshResourceImpl::Create(
    SceneTH::SceneResourceParameters const &params, ::SceneTypes::GeometryDefinitionType const &geometry)
{
    BASE_NS::unique_ptr<Geometry::GeometryDefinition> gd{nullptr};
    if (geometry.holds_custom()) {
        const SceneTypes::CustomGeometry &customGeometry = geometry.get_custom_ref();
        gd = MakeCustomETS(customGeometry);
    } else if (geometry.holds_cube()) {
        const SceneTypes::CubeGeometry &cubeGeometry = geometry.get_cube_ref();
        SceneTypes::Vec3 size = cubeGeometry->getSize();
        gd = BASE_NS::make_unique<Geometry::CubeETS>(
            BASE_NS::Math::Vec3(size->getX(), size->getY(), size->getZ()));
    } else if (geometry.holds_plane()) {
        const SceneTypes::PlaneGeometry &planeGeometry = geometry.get_plane_ref();
        SceneTypes::Vec2 size = planeGeometry->getSize();
        gd = BASE_NS::make_unique<Geometry::PlaneETS>(BASE_NS::Math::Vec2(size->getX(), size->getY()));
    } else if (geometry.holds_sphere()) {
        const SceneTypes::SphereGeometry &sphereGeometry = geometry.get_sphere_ref();
        float radius = sphereGeometry->getRadius();
        int segmentCount = sphereGeometry->getSegmentCount();
        gd = BASE_NS::make_unique<Geometry::SphereETS>(radius, segmentCount);
    } else if (geometry.holds_cylinder()) {
        const SceneTypes::CylinderGeometry &cylinderGeometry = geometry.get_cylinder_ref();
        float radius = cylinderGeometry->getRadius();
        float height = cylinderGeometry->getHeight();
        int segmentCount = cylinderGeometry->getSegmentCount();
        if (radius > 0 && height > 0 && segmentCount >= CYLINDER_MIN_SEGMENTS) {
            gd = BASE_NS::make_unique<Geometry::CylinderETS>(radius, height, segmentCount);
        } else {
            taihe::set_error("Unable to create CylinderETS: Invalid parameters given");
        }
    } else {
        taihe::set_error("Unknown type of GeometryDefinition");
        return SceneResources::MeshResource({nullptr, nullptr});
    }
    const std::string name(params.name);
    const std::string uri = ExtractUri(params.uri);
    return taihe::make_holder<MeshResourceImpl, ::SceneResources::MeshResource>(
        std::make_shared<MeshResourceETS>(name, uri, BASE_NS::move(gd)));
}

MeshResourceImpl::MeshResourceImpl(const std::shared_ptr<MeshResourceETS> mrETS)
    : SceneResourceImpl(SceneResources::SceneResourceType::key_t::MESH_RESOURCE, mrETS), mrETS_(mrETS)
{}

MeshResourceImpl::~MeshResourceImpl()
{
    mrETS_.reset();
}

void MeshResourceImpl::destroy()
{
    if (mrETS_) {
        mrETS_->Destroy();
        mrETS_.reset();
    }
}
}  // namespace OHOS::Render3D::KITETS