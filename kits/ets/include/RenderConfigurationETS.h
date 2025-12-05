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

#ifndef OHOS_3D_RENDER_CONFIGURATION_ETS_H
#define OHOS_3D_RENDER_CONFIGURATION_ETS_H

#include <string>
#include <scene/interface/intf_scene.h>
#include "Vec2Proxy.h"

namespace OHOS::Render3D {
class RenderConfigurationETS {
public:

    RenderConfigurationETS(SCENE_NS::IRenderConfiguration::Ptr rc, const SCENE_NS::IScene::Ptr scene);

    SCENE_NS::IScene::Ptr GetScene() const
    {
        return scene_.lock();
    }

    ~RenderConfigurationETS();

    META_NS::IObject::Ptr GetNativeObj() const;

    std::shared_ptr<UVec2Proxy> GetShadowResolution();
    void SetShadowResolution(const BASE_NS::Math::UVec2 &res);

private:
    SCENE_NS::IRenderConfiguration::Ptr rc_{nullptr};
    SCENE_NS::IScene::WeakPtr scene_{nullptr};
    std::shared_ptr<UVec2Proxy> shadowResolution_{nullptr};
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_RENDERCONFIGURATION_ETS_H
