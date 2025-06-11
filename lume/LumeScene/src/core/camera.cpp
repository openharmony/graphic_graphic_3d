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

#include "util.h"

SCENE_BEGIN_NAMESPACE()
bool UpdateCameraRenderTarget(const IEcsObject::Ptr& camera, const IRenderTarget::Ptr& target)
{
    if (auto ecs = GetEcs(camera)) {
        CORE_NS::EntityReference ent;
        BASE_NS::Math::UVec2 size;
        if (target) {
            ent = HandleFromRenderResource(camera->GetScene(), target->GetRenderHandle());
            if (ent) {
                size = target->Size()->GetValue();
            }
        }
        bool changed = false;
        bool success = false;
        if (auto c = GetScopedHandle<const CORE3D_NS::CameraComponent>(camera)) {
            changed = changed || c->renderResolution[0] != size.x;
            changed = changed || c->renderResolution[1] != size.y;
            if ((ent && c->customColorTargets.empty()) ||
                (!c->customColorTargets.empty() && c->customColorTargets[0] != ent)) {
                changed = true;
            }
            success = true;
        }
        if (changed) {
            if (auto c = GetScopedHandle<CORE3D_NS::CameraComponent>(camera)) {
                if (ent) {
                    c->renderResolution[0] = static_cast<float>(size.x);
                    c->renderResolution[1] = static_cast<float>(size.y);
                    c->customColorTargets = { ent };
                } else {
                    c->renderResolution[0] = 0.0;
                    c->renderResolution[1] = 0.0;
                    c->customColorTargets = {};
                }
            }
        }
        return success;
    }
    CORE_LOG_W("Failed to set camera render target");
    return false;
}
SCENE_END_NAMESPACE()
