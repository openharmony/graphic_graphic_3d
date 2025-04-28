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

#include "mesh.h"

#include <3d/ecs/components/mesh_component.h>
#include <3d/util/intf_picking.h>

SCENE_BEGIN_NAMESPACE()

void Mesh::SetInternalMesh(IInternalMesh::Ptr m)
{
    mesh_ = std::move(m);
}

void Mesh::OnPropertyChanged(const META_NS::IProperty& p)
{
    if (p.GetName() == "MaterialOverride") {
        OnMaterialOverrideChanged();
    }
}

IEcsObject::Ptr Mesh::GetMeshEcsObject() const
{
    auto comp = interface_cast<IEcsObjectAccess>(mesh_);
    return comp ? comp->GetEcsObject() : nullptr;
}

Future<BASE_NS::vector<ISubMesh::Ptr>> Mesh::GetSubmeshes() const
{
    if (auto obj = GetMeshEcsObject()) {
        if (auto scene = obj->GetScene()) {
            return scene->AddTask([=] {
                BASE_NS::vector<ISubMesh::Ptr> res;
                for (auto&& v : mesh_->SubMeshes()->GetValue()) {
                    auto sm = META_NS::GetObjectRegistry().Create<ISubMesh>(ClassId::SubMesh);
                    if (auto i = interface_cast<IInternalSubMesh>(sm)) {
                        if (i->SetSubMesh(scene->GetEcsContext(), v)) {
                            res.push_back(BASE_NS::move(sm));
                        } else {
                            CORE_LOG_E("Failed to create submesh");
                        }
                    }
                }
                return res;
            });
        }
    }
    return Future<BASE_NS::vector<ISubMesh::Ptr>> {};
}

Future<bool> Mesh::SetSubmeshes(const BASE_NS::vector<ISubMesh::Ptr>& submeshes)
{
    const auto obj = GetMeshEcsObject();
    if (!mesh_ || !obj) {
        CORE_LOG_E("Unable to set submeshes: Mesh is uninitialized");
        return Future<bool> {};
    }
    auto scene = obj->GetScene();
    if (!scene) {
        CORE_LOG_E("Unable to set submeshes: Invalid object");
        return {};
    }
    const auto manager = obj->GetEngineValueManager();
    if (!manager) {
        CORE_LOG_E("Unable to set submeshes: No engine value manager");
        return Future<bool> {};
    }

    auto componentSubmeshes = ConstructComponentSubmeshes(submeshes);
    ApplyMaterialOverride(componentSubmeshes, true);
    mesh_->SubMeshes()->SetValue(componentSubmeshes);

    CORE3D_NS::MinAndMax minMax;
    for (const auto& submesh : componentSubmeshes) {
        minMax.minAABB = min(minMax.minAABB, submesh.aabbMin);
        minMax.maxAABB = max(minMax.maxAABB, submesh.aabbMax);
    }
    mesh_->AABBMin()->SetValue(minMax.minAABB);
    mesh_->AABBMax()->SetValue(minMax.maxAABB);

    return scene->AddTask([=]() -> bool {
        if (auto s = obj->GetScene()) {
            s->SyncProperty(mesh_->AABBMin(), META_NS::EngineSyncDirection::TO_ENGINE);
            s->SyncProperty(mesh_->AABBMax(), META_NS::EngineSyncDirection::TO_ENGINE);
            s->SyncProperty(mesh_->SubMeshes(), META_NS::EngineSyncDirection::TO_ENGINE);
        }
        return true;
    });
}

BASE_NS::vector<CORE3D_NS::MeshComponent::Submesh> Mesh::ConstructComponentSubmeshes(
    const BASE_NS::vector<ISubMesh::Ptr>& submeshes) const
{
    BASE_NS::vector<CORE3D_NS::MeshComponent::Submesh> componentSubmeshes;
    componentSubmeshes.reserve(submeshes.size());
    for (auto&& submesh : submeshes) {
        if (const auto internalSubMesh = interface_cast<IInternalSubMesh>(submesh)) {
            componentSubmeshes.emplace_back(internalSubMesh->GetSubMesh());
        }
    }
    return componentSubmeshes;
}

void Mesh::ApplyMaterialOverride(BASE_NS::vector<CORE3D_NS::MeshComponent::Submesh>& submeshes, bool forceStore)
{
    const auto materialOverride = interface_cast<IEcsObjectAccess>(MaterialOverride()->GetValue());
    if (!materialOverride) {
        return;
    }
    std::unique_lock lock { mutex_ };
    const auto materialOverrideEntity = materialOverride->GetEcsObject()->GetEntity();
    bool store = forceStore || submeshMaterials_.empty();
    submeshMaterials_.resize(submeshes.size());
    size_t i = 0;
    for (auto&& submesh : submeshes) {
        if (store) {
            submeshMaterials_.at(i) = submesh.material;
        }
        submesh.material = materialOverrideEntity;
        i++;
    }
}

void Mesh::RestoreOriginalMaterials(BASE_NS::vector<CORE3D_NS::MeshComponent::Submesh>& submeshes)
{
    std::unique_lock lock { mutex_ };
    if (!submeshMaterials_.empty()) {
        if (submeshes.size() != submeshMaterials_.size()) {
            CORE_LOG_W("submesh size not matching saved submesh materials?!");
        }
        size_t i = 0;
        for (auto&& submesh : submeshes) {
            submesh.material = submeshMaterials_.at(i);
            ++i;
        }
        submeshMaterials_.clear();
    }
}

void Mesh::OnMaterialOverrideChanged()
{
    auto material = META_NS::GetValue(MaterialOverride());
    auto p = mesh_->SubMeshes().GetLockedAccess();
    auto values = p->GetValue();
    if (material) {
        ApplyMaterialOverride(values);
    } else {
        RestoreOriginalMaterials(values);
    }
    p->SetValue(values);
}

bool Mesh::SetEcsObject(const IEcsObject::Ptr& obj)
{
    if (Super::SetEcsObject(obj)) {
        if (auto att = GetSelf<META_NS::IAttach>()) {
            if (auto attc = att->GetAttachmentContainer(false)) {
                if (auto c = attc->FindByName<IInternalMesh>("MeshComponent")) {
                    SetInternalMesh(c);
                }
            }
        }
        if (!mesh_) {
            auto p = META_NS::GetObjectRegistry().Create<IInternalMesh>(ClassId::MeshComponent);
            if (auto acc = interface_cast<IEcsObjectAccess>(p)) {
                if (acc->SetEcsObject(obj)) {
                    SetInternalMesh(p);
                    if (auto scene = obj->GetScene()) {
                        meshEntity_ = scene->GetEcsContext().GetEntityReference(obj->GetEntity());
                    }
                }
            }
        }
    }
    return mesh_ != nullptr;
}

CORE_NS::Entity Mesh::CreateEntity(const IInternalScene::Ptr& scene)
{
    const auto& ecs = scene->GetEcsContext().GetNativeEcs();
    const auto meshManager = CORE_NS::GetManager<CORE3D_NS::IMeshComponentManager>(*ecs);
    if (!meshManager) {
        return {};
    }
    auto ent = ecs->GetEntityManager().Create();
    meshManager->Create(ent);
    return ent;
}

SCENE_END_NAMESPACE()