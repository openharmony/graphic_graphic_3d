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

#ifndef SCENE_SRC_COMPONENT_MATERIAL_COMPONENT_H
#define SCENE_SRC_COMPONENT_MATERIAL_COMPONENT_H

#include <scene/ext/component.h>
#include <scene/interface/intf_material.h>

#include <3d/ecs/components/material_component.h>

#include <meta/ext/object.h>

META_TYPE(CORE3D_NS::MaterialComponent::Type)
META_TYPE(CORE3D_NS::MaterialComponent::Shader)
META_TYPE(CORE3D_NS::MaterialComponent::TextureInfo)
META_TYPE(CORE3D_NS::MaterialComponent::RenderSort)

META_BEGIN_NAMESPACE()
template<bool B>
struct DefaultCompare<CORE3D_NS::MaterialComponent::Shader, B> {
    using T = CORE3D_NS::MaterialComponent::Shader;
    static constexpr bool Equal(const T& v1, const T& v2)
    {
        return v1.shader == v2.shader && v1.graphicsState == v2.graphicsState;
    }

    template<typename NewType>
    using Rebind = DefaultCompare<NewType>;
};
META_END_NAMESPACE()

SCENE_BEGIN_NAMESPACE()

class IInternalMaterial : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInternalMaterial, "63a98b32-f062-42d0-81f7-490f3290df8a")
public:
    META_PROPERTY(MaterialType, Type)
    META_PROPERTY(float, AlphaCutoff)
    META_PROPERTY(uint32_t, LightingFlags)

    META_PROPERTY(CORE3D_NS::MaterialComponent::RenderSort, RenderSort)

    META_PROPERTY(CORE3D_NS::MaterialComponent::Shader, MaterialShader)
    META_PROPERTY(CORE3D_NS::MaterialComponent::Shader, DepthShader)

    META_ARRAY_PROPERTY(CORE3D_NS::MaterialComponent::TextureInfo, Textures)

    META_PROPERTY(uint32_t, UseTexcoordSetBit)
    META_PROPERTY(uint32_t, CustomRenderSlotId)

    META_ARRAY_PROPERTY(CORE_NS::EntityReference, CustomResources)
    META_PROPERTY(CORE_NS::IPropertyHandle*, CustomProperties)

    struct ActiveTextureSlotInfo {
        size_t count {};
        struct TextureSlot {
            BASE_NS::string name;
        };
        BASE_NS::vector<TextureSlot> slots;
    };

    virtual ActiveTextureSlotInfo GetActiveTextureSlotInfo() = 0;
    virtual bool UpdateMetadata() = 0;
};

META_REGISTER_CLASS(MaterialComponent, "bc819033-2a5b-4182-bc7a-365ff8e33822", META_NS::ObjectCategoryBits::NO_CATEGORY)

class MaterialComponent : public META_NS::IntroduceInterfaces<Component, IInternalMaterial> {
    META_OBJECT(MaterialComponent, ClassId::MaterialComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_PROPERTY_DATA(IInternalMaterial, MaterialType, Type, "MaterialComponent.type")
    SCENE_STATIC_PROPERTY_DATA(IInternalMaterial, float, AlphaCutoff, "MaterialComponent.alphaCutoff")
    SCENE_STATIC_PROPERTY_DATA(IInternalMaterial, uint32_t, LightingFlags, "MaterialComponent.materialLightingFlags")
    SCENE_STATIC_PROPERTY_DATA(
        IInternalMaterial, CORE3D_NS::MaterialComponent::RenderSort, RenderSort, "MaterialComponent.renderSort")

    SCENE_STATIC_PROPERTY_DATA(
        IInternalMaterial, CORE3D_NS::MaterialComponent::Shader, MaterialShader, "MaterialComponent.materialShader")
    SCENE_STATIC_PROPERTY_DATA(
        IInternalMaterial, CORE3D_NS::MaterialComponent::Shader, DepthShader, "MaterialComponent.depthShader")

    SCENE_STATIC_ARRAY_PROPERTY_DATA(
        IInternalMaterial, CORE3D_NS::MaterialComponent::TextureInfo, Textures, "MaterialComponent.textures")

    SCENE_STATIC_PROPERTY_DATA(IInternalMaterial, uint32_t, UseTexcoordSetBit, "MaterialComponent.useTexcoordSetBit")
    SCENE_STATIC_PROPERTY_DATA(IInternalMaterial, uint32_t, CustomRenderSlotId, "MaterialComponent.customRenderSlotId")

    SCENE_STATIC_ARRAY_PROPERTY_DATA(
        IInternalMaterial, CORE_NS::EntityReference, CustomResources, "MaterialComponent.customResources")
    SCENE_STATIC_PROPERTY_DATA(
        IInternalMaterial, CORE_NS::IPropertyHandle*, CustomProperties, "MaterialComponent.customProperties")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(MaterialType, Type)
    META_IMPLEMENT_PROPERTY(float, AlphaCutoff)
    META_IMPLEMENT_PROPERTY(uint32_t, LightingFlags)
    META_IMPLEMENT_PROPERTY(CORE3D_NS::MaterialComponent::RenderSort, RenderSort)

    META_IMPLEMENT_PROPERTY(CORE3D_NS::MaterialComponent::Shader, MaterialShader)
    META_IMPLEMENT_PROPERTY(CORE3D_NS::MaterialComponent::Shader, DepthShader)

    META_IMPLEMENT_ARRAY_PROPERTY(CORE3D_NS::MaterialComponent::TextureInfo, Textures)

    META_IMPLEMENT_PROPERTY(uint32_t, UseTexcoordSetBit)
    META_IMPLEMENT_PROPERTY(uint32_t, CustomRenderSlotId)

    META_IMPLEMENT_ARRAY_PROPERTY(CORE_NS::EntityReference, CustomResources)
    META_IMPLEMENT_PROPERTY(CORE_NS::IPropertyHandle*, CustomProperties)

public:
    BASE_NS::string GetName() const override;
    ActiveTextureSlotInfo GetActiveTextureSlotInfo() override;
    bool UpdateMetadata() override;

private:
    struct CachedShader {
        RENDER_NS::RenderHandle shader {};
        uint64_t frameIndex {};
    };
    CachedShader cachedShader_;
};

SCENE_END_NAMESPACE()

#endif
