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
#include "shadow_configuration/PCFConfigETS.h"

namespace OHOS::Render3D::ShadowConfiguration {

PCFConfigETS::PCFConfigETS(float radius, int32_t count) : SoftShadowConfigETS(), radius_(radius), count_(count)
{}

ShadowAlgorithmType PCFConfigETS::GetType()
{
    return ShadowAlgorithmType::PCF;
}

SCENE_NS::IRenderConfiguration::Ptr PCFConfigETS::GetRenderConfiguration() const
{
    return renderConfiguration_.lock();
}

void PCFConfigETS::SetRenderConfiguration(SCENE_NS::IRenderConfiguration::Ptr rc)
{
    if (!rc) {
        CORE_LOG_E("no rc return it");
        renderConfiguration_.reset();
        return;
    }

    rc->ShadowType()->SetValue(SCENE_NS::SceneShadowType::VARIABLE_PCF);
    rc->VariablePcfRadius()->SetValue(radius_);
    rc->VariablePcfSampleCount()->SetValue(count_);

    renderConfiguration_ = rc;
}

int32_t PCFConfigETS::GetCount()
{
    return count_;
}

void PCFConfigETS::SetCount(int32_t shadowSampleCount)
{
    if (shadowSampleCount < 0) {
        CORE_LOG_E("Invalid shadowSampleCount given.");
        return;
    }
    count_ = shadowSampleCount;

    auto rc = GetRenderConfiguration();
    if (!rc) {
        CORE_LOG_E("no rc return it");
        return;
    } else {
        rc->VariablePcfSampleCount()->SetValue(count_);
    }
}

float PCFConfigETS::GetRadius()
{
    return radius_;
}

void PCFConfigETS::SetRadius(float shadowSampleRadius)
{
    if (!std::isfinite(shadowSampleRadius) || shadowSampleRadius < 0.0f) {
        CORE_LOG_E("Invalid shadowSampleRadius given.");
        return;
    }
    radius_ = shadowSampleRadius;

    auto rc = GetRenderConfiguration();
    if (!rc) {
        CORE_LOG_E("no rc return it");
        return;
    } else {
        rc->VariablePcfRadius()->SetValue(radius_);
    }
}

}  // namespace OHOS::Render3D::ShadowConfiguration