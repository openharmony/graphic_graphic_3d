/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef CORE_UTIL_MESH_UTIL_H
#define CORE_UTIL_MESH_UTIL_H

#include <3d/util/intf_mesh_builder.h>
#include <3d/util/intf_mesh_util.h>
#include <base/containers/array_view.h>
#include <base/containers/string_view.h>
#include <core/namespace.h>

BASE_BEGIN_NAMESPACE()
namespace Math {
class Vec2;
class Vec3;
class Vec4;
} // namespace Math
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
class IClassFactory;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class MeshUtil : public IMeshUtil {
public:
    CORE_NS::Entity GeneratePlaneMesh(const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material,
        float width, float depth) override;
    CORE_NS::Entity GenerateSphereMesh(const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material,
        float radius, uint32_t rings, uint32_t sectors) override;
    CORE_NS::Entity GenerateConeMesh(const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material,
        float radius, float length, uint32_t sectors) override;
    CORE_NS::Entity GenerateTorusMesh(const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material,
        float majorRadius, float minorRadius, uint32_t majorSectors, uint32_t minorSectors) override;
    CORE_NS::Entity GenerateCubeMesh(const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material,
        float width, float height, float depth) override;

    CORE_NS::Entity GenerateEntity(
        const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity meshEntity) override;

    CORE_NS::Entity GenerateCube(const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material,
        float width, float height, float depth) override;
    CORE_NS::Entity GeneratePlane(const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material,
        float width, float depth) override;

    CORE_NS::Entity GenerateSphere(const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material,
        float radius, uint32_t rings, uint32_t sectors) override;

    CORE_NS::Entity GenerateCone(const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material,
        float radius, float length, uint32_t sectors) override;

    CORE_NS::Entity GenerateTorus(const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material,
        float majorRadius, float minorRadius, uint32_t majorSectors, uint32_t minorSectors) override;

    explicit MeshUtil(CORE_NS::IClassFactory& factory);
    ~MeshUtil() override = default;

    static void CalculateTangents(const BASE_NS::array_view<const uint8_t>& indices,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3>& positions,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3>& normals,
        const BASE_NS::array_view<const BASE_NS::Math::Vec2>& uvs,
        BASE_NS::array_view<BASE_NS::Math::Vec4> outTangents);
    static void CalculateTangents(const BASE_NS::array_view<const uint16_t>& indices,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3>& positions,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3>& normals,
        const BASE_NS::array_view<const BASE_NS::Math::Vec2>& uvs,
        BASE_NS::array_view<BASE_NS::Math::Vec4> outTangents);
    static void CalculateTangents(const BASE_NS::array_view<const uint32_t>& indices,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3>& positions,
        const BASE_NS::array_view<const BASE_NS::Math::Vec3>& normals,
        const BASE_NS::array_view<const BASE_NS::Math::Vec2>& uvs,
        BASE_NS::array_view<BASE_NS::Math::Vec4> outTangents);

protected:
    IMeshBuilder::Ptr InitializeBuilder(const IMeshBuilder::Submesh& submesh) const;
    CORE_NS::Entity CreateMesh(const CORE_NS::IEcs& ecs, const IMeshBuilder& builder, BASE_NS::string_view name) const;

    CORE_NS::IClassFactory& factory_;
};

CORE3D_END_NAMESPACE()
#endif // CORE_UTIL_MESH_UTIL_H
