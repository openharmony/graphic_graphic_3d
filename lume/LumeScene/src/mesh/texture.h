
#ifndef SCENE_SRC_MESH_TEXTURE_H
#define SCENE_SRC_MESH_TEXTURE_H

#include <scene/ext/component.h>
#include <scene/ext/ecs_lazy_property.h>
#include <scene/ext/intf_create_entity.h>
#include <scene/interface/intf_texture.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

#include "../util_interfaces.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(Texture, "07e6dc19-57ca-4a3a-aa71-a53db9bf2e58", META_NS::ObjectCategoryBits::NO_CATEGORY)

class Texture : public META_NS::IntroduceInterfaces<EcsLazyProperty, ArrayElementIndex, META_NS::INamed, ITexture> {
    META_OBJECT(Texture, ClassId::Texture, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITexture, ISampler::Ptr, Sampler, "sampler")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITexture, IImage::Ptr, Image, "image")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITexture, BASE_NS::Math::Vec4, Factor, "factor")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITexture, BASE_NS::Math::Vec2, Translation, "transform.translation")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITexture, float, Rotation, "transform.rotation")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITexture, BASE_NS::Math::Vec2, Scale, "transform.scale")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)
    META_IMPLEMENT_READONLY_PROPERTY(ISampler::Ptr, Sampler)
    META_IMPLEMENT_PROPERTY(IImage::Ptr, Image)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec4, Factor)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec2, Translation)
    META_IMPLEMENT_PROPERTY(float, Rotation)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec2, Scale)

    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;

    BASE_NS::string GetName() const override
    {
        return META_NS::GetValue(Name());
    }
};

SCENE_END_NAMESPACE()

#endif
