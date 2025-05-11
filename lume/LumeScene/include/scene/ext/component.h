
#ifndef SCENE_EXT_COMPONENT_H
#define SCENE_EXT_COMPONENT_H

#include <scene/base/namespace.h>
#include <scene/ext/ecs_lazy_property.h>
#include <scene/ext/intf_component.h>
#include <scene/ext/scene_property.h>

SCENE_BEGIN_NAMESPACE()

class Component : public META_NS::IntroduceInterfaces<EcsLazyProperty, IComponent> {
public:
    bool PopulateAllProperties() override
    {
        if (populated_) {
            return true;
        }
        populated_ =
            this->object_->AddAllProperties(this->template GetSelf<META_NS::IMetadata>(), this->GetName()).GetResult();
        return populated_;
    }

private:
    bool populated_ {};
};

inline META_NS::IProperty::ConstPtr GetComponentProperty(
    const META_NS::IAttach* obj, BASE_NS::string_view component, BASE_NS::string_view name)
{
    if (auto c = obj->GetAttachmentContainer(true)->FindByName(component)) {
        if (auto m = interface_cast<META_NS::IMetadata>(c)) {
            return m->GetProperty(name);
        }
    }
    return nullptr;
}

inline META_NS::IProperty::Ptr GetComponentProperty(
    META_NS::IAttach* obj, BASE_NS::string_view component, BASE_NS::string_view name)
{
    if (auto c = obj->GetAttachmentContainer(true)->FindByName(component)) {
        if (auto m = interface_cast<META_NS::IMetadata>(c)) {
            return m->GetProperty(name);
        }
    }
    return nullptr;
}

#define SCENE_USE_READONLY_COMPONENT_PROPERTY(type, name, component)        \
    ::META_NS::IProperty::ConstPtr Property##name() const noexcept override \
    {                                                                       \
        return GetComponentProperty(this, component, #name);                \
    }

#define SCENE_USE_COMPONENT_PROPERTY(type, name, component)      \
    SCENE_USE_READONLY_COMPONENT_PROPERTY(type, name, component) \
    ::META_NS::IProperty::Ptr Property##name() noexcept override \
    {                                                            \
        return GetComponentProperty(this, component, #name);     \
    }

// the type is not used, so just forward for now
#define SCENE_USE_ARRAY_COMPONENT_PROPERTY(type, name, component) SCENE_USE_COMPONENT_PROPERTY(type, name, component)

#define SCENE_USE_READONLY_ARRAY_COMPONENT_PROPERTY(type, name, component) \
    SCENE_USE_READONLY_COMPONENT_PROPERTY(type, name, component)

SCENE_END_NAMESPACE()

#endif
