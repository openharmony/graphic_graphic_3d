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

#include "RenderConfigurationETS.h"

namespace OHOS::Render3D {
RenderConfigurationETS::RenderConfigurationETS(
    SCENE_NS::IRenderConfiguration::Ptr rc, const SCENE_NS::IScene::Ptr scene)
    : rc_(rc), scene_(scene)
{}

RenderConfigurationETS::~RenderConfigurationETS()
{}

META_NS::IObject::Ptr RenderConfigurationETS::GetNativeObj() const
{
    if (!rc_) {
        CORE_LOG_E("empty render configuration object");
        return nullptr;
    }
    return interface_pointer_cast<META_NS::IObject>(rc_);
}

std::shared_ptr<UVec2Proxy> RenderConfigurationETS::GetShadowResolution()
{
    if (!rc_) {
        CORE_LOG_E("empty render configuration object");
        return nullptr;
    }
    if (!shadowResolution_) {
        shadowResolution_ = std::make_shared<UVec2Proxy>(rc_->ShadowResolution());
    }
    return shadowResolution_;
}

void RenderConfigurationETS::SetShadowResolution(const BASE_NS::Math::UVec2& res)
{
    if (!rc_) {
        CORE_LOG_E("empty render configuration object");
        return;
    }
    if (!shadowResolution_) {
        shadowResolution_ = std::make_shared<UVec2Proxy>(rc_->ShadowResolution());
    }
    shadowResolution_->SetValue(res);
}

void RenderConfigurationETS::SetSoftShadowConfig(const std::shared_ptr<SoftShadowConfigETS> softShadowConfigETS)
{
    if (!rc_) {
        CORE_LOG_E("empty render configuration object");
        return;
    }

    if (!softShadowConfigETS) {
        CORE_LOG_E("Undefined soft shadow config given, back to hard shadow config.");
        rc_->ShadowType()->SetValue(SCENE_NS::SceneShadowType::PCF);
        softShadowConfigETS_.reset();
        return;
    }
    softShadowConfigETS->SetRenderConfiguration(rc_);
    softShadowConfigETS_ = std::move(softShadowConfigETS);
}
std::shared_ptr<SoftShadowConfigETS> RenderConfigurationETS::GetSoftShadowConfig()
{
    if (!rc_) {
        CORE_LOG_E("empty render configuration object");
        return nullptr;
    }
    return softShadowConfigETS_;
}

}  // namespace OHOS::Render3D