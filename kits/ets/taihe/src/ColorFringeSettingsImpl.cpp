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

#include "ColorFringeSettingsImpl.h"

namespace OHOS::Render3D::KITETS {
std::shared_ptr<ColorFringeETS> ColorFringeSettingsImpl::CreateInternal(
    const ScenePostProcessSettings::ColorFringeSettings &data)
{
    float intensity;
    if (data->getIntensity().has_value()) {
        intensity = ColorFringeETS::ScaleToNativeForIntensity(data->getIntensity().value());
    } else {
        intensity = ColorFringeETS::DEFAULT_INTENSITY;
    }
    return std::make_shared<ColorFringeETS>(intensity);
}

ColorFringeSettingsImpl::ColorFringeSettingsImpl(const std::shared_ptr<ColorFringeETS> colorFringeETS)
    : colorFringeETS_(colorFringeETS)
{}

ColorFringeSettingsImpl::~ColorFringeSettingsImpl()
{
    colorFringeETS_.reset();
}

::taihe::optional<double> ColorFringeSettingsImpl::getIntensity()
{
    if (colorFringeETS_) {
        return taihe::optional<double>(std::in_place, colorFringeETS_->GetIntensity());
    } else {
        return taihe::optional<double>(std::in_place, 0.0);
    }
}

void ColorFringeSettingsImpl::setIntensity(::taihe::optional_view<double> intensity)
{
    if (colorFringeETS_) {
        if (intensity.has_value()) {
            colorFringeETS_->SetIntensity(intensity.value());
        } else {
            colorFringeETS_->SetIntensity(0.0F);
        }
    }
}
}  // namespace OHOS::Render3D::KITETS