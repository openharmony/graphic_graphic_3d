#include "camera.h"

#include <scene/ext/intf_render_resource.h>

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/intf_gpu_resource_manager.h>

#include "../util.h"

SCENE_BEGIN_NAMESPACE()

bool UpdateCameraRenderTarget(const IEcsObject::Ptr& camera, const IRenderTarget::Ptr& target)
{
    if (auto ecs = GetEcs(camera)) {
        CORE_NS::EntityReference ent;
        if (target) {
            ent = HandleFromRenderResource(camera->GetScene(), target->GetRenderHandle());
        }
        if (auto c = GetScopedHandle<CORE3D_NS::CameraComponent>(camera)) {
            if (ent) {
                auto size = target->Size()->GetValue();
                c->renderResolution[0] = static_cast<float>(size.x);
                c->renderResolution[1] = static_cast<float>(size.y);
                c->customColorTargets = { ent };
            } else {
                c->renderResolution[0] = 0.0;
                c->renderResolution[1] = 0.0;
                c->customColorTargets = {};
            }
            return true;
        }
    }
    CORE_LOG_W("Failed to set camera render target");
    return false;
}

SCENE_END_NAMESPACE()
