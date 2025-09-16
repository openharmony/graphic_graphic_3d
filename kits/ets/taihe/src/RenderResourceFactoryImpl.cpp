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

#include "RenderResourceFactoryImpl.h"

namespace OHOS::Render3D::KITETS {
::SceneResources::Shader RenderResourceFactoryImpl::createShaderSync(::SceneTH::SceneResourceParameters const &params)
{
    const std::string name = ExtractResourceName(params);
    const std::string uri = ExtractUri(params.uri);
    const InvokeReturn<std::shared_ptr<ShaderETS>> shader = RenderContextETS::GetInstance().CreateShader(name, uri);
    if (shader) {
        return ::taihe::make_holder<ShaderImpl, ::SceneResources::Shader>(shader.value);
    } else {
        ::taihe::set_error(shader.error);
        return ::SceneResources::Shader({nullptr, nullptr});
    }
}

::SceneResources::Sampler RenderResourceFactoryImpl::createSamplerSync(::SceneTH::SceneResourceParameters const &params)
{
    std::shared_ptr<SamplerETS> sampler = std::make_shared<SamplerETS>();
    return ::taihe::make_holder<SamplerImpl, ::SceneResources::Sampler>(sampler);
}

}  // namespace OHOS::Render3D::KITETS