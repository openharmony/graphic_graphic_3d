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

#ifndef API_3D_UTIL_IPICKING_H
#define API_3D_UTIL_IPICKING_H

#include <limits>

#include <3d/namespace.h>
#include <base/containers/array_view.h>
#include <base/containers/refcnt_ptr.h>
#include <base/containers/vector.h>
#include <base/math/matrix.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>

BASE_BEGIN_NAMESPACE()
namespace Math {
class Mat4X4;
} // namespace Math
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
class IEcs;
struct Entity;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class ISceneNode;

/** \addtogroup group_util_picking
 *  @{
 */
struct MinAndMax {
#define CORE_FMAX std::numeric_limits<float>::max()
    BASE_NS::Math::Vec3 minAABB { CORE_FMAX, CORE_FMAX, CORE_FMAX };
    BASE_NS::Math::Vec3 maxAABB { -CORE_FMAX, -CORE_FMAX, -CORE_FMAX };
#undef CORE_FMAX
};

/** Raycast result. */
struct RayCastResult {
    /** Node that was hit. */
    ISceneNode* node { nullptr };

    /** Distance to AABB center. */
    float centerDistance { 0.0f };

    /** Distance to the hit position. */
    float distance { 0.0f };

    /** Position of the hit. */
    BASE_NS::Math::Vec3 worldPosition { 0.0f, 0.0f, 0.0f };
};

/** Raycast result for ray-triangle intersection. */
struct RayTriangleCastResult {
    /** Distance to the hit position. */
    float distance { 0.0f };

    /** Position of the hit. */
    BASE_NS::Math::Vec3 worldPosition { 0.0f, 0.0f, 0.0f };

    /** Ray hit UV for ray-triangle intersection. */
    BASE_NS::Math::Vec2 hitUv { 0.0f, 0.0f };

    /** The index of the triangle hit. */
    uint64_t triangleIndex { 0 };
};

