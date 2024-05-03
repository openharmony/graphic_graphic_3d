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

#if !defined(API_3D_ECS_COMPONENTS_MATERIAL_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_MATERIAL_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>

CORE3D_BEGIN_NAMESPACE()
/** \addtogroup group_material_materialdesc
 *  @{
 */
#endif

/** Material properties.
 * With full customization one can use custom resources property
 */
BEGIN_COMPONENT(IMaterialComponentManager, MaterialComponent)
#if !defined(IMPLEMENT_MANAGER)
    /** Material type enumeration */
    enum class Type : uint8_t {
        /** Enumeration for Metallic roughness workflow */
        METALLIC_ROUGHNESS = 0,
        /** Enumumeration for Specular glossiness workflow */
        SPECULAR_GLOSSINESS = 1,
        /** Enumumeration for KHR materials unlit workflow */
        UNLIT = 2,
        /** Enumumeration for special unlit shadow receiver */
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
        CUSTOM_COMPLEX = 5,
    };

    /** Material specialization flags */
    enum LightingFlagBits : uint32_t {
        /** Defines whether this material receives shadow */
        SHADOW_RECEIVER_BIT = (1 << 0),
        /** Defines whether this material is a shadow caster */
        SHADOW_CASTER_BIT = (1 << 1),
        /** Defines whether this material will receive light from punctual lights (points, spots, directional) */
        PUNCTUAL_LIGHT_RECEIVER_BIT = (1 << 2),
        /** Defines whether this material will receive indirect light from SH and cubemaps */
        INDIRECT_LIGHT_RECEIVER_BIT = (1 << 3),
    };
    /** Container for material flag bits */
    using LightingFlags = uint32_t;

    /** Rendering flags (specialized rendering flags) */
    enum ExtraRenderingFlagBits : uint32_t {
        /** Is an additional flag which can be used to discard some materials from rendering from render node graph */
        DISCARD_BIT = (1 << 0),
        /** Is an additional flag which disables default render system push to render data stores and rendering */
        DISABLE_BIT = (1 << 1),
        /** Allow rendering mutiple instances of the same mesh using GPU instancing. materialShader must support
           instancing. */
        ALLOW_GPU_INSTANCING_BIT = (1 << 2),
    };
    /** Container for extra material rendering flag bits */
    using ExtraRenderingFlags = uint32_t;

    /** Needs to match the texture ordering with default material shader pipeline layout. The names are for default
     * materials.
     *
     * For other predefined material shaders included in 3D:
     *
     * "core3d_dm_fw_reflection_plane.shader":
     * CLEARCOAT_ROUGHNESS index has been reserved for reflection image.
     */
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

    /** Channel mapping for materials using Type::METALLIC_ROUGHNESS
     *  that can be used to access a specific channel in a TextureIndex::MATERIAL texture.
     */
    enum MetallicRoughnessChannel : uint8_t {
        /** Index of the roughness channel in the material texture. */
        ROUGHNESS = 1,

        /** Index of the metallic channel in the material texture. */
        METALLIC,
    };

    /** Channel mapping for materials using Type::SPECULAR_GLOSSINESS
     *  that can be used to access a specific channel in a TextureIndex::MATERIAL texture.
     */
    enum SpecularGlossinessChannel : uint8_t {
        /** Index of the specular red channel in the material texture. */
        SPECULAR_R = 0,

        /** Index of the specular green channel in the material texture. */
        SPECULAR_G,

        /** Index of the specular blue channel in the material texture. */
        SPECULAR_B,

        /** Index of the glossiness channel in the material texture. */
        GLOSSINESS,
    };

    struct TextureTransform {
        BASE_NS::Math::Vec2 translation { 0.f, 0.f };
        float rotation { 0.f };
        BASE_NS::Math::Vec2 scale { 1.f, 1.f };
    };

    struct TextureInfo {
        CORE_NS::EntityReference image;
        CORE_NS::EntityReference sampler;
        BASE_NS::Math::Vec4 factor;
        TextureTransform transform;
    };

    /** Default material component shader */
    struct Shader {
        /** Shader to be used. (If invalid, a default is chosen by the default material renderer)
         * NOTE: the material medata and custom properties are updated when the shader is updated.
         */
        CORE_NS::EntityReference shader;
        /** Shader graphics state to be used. (If invalid, a default is chosen by the default material renderer) */
        CORE_NS::EntityReference graphicsState;
    };
