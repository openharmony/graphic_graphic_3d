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

#include "LightETS.h"

namespace OHOS::Render3D {
LightETS::LightETS(
    const SCENE_NS::ILight::Ptr light, LightType lightType, const std::string &name, const std::string &uri)
    : NodeETS(NodeETS::NodeType::LIGHT, interface_pointer_cast<SCENE_NS::INode>(light)), light_(light),
      lightType_(lightType)
{
    CORE_LOG_D("LightETS ++");
    SetName(name);
    SetUri(uri);
    SCENE_NS::LightType sceneLightType;
    switch (lightType) {
        case LightETS::LightType::DIRECTIONAL:
            sceneLightType = SCENE_NS::LightType::DIRECTIONAL;
            break;
        case LightETS::LightType::SPOT:
            sceneLightType = SCENE_NS::LightType::SPOT;
            break;
        case LightETS::LightType::POINT:
            sceneLightType = SCENE_NS::LightType::POINT;
            break;
        default:
            sceneLightType = SCENE_NS::LightType::DIRECTIONAL;
            break;
    }
    if (auto light = light_.lock()) {
        light->Type()->SetValue(sceneLightType);
    }
}

LightETS::LightETS(const SCENE_NS::ILight::Ptr light)
    : NodeETS(NodeETS::NodeType::LIGHT, interface_pointer_cast<SCENE_NS::INode>(light)), light_(light)
{
    SCENE_NS::LightType type = light->Type()->GetValue();
    switch (type) {
        case SCENE_NS::LightType::DIRECTIONAL:
            lightType_ = LightType::DIRECTIONAL;
            break;
        case SCENE_NS::LightType::SPOT:
            lightType_ = LightType::SPOT;
            break;
        case SCENE_NS::LightType::POINT:
            lightType_ = LightType::POINT;
            break;
        default:
            lightType_ = LightType::DIRECTIONAL;
            break;
    }
}

LightETS::~LightETS()
{
    CORE_LOG_D("LightETS --");
    light_.reset();
    colorProxy_.reset();
}

LightETS::LightType LightETS::GetLightType()
{
    if (auto light = light_.lock()) {
        return lightType_;
    } else {
        CORE_LOG_E("no light object");
        return lightType_;
    }
}

std::shared_ptr<ColorProxy> LightETS::GetColor()
{
    if (auto light = light_.lock()) {
        if (colorProxy_ == nullptr) {
            colorProxy_ = std::make_shared<ColorProxy>(light->Color());
        }
        return colorProxy_;
    } else {
        CORE_LOG_E("no light object");
        return nullptr;
    }
}

void LightETS::SetColor(const BASE_NS::Color &color)
{
    if (auto light = light_.lock()) {
        if (colorProxy_ == nullptr) {
            colorProxy_ = std::make_shared<ColorProxy>(light->Color());
        }
        colorProxy_->SetValue(color);
    } else {
        CORE_LOG_E("no light object");
    }
}

float LightETS::GetIntensity()
{
    if (auto light = light_.lock()) {
        return light->Intensity()->GetValue();
    } else {
        CORE_LOG_E("no light object");
        return 0.0f;
    }
}

void LightETS::SetIntensity(float intensity)
{
    if (auto light = light_.lock()) {
        light->Intensity()->SetValue(intensity);
    } else {
        CORE_LOG_E("no light object");
    }
}

bool LightETS::GetShadowEnabled()
{
    if (auto light = light_.lock()) {
        return light->ShadowEnabled()->GetValue();
    } else {
        CORE_LOG_E("no light object");
        return false;
    }
}

void LightETS::SetShadowEnabled(bool enabled)
{
    if (auto light = light_.lock()) {
        light->ShadowEnabled()->SetValue(enabled);
    } else {
        CORE_LOG_E("no light object");
    }
}

bool LightETS::GetEnabled()
{
    if (auto light = interface_pointer_cast<SCENE_NS::INode>(light_.lock())) {
        return light->Enabled()->GetValue();
    } else {
        CORE_LOG_E("no light object");
        return false;
    }
}

void LightETS::SetEnabled(bool enable)
{
    if (auto light = interface_pointer_cast<SCENE_NS::INode>(light_.lock())) {
        light->Enabled()->SetValue(enable);
    } else {
        CORE_LOG_E("no light object");
    }
}
}  // namespace OHOS::Render3D