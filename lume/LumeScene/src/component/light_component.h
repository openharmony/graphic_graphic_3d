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

#ifndef SCENE_SRC_COMPONENT_LIGHT_COMPONENT_H
#define SCENE_SRC_COMPONENT_LIGHT_COMPONENT_H

#include <scene/ext/component_fwd.h>
#include <scene/interface/intf_light.h>

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(LightComponent, "1e25f1c9-723d-4a5a-9cc1-770b0eade707", META_NS::ObjectCategoryBits::NO_CATEGORY)

class LightComponent : public META_NS::IntroduceInterfaces<ComponentFwd, ILight> {
    META_OBJECT(LightComponent, ClassId::LightComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ILight, BASE_NS::Color, Color, "LightComponent.color")
    SCENE_STATIC_PROPERTY_DATA(ILight, float, Intensity, "LightComponent.intensity")
    SCENE_STATIC_PROPERTY_DATA(ILight, float, NearPlane, "LightComponent.nearPlane")
    SCENE_STATIC_PROPERTY_DATA(ILight, bool, ShadowEnabled, "LightComponent.shadowEnabled")
    SCENE_STATIC_PROPERTY_DATA(ILight, float, ShadowStrength, "LightComponent.shadowStrength")
    SCENE_STATIC_PROPERTY_DATA(ILight, float, ShadowDepthBias, "LightComponent.shadowDepthBias")
    SCENE_STATIC_PROPERTY_DATA(ILight, float, ShadowNormalBias, "LightComponent.shadowNormalBias")
    SCENE_STATIC_PROPERTY_DATA(ILight, float, SpotInnerAngle, "LightComponent.spotInnerAngle")
    SCENE_STATIC_PROPERTY_DATA(ILight, float, SpotOuterAngle, "LightComponent.spotOuterAngle")
    SCENE_STATIC_PROPERTY_DATA(ILight, LightType, Type, "LightComponent.type")
    SCENE_STATIC_PROPERTY_DATA(ILight, BASE_NS::Math::Vec4, AdditionalFactor, "LightComponent.additionalFactor")
    SCENE_STATIC_PROPERTY_DATA(ILight, uint64_t, LightLayerMask, "LightComponent.lightLayerMask")
    SCENE_STATIC_PROPERTY_DATA(ILight, uint64_t, ShadowLayerMask, "LightComponent.shadowLayerMask")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::Color, Color)
    META_IMPLEMENT_PROPERTY(float, Intensity)
    META_IMPLEMENT_PROPERTY(float, NearPlane)
    META_IMPLEMENT_PROPERTY(bool, ShadowEnabled)
    META_IMPLEMENT_PROPERTY(float, ShadowStrength)
    META_IMPLEMENT_PROPERTY(float, ShadowDepthBias)
    META_IMPLEMENT_PROPERTY(float, ShadowNormalBias)
    META_IMPLEMENT_PROPERTY(float, SpotInnerAngle)
    META_IMPLEMENT_PROPERTY(float, SpotOuterAngle)
    META_IMPLEMENT_PROPERTY(LightType, Type)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec4, AdditionalFactor)
    META_IMPLEMENT_PROPERTY(uint64_t, LightLayerMask)
    META_IMPLEMENT_PROPERTY(uint64_t, ShadowLayerMask)

    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;

public:
    BASE_NS::string GetName() const override;
};

SCENE_END_NAMESPACE()

#endif