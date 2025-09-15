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

#ifndef OHOS_3D_SHADER_IMPL_H
#define OHOS_3D_SHADER_IMPL_H

#include "SceneResourceImpl.h"
#include "SceneTH.user.hpp"
#include "stdexcept"
#include "taihe/optional.hpp"
#include "taihe/runtime.hpp"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include "ANIUtils.h"
#include "ShaderETS.h"

namespace OHOS::Render3D::KITETS {
class ShaderImpl : public SceneResourceImpl {
public:
    ShaderImpl(const std::shared_ptr<ShaderETS> &shader);
    ~ShaderImpl();

    int32_t getInputsSize();

    ::taihe::array<::taihe::string> getInputsKeys();

    ::taihe::optional<::SceneResources::ShaderInputType> getInput(::taihe::string_view key);
    void setInput(::taihe::string_view key, ::SceneResources::ShaderInputType const& value);

    std::shared_ptr<ShaderETS> getInternalShader() const
    {
        return shaderETS_;
    }

private:
    std::shared_ptr<ShaderETS> shaderETS_{nullptr};
};
}  // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_SHADER_IMPL_H