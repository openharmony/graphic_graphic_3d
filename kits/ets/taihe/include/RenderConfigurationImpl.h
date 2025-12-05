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

#ifndef OHOS_3D_RENDER_CONFIGURATION_IMPL_H
#define OHOS_3D_RENDER_CONFIGURATION_IMPL_H

#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"
#include "stdexcept"

#include "SceneTH.user.hpp"

#include "BaseObjectJS.h"
#include <napi_api.h>
#include "interop_js/arkts_interop_js_api.h"
#include "interop_js/arkts_esvalue.h"
#include "napi/native_api.h"
#include "CheckNapiEnv.h"

#include "RenderConfigurationETS.h"

namespace OHOS::Render3D::KITETS {
class RenderConfigurationImpl {
private:
    std::shared_ptr<RenderConfigurationETS> rcETS_{nullptr};
    BASE_NS::Math::UVec2 resolution_{BASE_NS::Math::ZERO_UVEC2};
public:
    explicit RenderConfigurationImpl(const std::shared_ptr<RenderConfigurationETS> rcETS);
    ~RenderConfigurationImpl();

    std::shared_ptr<RenderConfigurationETS> GetRenderConfigurationETS();

    ::taihe::optional<::SceneTypes::Vec2> getShadowResolution();
    void setShadowResolution(::taihe::optional_view<::SceneTypes::Vec2> resolution);
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_ENVIRONMENT_IMPL_H