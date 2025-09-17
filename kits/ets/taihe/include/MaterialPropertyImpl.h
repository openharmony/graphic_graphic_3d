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

#include "ANIUtils.h"
#include "SamplerImpl.h"
#include "SceneResourceImpl.h"

#include "MaterialPropertyETS.h"
#include "SamplerETS.h"

namespace OHOS::Render3D::KITETS {
class MaterialPropertyImpl {
public:
    MaterialPropertyImpl(const std::shared_ptr<MaterialPropertyETS> matProp);
    ~MaterialPropertyImpl();

    ::SceneResources::ImageOrNull getImage();
    void setImage(::SceneResources::ImageOrNull const& img);

    ::SceneTypes::Vec4 getFactor();
    void setFactor(::SceneTypes::weak::Vec4 const &factor);

    ::taihe::optional<::SceneResources::Sampler> getSampler();
    void setSampler(::taihe::optional_view<::SceneResources::Sampler> sampler);

    ::taihe::optional<int64_t> getImpl()
    {
        return taihe::optional<int64_t>(std::in_place, reinterpret_cast<uintptr_t>(this));
    }

    std::shared_ptr<MaterialPropertyETS> getInternalMaterialProperty() const
    {
        return materialPropertyETS_;
    }

private:
    std::shared_ptr<MaterialPropertyETS> materialPropertyETS_{nullptr};
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_MATERIAL_PROPERTY_IMPL_H