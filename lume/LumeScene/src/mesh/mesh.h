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
#include <scene/ext/ecs_lazy_property.h>
#include <scene/ext/intf_create_entity.h>
#include <scene/interface/intf_mesh.h>

#include <meta/api/event_handler.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/resource/resource.h>
#include <meta/interface/intf_owner.h>

#include "../component/mesh_component.h"

SCENE_BEGIN_NAMESPACE()

class Mesh : public META_NS::IntroduceInterfaces<NamedSceneObject, IMesh, ICreateEntity, CORE_NS::IResource,
                 IMaterialOverride> {
    META_OBJECT(Mesh, ClassId::Mesh, IntroduceInterfaces)
public:
    void Destroy() override;

    META_BEGIN_STATIC_DATA()
    META_STATIC_FORWARDED_PROPERTY_DATA(IMesh, BASE_NS::Math::Vec3, AABBMin)
    META_STATIC_FORWARDED_PROPERTY_DATA(IMesh, BASE_NS::Math::Vec3, AABBMax)
    SCENE_STATIC_DYNINIT_ARRAY_PROPERTY_DATA(IMesh, ISubMesh::Ptr, SubMeshes, "")
    META_END_STATIC_DATA()

    META_FORWARD_READONLY_PROPERTY(BASE_NS::Math::Vec3, AABBMin, mesh_->AABBMin())
    META_FORWARD_READONLY_PROPERTY(BASE_NS::Math::Vec3, AABBMax, mesh_->AABBMax())
    META_IMPLEMENT_READONLY_ARRAY_PROPERTY(ISubMesh::Ptr, SubMeshes)

    void SetMaterialOverride(const IMaterial::Ptr& material) override;
    IMaterial::Ptr GetMaterialOverride() const override;

public:
    bool SetEcsObject(const IEcsObject::Ptr&) override;
    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;
    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;

public: // IResource (empty implementation as placeholder, needed to initialize API Mesh resource with IMesh::Ptr
    CORE_NS::ResourceType GetResourceType() const override
    {
        return {};
    }
    CORE_NS::ResourceId GetResourceId() const override
    {
        return {};
    }

private:
    void SetInternalMesh(IInternalMesh::Ptr m);
    BASE_NS::vector<CORE3D_NS::MeshComponent::Submesh> ConstructComponentSubmeshes(
        const BASE_NS::vector<ISubMesh::Ptr>& submeshes) const;

    IEcsObject::Ptr GetMeshEcsObject() const;
    void RecalculateAABB();
    bool UpdateSubmeshes(META_NS::ArrayProperty<ISubMesh::Ptr> prop);
    void RefreshSubmeshes(const BASE_NS::vector<ISubMesh::Ptr>& subs);
    META_NS::ArrayProperty<ISubMesh::Ptr> GetSubmeshesOrNull();

protected:
    mutable std::mutex mutex_;
    CORE_NS::EntityReference meshEntity_;
    IInternalMesh::Ptr mesh_;
    META_NS::EventHandler submeshEvent_;
    IMaterial::Ptr overrideMaterial_;
};

SCENE_END_NAMESPACE()

#endif
