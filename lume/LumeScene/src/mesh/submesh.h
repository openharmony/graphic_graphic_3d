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

#include <scene/ext/component.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_shader.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

#include "../util_interfaces.h"

SCENE_BEGIN_NAMESPACE()

class ISubMeshInternal : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISubMeshInternal, "3ed7de72-52b3-426b-96d6-cb2bdd6620d0")
public:
    /// Inform submesh about material override. Must be called in engine thread
    virtual void SetMaterialOverride(const IMaterial::Ptr& material) = 0;
    /// Returns the material override set with SetMaterialOverride(). Can be called from any thread.
    virtual IMaterial::Ptr GetMaterialOverride() const = 0;
};

class SubMesh : public META_NS::IntroduceInterfaces<EcsLazyProperty, ArrayElementIndex, META_NS::INotifyOnChange,
                    META_NS::INamed, META_NS::IPropertyOwner, ISubMesh, ISubMeshInternal> {
    META_OBJECT(SubMesh, ClassId::SubMesh, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ISubMesh, IMaterial::Ptr, Material, "material")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ISubMesh, BASE_NS::Math::Vec3, AABBMin, "aabbMin")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ISubMesh, BASE_NS::Math::Vec3, AABBMax, "aabbMax")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)
    META_IMPLEMENT_PROPERTY(IMaterial::Ptr, Material)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, AABBMin)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, AABBMax)

    bool SetEcsObject(const IEcsObject::Ptr& ecso) override;
    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;
    void OnPropertyChanged(const META_NS::IProperty&) override;

    BASE_NS::string GetName() const override
    {
        return META_NS::GetValue(Name());
    }

    BASE_NS::shared_ptr<META_NS::IEvent> EventOnChanged(META_NS::MetadataQuery) const override
    {
        return event_;
    }

public: // ISubMeshInternal
    void SetMaterialOverride(const IMaterial::Ptr& material) override;
    IMaterial::Ptr GetMaterialOverride() const override;

private:
    BASE_NS::shared_ptr<META_NS::EventImpl<META_NS::IOnChanged>> event_ { new META_NS::EventImpl<META_NS::IOnChanged>(
        "OnChanged") };

    IInternalScene::Ptr GetScene() const;

    META_NS::IAny::Ptr overrideAny_;
    IMaterial::WeakPtr overrideMaterial_; // Override material set to Mesh
};

SCENE_END_NAMESPACE()

#endif
