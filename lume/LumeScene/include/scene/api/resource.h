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

#ifndef SCENE_API_RESOURCE_H
#define SCENE_API_RESOURCE_H

#include <scene/interface/intf_environment.h>
#include <scene/interface/intf_image.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_shader.h>

#include <meta/api/object.h>
#include <meta/api/object_name.h>
#include <meta/interface/resource/intf_resource.h>

SCENE_BEGIN_NAMESPACE()

class Resource : public META_NS::Named {
public:
    META_INTERFACE_OBJECT(Resource, META_NS::Named, CORE_NS::IResource)
    /// @see IResource::GetResourceType
    CORE_NS::ResourceType GetResourceType() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetResourceType());
    }
    /// @see IResource::GetResourceId
    CORE_NS::ResourceId GetResourceId() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetResourceId());
    }
};

class Image : public Resource {
public:
    META_INTERFACE_OBJECT(Image, Resource, IImage)
    /// @see IBitmap::Size
    META_INTERFACE_OBJECT_READONLY_PROPERTY(BASE_NS::Math::UVec2, Size)
};

class Environment : public Resource {
public:
    META_INTERFACE_OBJECT(Environment, Resource, IEnvironment)
    /// @see IEnvironment::Background
    META_INTERFACE_OBJECT_PROPERTY(EnvBackgroundType, Background)
    /// @see IEnvironment::IndirectDiffuseFactor
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec4, IndirectDiffuseFactor)
    /// @see IEnvironment::IndirectSpecularFactor
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec4, IndirectSpecularFactor)
    /// @see IEnvironment::EnvMapFactor
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec4, EnvMapFactor)
    /// @see IEnvironment::RadianceImage
    META_INTERFACE_OBJECT_PROPERTY(Image, RadianceImage)
    /// @see IEnvironment::RadianceCubemapMipCount
    META_INTERFACE_OBJECT_PROPERTY(uint32_t, RadianceCubemapMipCount)
    /// @see IEnvironment::EnvironmentImage
    META_INTERFACE_OBJECT_PROPERTY(Image, EnvironmentImage)
    /// @see IEnvironment::EnvironmentMapLodLevel
    META_INTERFACE_OBJECT_PROPERTY(float, EnvironmentMapLodLevel)
    /// @see IEnvironment::IrradianceCoefficients
    META_INTERFACE_OBJECT_ARRAY_PROPERTY(BASE_NS::Math::Vec3, IrradianceCoefficients, IrradianceCoefficient)
    /// @see IEnvironment::EnvironmentRotation
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Quat, EnvironmentRotation)
};

class Shader : public Resource {
public:
    META_INTERFACE_OBJECT(Shader, Resource, IShader)
    /// @see IShader::CullMode
    META_INTERFACE_OBJECT_PROPERTY(CullModeFlags, CullMode)
    /// @see IShader::PolygonMode
    META_INTERFACE_OBJECT_PROPERTY(PolygonMode, PolygonMode)
    /// @see IShader::Blend
    META_INTERFACE_OBJECT_PROPERTY(bool, Blend)
};

class Sampler : public META_NS::ResetableObject {
public:
    META_INTERFACE_OBJECT(Sampler, META_NS::ResetableObject, ISampler)
    /// @see ISampler::MinFilter
    META_INTERFACE_OBJECT_PROPERTY(SamplerFilter, MinFilter)
    /// @see ISampler::MagFilter
    META_INTERFACE_OBJECT_PROPERTY(SamplerFilter, MagFilter)
    /// @see ISampler::MipMapMode
    META_INTERFACE_OBJECT_PROPERTY(SamplerFilter, MipMapMode)
    /// @see ISampler::AddressModeU
    META_INTERFACE_OBJECT_PROPERTY(SamplerAddressMode, AddressModeU)
    /// @see ISampler::AddressModeV
    META_INTERFACE_OBJECT_PROPERTY(SamplerAddressMode, AddressModeV)
    /// @see ISampler::AddressModeW
    META_INTERFACE_OBJECT_PROPERTY(SamplerAddressMode, AddressModeW)
};

