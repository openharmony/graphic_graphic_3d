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

bool CompareTargets(
    BASE_NS::array_view<const CORE_NS::EntityReference> a1, BASE_NS::array_view<const CORE_NS::EntityReference> a2)
{
    if (!a1.empty() && !a2.empty()) {
        // both not empty, check the first elements (because we only every set one custom color target)
        return a1[0] == a2[0];
    }
    // Both empty -> are the same, size different -> not the same.
    return a1.size() == a2.size();
}

bool UpdateCameraRenderTarget(const IEcsObject::Ptr& camera, const IRenderTarget::Ptr& target)
{
    if (auto ecs = GetEcs(camera)) {
        CORE_NS::EntityReference ent;
        BASE_NS::Math::UVec2 size;
        BASE_NS::vector<CORE_NS::EntityReference> targets;
        if (target) {
            if (ent = HandleFromRenderResource(camera->GetScene(), target->GetRenderHandle()); ent) {
                size = META_NS::GetValue(target->Size());
                targets = { ent };
            }
        }
        // First read handle
        if (auto rh = GetScopedHandle<const CORE3D_NS::CameraComponent>(camera)) {
            if (rh->renderResolution != size || !CompareTargets(targets, rh->customColorTargets)) {
                // There are changes, take write lock and update
                if (auto wh = GetScopedHandle<CORE3D_NS::CameraComponent>(camera)) {
                    wh->renderResolution[0] = size.x;
                    wh->renderResolution[1] = size.y;
                    wh->customColorTargets.swap(targets);
                }
            }
            return true;
        }
    }
    CORE_LOG_W("Failed to set camera render target");
    return false;
}

SCENE_END_NAMESPACE()
