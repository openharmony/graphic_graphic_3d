
#include "environment_component.h"

#include "../entity_converting_value.h"

SCENE_BEGIN_NAMESPACE()

BASE_NS::string EnvironmentComponent::GetName() const
{
    return "EnvironmentComponent";
}

bool EnvironmentComponent::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (p->GetName() == "RadianceImage" || p->GetName() == "EnvironmentImage") {
        auto ep = object_->CreateProperty(path).GetResult();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i &&
               i->PushValue(
                   META_NS::IValue::Ptr(new RenderResourceValue<IImage>(ep, { object_->GetScene(), ClassId::Image })));
    }
    return AttachEngineProperty(p, path);
}

SCENE_END_NAMESPACE()
