
#ifndef SCENE_SRC_COMPONENT_GENERIC_COMPONENT_H
#define SCENE_SRC_COMPONENT_GENERIC_COMPONENT_H

#include <scene/ext/component.h>

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(GenericComponent, "1eb22ecd-b8dd-4a1c-a491-1d4ed83c5179", META_NS::ObjectCategoryBits::NO_CATEGORY)

class GenericComponent : public META_NS::IntroduceInterfaces<Component, IGenericComponent> {
    META_OBJECT(GenericComponent, ClassId::GenericComponent, IntroduceInterfaces)
public:
    bool Build(const META_NS::IMetadata::Ptr&) override;
    BASE_NS::string GetName() const override;

private:
    BASE_NS::string component_;
};

SCENE_END_NAMESPACE()

#endif