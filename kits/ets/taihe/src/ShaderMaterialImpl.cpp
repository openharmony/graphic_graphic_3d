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

#include "ShaderMaterialImpl.h"

#include "ShaderImpl.h"

namespace OHOS::Render3D::KITETS {
ShaderMaterialImpl::ShaderMaterialImpl(const std::shared_ptr<MaterialETS> mat) : MaterialImpl(mat), materialETS_(mat)
{
}

ShaderMaterialImpl::~ShaderMaterialImpl()
{
    materialETS_.reset();
}

::taihe::optional<::SceneResources::Shader> ShaderMaterialImpl::getColorShader()
{
    if (!materialETS_) {
        WIDGET_LOGE("get color shader failed, internal material is null");
        return std::nullopt;
    }
    auto shader = materialETS_->GetColorShader();
    ::SceneResources::Shader shaderImpl = ::taihe::make_holder<ShaderImpl, ::SceneResources::Shader>(shader);
    return ::taihe::optional<::SceneResources::Shader>(std::in_place, shaderImpl);
}

void ShaderMaterialImpl::setColorShader(::taihe::optional_view<::SceneResources::Shader> shader)
{
    if (!materialETS_) {
        WIDGET_LOGE("set color shader failed, internal material is null");
        return;
    }
    std::shared_ptr<ShaderETS> shaderETS = nullptr;
    if (shader) {
        ::SceneResources::Shader sd = shader.value();
        ::taihe::optional<int64_t> implOp = static_cast<::SceneResources::SceneResource>(sd)->getImpl();
        ShaderImpl *shaderImpl = GetImplPointer<ShaderImpl>(implOp);
        if (shaderImpl == nullptr) {
            WIDGET_LOGE("set color shader failed, get shader pointer failed");
        } else {
            shaderETS = shaderImpl->getInternalShader();
        }
    } else {
        WIDGET_LOGE("set color shader failed, shader is null");
    }
    materialETS_->SetColorShader(shaderETS);
}
}  // namespace OHOS::Render3D::KITETS