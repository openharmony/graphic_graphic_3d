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

#ifndef OHOS_3D_POST_PROCESS_SETTINGS_IMPL_H
#define OHOS_3D_POST_PROCESS_SETTINGS_IMPL_H

#include "ScenePostProcessSettings.proj.hpp"
#include "ScenePostProcessSettings.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include "PostProcessETS.h"
#include "ToneMappingSettingsImpl.h"
#include "BloomSettingsImpl.h"
#include "VignetteSettingsImpl.h"
#include "ColorFringeSettingsImpl.h"

namespace OHOS::Render3D::KITETS {
class PostProcessSettingsImpl {
public:
    static std::shared_ptr<PostProcessETS> CreateInternal(
        const ScenePostProcessSettings::ToneMappingSettings &tonemapData,
        const ScenePostProcessSettings::BloomSettings &bloomData,
        const ScenePostProcessSettings::VignetteSettings &vignetteData,
        const ScenePostProcessSettings::ColorFringeSettings &colorFringeData);

    PostProcessSettingsImpl(const std::shared_ptr<PostProcessETS> postProcessETS);
    ~PostProcessSettingsImpl();
    ::taihe::optional<::ScenePostProcessSettings::ToneMappingSettings> getToneMapping();
    void setToneMapping(::taihe::optional_view<::ScenePostProcessSettings::ToneMappingSettings> toneMapping);
    ::taihe::optional<::ScenePostProcessSettings::BloomSettings> getBloom();
    void setBloom(::taihe::optional_view<::ScenePostProcessSettings::BloomSettings> bloom);
    ::taihe::optional<::ScenePostProcessSettings::VignetteSettings> getVignette();
    void setVignette(::taihe::optional_view<::ScenePostProcessSettings::VignetteSettings> vignette);
    ::taihe::optional<::ScenePostProcessSettings::ColorFringeSettings> getColorFringe();
    void setColorFringe(::taihe::optional_view<::ScenePostProcessSettings::ColorFringeSettings> colorFringe);

    std::shared_ptr<PostProcessETS> GetInternalSettings() const
    {
        return postProcessETS_;
    }

private:
    std::shared_ptr<PostProcessETS> postProcessETS_;
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_POST_PROCESS_SETTINGS_IMPL_H