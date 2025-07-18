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

#include "SceneResourceImpl.h"
#include "EnvironmentETS.h"

class EnvironmentImpl : public SceneResourceImpl {
private:
    std::shared_ptr<EnvironmentETS> envETS_{nullptr};
public:
    explicit EnvironmentImpl(const std::shared_ptr<EnvironmentETS> envETS);

    std::shared_ptr<EnvironmentETS> GetEnvETS();

    ::SceneResources::EnvironmentBackgroundType getBackgroundType();
    void setBackgroundType(::SceneResources::EnvironmentBackgroundType type);

    ::SceneTypes::Vec4 getIndirectDiffuseFactor();
    void setIndirectDiffuseFactor(::SceneTypes::weak::Vec4 factor);

    ::SceneTypes::Vec4 getIndirectSpecularFactor();
    void setIndirectSpecularFactor(::SceneTypes::weak::Vec4 factor);

    ::SceneTypes::Vec4 getEnvironmentMapFactor()
    {
        TH_THROW(std::runtime_error, "getEnvironmentMapFactor not implemented");
    }
    void setEnvironmentMapFactor(::SceneTypes::weak::Vec4 factor)
    {
        TH_THROW(std::runtime_error, "setEnvironmentMapFactor not implemented");
    }

    ::SceneResources::ImageOrNullOrUndefined getEnvironmentImage()
    {
        TH_THROW(std::runtime_error, "getEnvironmentImage not implemented");
    }
    void setEnvironmentImage(::SceneResources::ImageOrNullOrUndefined const& image)
    {
        TH_THROW(std::runtime_error, "setEnvironmentImage not implemented");
    }

    ::SceneResources::ImageOrNullOrUndefined getRadianceImage()
    {
        TH_THROW(std::runtime_error, "getRadianceImage not implemented");
    }
    void setRadianceImage(::SceneResources::ImageOrNullOrUndefined const& image)
    {
        TH_THROW(std::runtime_error, "setRadianceImage not implemented");
    }

    ::taihe::optional<::taihe::array<::SceneTypes::Vec3>> getIrradianceCoefficients()
    {
        TH_THROW(std::runtime_error, "getIrradianceCoefficients not implemented");
    }
    void setIrradianceCoefficients(::taihe::optional_view<::taihe::array<::SceneTypes::Vec3>> coefficients)
    {
        TH_THROW(std::runtime_error, "setIrradianceCoefficients not implemented");
    }

    int64_t GetImpl()
    {
        return reinterpret_cast<int64_t>(this);
    }
};
#endif  // OHOS_3D_ENVIRONMENT_IMPL_H