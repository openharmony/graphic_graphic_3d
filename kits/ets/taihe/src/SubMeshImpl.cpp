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

#include "SubMeshImpl.h"
#include "Vec3Impl.h"

SubMeshImpl::SubMeshImpl(const std::shared_ptr<SubMeshETS> subMeshETS) : subMeshETS_(subMeshETS)
{}

SubMeshImpl::~SubMeshImpl()
{
    subMeshETS_.reset();
}

::taihe::string SubMeshImpl::getName()
{
    if (subMeshETS_) {
        return subMeshETS_->GetName().c_str();
    }
    return "";
}

void SubMeshImpl::setName(::taihe::string_view name)
{
    if (subMeshETS_) {
        subMeshETS_->SetName(name.c_str());
    }
}

::SceneResources::Material SubMeshImpl::getMaterial()
{
    // The parameters in the make_holder function should be of the same type
    // as the parameters in the constructor of the actual implementation class.
    TH_THROW(std::runtime_error, "getMaterial not implemented");
}

void SubMeshImpl::setMaterial(::SceneResources::weak::Material mat)
{
    TH_THROW(std::runtime_error, "setMaterial not implemented");
}

::SceneTypes::Aabb SubMeshImpl::getAabb()
{
    if (subMeshETS_) {
        auto aabbMin = subMeshETS_->GetAABBMin();
        auto aabbMax = subMeshETS_->GetAABBMax();
        return SceneTypes::Aabb{taihe::make_holder<Vec3Impl, SceneTypes::Vec3>(aabbMin),
            taihe::make_holder<Vec3Impl, SceneTypes::Vec3>(aabbMax)};
    } else {
        return SceneTypes::Aabb{taihe::make_holder<Vec3Impl, SceneTypes::Vec3>(BASE_NS::Math::ZERO_VEC3),
            taihe::make_holder<Vec3Impl, SceneTypes::Vec3>(BASE_NS::Math::ZERO_VEC3)};
    }
}
