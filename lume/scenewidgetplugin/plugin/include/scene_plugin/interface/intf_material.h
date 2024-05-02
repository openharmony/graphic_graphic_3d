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
#ifndef SCENEPLUGIN_INTF_MATERIAL_H
#define SCENEPLUGIN_INTF_MATERIAL_H

#include <scene_plugin/interface/intf_node.h>

#include <base/containers/vector.h>
#include <render/device/intf_shader_manager.h>
#include <render/resource_handle.h>

#include <meta/api/animation/animation.h>
#include <meta/base/meta_types.h>
#include <meta/interface/intf_container.h>

#include <scene_plugin/interface/compatibility.h>
#include <scene_plugin/interface/intf_bitmap.h>

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(ITextureInfo, "ab86c6e2-fac5-4425-b13a-16459bf51473")
class ITextureInfo : public META_NS::INamed {
    META_INTERFACE(META_NS::INamed, ITextureInfo, InterfaceId::ITextureInfo)
public:
    /**
     * @brief Get index of texture slot this info is mapped to
     * @return Index of the texture slot
     */
    META_PROPERTY(uint32_t, TextureSlotIndex)

    /**
     * @brief Entity identifier of the image. DEPRECATED
     * @return pointer to property
     */
    META_PROPERTY(SCENE_NS::IBitmap::Ptr, Image)

    /**
     * @brief Predefined sampler types
     */
    enum SamplerId : uint8_t {
        CORE_DEFAULT_SAMPLER_UNKNOWN,
        /** Default sampler, nearest repeat */
        CORE_DEFAULT_SAMPLER_NEAREST_REPEAT,
        /** Default sampler, nearest clamp */
        CORE_DEFAULT_SAMPLER_NEAREST_CLAMP,
        /** Default sampler, linear repeat */
        CORE_DEFAULT_SAMPLER_LINEAR_REPEAT,
        /** Default sampler, linear clamp */
        CORE_DEFAULT_SAMPLER_LINEAR_CLAMP,
        /** Default sampler, linear mipmap repeat */
        CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_REPEAT,
        /** Default sampler, linear mipmap clamp */
        CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP
    };

    /**
     * @brief Sampler type to be used with texture.
     *        BOUND ONLY TOWARDS ECS CURRENTLY, does not report back
     * @return pointer to property
     */
    META_PROPERTY(uint8_t, Sampler)

    /**
     * @brief Color components.
     * @return pointer to property
     */
    META_PROPERTY(BASE_NS::Math::Vec4, Factor)

    /**
     * @brief Translation of the texture.
     * @return pointer to property
     */
    META_PROPERTY(BASE_NS::Math::Vec2, Translation)

    /**
     * @brief Rotation of the texture.
     * @return pointer to property
     */
    META_PROPERTY(float, Rotation)

    /**
     * @brief Scale of the texture.
     * @return pointer to property
     */
    META_PROPERTY(BASE_NS::Math::Vec2, Scale)

    /**
     * @brief Set index of the texture slot the info is mapped to.
     * @param index New index of the texture slot.
     */
    virtual void SetTextureSlotIndex(uint32_t index) = 0;

    /**
     * @brief Retrieve index of the texture slot this info is mapped into.
     * @return Index of the texture slot.
     */
    virtual uint32_t GetTextureSlotIndex() const = 0;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::ITextureInfo::WeakPtr);
META_TYPE(SCENE_NS::ITextureInfo::Ptr);

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IShader, "b75ef889-ceee-47f8-8679-4ea43801c022")
class IShader : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IShader, InterfaceId::IShader)
public:
    /**
     * @brief Uri of the shader definition
     * @return pointer to property
     */
    META_PROPERTY(BASE_NS::string, Uri)

    virtual RENDER_NS::RenderHandleReference GetRenderHandleReference(RENDER_NS::IShaderManager& shaderManager) = 0;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IShader::WeakPtr);
META_TYPE(SCENE_NS::IShader::Ptr);

SCENE_BEGIN_NAMESPACE()

class IMaterial;

using IShaderGraphicsState = IPendingRequest<RENDER_NS::GraphicsState>;

REGISTER_INTERFACE(IGraphicsState, "44e046fa-1a5b-4601-9fc9-bbed2e1f30bc")
class IGraphicsState : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IGraphicsState, InterfaceId::IGraphicsState)
public:
    /**
     * @brief Uri of the graphics state definition
     * @return pointer to property
     */
    META_PROPERTY(BASE_NS::string, Uri)

    /**
     * @brief Variant of the graphics state definition
     * @return pointer to property
     */
    META_PROPERTY(BASE_NS::string, Variant)

    virtual RENDER_NS::RenderHandleReference GetRenderHandleReference(RENDER_NS::IShaderManager& shaderManager) = 0;

    virtual void SetGraphicsState(
        const RENDER_NS::GraphicsState& state, BASE_NS::shared_ptr<SCENE_NS::IMaterial> mat) = 0;
    virtual IShaderGraphicsState::Ptr GetGraphicsState(BASE_NS::shared_ptr<SCENE_NS::IMaterial> mat) = 0;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IGraphicsState::WeakPtr);
