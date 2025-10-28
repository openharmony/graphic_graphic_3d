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

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include "VignetteSettingsImpl.h"

namespace OHOS::Render3D::KITETS {
std::shared_ptr<VignetteETS> VignetteSettingsImpl::CreateInternal(
    const ScenePostProcessSettings::VignetteSettings &data)
{
    float roundness;
    if (data->getRoundness().has_value()) {
        roundness = VignetteETS::ScaleToNativeForRoundness(data->getRoundness().value());
    } else {
        roundness = VignetteETS::DEFAULT_ROUNDNESS;
    }

    float intensity;
    if (data->getIntensity().has_value()) {
        intensity = data->getIntensity().value();
    } else {
        intensity = VignetteETS::DEFAULT_INTENSITY;
    }
    return std::make_shared<VignetteETS>(roundness, intensity);
}

VignetteSettingsImpl::VignetteSettingsImpl(const std::shared_ptr<VignetteETS> vignetteETS) : vignetteETS_(vignetteETS)
{}

VignetteSettingsImpl::~VignetteSettingsImpl()
{
    vignetteETS_.reset();
}

::taihe::optional<double> VignetteSettingsImpl::getRoundness()
{
    if (vignetteETS_) {
        return taihe::optional<double>(std::in_place, vignetteETS_->GetRoundness());
    } else {
        return taihe::optional<double>(std::in_place, 0.0);
    }
}

void VignetteSettingsImpl::setRoundness(::taihe::optional_view<double> roundness)
{
    if (vignetteETS_) {
        if (roundness.has_value()) {
            vignetteETS_->SetRoundness(roundness.value());
        } else {
            vignetteETS_->SetRoundness(0.0F);
        }
    }
}

::taihe::optional<double> VignetteSettingsImpl::getIntensity()
{
    if (vignetteETS_) {
        return taihe::optional<double>(std::in_place, vignetteETS_->GetIntensity());
    } else {
        return taihe::optional<double>(std::in_place, 0.0);
    }
}

void VignetteSettingsImpl::setIntensity(::taihe::optional_view<double> intensity)
{
    if (vignetteETS_) {
        if (intensity.has_value()) {
            vignetteETS_->SetIntensity(intensity.value());
        } else {
            vignetteETS_->SetIntensity(0.0F);
        }
    }
}
}  // namespace OHOS::Render3D::KITETS
