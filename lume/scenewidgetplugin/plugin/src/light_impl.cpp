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
#include <scene_plugin/api/light_uid.h>

#include <meta/ext/concrete_base_object.h>

#include "bind_templates.inl"
#include "node_impl.h"

namespace {
class LightImpl
    : public META_NS::ConcreteBaseMetaObjectFwd<LightImpl, NodeImpl, SCENE_NS::ClassId::Light, SCENE_NS::ILight> {
    static constexpr BASE_NS::string_view LIGHT_COMPONENT_NAME = "LightComponent";
    static constexpr size_t LIGHT_COMPONENT_NAME_LEN = LIGHT_COMPONENT_NAME.size() + 1;
    static constexpr BASE_NS::string_view LIGHT_COLOR = "LightComponent.color";
    static constexpr BASE_NS::string_view LIGHT_INTENSITY = "LightComponent.intensity";
    static constexpr BASE_NS::string_view LIGHT_NEARPLANE = "LightComponent.nearPlane";
    static constexpr BASE_NS::string_view LIGHT_SHADOWENABLED = "LightComponent.shadowEnabled";
    static constexpr BASE_NS::string_view LIGHT_SHADOWSTRENGTH = "LightComponent.shadowStrength";
    static constexpr BASE_NS::string_view LIGHT_SHADOWDEPTHBIAS = "LightComponent.shadowDepthBias";
    static constexpr BASE_NS::string_view LIGHT_SHADOWNORMALBIAS = "LightComponent.shadowNormalBias";
    static constexpr BASE_NS::string_view LIGHT_SPOTINNERANGLE = "LightComponent.spotInnerAngle";
    static constexpr BASE_NS::string_view LIGHT_SPOTOUTERANGLE = "LightComponent.spotOuterAngle";
    static constexpr BASE_NS::string_view LIGHT_TYPE = "LightComponent.type";
    static constexpr BASE_NS::string_view LIGHT_ADDITIONALFACTOR = "LightComponent.additionalFactor";
    static constexpr BASE_NS::string_view LIGHT_LAYERMASK = "LightComponent.lightLayerMask";
    static constexpr BASE_NS::string_view LIGHT_SHADOWLAYERMASK = "LightComponent.shadowLayerMask";

    bool Build(const IMetadata::Ptr& data) override
    {
        bool ret = false;
        if (ret = NodeImpl::Build(data); ret) {
            PropertyNameMask()[LIGHT_COMPONENT_NAME] = { LIGHT_COLOR.substr(LIGHT_COMPONENT_NAME_LEN),
                LIGHT_INTENSITY.substr(LIGHT_COMPONENT_NAME_LEN), LIGHT_NEARPLANE.substr(LIGHT_COMPONENT_NAME_LEN),
                LIGHT_SHADOWENABLED.substr(LIGHT_COMPONENT_NAME_LEN),
                LIGHT_SHADOWSTRENGTH.substr(LIGHT_COMPONENT_NAME_LEN),
                LIGHT_SHADOWDEPTHBIAS.substr(LIGHT_COMPONENT_NAME_LEN),
                LIGHT_SHADOWNORMALBIAS.substr(LIGHT_COMPONENT_NAME_LEN),
                LIGHT_SPOTINNERANGLE.substr(LIGHT_COMPONENT_NAME_LEN),
                LIGHT_SPOTOUTERANGLE.substr(LIGHT_COMPONENT_NAME_LEN), LIGHT_TYPE.substr(LIGHT_COMPONENT_NAME_LEN),
                LIGHT_ADDITIONALFACTOR.substr(LIGHT_COMPONENT_NAME_LEN),
                LIGHT_LAYERMASK.substr(LIGHT_COMPONENT_NAME_LEN),
                LIGHT_SHADOWLAYERMASK.substr(LIGHT_COMPONENT_NAME_LEN) };
            DisableInputHandling();
        }
        return ret;
    }

    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ILight, SCENE_NS::Color, Color, SCENE_NS::Colors::WHITE)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ILight, float, Intensity, 1.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ILight, float, NearPlane, 0.5f)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ILight, bool, ShadowEnabled, true)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ILight, float, ShadowStrength, 1.0f)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ILight, float, ShadowDepthBias, 0.005f)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ILight, float, ShadowNormalBias, 0.025f)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ILight, float, SpotInnerAngle, 0.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ILight, float, SpotOuterAngle, 0.78539816339f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ILight, uint8_t, Type,
        SCENE_NS::ILight::SceneLightType::SCENE_LIGHT_DIRECTIONAL)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ILight, BASE_NS::Math::Vec4, AdditionalFactor,
        BASE_NS::Math::Vec4(0.f, 0.f, 0.f, 0.f))
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ILight, uint64_t, LightLayerMask, CORE3D_NS::CORE_LAYER_FLAG_BIT_ALL)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ILight, uint64_t, ShadowLayerMask, CORE3D_NS::CORE_LAYER_FLAG_BIT_ALL)

    bool CompleteInitialization(const BASE_NS::string& path) override
    {
        if (!NodeImpl::CompleteInitialization(path)) {
            return false;
        }

        auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_);
        BASE_NS::vector<size_t> slots = { 0, 0, 1, 1, 2, 2 };
        BindSlottedChanges<BASE_NS::Math::Vec3, SCENE_NS::Color>(
            propHandler_, META_ACCESS_PROPERTY(Color), meta, LIGHT_COLOR, slots);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(Intensity), meta, LIGHT_INTENSITY);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(NearPlane), meta, LIGHT_NEARPLANE);
        BindChanges<bool>(propHandler_, META_ACCESS_PROPERTY(ShadowEnabled), meta, LIGHT_SHADOWENABLED);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(ShadowStrength), meta, LIGHT_SHADOWSTRENGTH);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(ShadowDepthBias), meta, LIGHT_SHADOWDEPTHBIAS);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(ShadowNormalBias), meta, LIGHT_SHADOWNORMALBIAS);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(SpotInnerAngle), meta, LIGHT_SPOTINNERANGLE);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(SpotOuterAngle), meta, LIGHT_SPOTOUTERANGLE);
        BindChanges<uint8_t>(propHandler_, META_ACCESS_PROPERTY(Type), meta, LIGHT_TYPE);
        BindChanges<BASE_NS::Math::Vec4>(
            propHandler_, META_ACCESS_PROPERTY(AdditionalFactor), meta, LIGHT_ADDITIONALFACTOR);
        BindChanges<uint64_t>(propHandler_, META_ACCESS_PROPERTY(LightLayerMask), meta, LIGHT_LAYERMASK);
        BindChanges<uint64_t>(propHandler_, META_ACCESS_PROPERTY(ShadowLayerMask), meta, LIGHT_SHADOWLAYERMASK);

        return true;
    }

    virtual ~LightImpl() {}
};
} // namespace
SCENE_BEGIN_NAMESPACE()
void RegisterLightImpl()
{
    META_NS::GetObjectRegistry().RegisterObjectType<LightImpl>();
}
void UnregisterLightImpl()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<LightImpl>();
}
SCENE_END_NAMESPACE()