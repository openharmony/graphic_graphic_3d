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

#ifndef OHOS_3D_MATERIAL_PROPERTY_IMPL_H
#define OHOS_3D_MATERIAL_PROPERTY_IMPL_H

#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"
#include "stdexcept"

#include "SceneResourceImpl.h"

namespace OHOS::Render3D::KITETS {
class MaterialPropertyImpl {
public:
    MaterialPropertyImpl()
    {
        // Don't forget to implement the constructor.
    }

    ::SceneResources::ImageOrNull getImage()
    {
        TH_THROW(std::runtime_error, "getImage not implemented");
    }

    void setImage(::SceneResources::ImageOrNull const& img)
    {
        TH_THROW(std::runtime_error, "setImage not implemented");
    }

    ::SceneTypes::Vec4 getFactor()
    {
        TH_THROW(std::runtime_error, "getFactor not implemented");
    }

    void setFactor(::SceneTypes::Vec4 const &factor)
    {
        TH_THROW(std::runtime_error, "setFactor not implemented");
    }

    ::taihe::optional<::SceneResources::Sampler> getSampler()
    {
        TH_THROW(std::runtime_error, "getSampler not implemented");
    }

    void setSampler(::taihe::optional_view<::SceneResources::Sampler> sampler)
    {
        TH_THROW(std::runtime_error, "setSampler not implemented");
    }
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_MATERIAL_PROPERTY_IMPL_H