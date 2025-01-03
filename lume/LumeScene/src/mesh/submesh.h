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

#ifndef SCENE_SRC_MESH_SUBMESH_H
#define SCENE_SRC_MESH_SUBMESH_H

#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_shader.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

#include "../component/mesh_component.h"

SCENE_BEGIN_NAMESPACE()


class SubMesh : public META_NS::IntroduceInterfaces<META_NS::MetaObject, ISubMesh, IInternalSubMesh, META_NS::INamed> {
    META_OBJECT(SubMesh, ClassId::SubMesh, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_STATIC_PROPERTY_DATA(ISubMesh, IMaterial::Ptr, Material)
    META_STATIC_PROPERTY_DATA(ISubMesh, BASE_NS::Math::Vec3, AABBMin)
    META_STATIC_PROPERTY_DATA(ISubMesh, BASE_NS::Math::Vec3, AABBMax)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)
    META_IMPLEMENT_PROPERTY(IMaterial::Ptr, Material)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, AABBMin)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, AABBMax)

    BASE_NS::string GetName() const override;

    CORE3D_NS::MeshComponent::Submesh GetSubMesh() const override;
    bool SetSubMesh(IEcsContext&, const CORE3D_NS::MeshComponent::Submesh&) override;

private:
    mutable CORE3D_NS::MeshComponent::Submesh submesh_;
};

SCENE_END_NAMESPACE()

#endif