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

#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"
#include "stdexcept"

#include "SceneResources.user.hpp"
#include "MaterialImpl.h"
#include "MaterialPropertyImpl.h"

class MetallicRoughnessMaterialImpl : public MaterialImpl {
public:
    // ::SceneTH::SceneResourceParameters const &params, ::SceneResources::MaterialType materialType
    MetallicRoughnessMaterialImpl() : MaterialImpl()
    {
        // Don't forget to implement the constructor.
    }

    ::SceneResources::MaterialProperty getBaseColor()
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        return taihe::make_holder<MaterialPropertyImpl, ::SceneResources::MaterialProperty>();
    }

    void setBaseColor(::SceneResources::weak::MaterialProperty color)
    {
        TH_THROW(std::runtime_error, "setBaseColor not implemented");
    }

    ::SceneResources::MaterialProperty getNormal()
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        return taihe::make_holder<MaterialPropertyImpl, ::SceneResources::MaterialProperty>();
    }

    void setNormal(::SceneResources::weak::MaterialProperty normal)
    {
        TH_THROW(std::runtime_error, "setNormal not implemented");
    }

    ::SceneResources::MaterialProperty getMaterial()
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        return taihe::make_holder<MaterialPropertyImpl, ::SceneResources::MaterialProperty>();
    }

    void setMaterial(::SceneResources::weak::MaterialProperty material)
    {
        TH_THROW(std::runtime_error, "setMaterial not implemented");
    }

    ::SceneResources::MaterialProperty getAmbientOcclusion()
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        return taihe::make_holder<MaterialPropertyImpl, ::SceneResources::MaterialProperty>();
    }

    void setAmbientOcclusion(::SceneResources::weak::MaterialProperty ao)
    {
        TH_THROW(std::runtime_error, "setAmbientOcclusion not implemented");
    }

    ::SceneResources::MaterialProperty getEmissive()
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        return taihe::make_holder<MaterialPropertyImpl, ::SceneResources::MaterialProperty>();
    }

    void setEmissive(::SceneResources::weak::MaterialProperty emissive)
    {
        TH_THROW(std::runtime_error, "setEmissive not implemented");
    }

    ::SceneResources::MaterialProperty getClearCoat()
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        return taihe::make_holder<MaterialPropertyImpl, ::SceneResources::MaterialProperty>();
    }

    void setClearCoat(::SceneResources::weak::MaterialProperty clearCoat)
    {
        TH_THROW(std::runtime_error, "setClearCoat not implemented");
    }

    ::SceneResources::MaterialProperty getClearCoatRoughness()
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        return taihe::make_holder<MaterialPropertyImpl, ::SceneResources::MaterialProperty>();
    }

    void setClearCoatRoughness(::SceneResources::weak::MaterialProperty roughness)
    {
        TH_THROW(std::runtime_error, "setClearCoatRoughness not implemented");
    }

    ::SceneResources::MaterialProperty getClearCoatNormal()
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        return taihe::make_holder<MaterialPropertyImpl, ::SceneResources::MaterialProperty>();
    }

    void setClearCoatNormal(::SceneResources::weak::MaterialProperty normal)
    {
        TH_THROW(std::runtime_error, "setClearCoatNormal not implemented");
    }

    ::SceneResources::MaterialProperty getSheen()
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        return taihe::make_holder<MaterialPropertyImpl, ::SceneResources::MaterialProperty>();
    }

    void setSheen(::SceneResources::weak::MaterialProperty sheen)
    {
        TH_THROW(std::runtime_error, "setSheen not implemented");
    }

    ::SceneResources::MaterialProperty getSpecular()
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        return taihe::make_holder<MaterialPropertyImpl, ::SceneResources::MaterialProperty>();
    }

    void setSpecular(::SceneResources::weak::MaterialProperty specular)
    {
        TH_THROW(std::runtime_error, "setSpecular not implemented");
    }
};
#endif  // OHOS_3D_METALLIC_ROUGHNESS_MATERIAL_IMPL_H