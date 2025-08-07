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

#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"
#include "stdexcept"

#include "SceneTH.user.hpp"

#include "SceneResourceImpl.h"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

namespace OHOS::Render3D::KITETS {
class ShaderImpl : public SceneResourceImpl {
public:
    ShaderImpl(SceneTH::SceneResourceParameters const &params)
        : SceneResourceImpl(SceneResources::SceneResourceType::key_t::SHADER)
    {
        // Don't forget to implement the constructor.
        WIDGET_LOGE("The shader hasn't been implemented yet, dear.");
    }

    ::taihe::map<::taihe::string, ::SceneResources::ShaderInputType> getInputs();
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_SHADER_IMPL_H