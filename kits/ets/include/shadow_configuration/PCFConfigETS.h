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

#ifndef SHADOW_CONFIGURATION_PCF_CONFIG_ETS_H
#define SHADOW_CONFIGURATION_PCF_CONFIG_ETS_H

#include <memory>
#include <optional>
#include "shadow_configuration/ShadowConfiguration.h"

namespace OHOS::Render3D::ShadowConfiguration {

class PCFConfigETS : public SoftShadowConfigETS {
public:
    ~PCFConfigETS() override = default;
    ShadowAlgorithmType GetType() override;

    SCENE_NS::IRenderConfiguration::Ptr GetRenderConfiguration() const override;
    void SetRenderConfiguration(SCENE_NS::IRenderConfiguration::Ptr rc) override;

    static constexpr float DEFAULT_RADIUS = 5.0f;
    static constexpr int32_t DEFAULT_COUNT = 16;

    void SetDefaultShadowSampleRadius()
    {
        SetRadius(DEFAULT_RADIUS);
    }
    void SetDefaultShadowSampleCount()
    {
        SetCount(DEFAULT_COUNT);
    }

    static std::shared_ptr<PCFConfigETS> Create(
        std::optional<float> radiusOpt = std::nullopt, std::optional<int32_t> countOpt = std::nullopt)
    {
        float radius = radiusOpt.value_or(DEFAULT_RADIUS);
        int32_t count = countOpt.value_or(DEFAULT_COUNT);

        if (!std::isfinite(radius) || radius < 0.0f || count < 0) {
            CORE_LOG_E("Unable to create PCFConfigETS: Invalid shadowSampleRadius or shadowSampleCount given");
            return nullptr;
        }
        return std::shared_ptr<PCFConfigETS>(new PCFConfigETS(radius, count));
    }

    int32_t GetCount();
    void SetCount(int32_t shadowSampleCount);

    float GetRadius();
    void SetRadius(float shadowSampleRadius);

private:
    float radius_;
    int32_t count_;
    explicit PCFConfigETS(float radius, int32_t count);
};

}  // namespace OHOS::Render3D::ShadowConfiguration
#endif  // SHADOW_CONFIGURATION_PCF_CONFIG_ETS_H