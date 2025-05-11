#ifndef SCENE_SRC_CORE_INTF_INTERNAL_RAYCAST_H
#define SCENE_SRC_CORE_INTF_INTERNAL_RAYCAST_H

#include <scene/base/types.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_raycast.h>

SCENE_BEGIN_NAMESPACE()

class IInternalRayCast : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInternalRayCast, "b525366f-7b21-4431-8e6c-d423d6e9350d")
public:
    virtual NodeHits CastRay(
        const BASE_NS::Math::Vec3& pos, const BASE_NS::Math::Vec3& dir, const RayCastOptions& options) const = 0;
    virtual NodeHits CastRay(
        const IEcsObject::ConstPtr& entity, const BASE_NS::Math::Vec2& pos, const RayCastOptions& options) const = 0;
    virtual BASE_NS::Math::Vec3 ScreenPositionToWorld(
        const IEcsObject::ConstPtr& entity, const BASE_NS::Math::Vec3& pos) const = 0;
    virtual BASE_NS::Math::Vec3 WorldPositionToScreen(
        const IEcsObject::ConstPtr& entity, const BASE_NS::Math::Vec3& pos) const = 0;
};

SCENE_END_NAMESPACE()

#endif
