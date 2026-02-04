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

#ifndef OHOS_3D_MESH_IMPL_H
#define OHOS_3D_MESH_IMPL_H

#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"
#include "stdexcept"

#include "MaterialImpl.h"
#include "taihe/include/SceneResourceImpl.h"
#include "SubMeshImpl.h"

#include "MaterialETS.h"
#include "MeshETS.h"

namespace OHOS::Render3D::KITETS {
class MeshImpl : public SceneResourceImpl {
public:
    MeshImpl(const std::shared_ptr<MeshETS> meshETS);

    ~MeshImpl();

    void destroy() override;

    ::taihe::array<::SceneResources::SubMesh> getSubMeshes();

    ::SceneTypes::Aabb getAabb();

    ::taihe::optional<::SceneResources::VariousMaterial> getMaterialOverride();

    void setMaterialOverride(::taihe::optional_view<::SceneResources::Material> mat);

private:
    std::shared_ptr<MeshETS> meshETS_;
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_MESH_IMPL_H