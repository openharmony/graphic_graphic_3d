
#ifndef SCENE_SRC_POSTPROCESS_TONEMAP_H
#define SCENE_SRC_POSTPROCESS_TONEMAP_H

#include <scene/ext/ecs_lazy_property.h>
#include <scene/ext/scene_property.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>
#include <scene/interface/postprocess/intf_tonemap.h>

#include <meta/ext/implementation_macros.h>

#include "util.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(Tonemap, "56c1fc6e-90aa-486d-846e-c9a5c780a90a", META_NS::ObjectCategoryBits::NO_CATEGORY)

class Tonemap : public META_NS::IntroduceInterfaces<EcsLazyProperty, ITonemap, IPPEffectInit> {
    META_OBJECT(Tonemap, ClassId::Tonemap, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcessEffect, bool, Enabled, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITonemap, TonemapType, Type, "tonemapType")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITonemap, float, Exposure, "exposure")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(TonemapType, Type)
    META_IMPLEMENT_PROPERTY(float, Exposure)

    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;
    bool Init(const META_NS::IProperty::Ptr& flags) override
    {
        flags_ = flags;
        return static_cast<bool>(flags_);
    }

private:
    META_NS::Property<uint32_t> flags_;
};

SCENE_END_NAMESPACE()

#endif