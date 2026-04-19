/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_3D_SHADOW_CONFIGURATION_IMPL_H
#define OHOS_3D_SHADOW_CONFIGURATION_IMPL_H

#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"
#include "stdexcept"

#include "SceneTH.user.hpp"

#include "interop_js/arkts_interop_js_api.h"
#include "interop_js/arkts_esvalue.h"
#include "napi/native_api.h"
#include "CheckNapiEnv.h"

#include "shadow_configuration/PCFConfigETS.h"

namespace OHOS::Render3D::KITETS {
using namespace ShadowConfiguration;
class SoftShadowConfigImpl {
public:
    SoftShadowConfigImpl() = default;
    virtual ~SoftShadowConfigImpl() = default;
    virtual ::SceneTypes::ShadowAlgorithmType getShadowAlgorithmType() = 0;
};

class PCFConfigImpl : public SoftShadowConfigImpl {
public:
    explicit PCFConfigImpl(const std::shared_ptr<PCFConfigETS> pcfConfigETS_);
    ~PCFConfigImpl() override;

    ::taihe::optional<double> getShadowSampleRadius();
    void setShadowSampleRadius(::taihe::optional<double> shadowSampleRadius);

    ::taihe::optional<int32_t> getShadowSampleCount();
    void setShadowSampleCount(::taihe::optional<int32_t> shadowSampleCount);

    ::SceneTypes::ShadowAlgorithmType getShadowAlgorithmType() override;

    int64_t getPCFConfigImpl();

    std::shared_ptr<PCFConfigETS> getPCFConfigETS() const
    {
        return pcfConfigETS_;
    }

private:
    std::shared_ptr<PCFConfigETS> pcfConfigETS_{nullptr};
    bool isRadiusUndefined_{false};
    bool isCountUndefined_{false};
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_SHADOW_CONFIGURATION_IMPL_H