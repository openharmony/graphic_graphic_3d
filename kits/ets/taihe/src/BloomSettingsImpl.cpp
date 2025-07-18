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

#include "BloomSettingsImpl.h"

std::shared_ptr<BloomETS> BloomSettingsImpl::CreateInternal(const ScenePostProcessSettings::BloomSettings &data)
{
    float thresholdHard, thresholdSoft, scaleFactor, scatter;
    if (data->getThresholdHard().has_value()) {
        thresholdHard = data->getThresholdHard().value();
    } else {
        thresholdHard = 0.0F;
    }
    if (data->getThresholdSoft().has_value()) {
        thresholdSoft = data->getThresholdSoft().value();
    } else {
        thresholdSoft = 0.0F;
    }
    if (data->getScaleFactor().has_value()) {
        scaleFactor = data->getScaleFactor().value();
    } else {
        scaleFactor = 0.0F;
    }
    if (data->getScatter().has_value()) {
        scatter = data->getScatter().value();
    } else {
        scatter = 0.0F;
    }
    WIDGET_LOGI("BloomSettingsImpl::CreateInternal, {\"thresholdHard\": %{public}f, \"thresholdSoft\": %{public}f, "
                "\"scaleFactor\": %{public}f, \"scatter\": %{public}f }",
        thresholdHard,
        thresholdSoft,
        scaleFactor,
        scatter);
    return BloomETS::FromJS(thresholdHard, thresholdSoft, scaleFactor, scatter);
}

BloomSettingsImpl::BloomSettingsImpl(const std::shared_ptr<BloomETS> bloomETS) : bloomETS_(bloomETS)
{}

BloomSettingsImpl::~BloomSettingsImpl()
{
    bloomETS_.reset();
}

::taihe::optional<double> BloomSettingsImpl::getThresholdHard()
{
    if (bloomETS_) {
        return taihe::optional<double>(std::in_place, bloomETS_->GetThresholdHard());
    } else {
        return taihe::optional<double>(std::in_place, 0.0);
    }
}

void BloomSettingsImpl::setThresholdHard(::taihe::optional_view<double> thresholdHard)
{
    if (bloomETS_) {
        if (thresholdHard.has_value()) {
            bloomETS_->SetThresholdHard(thresholdHard.value());
        } else {
            bloomETS_->SetThresholdHard(0.0F);
        }
    }
}

::taihe::optional<double> BloomSettingsImpl::getThresholdSoft()
{
    if (bloomETS_) {
        return taihe::optional<double>(std::in_place, bloomETS_->GetThresholdSoft());
    } else {
        return taihe::optional<double>(std::in_place, 0.0);
    }
}

void BloomSettingsImpl::setThresholdSoft(::taihe::optional_view<double> thresholdSoft)
{
    if (bloomETS_) {
        if (thresholdSoft.has_value()) {
            bloomETS_->SetThresholdSoft(thresholdSoft.value());
        } else {
            bloomETS_->SetThresholdSoft(0.0F);
        }
    }
}

::taihe::optional<double> BloomSettingsImpl::getScaleFactor()
{
    if (bloomETS_) {
        return taihe::optional<double>(std::in_place, bloomETS_->GetScaleFactor());
    } else {
        return taihe::optional<double>(std::in_place, 0.0);
    }
}

void BloomSettingsImpl::setScaleFactor(::taihe::optional_view<double> scaleFactor)
{
    if (bloomETS_) {
        if (scaleFactor.has_value()) {
            bloomETS_->SetScaleFactor(scaleFactor.value());
        } else {
            bloomETS_->SetScaleFactor(0.0F);
        }
    }
}

::taihe::optional<double> BloomSettingsImpl::getScatter()
{
    if (bloomETS_) {
        return taihe::optional<double>(std::in_place, bloomETS_->GetScatter());
    } else {
        return taihe::optional<double>(std::in_place, 0.0);
    }
}

void BloomSettingsImpl::setScatter(::taihe::optional_view<double> scatter)
{
    if (bloomETS_) {
        if (scatter.has_value()) {
            bloomETS_->SetScatter(scatter.value());
        } else {
            bloomETS_->SetScatter(0.0F);
        }
    }
}