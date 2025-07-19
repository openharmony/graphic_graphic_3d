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

#ifndef OHOS_3D_SHADER_MATERIAL_IMPL_H
#define OHOS_3D_SHADER_MATERIAL_IMPL_H

#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"
#include "stdexcept"

#include "SceneResources.user.hpp"
#include "MaterialImpl.h"

class ShaderMaterialImpl : public MaterialImpl {
public:
    // ::SceneTH::SceneResourceParameters const &params, ::SceneResources::MaterialType materialType
    ShaderMaterialImpl() : MaterialImpl()
    {
        // Don't forget to implement the constructor.
    }

    ::taihe::optional<::SceneResources::Shader> getColorShader()
    {
        TH_THROW(std::runtime_error, "getColorShader not implemented");
    }

    void setColorShader(::taihe::optional_view<::SceneResources::Shader> shader)
    {
        TH_THROW(std::runtime_error, "setColorShader not implemented");
    }
};
#endif  // OHOS_3D_SHADER_MATERIAL_IMPL_H