class MaterialProperty : public META_NS::Object {
public:
    META_INTERFACE_OBJECT(MaterialProperty, META_NS::Object, ITexture)
    /// @see ITexture::Image
    META_INTERFACE_OBJECT_PROPERTY(SCENE_NS::Image, Image)
    /// @see ITexture::Sampler
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::Sampler, Sampler)
    /// @see ITexture::Factor
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec4, Factor)
    /// @see ITexture::Translation
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec2, Translation)
    /// @see ITexture::Rotation
    META_INTERFACE_OBJECT_PROPERTY(float, Rotation)
    /// @see ITexture::Scale
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec2, Scale)
};

/**
 * @brief The base IMaterial::Ptr wrapper for all materials
 */
class Material : public Resource {
public:
    META_INTERFACE_OBJECT(Material, Resource, IMaterial)
    /// @see IMaterial::Type
    META_INTERFACE_OBJECT_PROPERTY(MaterialType, Type)
    /// @see IMaterial::AlphaCutoff
    META_INTERFACE_OBJECT_PROPERTY(float, AlphaCutoff)
    /// @see IMaterial::LightingFlags
    META_INTERFACE_OBJECT_PROPERTY(SCENE_NS::LightingFlags, LightingFlags)
    /// @see IMaterial::MaterialShader
    META_INTERFACE_OBJECT_PROPERTY(Shader, MaterialShader)
    /// @see IMaterial::DepthShader
    META_INTERFACE_OBJECT_PROPERTY(Shader, DepthShader)
    /// @see IMaterial::RenderSort
    META_INTERFACE_OBJECT_PROPERTY(SCENE_NS::RenderSort, RenderSort)
    /// Returns custom properties assigned to this material.
    auto GetCustomProperties() const
    {
        return META_NS::Metadata(META_INTERFACE_OBJECT_CALL_PTR(GetCustomProperties()));
    }
    /// Returns a named custom property with given type. Null if property does not exist or given type is not compatible
    /// with the property value.
    template<typename Type>
    auto GetCustomProperty(BASE_NS::string_view name) const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetCustomProperty(name));
    }
    /// Returns a named custom array property with given type. Null if property does not exist or given type is not
    /// compatible with the property value.
    template<typename Type>
    META_NS::ArrayProperty<Type> GetCustomArrayProperty(BASE_NS::string_view name) const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetCustomArrayProperty(name));
    }
    /**
     * @brief Returns MaterialProperty with given name.
     * @param name Name of the material property to find.
     * @return MaterialProperty with given name or an invalid object if match not found.
     */
    auto GetMaterialProperty(BASE_NS::string_view name) const
    {
        auto tis = META_INTERFACE_OBJECT_CALL_PTR(Textures()->GetValue());
        for (auto&& ti : tis) {
            if (META_NS::GetName(ti) == name) {
                return MaterialProperty(ti);
            }
        }
        return MaterialProperty(nullptr);
    }
    /**
     * @brief Returns the MaterialProperty at given texture slot index.
     * @param index The texture slot index to retrieve.
     * @return The MaterialProperty at given index or an invalid object in case of invalid index.
     */
    auto GetMaterialPropertyAt(size_t index) const
    {
        auto ts = META_INTERFACE_OBJECT_CALL_PTR(Textures()->GetValueAt(index));
        return MaterialProperty(ts);
    }
    /**
     * @brief Returns all material properties associated with the material.
     */
    auto GetMaterialProperties()
    {
        auto tis = META_INTERFACE_OBJECT_CALL_PTR(Textures()->GetValue());
        return META_NS::Internal::ArrayCast<MaterialProperty>(BASE_NS::move(tis));
    }
};
/**
 * @brief Represents a material as a custom shader
 */
class ShaderMaterial : public Material {
public:
    META_INTERFACE_OBJECT(ShaderMaterial, Material, IMaterial)
};

/**
 * @brief Represents a material as a physically-based metallic roughenss material.
 */
class MetallicRoughnessMaterial : public Material {
public:
    META_INTERFACE_OBJECT(MetallicRoughnessMaterial, Material, IMaterial)

    /// MaterialProperty indices for metallic roughness materials
    enum MaterialPropertyIndex {
        BASE_COLOR = 0,
        NORMAL,
        MATERIAL,
        EMISSIVE,
        AO,
        CLEARCOAT,
        CLEARCOAT_ROUGHNESS,
        CLEARCOAT_NORMAL,
        SHEEN,
        TRANSMISSION,
        SPECULAR
    };

