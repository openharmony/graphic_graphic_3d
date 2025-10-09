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

#ifndef OHOS_3D_METALLIC_ROUGHNESS_MATERIAL_IMPL_H
#define OHOS_3D_METALLIC_ROUGHNESS_MATERIAL_IMPL_H

#include "MaterialImpl.h"
#include "SceneResources.user.hpp"
#include "stdexcept"
#include "taihe/optional.hpp"
#include "taihe/runtime.hpp"

namespace OHOS::Render3D::KITETS {
class MetallicRoughnessMaterialImpl : virtual public MaterialImpl {
public:
    MetallicRoughnessMaterialImpl(const std::shared_ptr<MaterialETS> mat);
    ~MetallicRoughnessMaterialImpl();

    ::SceneResources::MaterialProperty getBaseColor();
    void setBaseColor(::SceneResources::weak::MaterialProperty color);

    ::SceneResources::MaterialProperty getNormal();
    void setNormal(::SceneResources::weak::MaterialProperty normal);

    ::SceneResources::MaterialProperty getMaterial();
    void setMaterial(::SceneResources::weak::MaterialProperty material);

    ::SceneResources::MaterialProperty getAmbientOcclusion();
    void setAmbientOcclusion(::SceneResources::weak::MaterialProperty ao);

    ::SceneResources::MaterialProperty getEmissive();
    void setEmissive(::SceneResources::weak::MaterialProperty emissive);

    ::SceneResources::MaterialProperty getClearCoat();
    void setClearCoat(::SceneResources::weak::MaterialProperty clearCoat);

    ::SceneResources::MaterialProperty getClearCoatRoughness();
    void setClearCoatRoughness(::SceneResources::weak::MaterialProperty roughness);

    ::SceneResources::MaterialProperty getClearCoatNormal();
    void setClearCoatNormal(::SceneResources::weak::MaterialProperty normal);

    ::SceneResources::MaterialProperty getSheen();
    void setSheen(::SceneResources::weak::MaterialProperty sheen);

    ::SceneResources::MaterialProperty getSpecular();
    void setSpecular(::SceneResources::weak::MaterialProperty specular);

private:
    std::shared_ptr<MaterialETS> materialETS_;
};
}  // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_METALLIC_ROUGHNESS_MATERIAL_IMPL_H