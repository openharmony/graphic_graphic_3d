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

#ifndef OHOS_3D_LIGHT_ETS_H
#define OHOS_3D_LIGHT_ETS_H

#include <meta/interface/intf_object.h>
#include <scene/interface/intf_light.h>

#include "ColorProxy.h"
#include "NodeETS.h"
#include "Utils.h"

namespace OHOS::Render3D {
class LightETS : public NodeETS {
public:
    enum LightType {
        /**
         * Directional light.
         */
        DIRECTIONAL = 1,
        /**
         * Spotlight.
         */
        SPOT = 2,
        /**
         * Point light.
         */
        POINT = 3,
    };

    LightETS(
        const SCENE_NS::ILight::Ptr light, LightType lightType, const std::string &name, const std::string &uri = "");
    // construct from existed light
    LightETS(const SCENE_NS::ILight::Ptr light);
    ~LightETS() override;

    LightType GetLightType();

    std::shared_ptr<ColorProxy> GetColor();
    void SetColor(const BASE_NS::Color &color);

    float GetIntensity();
    void SetIntensity(float intensity);

    bool GetShadowEnabled();
    void SetShadowEnabled(bool enabled);

    bool GetEnabled();
    void SetEnabled(bool enable);

    float GetInnerAngle();
    void SetInnerAngle(float innerAngle);

    float GetOuterAngle();
    void SetOuterAngle(float outerAngle);

    float GetRange();
    void SetRange(float range);

private:
    SCENE_NS::ILight::WeakPtr light_{nullptr};
    std::shared_ptr<ColorProxy> colorProxy_{nullptr};
    LightType lightType_{LightType::DIRECTIONAL};
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_LIGHT_ETS_H
