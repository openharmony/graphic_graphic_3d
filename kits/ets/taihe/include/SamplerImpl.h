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

#ifndef OHOS_3D_SAMPLER_IMPL_H
#define OHOS_3D_SAMPLER_IMPL_H

#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"
#include "stdexcept"

#include "SceneResources.user.hpp"
#include "SceneTH.user.hpp"

class SamplerImpl {
public:
    SamplerImpl(::SceneTH::SceneResourceParameters const &params)
    {
        // Don't forget to implement the constructor.
    }

    ::taihe::optional<::SceneResources::SamplerFilter> getMagFilter()
    {
        TH_THROW(std::runtime_error, "getMagFilter not implemented");
    }

    void setMagFilter(::taihe::optional_view<::SceneResources::SamplerFilter> filter)
    {
        TH_THROW(std::runtime_error, "setMagFilter not implemented");
    }

    ::taihe::optional<::SceneResources::SamplerFilter> getMinFilter()
    {
        TH_THROW(std::runtime_error, "getMinFilter not implemented");
    }

    void setMinFilter(::taihe::optional_view<::SceneResources::SamplerFilter> filter)
    {
        TH_THROW(std::runtime_error, "SetminFilter not implemented");
    }

    ::taihe::optional<::SceneResources::SamplerFilter> getMipMapMode()
    {
        TH_THROW(std::runtime_error, "getMipMapMode not implemented");
    }

    void setMipMapMode(::taihe::optional_view<::SceneResources::SamplerFilter> filter)
    {
        TH_THROW(std::runtime_error, "setMipMapMode not implemented");
    }

    ::taihe::optional<::SceneResources::SamplerAddressMode> getAddressModeU()
    {
        TH_THROW(std::runtime_error, "getAddressModeU not implemented");
    }

    void setAddressModeU(::taihe::optional_view<::SceneResources::SamplerAddressMode> mode)
    {
        TH_THROW(std::runtime_error, "setAddressModeU not implemented");
    }

    ::taihe::optional<::SceneResources::SamplerAddressMode> getAddressModeV()
    {
        TH_THROW(std::runtime_error, "getAddressModeV not implemented");
    }

    void setAddressModeV(::taihe::optional_view<::SceneResources::SamplerAddressMode> mode)
    {
        TH_THROW(std::runtime_error, "setAddressModeV not implemented");
    }
};
#endif  // OHOS_3D_SAMPLER_IMPL_H