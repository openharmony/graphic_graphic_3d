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

#ifndef OHOS_3D_BLOOM_SETTINGS_IMPL_H
#define OHOS_3D_BLOOM_SETTINGS_IMPL_H

#include "ScenePostProcessSettings.proj.hpp"
#include "ScenePostProcessSettings.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

#include "BloomETS.h"

namespace OHOS::Render3D::KITETS {
class BloomSettingsImpl {
    friend class PostProcessSettingsImpl;

public:
    static std::shared_ptr<BloomETS> CreateInternal(const ScenePostProcessSettings::BloomSettings &data);

    BloomSettingsImpl(const std::shared_ptr<BloomETS> bloomETS);
    ~BloomSettingsImpl();
    ::taihe::optional<double> getThresholdHard();
    void setThresholdHard(::taihe::optional_view<double> thresholdHard);
    ::taihe::optional<double> getThresholdSoft();
    void setThresholdSoft(::taihe::optional_view<double> thresholdSoft);
    ::taihe::optional<double> getScaleFactor();
    void setScaleFactor(::taihe::optional_view<double> scaleFactor);
    ::taihe::optional<double> getScatter();
    void setScatter(::taihe::optional_view<double> scatter);

    ::taihe::optional<int64_t> getImpl()
    {
        return taihe::optional<int64_t>(std::in_place, reinterpret_cast<int64_t>(this));
    }

private:
    std::shared_ptr<BloomETS> bloomETS_;
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_BLOOM_SETTINGS_IMPL_H