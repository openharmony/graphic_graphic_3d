
#ifndef SCENE_SRC_MESH_SUBMESH_H
#define SCENE_SRC_MESH_SUBMESH_H

#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_shader.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

#include "../component/mesh_component.h"
#include "../util_interfaces.h"

SCENE_BEGIN_NAMESPACE()

class SubMesh : public META_NS::IntroduceInterfaces<EcsLazyProperty, ArrayElementIndex, META_NS::INotifyOnChange,
                    META_NS::INamed, META_NS::IPropertyOwner, ISubMesh> {
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

private:
    BASE_NS::shared_ptr<META_NS::EventImpl<META_NS::IOnChanged>> event_ { new META_NS::EventImpl<META_NS::IOnChanged>(
        "OnChanged") };
};

SCENE_END_NAMESPACE()

#endif