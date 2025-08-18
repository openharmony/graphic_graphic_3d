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

#include "SubMeshETS.h"

namespace OHOS::Render3D {
SubMeshETS::SubMeshETS(const SCENE_NS::ISubMesh::Ptr subMesh) : subMesh_(subMesh)
{
    CORE_LOG_D("SubMeshETS ++");
}

SubMeshETS::~SubMeshETS()
{
    CORE_LOG_D("SubMeshETS --");
    subMesh_.reset();
}

BASE_NS::string SubMeshETS::GetName()
{
    if (auto named = interface_cast<META_NS::INamed>(subMesh_)) {
        return META_NS::GetValue(named->Name());
    } else {
        return "";
    }
}

void SubMeshETS::SetName(const BASE_NS::string &name)
{
    if (auto named = interface_cast<META_NS::INamed>(subMesh_)) {
        named->Name()->SetValue(name);
    }
}

BASE_NS::Math::Vec3 SubMeshETS::GetAABBMin()
{
    if (subMesh_) {
        return META_NS::GetValue(subMesh_->AABBMin());
    } else {
        return BASE_NS::Math::ZERO_VEC3;
    }
}

BASE_NS::Math::Vec3 SubMeshETS::GetAABBMax()
{
    if (subMesh_) {
        return META_NS::GetValue(subMesh_->AABBMax());
    } else {
        return BASE_NS::Math::ZERO_VEC3;
    }
}

SCENE_NS::IMaterial::Ptr SubMeshETS::GetMaterial()
{
    if (subMesh_) {
        return META_NS::GetValue(subMesh_->Material());
    } else {
        return nullptr;
    }
}

void SubMeshETS::SetMaterial(const SCENE_NS::IMaterial::Ptr &material)
{
    if (subMesh_) {
        META_NS::SetValue(subMesh_->Material(), material);
    }
}
}  // namespace OHOS::Render3D
