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

#ifndef CAMERA_ETS_H
#define CAMERA_ETS_H

#include <meta/interface/intf_object.h>
#include <scene/interface/intf_raycast.h>
#include <scene/interface/intf_camera.h>

#include <base/containers/pair.h>
#include <base/containers/unordered_map.h>

#include "NodeETS.h"
#include "PostProcessETS.h"
#include "Utils.h"
#include "Vec4Proxy.h"

class CameraETS : public NodeETS {
public:
    static std::shared_ptr<CameraETS> FromJS(
        const SCENE_NS::ICamera::Ptr camera, const std::string &name, const std::string &uri = "");

    struct RaycastResult {
        std::shared_ptr<NodeETS> node;
        float centerDistance;
        BASE_NS::Math::Vec3 hitPosition;
    };
    CameraETS(const SCENE_NS::ICamera::Ptr camera);
    ~CameraETS() override;

    float GetFov();
    void SetFov(const float fov);

    float GetFar();
    void SetFar(const float far);
    float GetNear();
    void SetNear(const float near);

    std::shared_ptr<PostProcessETS> GetPostProcess();
    void SetPostProcess(const std::shared_ptr<PostProcessETS> pp);

    bool GetEnabled();
    void SetEnabled(const bool enabled);

    bool GetMSAA();
    void SetMSAA(const bool msaaEnabled);

    InvokeReturn<std::shared_ptr<Vec4Proxy>> GetClearColor();
    void SetClearColor(const bool enabled, const BASE_NS::Math::Vec4 &color);

    BASE_NS::Math::Vec3 WorldToScreen(const BASE_NS::Math::Vec3 &world);
    BASE_NS::Math::Vec3 ScreenToWorld(const BASE_NS::Math::Vec3 &screen);

    InvokeReturn<std::vector<CameraETS::RaycastResult>> Raycast(const BASE_NS::Math::Vec2 &position,
        const std::shared_ptr<NodeETS> rootNode = nullptr, const std::shared_ptr<NodeETS> layerMask = nullptr);

private:
    SCENE_NS::ICamera::WeakPtr camera_{nullptr};
    bool msaaEnabled_{false};
    bool clearColorEnabled_{false};
    std::shared_ptr<Vec4Proxy> clearColorProxy_{nullptr};
    std::shared_ptr<PostProcessETS> postProcess_{nullptr};
};
#endif  // CAMERA_ETS_H