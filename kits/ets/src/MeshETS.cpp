/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "MeshETS.h"

namespace OHOS::Render3D {
MeshETS::MeshETS(const SCENE_NS::IMesh::Ptr mesh)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::MESH), mesh_(mesh)
{}

MeshETS::~MeshETS()
{
    mesh_.reset();
}

void MeshETS::Destroy()
{
    mesh_.reset();
}

META_NS::IObject::Ptr MeshETS::GetNativeObj() const
{
    auto mesh = mesh_.lock();
    if (!mesh) {
        CORE_LOG_E("empty mesh_!");
        return {};
    }
    return interface_pointer_cast<META_NS::IObject>(mesh);
}

std::vector<std::shared_ptr<SubMeshETS>> MeshETS::GetSubMeshes()
{
    auto mesh = mesh_.lock();
    if (!mesh) {
        return {};
    }
    BASE_NS::vector<SCENE_NS::ISubMesh::Ptr> sms = mesh->SubMeshes()->GetValue();
    std::vector<std::shared_ptr<SubMeshETS>> subs(sms.size());
    for (size_t i = 0; i < sms.size(); ++i) {
        subs[i] = std::make_shared<SubMeshETS>(sms[i]);
    }
    return subs;
}

BASE_NS::Math::Vec3 MeshETS::GetAABBMin()
{
    if (auto mesh = mesh_.lock()) {
        return META_NS::GetValue(mesh->AABBMin());
    } else {
        return BASE_NS::Math::ZERO_VEC3;
    }
}

BASE_NS::Math::Vec3 MeshETS::GetAABBMax()
{
    if (auto mesh = mesh_.lock()) {
        return META_NS::GetValue(mesh->AABBMax());
    } else {
        return BASE_NS::Math::ZERO_VEC3;
    }
}

std::shared_ptr<MaterialETS> MeshETS::GetMaterialOverride()
{
    CORE_LOG_E("GetMaterialOverride is not supported now");
    return nullptr;
}

void MeshETS::SetMaterialOverride(const std::shared_ptr<MaterialETS> &mat)
{
    SCENE_NS::IMaterial::Ptr material = mat ? mat->GetNativeMaterial() : nullptr;
    if (auto mesh = mesh_.lock()) {
        SCENE_NS::SetMaterialForAllSubMeshes(mesh, material);
    }
}
}  // namespace OHOS::Render3D