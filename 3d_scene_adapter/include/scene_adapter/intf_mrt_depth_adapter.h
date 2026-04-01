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
#ifndef OHOS_RENDER_3D_INTF_MRT_DEPTH_ADAPTER_H
#define OHOS_RENDER_3D_INTF_MRT_DEPTH_ADAPTER_H

#include <vector>
#include "intf_camera_utils.h"
#include "intf_scene_adapter.h"

#include <meta/interface/intf_object.h>
#include <base/math/vector.h>

// ecs
#include <scene/ext/intf_ecs_context.h>
#include <core/ecs/intf_ecs.h>

#include <scene/interface/intf_camera.h>

namespace OHOS::Render3D {
class TextureLayer;

// Multi Render Target with depth info
// receives multi windowchangeInfo
class IMrtDepthAdapter {
public:
    virtual void CreateSceneByGltfUri(std::string u);
    virtual bool CreateCamera(const CameraConfigs& p);
    virtual bool SetCameraConfigs(const CameraConfigs& p);

    virtual bool OnWindowChange(const std::vector<WindowChangeInfo>& vWindowChangeInfo);
    virtual bool RenderFrame();

    virtual CORE_NS::IEcs::Ptr GetEcs();
    virtual META_NS::IObject::Ptr GetSceneObj();
    virtual BASE_NS::shared_ptr<SCENE_NS::ICamera> GetCamera();
    virtual void Deinit(bool deinitEngine = false);
    virtual ~IMrtDepthAdapter() = default;
};

BASE_NS::shared_ptr<IMrtDepthAdapter> GetMrtDepthAdapterInstance();
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_INTF_MRT_DEPTH_ADAPTER_H
