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

#include "MeshImpl.h"
#include "PBRMaterialImpl.h"
#include "Vec3Impl.h"

namespace OHOS::Render3D::KITETS {
MeshImpl::MeshImpl(const std::shared_ptr<MeshETS> meshETS)
    : SceneResourceImpl(SceneResources::SceneResourceType::key_t::MESH, meshETS), meshETS_(meshETS)
{
}

MeshImpl::~MeshImpl()
{
    meshETS_.reset();
}

::taihe::array<::SceneResources::SubMesh> MeshImpl::getSubMeshes()
{
    if (!meshETS_) {
        WIDGET_LOGE("get submeshes failed, invalid mesh");
        return {};
    }
    const std::vector<std::shared_ptr<SubMeshETS>> subMeshes = meshETS_->GetSubMeshes();
    if (subMeshes.empty()) {
        return {};
    }
    std::vector<::SceneResources::SubMesh> smVector;
    smVector.reserve(subMeshes.size());
    for (size_t i = 0; i < subMeshes.size(); i++) {
        smVector.emplace_back(::taihe::make_holder<SubMeshImpl, ::SceneResources::SubMesh>(subMeshes[i]));
    }
    return taihe::array_view<SceneResources::SubMesh>(smVector);
}

::SceneTypes::Aabb MeshImpl::getAabb()
{
    if (meshETS_) {
        auto aabbMin = meshETS_->GetAABBMin();
        auto aabbMax = meshETS_->GetAABBMax();
        return SceneTypes::Aabb{taihe::make_holder<Vec3Impl, SceneTypes::Vec3>(aabbMin),
                                taihe::make_holder<Vec3Impl, SceneTypes::Vec3>(aabbMax)};
    } else {
        return SceneTypes::Aabb{taihe::make_holder<Vec3Impl, SceneTypes::Vec3>(BASE_NS::Math::ZERO_VEC3),
                                taihe::make_holder<Vec3Impl, SceneTypes::Vec3>(BASE_NS::Math::ZERO_VEC3)};
    }
}

::taihe::optional<::SceneResources::VariousMaterial> MeshImpl::getMaterialOverride()
{
    if (!meshETS_) {
        return std::nullopt;
    }
    std::shared_ptr<MaterialETS> matETS = meshETS_->GetMaterialOverride();
    if (!matETS) {
        return std::nullopt;
    }
    auto mat = ::taihe::make_holder<PBRMaterialImpl, ::SceneResources::PBRMaterial>(matETS);
    return ::taihe::optional<::SceneResources::VariousMaterial>(std::in_place,
                                                                ::SceneResources::VariousMaterial::make_pbr(mat));
}

void MeshImpl::setMaterialOverride(::taihe::optional_view<::SceneResources::Material> mat)
{
    if (!meshETS_) {
        WIDGET_LOGE("set material override failed, mesh is null");
        return;
    }
    std::shared_ptr<MaterialETS> matETS = nullptr;
    if (mat) {
        ::SceneResources::Material mtr = *mat;
        if (mtr.is_error()) {
            return;
        }
        MaterialImpl *mi = GetImplPointer<MaterialImpl>(static_cast<::SceneResources::SceneResource>(mtr)->getImpl());
        if (mi) {
            matETS = mi->getInternalMaterial();
        }
    }
    meshETS_->SetMaterialOverride(matETS);
}
}  // namespace OHOS::Render3D::KITETS