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

#ifndef OHOS_3D_VIGNETTE_SETTINGS_IMPL_H
#define OHOS_3D_VIGNETTE_SETTINGS_IMPL_H

#include "ScenePostProcessSettings.proj.hpp"
#include "ScenePostProcessSettings.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

#include "VignetteETS.h"

namespace OHOS::Render3D::KITETS {
class VignetteSettingsImpl {
    friend class PostProcessSettingsImpl;

public:
    static std::shared_ptr<VignetteETS> CreateInternal(const ScenePostProcessSettings::VignetteSettings &data);

    VignetteSettingsImpl(const std::shared_ptr<VignetteETS> vignetteETS);
    ~VignetteSettingsImpl();
    ::taihe::optional<double> getRoundness();
    void setRoundness(::taihe::optional_view<double> roundness);
    ::taihe::optional<double> getIntensity();
    void setIntensity(::taihe::optional_view<double> intensity);

    ::taihe::optional<int64_t> getImpl()
    {
        return taihe::optional<int64_t>(std::in_place, reinterpret_cast<uintptr_t>(this));
    }

private:
    std::shared_ptr<VignetteETS> vignetteETS_;
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_VIGNETTE_SETTINGS_IMPL_H