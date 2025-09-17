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
#include "PBRMaterialImpl.h"
#include "Vec3Impl.h"

namespace OHOS::Render3D::KITETS {
SubMeshImpl::SubMeshImpl(const std::shared_ptr<SubMeshETS> subMeshETS) : subMeshETS_(subMeshETS)
{}

SubMeshImpl::~SubMeshImpl()
{
    if (subMeshETS_) {
        subMeshETS_.reset();
    }
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

::SceneResources::VariousMaterial SubMeshImpl::getMaterial()
{
    if (!subMeshETS_) {
        WIDGET_LOGE("get material failed, internal submesh is null");
        return ::SceneResources::VariousMaterial::make_pbr(::SceneResources::PBRMaterial({nullptr, nullptr}));
    }
    std::shared_ptr<MaterialETS> matETS = subMeshETS_->GetMaterial();
    if (!matETS) {
        return ::SceneResources::VariousMaterial::make_pbr(::SceneResources::PBRMaterial({nullptr, nullptr}));
    }
    auto mat = ::taihe::make_holder<PBRMaterialImpl, ::SceneResources::PBRMaterial>(matETS);
    return ::SceneResources::VariousMaterial::make_pbr(mat);
}

void SubMeshImpl::setMaterial(::SceneResources::weak::Material mat)
{
    if (!subMeshETS_) {
        WIDGET_LOGE("set material failed, internal submesh is null");
        return;
    }
    ::taihe::optional<int64_t> implOp = static_cast<::SceneResources::weak::SceneResource>(mat)->getImpl();
    if (!implOp) {
        WIDGET_LOGE("set material failed, material is null");
        return;
    }
    MaterialImpl *matImpl = reinterpret_cast<MaterialImpl *>(implOp.value());
    if (matImpl == nullptr) {
        WIDGET_LOGE("set material failed, material is null");
        return;
    }
    subMeshETS_->SetMaterial(matImpl->getInternalMaterial());
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
}  // namespace OHOS::Render3D::KITETS