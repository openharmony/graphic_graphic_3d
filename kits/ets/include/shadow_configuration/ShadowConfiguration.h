/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef SHADOW_CONFIGURATION_SOFT_SHADOW_CONFIG_ETS_H
#define SHADOW_CONFIGURATION_SOFT_SHADOW_CONFIG_ETS_H

#include <scene/interface/intf_scene.h>

namespace OHOS::Render3D::ShadowConfiguration {
enum class ShadowAlgorithmType : uint32_t { PCF = 0 };

class SoftShadowConfigETS {
public:
    virtual ~SoftShadowConfigETS() = default;
    virtual ShadowAlgorithmType GetType() = 0;

    virtual SCENE_NS::IRenderConfiguration::Ptr GetRenderConfiguration() const = 0;
    virtual void SetRenderConfiguration(SCENE_NS::IRenderConfiguration::Ptr rc) = 0;

protected:
    SoftShadowConfigETS() = default;
    SCENE_NS::IRenderConfiguration::WeakPtr renderConfiguration_;
};

}  // namespace OHOS::Render3D::ShadowConfiguration

#endif