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

#ifndef OHOS_3D_TONE_MAPPING_SETTINGS_IMPL_H
#define OHOS_3D_TONE_MAPPING_SETTINGS_IMPL_H

#include "ScenePostProcessSettings.proj.hpp"
#include "ScenePostProcessSettings.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include "TonemapETS.h"

class ToneMappingSettingsImpl {
    friend class PostProcessSettingsImpl;

public:
    static std::shared_ptr<TonemapETS> CreateInternal(const ScenePostProcessSettings::ToneMappingSettings &data);

    ToneMappingSettingsImpl(const std::shared_ptr<TonemapETS> tonemapETS);
    ~ToneMappingSettingsImpl();
    ::taihe::optional<::ScenePostProcessSettings::ToneMappingType> getType();
    void setType(::taihe::optional_view<::ScenePostProcessSettings::ToneMappingType> type);
    ::taihe::optional<double> getExposure();
    void setExposure(::taihe::optional_view<double> exposure);

    ::taihe::optional<int64_t> getImpl()
    {
        return taihe::optional<int64_t>(std::in_place, reinterpret_cast<int64_t>(this));
    }

private:
    std::shared_ptr<TonemapETS> tonemapETS_;
};
#endif  // OHOS_3D_TONE_MAPPING_SETTINGS_IMPL_H