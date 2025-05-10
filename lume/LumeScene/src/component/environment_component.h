
#ifndef SCENE_SRC_COMPONENT_ENVIRONMENT_COMPONENT_H
#define SCENE_SRC_COMPONENT_ENVIRONMENT_COMPONENT_H

#include <scene/ext/component.h>
#include <scene/interface/intf_environment.h>

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(
    EnvironmentComponent, "0dc6c2ca-ed0e-4ee5-bdde-91eeb7a67b91", META_NS::ObjectCategoryBits::NO_CATEGORY)

class EnvironmentComponent : public META_NS::IntroduceInterfaces<Component, IEnvironment> {
    META_OBJECT(EnvironmentComponent, ClassId::EnvironmentComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_PROPERTY_DATA(IEnvironment, EnvBackgroundType, Background, "EnvironmentComponent.background")
    SCENE_STATIC_PROPERTY_DATA(
        IEnvironment, BASE_NS::Math::Vec4, IndirectDiffuseFactor, "EnvironmentComponent.indirectDiffuseFactor")
    SCENE_STATIC_PROPERTY_DATA(
        IEnvironment, BASE_NS::Math::Vec4, IndirectSpecularFactor, "EnvironmentComponent.indirectSpecularFactor")
    SCENE_STATIC_PROPERTY_DATA(IEnvironment, BASE_NS::Math::Vec4, EnvMapFactor, "EnvironmentComponent.envMapFactor")
    SCENE_STATIC_PROPERTY_DATA(
        IEnvironment, uint32_t, RadianceCubemapMipCount, "EnvironmentComponent.radianceCubemapMipCount")
    SCENE_STATIC_PROPERTY_DATA(IEnvironment, float, EnvironmentMapLodLevel, "EnvironmentComponent.envMapLodLevel")
    SCENE_STATIC_PROPERTY_DATA(
        IEnvironment, BASE_NS::Math::Quat, EnvironmentRotation, "EnvironmentComponent.environmentRotation")
    SCENE_STATIC_ARRAY_PROPERTY_DATA(
        IEnvironment, BASE_NS::Math::Vec3, IrradianceCoefficients, "EnvironmentComponent.irradianceCoefficients")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IEnvironment, IImage::Ptr, RadianceImage, "EnvironmentComponent.radianceCubemap")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IEnvironment, IImage::Ptr, EnvironmentImage, "EnvironmentComponent.envMap")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(EnvBackgroundType, Background)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec4, IndirectDiffuseFactor)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec4, IndirectSpecularFactor)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec4, EnvMapFactor)
    META_IMPLEMENT_PROPERTY(uint32_t, RadianceCubemapMipCount)
    META_IMPLEMENT_PROPERTY(float, EnvironmentMapLodLevel)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Quat, EnvironmentRotation)
    META_IMPLEMENT_ARRAY_PROPERTY(BASE_NS::Math::Vec3, IrradianceCoefficients)
    META_IMPLEMENT_PROPERTY(IImage::Ptr, RadianceImage)
    META_IMPLEMENT_PROPERTY(IImage::Ptr, EnvironmentImage)

    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;

public:
    BASE_NS::string GetName() const override;
};

SCENE_END_NAMESPACE()

#endif
