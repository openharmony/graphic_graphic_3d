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
#include "SceneResources.user.hpp"
#include "SceneResourceImpl.h"

namespace OHOS::Render3D::KITETS {
class MaterialImpl : public SceneResourceImpl {
public:
    MaterialImpl() : SceneResourceImpl(SceneResources::SceneResourceType::key_t::MATERIAL)
    {
        // Don't forget to implement the constructor.
    }

    ::SceneResources::MaterialType getMaterialType()
    {
        TH_THROW(std::runtime_error, "getMaterialType not implemented");
    }

    ::taihe::optional<bool> getShadowReceiver()
    {
        TH_THROW(std::runtime_error, "getShadowReceiver not implemented");
    }

    void setShadowReceiver(::taihe::optional_view<bool> value)
    {
        TH_THROW(std::runtime_error, "setShadowReceiver not implemented");
    }

    ::taihe::optional<::SceneResources::CullMode> getCullMode()
    {
        TH_THROW(std::runtime_error, "getCullMode not implemented");
    }

    void setCullMode(::taihe::optional_view<::SceneResources::CullMode> mode)
    {
        TH_THROW(std::runtime_error, "setCullMode not implemented");
    }

    ::taihe::optional<::SceneResources::Blend> getBlend()
    {
        TH_THROW(std::runtime_error, "getBlend not implemented");
    }

    void setBlend(::taihe::optional_view<::SceneResources::Blend> blend)
    {
        TH_THROW(std::runtime_error, "setBlend not implemented");
    }

    ::taihe::optional<double> getAlphaCutoff()
    {
        TH_THROW(std::runtime_error, "getAlphaCutoff not implemented");
    }

    void setAlphaCutoff(::taihe::optional_view<double> cutoff)
    {
        TH_THROW(std::runtime_error, "setAlphaCutoff not implemented");
    }

    ::taihe::optional<::SceneResources::RenderSort> getRenderSort()
    {
        TH_THROW(std::runtime_error, "getRenderSort not implemented");
    }

    void setRenderSort(::taihe::optional_view<::SceneResources::RenderSort> renderSort)
    {
        TH_THROW(std::runtime_error, "setRenderSort not implemented");
    }
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_MATERIAL_IMPL_H