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

#ifndef SCENE_SRC_MESH_MESH_H
#define SCENE_SRC_MESH_MESH_H

#include <mutex>
#include <scene/ext/intf_create_entity.h>
#include <scene/ext/named_scene_object.h>
#include <scene/interface/intf_mesh.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/interface/intf_owner.h>

#include "../component/mesh_component.h"

SCENE_BEGIN_NAMESPACE()

class Mesh : public META_NS::IntroduceInterfaces<NamedSceneObject, IMesh, ICreateEntity, META_NS::IPropertyOwner> {
    META_OBJECT(Mesh, ClassId::Mesh, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IMesh, IMaterial::Ptr, MaterialOverride)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(IMaterial::Ptr, MaterialOverride)
    META_FORWARD_READONLY_PROPERTY(BASE_NS::Math::Vec3, AABBMin, mesh_->AABBMin())
    META_FORWARD_READONLY_PROPERTY(BASE_NS::Math::Vec3, AABBMax, mesh_->AABBMax())

    Future<BASE_NS::vector<ISubMesh::Ptr>> GetSubmeshes() const override;
    Future<bool> SetSubmeshes(const BASE_NS::vector<ISubMesh::Ptr>&) override;

public:
    bool SetEcsObject(const IEcsObject::Ptr&) override;
    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;

private:
    void OnPropertyChanged(const META_NS::IProperty&) override;

    void SetInternalMesh(IInternalMesh::Ptr m);
    BASE_NS::vector<CORE3D_NS::MeshComponent::Submesh> ConstructComponentSubmeshes(
        const BASE_NS::vector<ISubMesh::Ptr>& submeshes) const;
    void ApplyMaterialOverride(BASE_NS::vector<CORE3D_NS::MeshComponent::Submesh>& submeshes, bool forceStore = false);
    void RestoreOriginalMaterials(BASE_NS::vector<CORE3D_NS::MeshComponent::Submesh>& submeshes);
    void OnMaterialOverrideChanged();

    IEcsObject::Ptr GetMeshEcsObject() const;

protected:
    mutable std::mutex mutex_;
    IInternalMesh::Ptr mesh_;
    BASE_NS::vector<CORE_NS::Entity> submeshMaterials_;
};

SCENE_END_NAMESPACE()

#endif