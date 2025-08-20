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

#ifndef OHOS_3D_SUB_MESH_IMPL_H
#define OHOS_3D_SUB_MESH_IMPL_H

#include "taihe/runtime.hpp"
#include "taihe/optional.hpp"
#include "stdexcept"

#include "MaterialImpl.h"
#include "SceneResources.user.hpp"

namespace OHOS::Render3D::KITETS {
class SubMeshImpl {
public:
    SubMeshImpl(const std::shared_ptr<SubMeshETS> subMeshETS);
    ~SubMeshImpl();
    ::taihe::string getName();
    void setName(::taihe::string_view name);
    ::SceneResources::Material getMaterial();
    void setMaterial(::SceneResources::weak::Material mat);
    ::SceneTypes::Aabb getAabb();

private:
    std::shared_ptr<SubMeshETS> subMeshETS_;
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_SUB_MESH_IMPL_H