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

#ifndef SCENEPLUGINAPI_MATERIAL_H
#define SCENEPLUGINAPI_MATERIAL_H

#include <scene_plugin/api/material_uid.h>
#include <scene_plugin/interface/intf_scene.h>

#include <meta/api/internal/object_api.h>
SCENE_BEGIN_NAMESPACE()

/**
 * @brief The TextureInfo class
 */
class TextureInfo final : public META_NS::Internal::ObjectInterfaceAPI<TextureInfo, ClassId::TextureInfo> {
    META_API(TextureInfo)
    META_API_OBJECT_CONVERTIBLE(ITextureInfo)
    META_API_CACHE_INTERFACE(ITextureInfo, TextureInfo)
};

/**
 * @brief The CustomPropertiesHolder class
 */
class CustomPropertiesHolder final
    : public META_NS::Internal::ObjectInterfaceAPI<CustomPropertiesHolder, ClassId::CustomPropertiesHolder> {
    META_API(CustomPropertiesHolder)
    META_API_OBJECT_CONVERTIBLE(META_NS::IMetadata)
};

/**
 * @brief The Shader class
 */
class Shader final : public META_NS::Internal::ObjectInterfaceAPI<Shader, ClassId::Shader> {
    META_API(Shader)
    META_API_OBJECT_CONVERTIBLE(IShader)
    META_API_CACHE_INTERFACE(IShader, Shader)
    META_API_INTERFACE_PROPERTY_CACHED(Shader, Uri, BASE_NS::string)
};

/**
 * @brief The GraphicsState class
 */
class GraphicsState final : public META_NS::Internal::ObjectInterfaceAPI<GraphicsState, ClassId::GraphicsState> {
    META_API(GraphicsState)
    META_API_OBJECT_CONVERTIBLE(IGraphicsState)
    META_API_CACHE_INTERFACE(IGraphicsState, GraphicsState)
public:
    META_API_INTERFACE_PROPERTY_CACHED(GraphicsState, Uri, BASE_NS::string)
    META_API_INTERFACE_PROPERTY_CACHED(GraphicsState, Variant, BASE_NS::string)
};

/**
 * @brief The Material class
 */
class Material final : public META_NS::Internal::ObjectInterfaceAPI<Material, ClassId::Material> {
    META_API(Material)
    META_API_OBJECT_CONVERTIBLE(IMaterial)
    META_API_CACHE_INTERFACE(IMaterial, Material)
    META_API_OBJECT_CONVERTIBLE(INode)
    META_API_CACHE_INTERFACE(INode, Node)
public:
    // From Node
    META_API_INTERFACE_PROPERTY_CACHED(Node, Name, BASE_NS::string)
    META_API_INTERFACE_PROPERTY_CACHED(Material, AlphaCutoff, float)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Material, CustomProperties, META_NS::IObject::Ptr)
    META_API_INTERFACE_ARRAY_PROPERTY_CACHED(Material, Inputs, SCENE_NS::ITextureInfo::Ptr)

public:
    /**
     * @brief Material
     * @param node
     */
    explicit Material(const INode::Ptr& node)
    {
        Initialize(interface_pointer_cast<META_NS::IObject>(node));
    }

    /**
     * @brief Material
     * @param node
     */
    Material(const IMaterial::Ptr& node)
    {
        Initialize(interface_pointer_cast<META_NS::IObject>(node));
    }

    /**
     * @brief OnLoaded
     * @return
     */
    auto OnLoaded()
    {
        return META_API_CACHED_INTERFACE(Node)->OnLoaded();
    }

    /**
     * @brief OnLoaded
     * @param callback
     * @return
     */
    template<class Callback>
    auto OnLoaded(Callback&& callback)
    {
        OnLoaded()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(callback));
        return *this;
    }

    // Material Base Color
    META_API_INTERFACE_PROPERTY_CACHED(Material, BaseColor, SCENE_NS::Color)

    // Material Normal Texture
    META_API_INTERFACE_PROPERTY_CACHED(Material, NormalMapScale, float)

    // Metallic Roughness types
    META_API_INTERFACE_PROPERTY_CACHED(Material, MetallicRoughness, float)
    META_API_INTERFACE_PROPERTY_CACHED(Material, Metallic, float)
    META_API_INTERFACE_PROPERTY_CACHED(Material, MetallicReflectance, float)

    // Specular Glossiness
    META_API_INTERFACE_PROPERTY_CACHED(Material, SpecularGlossinessColor, SCENE_NS::Color)
    META_API_INTERFACE_PROPERTY_CACHED(Material, SpecularGlossiness, float)

    // Emissive
    META_API_INTERFACE_PROPERTY_CACHED(Material, EmissiveColor, SCENE_NS::Color)
    META_API_INTERFACE_PROPERTY_CACHED(Material, EmissiveColorIntensity, float)

    // Ambient Occlusion
    META_API_INTERFACE_PROPERTY_CACHED(Material, AmbientOcclusionStrength, float)

    // Clear Coat
    META_API_INTERFACE_PROPERTY_CACHED(Material, ClearCoatIntensity, float)
    META_API_INTERFACE_PROPERTY_CACHED(Material, ClearCoatRoughness, float)
    META_API_INTERFACE_PROPERTY_CACHED(Material, ClearCoatNormalScale, float)

    // Sheen
    META_API_INTERFACE_PROPERTY_CACHED(Material, SheenColor, SCENE_NS::Color)
    META_API_INTERFACE_PROPERTY_CACHED(Material, SheenRoughness, float)

    // Transmission
    META_API_INTERFACE_PROPERTY_CACHED(Material, Transmission, float)

    // Specular color
    META_API_INTERFACE_PROPERTY_CACHED(Material, SpecularColor, SCENE_NS::Color)
    META_API_INTERFACE_PROPERTY_CACHED(Material, SpecularColorStrength, float)

    CustomPropertiesHolder GetCustomProperties()
    {
        CustomPropertiesHolder ret;
        ret.Initialize(META_API_CACHED_INTERFACE(Material)->CustomProperties()->GetValue());
        return ret;
    }
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGINAPI_MATERIAL_H