#endif
    /** Material type which can be one of the following Type::METALLIC_ROUGHNESS, Type::SPECULAR_GLOSSINESS */
    DEFINE_PROPERTY(Type, type, "Material Type", 0, VALUE(Type::METALLIC_ROUGHNESS))

    /** Alpha cut off value, set the cutting value for alpha (0.0 - 1.0). Below 1.0 starts to affect.
     */
    DEFINE_PROPERTY(float, alphaCutoff, "Alpha Cutoff", 0, VALUE(1.0f))

    /** Material lighting flags that define the lighting related settings for this material */
    DEFINE_BITFIELD_PROPERTY(LightingFlags, materialLightingFlags, "Material Lighting Flags",
        PropertyFlags::IS_BITFIELD,
        VALUE(MaterialComponent::LightingFlagBits::SHADOW_RECEIVER_BIT |
              MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT |
              MaterialComponent::LightingFlagBits::PUNCTUAL_LIGHT_RECEIVER_BIT |
              MaterialComponent::LightingFlagBits::INDIRECT_LIGHT_RECEIVER_BIT),
        MaterialComponent::LightingFlagBits)

    /** Material shader. Prefer using automatic selection (or editor selection) if no custom shaders.
     * Needs to match default material layouts and specializations (api/3d/shaders/common).
     * If no default slot given to shader default material shader slots are used automatically.
     * Therefore, do not set slots and their graphics states if no special handling is needed.
     * Use OPAQUE_FW core3d_dm_fw.shader as an example reference.
     * (I.e. if one wants to things just work, do not specify slots or additional custom graphics states per slots)
     * NOTE: when material shader is updated the possible material metadata and custom properties are updated
     * NOTE: one needs to reload the shader file(s) with shader manager to get dynamic updated custom property data
     */
    DEFINE_PROPERTY(Shader, materialShader, "Material Shader", 0, )

    /** Depth shader. Prefer using automatic selection (or editor selection) if no custom shaders.
     * Needs to match default material layouts and specializations (api/3d/shaders/common).
     * If no default slot given to shader default material shader slots are used automatically.
     * (I.e. if one wants to things just work, do not specify slots or additional custom graphics states per slots)
     */
    DEFINE_PROPERTY(Shader, depthShader, "Depth Shader", 0, )

    /** Extra material rendering flags define special rendering hints */
    DEFINE_BITFIELD_PROPERTY(ExtraRenderingFlags, extraRenderingFlags, "ExtraRenderingFlags",
        PropertyFlags::IS_BITFIELD, VALUE(0u), MaterialComponent::ExtraRenderingFlagBits)

    /** Array of texture information. With default shaders TextureIndex is used for identifying texures for different
     * material properties. Use of TextureInfo::factor depends on the index.
     *
     * BASE_COLOR: RGBA, base color, if an image is specified, this value is multiplied with the texel values.
     * NOTE: the pre-multiplication is done always, i.e. use only for base color with custom materials
     * NOTE: the built-in default material shaders write out alpha values from 0.0 - 1.0
     *       opaque flag is enabled to shader is graphics state's blending mode is not active -> alpha 1.0
     *
     * NORMAL: R, normal scale, scalar multiplier applied to each normal vector of the texture. (Ignored if image is not
     * specified, this value is linear).
     *
     * MATERIAL: For Type::METALLIC_ROUGHNESS: G roughness (smooth 0.0 - 1.0 rough), B metallic (dielectric 0.0 - 1.0
     * metallic)., and A reflectance at normal incidence
     * For Type::SPECULAR_GLOSSINESS: RGB specular color (linear), A glossiness (rough 0.0 - 1.0 glossy). Texel values
     * are multiplied with the corresponding factors.
     *
     * EMISSIVE: RGB, emissive color, A intensity, if an image is specified, this value is multiplied with the texel
     * values.
     *
     * AO: R, ambient occlusion factor, this value is multiplied with the texel values (no ao 0.0 - 1.0 full ao)
     *
     * CLEARCOAT: R, clearcoat layer intensity, if an image is specified, this value is multiplied with the texel
     * values.
     *
     * CLEARCOAT_ROUGHNESS: G, clearcoat layer roughness, if an image is specified, this value is multiplied with the
     * texel values.
     *
     * CLEARCOAT_NORMAL: RGB, clearcoat normal scale, scalar multiplier applied to each normal vector of the clearcoat
     * normal texture.
     *
     * SHEEN: RGB, sheen color, if an image is specified, this value is multiplied with the texel values.
     * SHEEN: A, sheen roughness, if an image is specified, this value is multiplied with the texel values.
     *
     * TRANSMISSION: R, Percentage of light that is transmitted through the surface, if an image is specified, this
     * value is multiplied with the texel values.
     *
     * SPECULAR: RGB color of the specular reflection, A strength of the specular reflection, if an image is specified,
     * this value is multiplied with the texel values.
     */
    DEFINE_ARRAY_PROPERTY(TextureInfo, TextureIndex::TEXTURE_COUNT, textures, "", PropertyFlags::IS_HIDDEN,
        ARRAY_VALUE(                                              //
            TextureInfo { {}, {}, { 1.f, 1.f, 1.f, 1.f }, {} },   // base color opaque white
            TextureInfo { {}, {}, { 1.f, 0.f, 0.f, 0.f }, {} },   // normal scale 1
            TextureInfo { {}, {}, { 0.f, 1.f, 1.f, 0.04f }, {} }, // material (empty, roughness, metallic, reflectance)
            TextureInfo { {}, {}, { 0.f, 0.f, 0.f, 1.f }, {} },   // emissive 0
            TextureInfo { {}, {}, { 1.f, 0.f, 0.f, 0.f }, {} },   // ambient occlusion 1
            TextureInfo { {}, {}, { 0.f, 0.f, 0.f, 0.f }, {} },   // clearcoat intensity 0
            TextureInfo { {}, {}, { 0.f, 0.f, 0.f, 0.f }, {} },   // clearcoat roughness 0
            TextureInfo { {}, {}, { 1.f, 0.f, 0.f, 0.f }, {} },   // clearcoat normal scale 1
            TextureInfo { {}, {}, { 0.f, 0.f, 0.f, 0.f }, {} },   // sheen color black, roughness 0
            TextureInfo { {}, {}, { 0.f, 0.f, 0.f, 0.f }, {} },   // transmission 0
            TextureInfo { {}, {}, { 1.f, 1.f, 1.f, 1.f }, {} }    // specular white
            ))

    /** Texture coordinates from set 0 or 1. */
    DEFINE_PROPERTY(uint32_t, useTexcoordSetBit, "Active Texture Coordinate", 0,
        VALUE(0u)) // if uses set 1 (1 << enum TextureIndex)

    /** Custom forced render slot id. One can force rendering with e.g. opaque or translucent render slots */
    DEFINE_PROPERTY(uint32_t, customRenderSlotId, "Custom Render Slot ID", ~0, VALUE(~0u))

    /** Custom material extension resources. Deprecates and prevents MaterialExtensionComponent usage.
     * Are automatically bound to custom shader, custom pipeline layout custom descriptor set if they are in order.
     */
    DEFINE_PROPERTY(
        BASE_NS::vector<CORE_NS::EntityReference>, customResources, "Custom Material Extension Resources", 0, )

    /** Per material additional user property data which is passed to shader UBO.
     * Max size is 256 bytes.
     */
    DEFINE_PROPERTY(CORE_NS::IPropertyHandle*, customProperties, "Custom Properties", 0, VALUE(nullptr))

END_COMPONENT(IMaterialComponentManager, MaterialComponent, "56430c14-cb12-4320-80d3-2bef4f86a041")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
