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

#ifndef OHOS_3D_MATERIAL_IMPL_H
#define OHOS_3D_MATERIAL_IMPL_H

#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"
#include "stdexcept"

#include "SceneTH.user.hpp"
#include "SceneResources.proj.hpp"
#include "SceneResources.impl.hpp"
#include "SceneResourceImpl.h"

#include <scene/interface/intf_material.h>

#include "MaterialETS.h"

namespace OHOS::Render3D::KITETS {
class MaterialImpl : public SceneResourceImpl {
public:
    MaterialImpl(const std::shared_ptr<MaterialETS> mat);
    ~MaterialImpl();

    ::SceneResources::MaterialType getMaterialType();

    ::taihe::optional<bool> getShadowReceiver();
    void setShadowReceiver(::taihe::optional_view<bool> value);

    ::taihe::optional<::SceneResources::CullMode> getCullMode();
    void setCullMode(::taihe::optional_view<::SceneResources::CullMode> mode);

    ::taihe::optional<::SceneResources::Blend> getBlend();
    void setBlend(::taihe::optional_view<::SceneResources::Blend> blend);

    ::taihe::optional<double> getAlphaCutoff();
    void setAlphaCutoff(::taihe::optional_view<double> cutoff);

    ::taihe::optional<::SceneResources::RenderSort> getRenderSort();
    void setRenderSort(::taihe::optional_view<::SceneResources::RenderSort> renderSort);

    std::shared_ptr<MaterialETS> getInternalMaterial() const
    {
        return materialETS_;
    }

private:
    std::shared_ptr<MaterialETS> materialETS_;
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_MATERIAL_IMPL_H