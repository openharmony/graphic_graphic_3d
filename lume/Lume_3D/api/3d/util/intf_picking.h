/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_3D_UTIL_IPICKING_H
#define API_3D_UTIL_IPICKING_H

#include <limits>

#include <3d/namespace.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <core/ecs/entity.h>
#include <core/plugin/intf_interface.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
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

/** Raycast result */
struct RayCastResult {
    /** Node that was hit. */
    ISceneNode* node { nullptr };

    /** Distance to intersection. */
    float distance { 0.0f };
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
     * Get all nodes node hit by ray.
     * @param ecs Entity component system where hit test is done.
     * @param start Starting point of the ray.
     * @param direction Direction of the ray.
     * @return Array of raycast results that describe node that was hit and distance to intersection (ordered by
     * distance).
     */
    virtual BASE_NS::vector<RayCastResult> RayCast(
        CORE_NS::IEcs const& ecs, const BASE_NS::Math::Vec3& start, const BASE_NS::Math::Vec3& direction) const = 0;

    /**
     * Get all nodes node hit by ray using a camera and 2D screen coordinates as input.
     * @param ecs EntityComponentSystem where hit test is done.
     * @param camera Camera entity to be used for the hit test.
     * @param screenPos screen coordinates for hit test. Where (0, 0) is the upper left corner of the screen and (1, 1)
     * the lower left corner.
     * @return Array of raycast results that describe node that was hit and distance to intersection (ordered by
     * distance).
     */
    virtual BASE_NS::vector<RayCastResult> RayCastFromCamera(
        CORE_NS::IEcs const& ecs, CORE_NS::Entity camera, const BASE_NS::Math::Vec2& screenPos) const = 0;

protected:
    IPicking() = default;
    virtual ~IPicking() = default;
};

inline constexpr BASE_NS::string_view GetName(const IPicking*)
{
    return "IPicking";
}

/** @} */
CORE3D_END_NAMESPACE()

#endif // API_3D_UTIL_IPICKING_H
