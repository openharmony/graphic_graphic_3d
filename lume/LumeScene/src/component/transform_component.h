
#ifndef SCENE_SRC_COMPONENT_TRANSFORM_COMPONENT_H
#define SCENE_SRC_COMPONENT_TRANSFORM_COMPONENT_H

#include <scene/ext/component.h>
#include <scene/interface/intf_transform.h>

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(
    TransformComponent, "534bdd95-3137-4f2f-89ab-0d795560facb", META_NS::ObjectCategoryBits::NO_CATEGORY)

class TransformComponent : public META_NS::IntroduceInterfaces<Component, ITransform> {
    META_OBJECT(TransformComponent, ClassId::TransformComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_PROPERTY_DATA(ITransform, BASE_NS::Math::Vec3, Position, "TransformComponent.position")
    SCENE_STATIC_PROPERTY_DATA(ITransform, BASE_NS::Math::Vec3, Scale, "TransformComponent.scale")
    SCENE_STATIC_PROPERTY_DATA(ITransform, BASE_NS::Math::Quat, Rotation, "TransformComponent.rotation")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, Position)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, Scale)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Quat, Rotation)

public:
    BASE_NS::string GetName() const override;
};

SCENE_END_NAMESPACE()

#endif