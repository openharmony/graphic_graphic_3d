/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_3D_CAMERA_IMPL_H
#define OHOS_3D_CAMERA_IMPL_H

#include "SceneNodes.proj.hpp"
#include "SceneNodes.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include "CameraETS.h"
#include "BloomETS.h"
#include "TonemapETS.h"
#include "PostProcessETS.h"

#include "NodeImpl.h"
#include "PostProcessSettingsImpl.h"

class CameraImpl : public NodeImpl {
public:
    CameraImpl(const std::shared_ptr<CameraETS> cameraETS);

    ~CameraImpl();

    double getFov();
    void setFov(double fov);

    double getNearPlane();
    void setNearPlane(double nearPlane);

    double getFarPlane();
    void setFarPlane(double farPlane);

    bool getEnabled();
    void setEnabled(bool enabled);

    ::SceneNodes::PostProcessSettingsOrNull getPostProcess();
    void setPostProcess(::SceneNodes::PostProcessSettingsOrNull const &process);

    ::SceneNodes::ColorOrNull getClearColor();
    void setClearColor(::SceneNodes::ColorOrNull const &color);

    ::taihe::array<::SceneTH::RaycastResult> raycastSync(
        ::SceneTypes::weak::Vec2 viewPosition, ::SceneTH::RaycastParameters const &params);

private:
    std::shared_ptr<CameraETS> cameraETS_;
};

#endif  // OHOS_3D_CAMERA_IMPL_H