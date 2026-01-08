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

#include "PostProcessSettingsImpl.h"

namespace OHOS::Render3D::KITETS {
std::shared_ptr<PostProcessETS> PostProcessSettingsImpl::CreateInternal(
    const ScenePostProcessSettings::ToneMappingSettings &tonemapData,
    const ScenePostProcessSettings::BloomSettings &bloomData,
    const ScenePostProcessSettings::VignetteSettings &vignetteData,
    const ScenePostProcessSettings::ColorFringeSettings &colorFringeData)
{
    return std::make_shared<PostProcessETS>(
        ToneMappingSettingsImpl::CreateInternal(tonemapData), BloomSettingsImpl::CreateInternal(bloomData),
        VignetteSettingsImpl::CreateInternal(vignetteData), ColorFringeSettingsImpl::CreateInternal(colorFringeData));
}

PostProcessSettingsImpl::PostProcessSettingsImpl(const std::shared_ptr<PostProcessETS> postProcessETS)
    : postProcessETS_(postProcessETS)
{}

PostProcessSettingsImpl::~PostProcessSettingsImpl()
{
    if (postProcessETS_) {
        postProcessETS_.reset();
    }
}

::taihe::optional<::ScenePostProcessSettings::ToneMappingSettings> PostProcessSettingsImpl::getToneMapping()
{
    if (postProcessETS_) {
        auto tm = taihe::make_holder<ToneMappingSettingsImpl, ScenePostProcessSettings::ToneMappingSettings>(
            postProcessETS_->GetToneMapping());
        return taihe::optional<::ScenePostProcessSettings::ToneMappingSettings>(std::in_place, tm);
    } else {
        return std::nullopt;
    }
}

void PostProcessSettingsImpl::setToneMapping(
    ::taihe::optional_view<::ScenePostProcessSettings::ToneMappingSettings> toneMapping)
{
    if (!postProcessETS_) {
        return;
    }
    if (toneMapping.has_value()) {
        ScenePostProcessSettings::ToneMappingSettings tonemapValue = toneMapping.value();
        postProcessETS_->SetToneMapping(ToneMappingSettingsImpl::CreateInternal(tonemapValue));
    } else {
        postProcessETS_->SetToneMapping(nullptr);
    }
}

::taihe::optional<::ScenePostProcessSettings::BloomSettings> PostProcessSettingsImpl::getBloom()
{
    if (postProcessETS_) {
        auto bloom =
            taihe::make_holder<BloomSettingsImpl, ScenePostProcessSettings::BloomSettings>(postProcessETS_->GetBloom());
        return taihe::optional<::ScenePostProcessSettings::BloomSettings>(std::in_place, bloom);
    } else {
        return std::nullopt;
    }
}

void PostProcessSettingsImpl::setBloom(::taihe::optional_view<::ScenePostProcessSettings::BloomSettings> bloom)
{
    if (!postProcessETS_) {
        return;
    }
    if (bloom.has_value()) {
        ScenePostProcessSettings::BloomSettings bloomValue = bloom.value();
        postProcessETS_->SetBloom(BloomSettingsImpl::CreateInternal(bloomValue));
    } else {
        postProcessETS_->SetBloom(nullptr);
    }
}

::taihe::optional<::ScenePostProcessSettings::VignetteSettings> PostProcessSettingsImpl::getVignette()
{
    if (postProcessETS_) {
        auto vignette = taihe::make_holder<VignetteSettingsImpl, ScenePostProcessSettings::VignetteSettings>(
            postProcessETS_->GetVignette());
        return taihe::optional<::ScenePostProcessSettings::VignetteSettings>(std::in_place, vignette);
    } else {
        return std::nullopt;
    }
}

void PostProcessSettingsImpl::setVignette(::taihe::optional_view<::ScenePostProcessSettings::VignetteSettings> vignette)
{
    if (!postProcessETS_) {
        return;
    }
    if (vignette.has_value()) {
        ScenePostProcessSettings::VignetteSettings vignetteValue = vignette.value();
        postProcessETS_->SetVignette(VignetteSettingsImpl::CreateInternal(vignetteValue));
    } else {
        postProcessETS_->SetVignette(nullptr);
    }
}

::taihe::optional<::ScenePostProcessSettings::ColorFringeSettings> PostProcessSettingsImpl::getColorFringe()
{
    if (postProcessETS_) {
        auto colorFringe = taihe::make_holder<ColorFringeSettingsImpl, ScenePostProcessSettings::ColorFringeSettings>(
            postProcessETS_->GetColorFringe());
        return taihe::optional<::ScenePostProcessSettings::ColorFringeSettings>(std::in_place, colorFringe);
    } else {
        return std::nullopt;
    }
}

void PostProcessSettingsImpl::setColorFringe(
    ::taihe::optional_view<::ScenePostProcessSettings::ColorFringeSettings> colorFringe)
{
    if (!postProcessETS_) {
        return;
    }
    if (colorFringe.has_value()) {
        ScenePostProcessSettings::ColorFringeSettings colorFringeValue = colorFringe.value();
        postProcessETS_->SetColorFringe(ColorFringeSettingsImpl::CreateInternal(colorFringeValue));
    } else {
        postProcessETS_->SetColorFringe(nullptr);
    }
}
}  // namespace OHOS::Render3D::KITETS