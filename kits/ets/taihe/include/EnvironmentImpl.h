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

#ifndef OHOS_3D_ENVIRONMENT_IMPL_H
#define OHOS_3D_ENVIRONMENT_IMPL_H

#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"
#include "stdexcept"

#include "SceneTH.user.hpp"
#include "SceneResources.user.hpp"
#include "SceneResources.Transfer.proj.hpp"
#include "SceneResources.Transfer.impl.hpp"

#include "BaseObjectJS.h"
#include <napi_api.h>
#include "interop_js/arkts_interop_js_api.h"
#include "interop_js/arkts_esvalue.h"
#include "napi/native_api.h"
#include "CheckNapiEnv.h"

#include "SceneResourceImpl.h"
#include "EnvironmentETS.h"

namespace OHOS::Render3D::KITETS {
class EnvironmentImpl : public SceneResourceImpl {
private:
    std::shared_ptr<EnvironmentETS> envETS_{nullptr};
public:
    explicit EnvironmentImpl(const std::shared_ptr<EnvironmentETS> envETS);
    ~EnvironmentImpl();

    std::shared_ptr<EnvironmentETS> GetEnvETS();

    ::SceneResources::EnvironmentBackgroundType getBackgroundType();
    void setBackgroundType(::SceneResources::EnvironmentBackgroundType type);

    ::SceneTypes::Vec4 getIndirectDiffuseFactor();
    void setIndirectDiffuseFactor(::SceneTypes::weak::Vec4 factor);

    ::SceneTypes::Vec4 getIndirectSpecularFactor();
    void setIndirectSpecularFactor(::SceneTypes::weak::Vec4 factor);

    ::SceneTypes::Vec4 getEnvironmentMapFactor();
    void setEnvironmentMapFactor(::SceneTypes::weak::Vec4 factor);

    ::SceneResources::ImageOrNullOrUndefined getEnvironmentImage();
    void setEnvironmentImage(::SceneResources::ImageOrNullOrUndefined const& image);

    ::SceneResources::ImageOrNullOrUndefined getRadianceImage();
    void setRadianceImage(::SceneResources::ImageOrNullOrUndefined const& image);

    ::taihe::optional<::taihe::array<::SceneTypes::Vec3>> getIrradianceCoefficients();
    void setIrradianceCoefficients(::taihe::optional_view<::taihe::array<::SceneTypes::Vec3>> coefficients);

    ::taihe::optional<::SceneTypes::Quaternion> getEnvironmentRotation();
    void setEnvironmentRotation(::taihe::optional_view<::SceneTypes::Quaternion> rotation);
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_ENVIRONMENT_IMPL_H