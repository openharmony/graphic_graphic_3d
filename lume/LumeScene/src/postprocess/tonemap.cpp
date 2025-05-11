
#include "tonemap.h"

#include <3d/ecs/components/post_process_component.h>

SCENE_BEGIN_NAMESPACE()

bool Tonemap::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    BASE_NS::string cpath = "PostProcessComponent.tonemapConfiguration." + path;
    if (p->GetName() == "Enabled") {
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return flags_ && i &&
               i->PushValue(META_NS::IValue::Ptr(
                   new ConvertingValue<PPEffectEnabledConverter<CORE3D_NS::PostProcessComponent::TONEMAP_BIT>>(
                       flags_, { flags_ })));
    }
    return AttachEngineProperty(p, cpath);
}

SCENE_END_NAMESPACE()