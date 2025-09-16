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

#include "SamplerETS.h"

namespace OHOS::Render3D::KITETS {
class SamplerImpl {
public:
    SamplerImpl(const std::shared_ptr<SamplerETS> &sampler);
    ~SamplerImpl();

    ::taihe::optional<::SceneResources::SamplerFilter> getMagFilter();
    void setMagFilter(::taihe::optional_view<::SceneResources::SamplerFilter> filter);

    ::taihe::optional<::SceneResources::SamplerFilter> getMinFilter();
    void setMinFilter(::taihe::optional_view<::SceneResources::SamplerFilter> filter);

    ::taihe::optional<::SceneResources::SamplerFilter> getMipMapMode();
    void setMipMapMode(::taihe::optional_view<::SceneResources::SamplerFilter> filter);

    ::taihe::optional<::SceneResources::SamplerAddressMode> getAddressModeU();
    void setAddressModeU(::taihe::optional_view<::SceneResources::SamplerAddressMode> mode);

    ::taihe::optional<::SceneResources::SamplerAddressMode> getAddressModeV();
    void setAddressModeV(::taihe::optional_view<::SceneResources::SamplerAddressMode> mode);

    ::taihe::optional<int64_t> getImpl()
    {
        return taihe::optional<int64_t>(std::in_place, reinterpret_cast<uintptr_t>(this));
    }

    std::shared_ptr<SamplerETS> getInternalObject() const
    {
        return sampler_;
    }

private:
    std::shared_ptr<SamplerETS> sampler_;
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_SAMPLER_IMPL_H