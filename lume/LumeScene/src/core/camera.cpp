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

#include "camera.h"

#include <scene/ext/intf_render_resource.h>

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/intf_gpu_resource_manager.h>

#include "../util.h"

SCENE_BEGIN_NAMESPACE()

bool EnableCamera(const IEcsObject::Ptr& camera, bool enabled)
{
    if (auto c = GetScopedHandle<CORE3D_NS::CameraComponent>(camera)) {
        if (enabled) {
            c->sceneFlags |= CORE3D_NS::CameraComponent::ACTIVE_RENDER_BIT;
        } else {
            c->sceneFlags &=
                ~(CORE3D_NS::CameraComponent::ACTIVE_RENDER_BIT | CORE3D_NS::CameraComponent::MAIN_CAMERA_BIT);
        }
        return true;
    }
    return false;
}

bool UpdateCameraRenderTarget(const IEcsObject::Ptr& camera, const IRenderTarget::Ptr& target)
{
    if (auto ecs = GetEcs(camera)) {
        CORE_NS::EntityReference ent;
        if (auto resource = interface_cast<IEcsResource>(target)) {
            ent = ecs->GetEntityReference(resource->GetEntity());
        } else if (target) {
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