class IPicking : public CORE_NS::IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid { "9a4791d7-19e2-4dc0-a4fd-b0804d153d70" };

    using Ptr = BASE_NS::refcnt_ptr<IPicking>;

    /** Projects normalized screen coordinates to world coordinates. Origin is at the top left corner. Z-value of the
     * screen coordinate is interpreted so that 0.0 maps to the near plane and 1.0 to the far plane.
     * @param ecs Entity component system where cameraEntity lives.
     * @param cameraEntity Camera entity to be used for projection.
     * @param screenCoordinate Normalized screen coordinates.
     * @return Projected world coordinates.
     */
    virtual BASE_NS::Math::Vec3 ScreenToWorld(
        CORE_NS::IEcs const& ecs, CORE_NS::Entity cameraEntity, BASE_NS::Math::Vec3 screenCoordinate) const = 0;

    /** Projects world coordinates to normalized screen coordinates.
     * @param ecs Entity component system where cameraEntity lives.
     * @param cameraEntity Camera entity to be used for projection.
     * @param worldCoordinate World coordinates to be projected.
     * @return Projected screen coordinates.
     */
    virtual BASE_NS::Math::Vec3 WorldToScreen(
        CORE_NS::IEcs const& ecs, CORE_NS::Entity cameraEntity, BASE_NS::Math::Vec3 worldCoordinate) const = 0;

    /** Calculates a world space AABB from local min max values. */
    virtual MinAndMax GetWorldAABB(const BASE_NS::Math::Mat4X4& world, const BASE_NS::Math::Vec3& aabbMin,
        const BASE_NS::Math::Vec3& aabbMax) const = 0;

    /**
     * Calculates a world space AABB for an entity and optionally all of it's children recursively. Uses the world
     * matrix component for the calculations i.e. the values are only valid after the ECS has updated all the systems
     * that manipulate the world matrix.
     */
    virtual MinAndMax GetWorldMatrixComponentAABB(
        CORE_NS::Entity entity, bool isRecursive, CORE_NS::IEcs& ecs) const = 0;

    /**
     * Calculates a world space AABB for an entity and optionally all of it's children recursively. Uses only the
     * transform component for the calculations. This way the call will also give valid results even when the ECS has
     * not been updated. Does not take skinning or animations into account.
     */
    virtual MinAndMax GetTransformComponentAABB(CORE_NS::Entity entity, bool isRecursive, CORE_NS::IEcs& ecs) const = 0;

    /**
     * Get all nodes hit by ray.
     * @param ecs Entity component system where hit test is done.
     * @param start Starting point of the ray.
     * @param direction Direction of the ray.
     * @return Array of raycast results that describe node that was hit and distance to intersection (ordered by
     * distance).
     */
    virtual BASE_NS::vector<RayCastResult> RayCast(
        CORE_NS::IEcs const& ecs, const BASE_NS::Math::Vec3& start, const BASE_NS::Math::Vec3& direction) const = 0;

    /**
     * Get nodes hit by ray. Only entities included in the given layer mask are in the result. Entities without
     * LayerComponent default to LayerConstants::DEFAULT_LAYER_MASK.
     * @param ecs Entity component system where hit test is done.
     * @param start Starting point of the ray.
     * @param direction Direction of the ray.
     * @param layerMask Layer mask for limiting the returned result.
     * @return Array of raycast results that describe node that was hit and distance to intersection (ordered by
     * distance).
     */
    virtual BASE_NS::vector<RayCastResult> RayCast(CORE_NS::IEcs const& ecs, const BASE_NS::Math::Vec3& start,
        const BASE_NS::Math::Vec3& direction, uint64_t layerMask) const = 0;

    /**
     * Raycast against a triangle.
     * @param start Starting point of the ray.
     * @param direction Direction of the ray.
     * @param triangles A list of triangles defined by 3 corner vertices. Must be TRIANGLE_LIST.
     * @return Array of ray-triangle cast results that describe the intersection against triangles.
     */
    virtual BASE_NS::vector<RayTriangleCastResult> RayCast(const BASE_NS::Math::Vec3& start,
        const BASE_NS::Math::Vec3& direction, BASE_NS::array_view<const BASE_NS::Math::Vec3> triangles) const = 0;

    /**
     * Get all nodes hit by ray using a camera and 2D screen coordinates as input.
     * @param ecs EntityComponentSystem where hit test is done.
     * @param camera Camera entity to be used for the hit test.
     * @param screenPos screen coordinates for hit test. Where (0, 0) is the upper left corner of the screen and (1, 1)
     * the lower left corner.
     * @return Array of raycast results that describe node that was hit and distance to intersection (ordered by
     * distance).
     */
    virtual BASE_NS::vector<RayCastResult> RayCastFromCamera(
        CORE_NS::IEcs const& ecs, CORE_NS::Entity camera, const BASE_NS::Math::Vec2& screenPos) const = 0;

    /**
     * Get nodes hit by ray using a camera and 2D screen coordinates as input. Only entities included in the given layer
     * mask are in the result. Entities without LayerComponent default to LayerConstants::DEFAULT_LAYER_MASK.
     * @param ecs EntityComponentSystem where hit test is done.
     * @param camera Camera entity to be used for the hit test.
     * @param screenPos screen coordinates for hit test. Where (0, 0) is the upper left corner of the screen and (1, 1)
     * the lower left corner.
     * @param layerMask Layer mask for limiting the returned result.
     * @return Array of raycast results that describe node that was hit and distance to intersection (ordered by
     * distance).
     */
    virtual BASE_NS::vector<RayCastResult> RayCastFromCamera(CORE_NS::IEcs const& ecs, CORE_NS::Entity camera,
        const BASE_NS::Math::Vec2& screenPos, uint64_t layerMask) const = 0;

    /**
     * Calculates the ray-triangle intersection between a ray and a triangle.
     * @param ecs EntityComponentSystem where hit test is done.
     * @param camera Camera entity to be used for the hit test.
     * @param screenPos screen coordinates for hit test. Where (0, 0) is the upper left corner of the screen and (1, 1)
     * the lower left corner.
     * @param triangles Triangle list where every triangle is defined by 3 corner vertices. Must be TRIANGLE_LIST.
     * @return Array of ray-triangle cast results.
     */
    virtual BASE_NS::vector<RayTriangleCastResult> RayCastFromCamera(CORE_NS::IEcs const& ecs, CORE_NS::Entity camera,
        const BASE_NS::Math::Vec2& screenPos, BASE_NS::array_view<const BASE_NS::Math::Vec3> triangles) const = 0;

protected:
    IPicking() = default;
    virtual ~IPicking() = default;
};

/** @} */
CORE3D_END_NAMESPACE()

#endif // API_3D_UTIL_IPICKING_H
