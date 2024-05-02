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

#ifndef API_3D_UTIL_IMESH_UTIL_H
#define API_3D_UTIL_IMESH_UTIL_H

#include <3d/namespace.h>
#include <base/containers/string_view.h>
#include <core/ecs/entity.h>
#include <render/resource_handle.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
/** \addtogroup group_util_meshutil
 *  @{
 */

class IMeshUtil {
public:
    /** Generate cube mesh.
     * @param ecs ECS instance where entity will live.
     * @param name Name of the mesh resource.
     * @param material Material for mesh resource, if not set will use default glTF material.
     * @param width Width of the cube (in x dimension).
     * @param height Height of the cube (in y dimension).
     * @param depth Depth of the cube (in z dimension).
     */
    virtual CORE_NS::Entity GenerateCubeMesh(const CORE_NS::IEcs& ecs, BASE_NS::string_view name,
        CORE_NS::Entity material, float width, float height, float depth) = 0;

    /** Generate plane mesh.
     * @param ecs ECS instance where entity will live.
     * @param name Name of the mesh resource.
     * @param material Material for mesh resource, if not set will use default glTF material.
     * @param width Width of the plane (in x dimension).
     * @param depth Depth of the plane (in z dimension).
     */
    virtual CORE_NS::Entity GeneratePlaneMesh(
        const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material, float width, float depth) = 0;

    /** Generate sphere mesh.
     * @param ecs ECS instance where entity will live.
     * @param name Name of the mesh resource.
     * @param material Material for mesh resource, if not set will use default glTF material.
     * @param radius Radius of the sphere.
     * @param rings Number of rings in sphere.
     * @param sectors Number of sectors per ring.
     */
    virtual CORE_NS::Entity GenerateSphereMesh(const CORE_NS::IEcs& ecs, BASE_NS::string_view name,
        CORE_NS::Entity material, float radius, uint32_t rings, uint32_t sectors) = 0;

    /** Generate cone mesh.
     * @param ecs ECS instance where entity will live.
     * @param name Name of the mesh resource.
     * @param material Material for mesh resource, if not set will use default glTF material.
     * @param radius Radius of the cone ring.
     * @param length Length of cone.
     * @param sectors Number of sectors in ring.
     */
    virtual CORE_NS::Entity GenerateConeMesh(const CORE_NS::IEcs& ecs, BASE_NS::string_view name,
        CORE_NS::Entity material, float radius, float length, uint32_t sectors) = 0;

    /** Generate torus mesh.
     * @param ecs ECS instance where entity will live.
     * @param name Name of the mesh resource.
     * @param material Material for mesh resource, if not set will use default glTF material.
     * @param majorRadius Radius of the torus.
     * @param minorRadius Radius of the torus tube.
     * @param majorSectors Number of sectors in the ring.
     * @param minorSectors Number of sectors in the tube of the torus.
     */
    virtual CORE_NS::Entity GenerateTorusMesh(const CORE_NS::IEcs& ecs, BASE_NS::string_view name,
        CORE_NS::Entity material, float majorRadius, float minorRadius, uint32_t majorSectors,
        uint32_t minorSectors) = 0;

    /** Generate renderable entity
     * @param ecs ECS instance where entity will live.
     * @param name Name of the mesh resource and entity.
     * @param meshEntity CORE_NS::Entity owning a MeshComponent.
     */
    virtual CORE_NS::Entity GenerateEntity(
        const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity meshEntity) = 0;

    /** Generate cube entity.
     * @param ecs ECS instance where entity will live.
     * @param name Name of the mesh resource and entity.
     * @param material Material for mesh resource, if not set will use default glTF material.
     * @param width Width of the cube (in x dimension).
     * @param height Height of the cube (in y dimension).
     * @param depth Depth of the cube (in z dimension).
     */
    virtual CORE_NS::Entity GenerateCube(const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material,
        float width, float height, float depth) = 0;

    /** Generate plane entity.
     * @param ecs ECS instance where entity will live.
     * @param name Name of the mesh resource and entity.
     * @param material Material for mesh resource, if not set will use default glTF material.
     * @param width Width of the plane (in x dimension).
     * @param depth Depth of the plane (in z dimension).
     */
    virtual CORE_NS::Entity GeneratePlane(
        const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material, float width, float depth) = 0;

    /** Generate sphere entity.
     * @param ecs ECS instance where entity will live.
     * @param name Name of the mesh resource and entity.
     * @param material Material for mesh resource, if not set will use default glTF material.
     * @param radius Radius of the sphere.
     * @param rings Number of rings in sphere.
     * @param sectors Number of sectors per ring.
     */
    virtual CORE_NS::Entity GenerateSphere(const CORE_NS::IEcs& ecs, BASE_NS::string_view name,
        CORE_NS::Entity material, float radius, uint32_t rings, uint32_t sectors) = 0;

    /** Generate cone entity.
     * @param ecs ECS instance where entity will live.
     * @param name Name of the mesh resource and entity.
     * @param material Material for mesh resource, if not set will use default glTF material.
     * @param radius Radius of the cone ring.
     * @param length Length of cone.
     * @param sectors Number of sectors in ring.
     */
    virtual CORE_NS::Entity GenerateCone(const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material,
        float radius, float length, uint32_t sectors) = 0;

    /** Generate torus entity.
     * @param ecs ECS instance where entity will live.
     * @param name Name of the mesh resource and entity.
     * @param material Material for mesh resource, if not set will use default glTF material.
     * @param majorRadius Radius of the torus.
     * @param minorRadius Radius of the torus tube.
     * @param majorSectors Number of sectors in the ring.
     * @param minorSectors Number of sectors in the tube of the torus.
     */
    virtual CORE_NS::Entity GenerateTorus(const CORE_NS::IEcs& ecs, BASE_NS::string_view name, CORE_NS::Entity material,
        float majorRadius, float minorRadius, uint32_t majorSectors, uint32_t minorSectors) = 0;

protected:
    IMeshUtil() = default;
    virtual ~IMeshUtil() = default;
};
/** @} */
CORE3D_END_NAMESPACE()

#endif // API_3D_UTIL_IMESH_UTIL_H
