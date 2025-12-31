/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef OHOS_RENDER_3D_INTF_OFFSCREEN_SCENE_H
#define OHOS_RENDER_3D_INTF_OFFSCREEN_SCENE_H

#include "intf_scene_adapter.h"
#include <meta/interface/intf_object.h>
#include <base/math/vector.h>

// ecs
#include <scene/ext/intf_ecs_context.h>
#include <core/ecs/intf_ecs.h>

#include <scene/interface/intf_camera.h>

namespace OHOS::Render3D {

using Vector4f = Base::Math::Vec4;
using Vector3f = Base::Math::Vec3;

struct OffscreenCameraIntrinsics {
    OffscreenCameraIntrinsics(float fov = 1, float near = 0.1, float far = 10) :
                              fov_(fov), near_(near), far_(far) {}
    float fov_ = 1;
    float near_ = 0.1;
    float far_ = 10;
};

struct OffscreenCameraConfigs {
    // camera pose
    Vector3f position_ = {0, 0, 0};
    Vector4f rotation_ = {0, 0, 0, 1};

    OffscreenCameraIntrinsics intrinsics_;
    Vector4f clearColor_ = {0, 0, 0, 1}; // RGBA = black opaque

    std::string Dump() const;
};
class IOffScreenScene {
public:
    virtual bool OnWindowChange(const WindowChangeInfo& windowChangeInfo) = 0;
    virtual void Deinit(bool deinitEngine = false) = 0;
    virtual bool RenderFrame() = 0;

    virtual bool CreateCamera(const OffscreenCameraConfigs& p) = 0;
    virtual bool SetCameraConfigs(const OffscreenCameraConfigs& p) = 0;

    virtual bool LoadPluginByUid(const std::string& uid) = 0;
    virtual META_NS::IObject::Ptr GetSceneObj() = 0;
    virtual bool EngineTickFrame(CORE_NS::IEcs::Ptr ecs) = 0;

    virtual ~IOffScreenScene() = default;
    virtual CORE_NS::IEcs::Ptr GetEcs() = 0;
    virtual BASE_NS::shared_ptr<SCENE_NS::ICamera> GetCamera() = 0;
};

BASE_NS::shared_ptr<IOffScreenScene> GetOffscreenSceneInstance();
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_INTF_OFFSCREEN_SCENE_H
