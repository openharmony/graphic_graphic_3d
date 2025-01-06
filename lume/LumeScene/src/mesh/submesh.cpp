/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "submesh.h"

#include "../util.h"

SCENE_BEGIN_NAMESPACE()

BASE_NS::string SubMesh::GetName() const
{
    return Name()->GetValue();
}

CORE3D_NS::MeshComponent::Submesh SubMesh::GetSubMesh() const
{
    submesh_.aabbMin = AABBMin()->GetValue();
    submesh_.aabbMax = AABBMax()->GetValue();
    if (auto mat = interface_cast<IEcsObjectAccess>(Material()->GetValue())) {
        submesh_.material = mat->GetEcsObject()->GetEntity();
    } else {
        submesh_.material = CORE_NS::Entity {};
    }
    return submesh_;
}
bool SubMesh::SetSubMesh(IEcsContext& context, const CORE3D_NS::MeshComponent::Submesh& sm)
{
    submesh_ = sm;
    AABBMin()->SetValue(submesh_.aabbMin);
    AABBMax()->SetValue(submesh_.aabbMax);
    if (CORE_NS::EntityUtil::IsValid(submesh_.material)) {
        if (auto obj = context.GetEcsObject(submesh_.material)) {
            if (auto mat = META_NS::GetObjectRegistry().Create<IMaterial>(ClassId::Material)) {
                if (auto i = interface_cast<IEcsObjectAccess>(mat)) {
                    if (!i->SetEcsObject(obj)) {
                        return false;
                    }
                }
                Material()->SetValue(mat);
            }
        }
    }
    return true;
}

SCENE_END_NAMESPACE()