    /**
     * @brief Base color factor of pbr material.
     *        Value of factor.xyzw defines rgba color.
     */
    auto GetBaseColor() const
    {
        return GetMaterialPropertyAt(BASE_COLOR);
    }
    /**
     * @brief Normal factor of pbr material.
     *        Value of factor.x defines normal scale.
     */
    MaterialProperty GetNormal()
    {
        return GetMaterialPropertyAt(NORMAL);
    }
    /**
     * @brief Metallic roughness material parameters.
     *        Value of factor.y defines roughness, factor.z defines metallic and factor.a defines reflectance.
     */
    MaterialProperty GetMaterialParameters()
    {
        return GetMaterialPropertyAt(MATERIAL);
    }
    /**
     * @brief Ambient occlusion of pbr material.
     *        Value of factor.x defines ambient occlusion factor.
     */
    MaterialProperty GetAmbientOcclusion()
    {
        return GetMaterialPropertyAt(AO);
    }
    /**
     * @brief Clearcoat normal.
     *        Value of factor.xyz defines RGB clearcoat normal scale.
     */
    MaterialProperty GetEmissive()
    {
        return GetMaterialPropertyAt(EMISSIVE);
    }
    /**
     * @brief Clearcoat intensity.
     *        Value of factor.x defines clearcoat layer intensity.
     */
    MaterialProperty GetClearCoat()
    {
        return GetMaterialPropertyAt(CLEARCOAT);
    }
    /**
     * @brief Clearcoat roughness.
     *        Value of factor.y defines clearcoat layer roughness.
     */
    MaterialProperty GetClearCoatRoughness()
    {
        return GetMaterialPropertyAt(CLEARCOAT_ROUGHNESS);
    }
    /**
     * @brief Clearcoat normal.
     *        Value of factor.xyz defines RGB clearcoat normal scale.
     */
    MaterialProperty GetClearCoatNormal()
    {
        return GetMaterialPropertyAt(CLEARCOAT_NORMAL);
    }
    /**
     * @brief Sheen color of pbr material.
     *        Value of factor.xyz defines RGB sheen color,
     *        Value of factor.w defines sheen roughness.
     */
    MaterialProperty GetSheen()
    {
        return GetMaterialPropertyAt(SHEEN);
    }
    /**
     * @brief Transmission percentage texture.
     *        Value of factor.r defines the percentage of light that is transmitted through the surface.
     *        If an image is specified, this value is multiplied with the texel values.
     */
    MaterialProperty GetTransmission()
    {
        return GetMaterialPropertyAt(TRANSMISSION);
    }
    /**
     * @brief Specular color of pbr material.
     *        Value of factor.xyz defines RGB specular color,
     *        Value of factor.w defines specular intensity.
     */
    MaterialProperty GetSpecular()
    {
        return GetMaterialPropertyAt(SPECULAR);
    }
};

class Morpher : public META_NS::Object {
public:
    META_INTERFACE_OBJECT(Morpher, META_NS::Object, IMorpher)
    /// @see IMorpher::MorphNames
    META_INTERFACE_OBJECT_ARRAY_PROPERTY(BASE_NS::string, MorphNames, MorphName)
    /// @see IMorpher::MorphWeights
    META_INTERFACE_OBJECT_ARRAY_PROPERTY(float, MorphWeights, MorphWeight)
};

class SubMesh : public META_NS::Object {
public:
    META_INTERFACE_OBJECT(SubMesh, META_NS::Object, ISubMesh)
    /// @see ISubMesh::Material
    META_INTERFACE_OBJECT_PROPERTY(SCENE_NS::Material, Material)
    /// @see ISubMesh::AABBMin
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec3, AABBMin)
    /// @see ISubMesh::AABBMax
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec3, AABBMax)
};

class Mesh : public Resource {
public:
    META_INTERFACE_OBJECT(Mesh, Resource, IMesh)
    /// @see IMesh::AABBMin
    META_INTERFACE_OBJECT_READONLY_PROPERTY(BASE_NS::Math::Vec3, AABBMin)
    /// @see IMesh::AABBMax
    META_INTERFACE_OBJECT_READONLY_PROPERTY(BASE_NS::Math::Vec3, AABBMax)
    /// @see IMesh::SubMeshes
    META_INTERFACE_OBJECT_READONLY_ARRAY_PROPERTY(SubMesh, SubMeshes, SubMesh)
};

SCENE_END_NAMESPACE()

#endif // SCENE_API_RESOURCE_H