META_TYPE(SCENE_NS::IGraphicsState::Ptr);

SCENE_BEGIN_NAMESPACE()

// Implemented by SCENE_NS::ClassId::Material
REGISTER_INTERFACE(IMaterial, "3a58fd01-78b1-46c3-b2ec-98b54422558d")
class IMaterial : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMaterial, InterfaceId::IMaterial)
public:
    enum MaterialType : uint8_t {
        /** Enumeration for Metallic roughness workflow */
        METALLIC_ROUGHNESS = 0,
        /** Enumeration for Specular glossiness workflow */
        SPECULAR_GLOSSINESS = 1,
        /** Enumeration for KHR materials unlit workflow */
        UNLIT = 2,
        /** Enumeration for special unlit shadow receiver */
        UNLIT_SHADOW_ALPHA = 3,
        /** Custom material. Could be used with custom material model e.g. with shader graph.
         * Disables automatic factor based modifications for flags.
         * Note: that base color is always automatically pre-multiplied in all cases
         */
        CUSTOM = 4,
        /** Custom complex material. Could be used with custom material model e.g. with shader graph.
         * Disables automatic factor based modifications for flags.
         * Does not use deferred rendering path in any case due to complex material model.
         * Note: that base color is always automatically pre-multiplied in all cases
         */
        CUSTOM_COMPLEX = 5
    };

    /**
     * @brief Type of the material.
     * @return pointer to property
     */
    META_PROPERTY(uint8_t, Type)

    /**
     * @brief Alpha cut off value, set the cutting value for alpha (0.0 - 1.0). Below 1.0 starts to affect.
     * @return pointer to property
     */
    META_PROPERTY(float, AlphaCutoff)

    /**
     * @brief Bitfield of the material lighting flags.
     * @return pointer to property
     */
    META_PROPERTY(uint32_t, LightingFlags)

    META_PROPERTY(IShader::Ptr, MaterialShader)
    META_PROPERTY(IGraphicsState::Ptr, MaterialShaderState)
    META_PROPERTY(IShader::Ptr, DepthShader)
    META_PROPERTY(IGraphicsState::Ptr, DepthShaderState)

    /**
     * @brief Primary inputs of the material. Depending the material type, different properties are
     *        available through provided object
     * @return pointer to property
     */
    META_READONLY_PROPERTY(META_NS::IObject::Ptr, CustomProperties)

    /**
     * @brief Get all material inputs
     * @return Array of all texture information structs.
     */
    META_ARRAY_PROPERTY(ITextureInfo::Ptr, Inputs)

    META_ARRAY_PROPERTY(ITextureInfo::Ptr, Textures)
    /**
     * @brief Sets the given bitmap to mesh texture.
     */
    virtual void SetImage(SCENE_NS::IBitmap::Ptr bitmap, size_t index = 0) = 0;

    /**
     * @brief Sets the given bitmap to the named texture slot.
     */
    virtual void SetImage(SCENE_NS::IBitmap::Ptr bitmap, BASE_NS::string_view textureSlot) = 0;

    static constexpr BASE_NS::string_view MAPPED_INPUTS_COLOR { "Color" };
    static constexpr BASE_NS::string_view MAPPED_INPUTS_FACTOR { "Factor" };
    static constexpr BASE_NS::string_view MAPPED_INPUTS_GLOSSINESS { "Glossiness" };
    static constexpr BASE_NS::string_view MAPPED_INPUTS_INTENSITY { "Intensity" };
    static constexpr BASE_NS::string_view MAPPED_INPUTS_METALLIC { "Metallic" };
    static constexpr BASE_NS::string_view MAPPED_INPUTS_NORMAL_SCALE { "Normal Scale" };
    static constexpr BASE_NS::string_view MAPPED_INPUTS_REFLECTANCE { "Reflectance" };
    static constexpr BASE_NS::string_view MAPPED_INPUTS_ROUGHNESS { "Roughness" };
    static constexpr BASE_NS::string_view MAPPED_INPUTS_STRENGTH { "Strength" };
    static constexpr BASE_NS::string_view MAPPED_INPUTS_TRANSMISSION { "Transmission" };

    /** @brief enumerations from engine. Fixed indices for built in shaders */
    enum TextureIndex : uint8_t {
        /** basecolor texture (expected to be premultiplied alpha) (aka. diffuse for specular-glossiness)*/
        BASE_COLOR,
        /** normal map texture */
        NORMAL,
        /** metallic-roughness or specular-glossiness texture */
        MATERIAL,
        /** emissive texture */
        EMISSIVE,
        /** ambient-occlusion texture */
        AO,

        /** clearcoat intensity texture */
        CLEARCOAT,
        /** clearcoat roughness texture */
        CLEARCOAT_ROUGHNESS,
        /** clearcoat normal map texture */
        CLEARCOAT_NORMAL,

        /** sheen color texture in rgb, sheen roughness in alpha */
        SHEEN,

        /** transmission percentage texture */
        TRANSMISSION,

        /** specular color and reflection strength texture */
        SPECULAR,

        /** number of textures */
        TEXTURE_COUNT
    };

    template<typename ValueType>
    META_NS::Property<ValueType> GetMappedProperty(
        size_t textureSlotIndex, const BASE_NS::string_view& name)
    {
        if (Inputs()) {
            for (auto& info : Inputs()->GetValue()) {
                if (info->GetTextureSlotIndex() == textureSlotIndex) {
                    if (auto meta = interface_pointer_cast<META_NS::IMetadata>(info)) {
                        return meta->GetPropertyByName<ValueType>(name);
                    }
                }
            }
        }

        return META_NS::Property<ValueType> {};
    }

    /** @brief Convenience proxy for material base color property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto BaseColor()
    {
        return GetMappedProperty<SCENE_NS::Color>(BASE_COLOR, MAPPED_INPUTS_COLOR);
    }

    /** @brief Convenience proxy for material normal map scale property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto NormalMapScale()
    {
        return GetMappedProperty<float>(NORMAL, MAPPED_INPUTS_NORMAL_SCALE);
    }

    /** @brief Convenience proxy for material metallic roughness property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto MetallicRoughness()
    {
        return GetMappedProperty<float>(MATERIAL, MAPPED_INPUTS_ROUGHNESS);
    }

    /** @brief Convenience proxy for material metallic property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto Metallic()
    {
        return GetMappedProperty<float>(MATERIAL, MAPPED_INPUTS_METALLIC);
    }

    /** @brief Convenience proxy for material metallic reflectance property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto MetallicReflectance()
    {
        return GetMappedProperty<float>(MATERIAL, MAPPED_INPUTS_REFLECTANCE);
    }

    /** @brief Convenience proxy for material specular glossiness color property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto SpecularGlossinessColor()
    {
        return GetMappedProperty<SCENE_NS::Color>(MATERIAL, MAPPED_INPUTS_COLOR);
    }

    /** @brief Convenience proxy for material specular glossiness property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto SpecularGlossiness()
    {
        return GetMappedProperty<float>(MATERIAL, MAPPED_INPUTS_GLOSSINESS);
    }

    /** @brief Convenience proxy for material emissive color property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto EmissiveColor()
    {
        return GetMappedProperty<SCENE_NS::Color>(EMISSIVE, MAPPED_INPUTS_COLOR);
    }

    /** @brief Convenience proxy for material emissive color intensity property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto EmissiveColorIntensity()
    {
        return GetMappedProperty<float>(EMISSIVE, MAPPED_INPUTS_INTENSITY);
    }

    /** @brief Convenience proxy for material ambient occlusion strength property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto AmbientOcclusionStrength()
    {
        return GetMappedProperty<float>(AO, MAPPED_INPUTS_STRENGTH);
    }

    /** @brief Convenience proxy for material emissive color intensity property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto ClearCoatIntensity()
    {
        return GetMappedProperty<float>(CLEARCOAT, MAPPED_INPUTS_INTENSITY);
    }

    /** @brief Convenience proxy for material emissive color roughness property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto ClearCoatRoughness()
    {
        return GetMappedProperty<float>(CLEARCOAT, MAPPED_INPUTS_ROUGHNESS);
    }

    /** @brief Convenience proxy for material normal map scale property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto ClearCoatNormalScale()
    {
        return GetMappedProperty<float>(CLEARCOAT, MAPPED_INPUTS_NORMAL_SCALE);
    }

    /** @brief Convenience proxy for material sheen color property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto SheenColor()
    {
        return GetMappedProperty<SCENE_NS::Color>(SHEEN, MAPPED_INPUTS_COLOR);
    }

    /** @brief Convenience proxy for material normal map scale property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto SheenRoughness()
    {
        return GetMappedProperty<float>(SHEEN, MAPPED_INPUTS_ROUGHNESS);
    }

    /** @brief Convenience proxy for material transmission property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto Transmission()
    {
        return GetMappedProperty<float>(TRANSMISSION, MAPPED_INPUTS_TRANSMISSION);
    }

    /** @brief Convenience proxy for material specular color property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto SpecularColor()
    {
        return GetMappedProperty<SCENE_NS::Color>(SPECULAR, MAPPED_INPUTS_COLOR);
    }

    /** @brief Convenience proxy for material specular color strength property. May return null.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    auto SpecularColorStrength()
    {
        return GetMappedProperty<float>(SPECULAR, MAPPED_INPUTS_STRENGTH);
    }

    /** @brief Convenience proxy for material custom properties. May return null if property is not
     * found or the given type is incorrect.
     * use of META_NS::GetValue() and META_NS::SetValue() recommended.
     */
    template<typename ValueType>
    typename META_NS::Property<ValueType>::Ptr GetCustomProperty(const BASE_NS::string_view& name)
    {
        if (auto customProperties = CustomProperties()) {
            if (auto meta = interface_pointer_cast<META_NS::IMetadata>(customProperties->GetValue())) {
                return meta->GetPropertyByName<ValueType>(name);
            }
        }
        return {};
    }
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IMaterial::WeakPtr);
META_TYPE(SCENE_NS::IMaterial::Ptr);

#endif // SCENEPLUGIN_INTF_MATERIAL_H
