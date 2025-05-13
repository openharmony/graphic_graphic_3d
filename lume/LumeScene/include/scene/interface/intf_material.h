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

#ifndef SCENE_INTERFACE_IMATERIAL_H
#define SCENE_INTERFACE_IMATERIAL_H

#include <scene/base/namespace.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/intf_texture.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/intf_metadata.h>

SCENE_BEGIN_NAMESPACE()

enum class MaterialType : uint8_t {
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

/** Default render sort layer id */
static constexpr uint32_t DEFAULT_RENDER_SORT_LAYER_ID = 32u;

struct RenderSort {
    /** Render sort layer. Within a render slot a layer can define a sort layer order.
     * There are 0-63 values available. Default id value is 32.
     * 0 first, 63 last
     * 1. Typical use case is to set render sort layer to objects which render with depth test without depth write.
     * 2. Typical use case is to always render character and/or camera object first to cull large parts of the view.
     * 3. Sort e.g. plane layers.
     */
    /** Sort layer used sorting submeshes in rendering in render slots. Valid ID values 0 - 63. */
    uint8_t renderSortLayer { DEFAULT_RENDER_SORT_LAYER_ID };
    /** Sort layer order to describe fine order within sort layer. Valid order 0 - 255 */
    uint8_t renderSortLayerOrder {};
};

enum class LightingFlags : uint32_t {
    /** Defines whether this material receives shadow */
    SHADOW_RECEIVER_BIT = (1 << 0),
    /** Defines whether this material is a shadow caster */
    SHADOW_CASTER_BIT = (1 << 1),
    /** Defines whether this material will receive light from punctual lights (points, spots, directional) */
    PUNCTUAL_LIGHT_RECEIVER_BIT = (1 << 2),
    /** Defines whether this material will receive indirect light from SH and cubemaps */
    INDIRECT_LIGHT_RECEIVER_BIT = (1 << 3),
};

inline LightingFlags operator|(LightingFlags l, LightingFlags r)
{
    return LightingFlags(static_cast<uint32_t>(l) | static_cast<uint32_t>(r));
}

class IMaterial : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMaterial, "c531141e-9d59-4ba3-9aca-b10986ed8805")
public:
    /**
     * @brief Type of the material.
     */
    META_PROPERTY(MaterialType, Type)

    /**
     * @brief Alpha cut off value, set the cutting value for alpha (0.0 - 1.0). Below 1.0 starts to affect.
     * @return pointer to property
     */
    META_PROPERTY(float, AlphaCutoff)

    /**
     * @brief Bitfield of the material lighting flags.
     * @return pointer to property
     */
    META_PROPERTY(SCENE_NS::LightingFlags, LightingFlags)

    META_PROPERTY(IShader::Ptr, MaterialShader)
    META_PROPERTY(IShader::Ptr, DepthShader)

    META_PROPERTY(SCENE_NS::RenderSort, RenderSort)

    META_READONLY_ARRAY_PROPERTY(ITexture::Ptr, Textures)
    META_READONLY_PROPERTY(META_NS::IMetadata::Ptr, CustomProperties);
    /**
     * @brief Synchronize custom properties from ECS to engine and return them.
     * @return CustomProperties members whose types registered to the engine, if any. Null pointer otherwise.
     */
    virtual META_NS::IMetadata::Ptr GetCustomProperties() const = 0;
    /**
     * @brief Synchronize custom properties from ECS to engine and return the requested sub property.
     * @param name The name of the property. "MaterialComponent.customProperties." prefix is optional.
     * @return The requested property, if it exists and its type is registered to the engine. Null pointer otherwise.
     */
    virtual META_NS::IProperty::Ptr GetCustomProperty(BASE_NS::string_view name) const = 0;

    template<typename Type>
    META_NS::Property<Type> GetCustomProperty(BASE_NS::string_view name) const
    {
        return META_NS::Property<Type>(GetCustomProperty(name));
    }
    template<typename Type>
    META_NS::ArrayProperty<Type> GetCustomArrayProperty(BASE_NS::string_view name) const
    {
        return META_NS::ArrayProperty<Type>(GetCustomProperty(name));
    }
};

META_REGISTER_CLASS(Material, "ffcb25d5-18fd-42ad-8df5-ebd5197bc8a6", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::MaterialType)
META_TYPE(SCENE_NS::RenderSort)
META_TYPE(SCENE_NS::LightingFlags)
META_INTERFACE_TYPE(SCENE_NS::IMaterial)

#endif
