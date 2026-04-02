/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <string>
#include "intf_camera_utils.h"
#include "3d_widget_adapter_log.h"

namespace OHOS::Render3D {
std::string CameraConfigs::Dump()
{
    std::string ret = "Camera:[";
    ret += " position_: " + std::to_string(position_.x) + '\t' + std::to_string(position_.y) + '\t' +
        std::to_string(position_.z) + '\t';
    ret += " rotation_: " + std::to_string(rotation_.x) + '\t' + std::to_string(rotation_.y) + '\t' +
            std::to_string(rotation_.z) + '\t' + std::to_string(rotation_.w) + '\t';
    ret += "clearColor: " + std::to_string(clearColor_.x) + '\t' + std::to_string(clearColor_.y) + '\t' +
        std::to_string(clearColor_.z) + '\t' + std::to_string(clearColor_.w) + '\t';
    ret += "fov: " + std::to_string(intrinsics_.fov_) + "near: " + std::to_string(intrinsics_.near_) +
        "far: " + std::to_string(intrinsics_.far_);
    ret += "]";
    return ret;
}

} // namespace OHOS::Render3D