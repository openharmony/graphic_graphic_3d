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

#ifndef SCENE_SRC_CORE_CAMERA_H
#define SCENE_SRC_CORE_CAMERA_H

#include <scene/ext/intf_ecs_object.h>
#include <scene/interface/intf_raycast.h>
#include <scene/interface/intf_render_target.h>

SCENE_BEGIN_NAMESPACE()

bool EnableCamera(const IEcsObject::Ptr& camera, bool enabled);
bool UpdateCameraRenderTarget(const IEcsObject::Ptr& camera, const IRenderTarget::Ptr& target);

SCENE_END_NAMESPACE()

#endif