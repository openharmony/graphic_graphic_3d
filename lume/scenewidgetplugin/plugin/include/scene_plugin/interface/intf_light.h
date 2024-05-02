/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef SCENEPLUGIN_INTF_LIGHT_H
#define SCENEPLUGIN_INTF_LIGHT_H

#include <scene_plugin/interface/intf_node.h>

#include <meta/base/types.h>

#include <scene_plugin/interface/compatibility.h>


SCENE_BEGIN_NAMESPACE()

// Implemented by SCENE_NS::ClassId::Light
REGISTER_INTERFACE(ILight, "63db8e75-3faa-4439-b346-23db8818370a")
class ILight : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ILight, InterfaceId::ILight)
public:
    /**
     * @brief SceneLightType enum defines the light type. Different types of lights use
     *        different properties
     */
    enum SceneLightType { SCENE_LIGHT_DIRECTIONAL = 0, SCENE_LIGHT_POINT = 1, SCENE_LIGHT_SPOT = 2 };
    /**
     * @brief Diffuse color of the light. Values from 0.0 to 1.0.
     * @return property pointer
     */
    META_PROPERTY(SCENE_NS::Color, Color)

    /**
     * @brief Intensity of the light.
     * @return property pointer
     */
    META_PROPERTY(float, Intensity)

    /**
     * @brief Near plane distance from the light source.
     * @return property pointer
     */
    META_PROPERTY(float, NearPlane)

    /**
     * @brief Shadow enabled.
     * @return property pointer
     */
    META_PROPERTY(bool, ShadowEnabled)

    /**
     * @brief Shadow strength.
     * @return property pointer
     */
    META_PROPERTY(float, ShadowStrength)

    /**
     * @brief Shadow depth bias.
     * @return property pointer
     */
    META_PROPERTY(float, ShadowDepthBias)

    /**
     * @brief Shadow normal bias.
     * @return property pointer
     */
    META_PROPERTY(float, ShadowNormalBias)

    /**
     * @brief Spotlight inner angle.
     * @return property pointer
     */
    META_PROPERTY(float, SpotInnerAngle)

    /**
     * @brief Spotlight outer angle.
     * @return property pointer
     */
    META_PROPERTY(float, SpotOuterAngle)

    /**
     * @brief Type of the light. Defaults to SCENE_LIGHT_DIRECTIONAL.
     * @return
     */
    META_PROPERTY(uint8_t, Type)

    /**
     * @brief Additional factor for e.g. shader customization.
     * @return property pointer
     */
    META_PROPERTY(BASE_NS::Math::Vec4, AdditionalFactor)

    /**
     * @brief Defines a layer mask which affects lighting of layer objects. Default is all layer mask, and then the
     * light affects objects on all layers.
     * @return
     */
    META_PROPERTY(uint64_t, LightLayerMask)

    /**
     * @brief Defines a layer mask which affects lighting of layer objects. Default is all layer mask, and then the
     * light affects objects on all layers.
     * @return
     */
    META_PROPERTY(uint64_t, ShadowLayerMask)
};
SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::ILight::WeakPtr);
META_TYPE(SCENE_NS::ILight::Ptr);

#endif // SCENEPLUGIN_INTF_LIGHT_H
