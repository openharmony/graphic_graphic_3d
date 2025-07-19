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

#include "ToneMappingSettingsImpl.h"

std::shared_ptr<TonemapETS> ToneMappingSettingsImpl::CreateInternal(
    const ScenePostProcessSettings::ToneMappingSettings &data)
{
    TonemapETS::ToneMappingType type;
    if (data->getType().has_value()) {
        type = static_cast<TonemapETS::ToneMappingType>(data->getType().value().get_value());
    } else {
        type = TonemapETS::ToneMappingType::ACES;
    }
    float exposure;
    if (data->getExposure().has_value()) {
        exposure = data->getExposure().value();
    } else {
        exposure = 0.0F;
    }
    WIDGET_LOGI("ToneMappingSettingsImpl::CreateInternal, {\"type\": %{public}d, \"exposure\": %{public}f}",
        static_cast<int32_t>(type),
        exposure);
    return TonemapETS::FromJS(type, exposure);
}

ToneMappingSettingsImpl::ToneMappingSettingsImpl(const std::shared_ptr<TonemapETS> tonemapETS) : tonemapETS_(tonemapETS)
{
    WIDGET_LOGI("ToneMappingSettingsImpl ++");
}

ToneMappingSettingsImpl::~ToneMappingSettingsImpl()
{
    WIDGET_LOGI("ToneMappingSettingsImpl --");
    tonemapETS_.reset();
}

::taihe::optional<::ScenePostProcessSettings::ToneMappingType> ToneMappingSettingsImpl::getType()
{
    ScenePostProcessSettings::ToneMappingType type = ScenePostProcessSettings::ToneMappingType::key_t::ACES;
    if (tonemapETS_) {
        type = ScenePostProcessSettings::ToneMappingType::from_value(static_cast<int32_t>(tonemapETS_->GetType()));
    }
    return taihe::optional<ScenePostProcessSettings::ToneMappingType>(std::in_place, type);
}

void ToneMappingSettingsImpl::setType(::taihe::optional_view<::ScenePostProcessSettings::ToneMappingType> type)
{
    if (tonemapETS_) {
        ScenePostProcessSettings::ToneMappingType typeV = ScenePostProcessSettings::ToneMappingType::key_t::ACES;
        if (type.has_value()) {
            typeV = type.value();
        }
        tonemapETS_->SetType((TonemapETS::ToneMappingType)typeV.get_value());
    }
}

::taihe::optional<double> ToneMappingSettingsImpl::getExposure()
{
    if (tonemapETS_) {
        return taihe::optional<double>(std::in_place, tonemapETS_->GetExposure());
    } else {
        return taihe::optional<double>(std::in_place, 0.0F);
    }
}

void ToneMappingSettingsImpl::setExposure(::taihe::optional_view<double> exposure)
{
    if (tonemapETS_) {
        if (exposure.has_value()) {
            tonemapETS_->SetExposure(exposure.value());
        } else {
            tonemapETS_->SetExposure(0.0F);
        }
    }
}
