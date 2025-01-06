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

#ifndef SCENE_SRC_COMPONENT_MESH_COMPONENT_H
#define SCENE_SRC_COMPONENT_MESH_COMPONENT_H

#include <scene/ext/component_fwd.h>
#include <scene/interface/intf_mesh.h>

#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/render_mesh_component.h>

#include <meta/ext/object.h>

META_TYPE(CORE3D_NS::MeshComponent::Submesh)

SCENE_BEGIN_NAMESPACE()

class IInternalSubMesh : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInternalSubMesh, "1f943376-8861-4438-b5da-497f28c21e5a")
public:
    virtual CORE3D_NS::MeshComponent::Submesh GetSubMesh() const = 0;
    virtual bool SetSubMesh(IEcsContext&, const CORE3D_NS::MeshComponent::Submesh&) = 0;
};

class IInternalMesh : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInternalMesh, "8926f642-01ee-411f-8fa0-c17d9cda4bb8")
public:
    META_ARRAY_PROPERTY(CORE3D_NS::MeshComponent::Submesh, SubMeshes)
    META_ARRAY_PROPERTY(float, JointBounds)
    META_PROPERTY(BASE_NS::Math::Vec3, AABBMin)
    META_PROPERTY(BASE_NS::Math::Vec3, AABBMax)
};

META_REGISTER_CLASS(MeshComponent, "3350d990-8d83-4d29-b916-db9854c7fa6f", META_NS::ObjectCategoryBits::NO_CATEGORY)

class MeshComponent : public META_NS::IntroduceInterfaces<ComponentFwd, IInternalMesh> {
    META_OBJECT(MeshComponent, ClassId::MeshComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_ARRAY_PROPERTY_DATA(
        IInternalMesh, CORE3D_NS::MeshComponent::Submesh, SubMeshes, "MeshComponent.submeshes")
    SCENE_STATIC_ARRAY_PROPERTY_DATA(IInternalMesh, float, JointBounds, "MeshComponent.jointBounds")
    SCENE_STATIC_PROPERTY_DATA(IInternalMesh, BASE_NS::Math::Vec3, AABBMin, "MeshComponent.aabbMin")
    SCENE_STATIC_PROPERTY_DATA(IInternalMesh, BASE_NS::Math::Vec3, AABBMax, "MeshComponent.aabbMax")
    META_END_STATIC_DATA()

    META_IMPLEMENT_ARRAY_PROPERTY(CORE3D_NS::MeshComponent::Submesh, SubMeshes)
    META_IMPLEMENT_ARRAY_PROPERTY(float, JointBounds)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, AABBMin)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, AABBMax)
public:
    BASE_NS::string GetName() const override;
};

class IInternalRenderMesh : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInternalRenderMesh, "2031ecad-eb1f-4017-8ebb-d4d2880a397f")
public:
    META_PROPERTY(CORE_NS::Entity, Mesh)
};

META_REGISTER_CLASS(
    RenderMeshComponent, "af2f8d45-b34d-4598-8374-ed48a8db69af", META_NS::ObjectCategoryBits::NO_CATEGORY)

class RenderMeshComponent : public META_NS::IntroduceInterfaces<ComponentFwd, IInternalRenderMesh> {
    META_OBJECT(RenderMeshComponent, SCENE_NS::ClassId::RenderMeshComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_PROPERTY_DATA(IInternalRenderMesh, CORE_NS::Entity, Mesh, "RenderMeshComponent.mesh")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(CORE_NS::Entity, Mesh)
public:
    BASE_NS::string GetName() const override;
};

SCENE_END_NAMESPACE()

#endif