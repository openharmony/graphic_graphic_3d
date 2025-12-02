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

#ifndef OHOS_3D_UNLIT_SHADOW_ALPHA_MATERIAL_IMPL_H
#define OHOS_3D_UNLIT_SHADOW_ALPHA_MATERIAL_IMPL_H

#include "MaterialImpl.h"
#include "SceneResources.user.hpp"
#include "stdexcept"
#include "taihe/optional.hpp"
#include "taihe/runtime.hpp"

namespace OHOS::Render3D::KITETS {
class UnlitShadowAlphaImpl : virtual public MaterialImpl {
public:
    UnlitShadowAlphaImpl(const std::shared_ptr<MaterialETS> mat);
    ~UnlitShadowAlphaImpl();

    ::SceneResources::MaterialProperty getShadowAlpha();
    void setShadowAlpha(::SceneResources::weak::MaterialProperty color);

private:
    std::shared_ptr<MaterialETS> materialETS_;
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_UNLIT_SHADOW_ALPHA_MATERIAL_IMPL_H