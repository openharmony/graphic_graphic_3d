
#ifndef SCENE_INTERFACE_ILIGHT_H
#define SCENE_INTERFACE_ILIGHT_H

#include <scene/base/namespace.h>

#include <base/util/color.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief LightType enum defines the light type. Different types of lights use
 *        different properties
 */
enum class LightType : uint8_t { DIRECTIONAL = 0, POINT = 1, SPOT = 2 };

class ILight : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ILight, "cbf423b5-e12d-4433-8967-2e4e8c38e5a6")
public:
    /**
     * @brief Diffuse color of the light. Values from 0.0 to 1.0.
     */
    META_PROPERTY(BASE_NS::Color, Color)
    /**
     * @brief Intensity of the light.
     */
    META_PROPERTY(float, Intensity)
    /**
     * @brief Near plane distance from the light source.
     */
    META_PROPERTY(float, NearPlane)
    /**
     * @brief Shadow enabled.
     */
    META_PROPERTY(bool, ShadowEnabled)
    /**
     * @brief Shadow strength.
     */
    META_PROPERTY(float, ShadowStrength)
    /**
     * @brief Shadow depth bias.
     */
    META_PROPERTY(float, ShadowDepthBias)
    /**
     * @brief Shadow normal bias.
     */
    META_PROPERTY(float, ShadowNormalBias)
    /**
     * @brief Spotlight inner angle.
     */
    META_PROPERTY(float, SpotInnerAngle)
    /**
     * @brief Spotlight outer angle.
     */
    META_PROPERTY(float, SpotOuterAngle)
    /**
     * @brief Type of the light. Defaults to DIRECTIONAL.
     */
    META_PROPERTY(LightType, Type)
    /**
     * @brief Additional factor for e.g. shader customization.
     */
    META_PROPERTY(BASE_NS::Math::Vec4, AdditionalFactor)
    /**
     * @brief Defines a layer mask which affects lighting of layer objects. Default is all layer mask, and then the
     * light affects objects on all layers.
     */
    META_PROPERTY(uint64_t, LightLayerMask)
    /**
     * @brief Defines a layer mask which affects lighting of layer objects. Default is all layer mask, and then the
     * light affects objects on all layers.
     */
    META_PROPERTY(uint64_t, ShadowLayerMask)
};

META_REGISTER_CLASS(LightNode, "416287c6-c7f2-4047-ad32-d247db42aef0", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::ILight)
META_TYPE(SCENE_NS::LightType)

#endif
