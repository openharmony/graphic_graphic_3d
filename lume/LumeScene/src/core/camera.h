#ifndef SCENE_SRC_CORE_CAMERA_H
#define SCENE_SRC_CORE_CAMERA_H

#include <scene/ext/intf_ecs_object.h>
#include <scene/interface/intf_raycast.h>
#include <scene/interface/intf_render_target.h>

SCENE_BEGIN_NAMESPACE()

bool UpdateCameraRenderTarget(const IEcsObject::Ptr& camera, const IRenderTarget::Ptr& target);

SCENE_END_NAMESPACE()

#endif