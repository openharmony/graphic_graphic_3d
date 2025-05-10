
#ifndef SCENE_INTERFACE_IRAYCAST_H
#define SCENE_INTERFACE_IRAYCAST_H

#include <scene/base/types.h>
#include <scene/interface/intf_layer.h>
#include <scene/interface/intf_node.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Options for casting a ray
 */
struct RayCastOptions {
    /**
     * @brief Include only nodes that belong to the layers in the mask.
     *        If casted from camera, the camera's LayerMask is used, otherwise ALL_LAYER_MASK.
     */
    uint64_t layerMask = NONE_LAYER_MASK;
    /**
     * @brief Include only nodes from this hierarchy
     */
    INode::ConstPtr node;
};

/**
 * @brief Ray/Node hit position
 */
struct NodeHit {
    /// Node the ray hit
    INode::Ptr node;
    /// Distance of the hit from the start position
    float distance {};
    /// Distance of the AABB center from the start position
    float distanceToCenter {};
    /// World position of the hit
    BASE_NS::Math::Vec3 position {};
};

using NodeHits = BASE_NS::vector<NodeHit>;

/**
 * @brief Interface to cast a ray and get list of intersections
 */
class IRayCast : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IRayCast, "050c1a67-e0f8-4fba-8207-4dbea377a2be")
public:
    /**
     * @brief Cast a ray from position and return all intersections
     * @param pos Start position (World coordinates)
     * @param dir Direction of the ray
     * @param options Ray cast options
     * @return List of hits, ordered from closest to furthest
     */
    virtual Future<NodeHits> CastRay(
        const BASE_NS::Math::Vec3& pos, const BASE_NS::Math::Vec3& dir, const RayCastOptions& options) const = 0;
};

/**
 * @brief Interface to cast a ray from camera and get list of intersections
 */
class ICameraRayCast : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ICameraRayCast, "b3bbdb5d-91aa-4107-adfe-c2e277607eeb")
public:
    /**
     * @brief Cast a ray from camera 2D screen coordinates to the camera direction
     * @param pos Start position (Normalized screen coordinates, where (0, 0) is the upper left corner of the screen and
     * (1, 1) the lower right corner.)
     * @param options Ray cast options
     * @return List of hits, ordered from closest to furthest
     */
    virtual Future<NodeHits> CastRay(const BASE_NS::Math::Vec2& pos, const RayCastOptions& options) const = 0;

    /**
     * @brief Project a screen position onto a plane in the 3D scene.
     * @param pos 2D normalized screen coordinates and the z coordinate determines the projection plane's
     *            location between the near clipping plane (0.0) and the far clipping plane (1.0).
     * @return Position in world coordinates
     */
    virtual Future<BASE_NS::Math::Vec3> ScreenPositionToWorld(const BASE_NS::Math::Vec3& pos) const = 0;
    /**
     * @brief Project a position from the 3D scene onto the screen.
     * @param pos 3D world coordinates.
     * @return Normalized screen coordinates.
     */
    virtual Future<BASE_NS::Math::Vec3> WorldPositionToScreen(const BASE_NS::Math::Vec3& pos) const = 0;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::RayCastOptions)
META_TYPE(SCENE_NS::NodeHit)

META_INTERFACE_TYPE(SCENE_NS::IRayCast)
META_INTERFACE_TYPE(SCENE_NS::ICameraRayCast)

#endif
