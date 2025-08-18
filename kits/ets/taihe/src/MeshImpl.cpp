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
#include "Vec3Impl.h"

namespace OHOS::Render3D::KITETS {
MeshImpl::MeshImpl(const std::shared_ptr<MeshETS> meshETS)
    : SceneResourceImpl(SceneResources::SceneResourceType::key_t::MESH, meshETS), meshETS_(meshETS)
{}

MeshImpl::~MeshImpl()
{
    meshETS_.reset();
}

::taihe::array<::SceneResources::SubMesh> MeshImpl::getSubMeshes()
{
    taihe::array<SceneResources::SubMesh> subMeshes = {};
    return subMeshes;
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

::taihe::optional<::SceneResources::Material> MeshImpl::getMaterialOverride()
{
    TH_THROW(std::runtime_error, "getMaterialOverride not implemented");
}

void MeshImpl::setMaterialOverride(::taihe::optional_view<::SceneResources::Material> mat)
{
    TH_THROW(std::runtime_error, "setMaterialOverride not implemented");
}
}  // namespace OHOS::Render3D::KITETS