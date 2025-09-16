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

#include "LightImpl.h"
#include "ColorImpl.h"

namespace OHOS::Render3D::KITETS {
LightImpl::LightImpl(const std::shared_ptr<LightETS> lightETS) : NodeImpl(lightETS), lightETS_(lightETS)
{
    WIDGET_LOGI("LightImpl ++");
}

SpotLightImpl::SpotLightImpl(const std::shared_ptr<LightETS> lightETS) : LightImpl(lightETS)
{}

DirectionalLightImpl::DirectionalLightImpl(const std::shared_ptr<LightETS> lightETS) : LightImpl(lightETS)
{}

::SceneNodes::LightType LightImpl::getLightType()
{
    if (!lightETS_) {
        WIDGET_LOGE("Invalid light");
        return SceneNodes::LightType(SceneNodes::LightType::key_t::DIRECTIONAL);
    }
    return SceneNodes::LightType::from_value(lightETS_->GetLightType());
}

::SceneTypes::Color LightImpl::getColor()
{
    if (!lightETS_) {
        WIDGET_LOGE("Invalid light");
        return SceneTypes::Color({nullptr, nullptr});
    }
    auto color = lightETS_->GetColor();
    if (color) {
        return taihe::make_holder<ColorImpl, SceneTypes::Color>(color);
    } else {
        WIDGET_LOGE("lightETS_ GetColor fail");
        return SceneTypes::Color({nullptr, nullptr});
    }
}

void LightImpl::setColor(::SceneTypes::weak::Color color)
{
    if (!lightETS_) {
        WIDGET_LOGE("Invalid light");
    }
    BASE_NS::Color colorVec{color->getR(), color->getG(), color->getB(), color->getA()};
    lightETS_->SetColor(colorVec);
}

double LightImpl::getIntensity()
{
    if (!lightETS_) {
        WIDGET_LOGE("Invalid light");
        return 0.0f;
    }
    return lightETS_->GetIntensity();
}

void LightImpl::setIntensity(double intensity)
{
    if (!lightETS_) {
        WIDGET_LOGE("Invalid light");
    }
    lightETS_->SetIntensity(intensity);
}

bool LightImpl::getShadowEnabled()
{
    if (!lightETS_) {
        WIDGET_LOGE("Invalid light");
        return false;
    }
    return lightETS_->GetShadowEnabled();
}

void LightImpl::setShadowEnabled(bool enabled)
{
    if (!lightETS_) {
        WIDGET_LOGE("Invalid light");
    }
    lightETS_->SetShadowEnabled(enabled);
}

bool LightImpl::getEnabled()
{
    if (!lightETS_) {
        WIDGET_LOGE("Invalid light");
        return false;
    }
    return lightETS_->GetEnabled();
}

void LightImpl::setEnabled(bool enable)
{
    if (!lightETS_) {
        WIDGET_LOGE("Invalid light");
    }
    lightETS_->SetEnabled(enable);
}
}  // namespace OHOS::Render3D::KITETS