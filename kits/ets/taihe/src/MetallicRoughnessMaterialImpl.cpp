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

#include "MetallicRoughnessMaterialImpl.h"

namespace OHOS::Render3D::KITETS {
MetallicRoughnessMaterialImpl::MetallicRoughnessMaterialImpl(const std::shared_ptr<MaterialETS> mat)
    : MaterialImpl(mat), materialETS_(mat)
{
}

MetallicRoughnessMaterialImpl::~MetallicRoughnessMaterialImpl()
{
    if (materialETS_) {
        materialETS_.reset();
    }
}

::SceneResources::MaterialProperty MetallicRoughnessMaterialImpl::getBaseColor()
{
    return getProperty<MaterialETS::TextureIndex::BASE_COLOR>();
}

void MetallicRoughnessMaterialImpl::setBaseColor(::SceneResources::weak::MaterialProperty color)
{
    setProperty<MaterialETS::TextureIndex::BASE_COLOR>(color);
}

::SceneResources::MaterialProperty MetallicRoughnessMaterialImpl::getNormal()
{
    return getProperty<MaterialETS::TextureIndex::NORMAL>();
}

void MetallicRoughnessMaterialImpl::setNormal(::SceneResources::weak::MaterialProperty normal)
{
    setProperty<MaterialETS::TextureIndex::NORMAL>(normal);
}

::SceneResources::MaterialProperty MetallicRoughnessMaterialImpl::getMaterial()
{
    return getProperty<MaterialETS::TextureIndex::MATERIAL>();
}

void MetallicRoughnessMaterialImpl::setMaterial(::SceneResources::weak::MaterialProperty material)
{
    setProperty<MaterialETS::TextureIndex::MATERIAL>(material);
}

::SceneResources::MaterialProperty MetallicRoughnessMaterialImpl::getAmbientOcclusion()
{
    return getProperty<MaterialETS::TextureIndex::AMBIENT_OCCLUSION>();
}

void MetallicRoughnessMaterialImpl::setAmbientOcclusion(::SceneResources::weak::MaterialProperty ao)
{
    setProperty<MaterialETS::TextureIndex::AMBIENT_OCCLUSION>(ao);
}

::SceneResources::MaterialProperty MetallicRoughnessMaterialImpl::getEmissive()
{
    return getProperty<MaterialETS::TextureIndex::EMISSIVE>();
}

void MetallicRoughnessMaterialImpl::setEmissive(::SceneResources::weak::MaterialProperty emissive)
{
    setProperty<MaterialETS::TextureIndex::EMISSIVE>(emissive);
}

::SceneResources::MaterialProperty MetallicRoughnessMaterialImpl::getClearCoat()
{
    return getProperty<MaterialETS::TextureIndex::CLEAR_COAT>();
}

void MetallicRoughnessMaterialImpl::setClearCoat(::SceneResources::weak::MaterialProperty clearCoat)
{
    setProperty<MaterialETS::TextureIndex::CLEAR_COAT>(clearCoat);
}

::SceneResources::MaterialProperty MetallicRoughnessMaterialImpl::getClearCoatRoughness()
{
    return getProperty<MaterialETS::TextureIndex::CLEAR_COAT_ROUGHNESS>();
}

void MetallicRoughnessMaterialImpl::setClearCoatRoughness(::SceneResources::weak::MaterialProperty roughness)
{
    setProperty<MaterialETS::TextureIndex::CLEAR_COAT_ROUGHNESS>(roughness);
}

::SceneResources::MaterialProperty MetallicRoughnessMaterialImpl::getClearCoatNormal()
{
    return getProperty<MaterialETS::TextureIndex::CLEAR_COAT_NORMAL>();
}

void MetallicRoughnessMaterialImpl::setClearCoatNormal(::SceneResources::weak::MaterialProperty normal)
{
    setProperty<MaterialETS::TextureIndex::CLEAR_COAT_NORMAL>(normal);
}

::SceneResources::MaterialProperty MetallicRoughnessMaterialImpl::getSheen()
{
    return getProperty<MaterialETS::TextureIndex::SHEEN>();
}

void MetallicRoughnessMaterialImpl::setSheen(::SceneResources::weak::MaterialProperty sheen)
{
    setProperty<MaterialETS::TextureIndex::SHEEN>(sheen);
}

::SceneResources::MaterialProperty MetallicRoughnessMaterialImpl::getSpecular()
{
    return getProperty<MaterialETS::TextureIndex::SPECULAR>();
}

void MetallicRoughnessMaterialImpl::setSpecular(::SceneResources::weak::MaterialProperty specular)
{
    setProperty<MaterialETS::TextureIndex::SPECULAR>(specular);
}
}  // namespace OHOS::Render3D::KITETS