/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "material.h"

#include <scene/interface/intf_image.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/intf_texture.h>
#include <scene/interface/resource/types.h>

#include <3d/ecs/components/material_component.h>

#include <meta/api/util.h>
#include <meta/interface/detail/property.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/property/construct_property.h>

#include "../diagnostics.h"
#include "../import_helpers.h"
#include "flag_tables.h"

SCENE_IMP_BEGIN_NAMESPACE()

namespace {
// clang-format off
static constexpr NamedValue<SCENE_NS::MaterialType> MATERIAL_TYPE_TABLE[] = {
    { "metallicRoughness",  SCENE_NS::MaterialType::METALLIC_ROUGHNESS },
    { "specularGlossiness", SCENE_NS::MaterialType::SPECULAR_GLOSSINESS },
    { "unlit",              SCENE_NS::MaterialType::UNLIT },
    { "unlitShadowAlpha",   SCENE_NS::MaterialType::UNLIT_SHADOW_ALPHA },
    { "custom",             SCENE_NS::MaterialType::CUSTOM },
    { "customComplex",      SCENE_NS::MaterialType::CUSTOM_COMPLEX },
    { "occlusion",          SCENE_NS::MaterialType::OCCLUSION },
};
// clang-format on
}  // namespace

template <typename EnumType, size_t N>
static IDiagnostics::Ptr ImportEnumToProperty(ImportContext& context, BASE_NS::string_view jsonName,
    const META_NS::IProperty::Ptr& prop, const NamedValue<EnumType> (&table)[N])
{
    ErrorHandler h(context);
    if (auto value = GetOptString(context, jsonName); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        auto result = LookupValue(table, value.GetValue());
        if (!result) {
            return context.CreateDiagnostics(BASE_NS::string("Invalid ") + jsonName + ": " + value.GetValue());
        }
        META_NS::SetValue<EnumType>(prop, *result);
    }
    return h;
}

static IDiagnostics::Ptr ImportFlagsToProperty(
    ImportContext& context, BASE_NS::string_view jsonName, const META_NS::IProperty::Ptr& prop)
{
    auto value = context.GetOptValue(jsonName);
    if (!value.is_array()) {
        return nullptr;
    }
    ErrorHandler h(context);
    auto flags = static_cast<SCENE_NS::LightingFlags>(0);
    if (!value.array_.empty()) {
        if (auto err = ImportFlagsFromArray(context, value.array_, LIGHTING_FLAGS_TABLE, jsonName, flags);
            h.Handle(err)) {
            return err;
        }
    }
    META_NS::SetValue<SCENE_NS::LightingFlags>(prop, flags);
    return h;
}

static IDiagnostics::Ptr ImportMaterialScalars(ImportContext& context, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    if (!context.GetOptValue("materialType").is_null()) {
        auto prop = META_NS::ConstructProperty<SCENE_NS::MaterialType>("Type");
        if (auto err = ImportEnumToProperty(context, "materialType", prop.GetProperty(), MATERIAL_TYPE_TABLE);
            h.Handle(err)) {
            return err;
        }
        meta.AddProperty(prop.GetProperty());
    }
    if (auto value = GetOptFloat(context, "alphaCutoff"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        auto prop = META_NS::ConstructProperty<float>("AlphaCutoff");
        META_NS::SetValue<float>(prop.GetProperty(), value.GetValue());
        meta.AddProperty(prop.GetProperty());
    }
    if (context.GetOptValue("lightingFlags").is_array()) {
        auto prop = META_NS::ConstructProperty<SCENE_NS::LightingFlags>("LightingFlags");
        if (auto err = ImportFlagsToProperty(context, "lightingFlags", prop.GetProperty()); h.Handle(err)) {
            return err;
        }
        meta.AddProperty(prop.GetProperty());
    }
    return h;
}

static IDiagnostics::Ptr ImportRenderSortToTemplate(ImportContext& context, META_NS::IMetadata& meta)
{
    auto sortObj = context.GetOptObject("renderSort");
    if (sortObj.empty()) {
        return nullptr;
    }
    auto sortContext = context.CreateContext(BASE_NS::move(sortObj));
    ErrorHandler h(context);
    SCENE_NS::RenderSort renderSort{};
    if (auto value = GetOptUInt(sortContext, "layer"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        renderSort.renderSortLayer = static_cast<uint8_t>(value.GetValue());
    }
    if (auto value = GetOptUInt(sortContext, "order"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        renderSort.renderSortLayerOrder = static_cast<uint8_t>(value.GetValue());
    }
    auto prop = META_NS::ConstructProperty<SCENE_NS::RenderSort>("RenderSort");
    META_NS::SetValue<SCENE_NS::RenderSort>(prop.GetProperty(), renderSort);
    meta.AddProperty(prop.GetProperty());
    return h;
}

struct DeferredResourceContext {
    ImportContextParameters params;
    SCENE_NS::IRenderContext::Ptr renderContext;
    BASE_NS::string filename;
    BASE_NS::vector<ErrorInfo> trace;

    IDiagnostics::Ptr CreateError(BASE_NS::string_view message) const
    {
        return MakeSimpleError(BASE_NS::string(message), filename, trace);
    }
};

static DeferredResourceContext MakeDeferredResourceContext(ImportContext& context)
{
    return {context.GetImportParameters(),
        context.GetRenderContext(),
        context.GetImportParameters().filename,
        context.GetTrace()};
}

static IDiagnostics::Ptr DeferShaderToProperty(
    ImportContext& context, BASE_NS::string_view jsonName, const META_NS::IProperty::Ptr& prop)
{
    ErrorHandler h(context);
    if (auto rid = GetOptResourceId(context, jsonName); h.HandleOptValue(rid)) {
        if (rid.error) {
            return rid.error;
        }
        auto resourceId = rid.GetValue();
        auto dc = MakeDeferredResourceContext(context);
        context.AddDeferredAction(ImportContext::DeferTier::Resource, [prop, resourceId, dc]() -> IDiagnostics::Ptr {
            auto resource = ResolveDeferredResource(dc.params, dc.renderContext, resourceId);
            if (!resource) {
                return dc.CreateError("Failed to resolve shader resource: " + resourceId.ToString());
            }
            auto shader = interface_pointer_cast<SCENE_NS::IShader>(resource);
            if (!shader) {
                return dc.CreateError("Resource is not a shader: " + resourceId.ToString());
            }
            META_NS::SetValue<SCENE_NS::IShader::Ptr>(prop, shader);
            return nullptr;
        });
    }
    return h;
}

static IDiagnostics::Ptr DeferImageToProperty(
    ImportContext& context, BASE_NS::string_view jsonName, const META_NS::IProperty::Ptr& prop)
{
    ErrorHandler h(context);
    if (auto rid = GetOptResourceId(context, jsonName); h.HandleOptValue(rid)) {
        if (rid.error) {
            return rid.error;
        }
        auto resourceId = rid.GetValue();
        auto dc = MakeDeferredResourceContext(context);
        context.AddDeferredAction(ImportContext::DeferTier::Resource, [prop, resourceId, dc]() -> IDiagnostics::Ptr {
            auto resource = ResolveDeferredResource(dc.params, dc.renderContext, resourceId);
            if (!resource) {
                return dc.CreateError("Failed to resolve image resource: " + resourceId.ToString());
            }
            auto image = interface_pointer_cast<SCENE_NS::IImage>(resource);
            if (!image) {
                return dc.CreateError("Resource is not an image: " + resourceId.ToString());
            }
            META_NS::SetValue<SCENE_NS::IImage::Ptr>(prop, image);
            return nullptr;
        });
    }
    return h;
}

static IDiagnostics::Ptr ImportMaterialShaders(ImportContext& context, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    if (!context.GetOptValue("materialShader").is_null()) {
        auto prop = META_NS::ConstructProperty<SCENE_NS::IShader::Ptr>("MaterialShader");
        if (auto err = DeferShaderToProperty(context, "materialShader", prop.GetProperty()); h.Handle(err)) {
            return err;
        }
        meta.AddProperty(prop.GetProperty());
    }
    if (!context.GetOptValue("depthShader").is_null()) {
        auto prop = META_NS::ConstructProperty<SCENE_NS::IShader::Ptr>("DepthShader");
        if (auto err = DeferShaderToProperty(context, "depthShader", prop.GetProperty()); h.Handle(err)) {
            return err;
        }
        meta.AddProperty(prop.GetProperty());
    }
    return h;
}

static IDiagnostics::Ptr ImportSamplerToTemplate(ImportContext& context, META_NS::IMetadata& sampler)
{
    auto samplerObj = context.GetOptObject("sampler");
    if (samplerObj.empty()) {
        return nullptr;
    }
    auto sc = context.CreateContext(BASE_NS::move(samplerObj));
    ErrorHandler h(context);
    auto getProp = [&sampler](BASE_NS::string_view name) { return sampler.GetProperty(name); };
    if (auto err = ImportEnumToProperty(sc, "magFilter", getProp("MagFilter"), SAMPLER_FILTER_TABLE); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportEnumToProperty(sc, "minFilter", getProp("MinFilter"), SAMPLER_FILTER_TABLE); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportEnumToProperty(sc, "mipMapMode", getProp("MipMapMode"), SAMPLER_FILTER_TABLE); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportEnumToProperty(sc, "addressModeU", getProp("AddressModeU"), SAMPLER_ADDRESS_MODE_TABLE);
        h.Handle(err)) {
        return err;
    }
    if (auto err = ImportEnumToProperty(sc, "addressModeV", getProp("AddressModeV"), SAMPLER_ADDRESS_MODE_TABLE);
        h.Handle(err)) {
        return err;
    }
    return ImportEnumToProperty(sc, "addressModeW", getProp("AddressModeW"), SAMPLER_ADDRESS_MODE_TABLE);
}

static IDiagnostics::Ptr ImportTextureScalars(ImportContext& context, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    if (auto value = GetOptString(context, "name"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        if (auto prop = meta.GetProperty<BASE_NS::string>("Name")) {
            META_NS::SetValue(prop, value.GetValue());
        }
    }
    if (auto value = GetOptVec4(context, "factor"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        META_NS::SetValue(meta.GetProperty<BASE_NS::Math::Vec4>("Factor"), value.GetValue());
    }
    if (auto value = GetOptVec2(context, "translation"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        META_NS::SetValue(meta.GetProperty<BASE_NS::Math::Vec2>("Translation"), value.GetValue());
    }
    if (auto value = GetOptFloat(context, "rotation"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        META_NS::SetValue(meta.GetProperty<float>("Rotation"), value.GetValue());
    }
    if (auto value = GetOptVec2(context, "scale"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        META_NS::SetValue(meta.GetProperty<BASE_NS::Math::Vec2>("Scale"), value.GetValue());
    }
    return h;
}

static IDiagnostics::Ptr ImportTextureToTemplate(ImportContext& context, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    if (auto err = ImportTextureScalars(context, meta); h.Handle(err)) {
        return err;
    }
    if (auto imageProp = meta.GetProperty("Image")) {
        if (auto err = DeferImageToProperty(context, "image", imageProp); h.Handle(err)) {
            return err;
        }
    }
    if (auto samplerMeta = META_NS::GetPointer<META_NS::IMetadata>(meta.GetProperty("Sampler"))) {
        if (auto err = ImportSamplerToTemplate(context, *samplerMeta); h.Handle(err)) {
            return err;
        }
    }
    return h;
}

static IDiagnostics::Ptr ImportTextureEntry(
    ImportContext& context, const CORE_NS::json::value& entry, const META_NS::IObject::Ptr& tex)
{
    if (!entry.is_object()) {
        return context.CreateDiagnostics("Invalid textures entry: expected object");
    }
    if (!tex) {
        return nullptr;
    }
    auto texMeta = interface_cast<META_NS::IMetadata>(tex.get());
    if (!texMeta) {
        return nullptr;
    }
    auto texContext = context.CreateContext(entry);
    return ImportTextureToTemplate(texContext, *texMeta);
}

static META_NS::IObject::Ptr CreateTemplateSampler()
{
    auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(META_NS::ClassId::Object);
    if (auto meta = interface_cast<META_NS::IMetadata>(obj.get())) {
        meta->AddProperty(META_NS::ConstructProperty<SCENE_NS::SamplerFilter>("MagFilter").GetProperty());
        meta->AddProperty(META_NS::ConstructProperty<SCENE_NS::SamplerFilter>("MinFilter").GetProperty());
        meta->AddProperty(META_NS::ConstructProperty<SCENE_NS::SamplerFilter>("MipMapMode").GetProperty());
        meta->AddProperty(META_NS::ConstructProperty<SCENE_NS::SamplerAddressMode>("AddressModeU").GetProperty());
        meta->AddProperty(META_NS::ConstructProperty<SCENE_NS::SamplerAddressMode>("AddressModeV").GetProperty());
        meta->AddProperty(META_NS::ConstructProperty<SCENE_NS::SamplerAddressMode>("AddressModeW").GetProperty());
    }
    return obj;
}

static META_NS::IObject::Ptr CreateTemplateTexture()
{
    auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(META_NS::ClassId::Object);
    if (auto meta = interface_cast<META_NS::IMetadata>(obj.get())) {
        meta->AddProperty(META_NS::ConstructProperty<BASE_NS::string>("Name").GetProperty());
        meta->AddProperty(META_NS::ConstructProperty<SCENE_NS::IImage::Ptr>("Image").GetProperty());
        meta->AddProperty(META_NS::ConstructProperty<BASE_NS::Math::Vec4>("Factor").GetProperty());
        meta->AddProperty(META_NS::ConstructProperty<BASE_NS::Math::Vec2>("Translation").GetProperty());
        meta->AddProperty(META_NS::ConstructProperty<float>("Rotation").GetProperty());
        meta->AddProperty(META_NS::ConstructProperty<BASE_NS::Math::Vec2>("Scale").GetProperty());
        auto sampler = CreateTemplateSampler();
        meta->AddProperty(META_NS::ConstructProperty<META_NS::IObject::Ptr>("Sampler", sampler).GetProperty());
    }
    return obj;
}

static void EnsureTexturesProperty(META_NS::IMetadata& meta)
{
    if (meta.GetProperty("Textures")) {
        return;
    }
    constexpr size_t textureCount = CORE3D_NS::MaterialComponent::TextureIndex::TEXTURE_COUNT;
    BASE_NS::vector<META_NS::IObject::Ptr> textures;
    textures.reserve(textureCount);
    for (size_t ix = 0; ix < textureCount; ++ix) {
        textures.push_back(CreateTemplateTexture());
    }
    auto prop = META_NS::ConstructArrayProperty<META_NS::IObject::Ptr>("Textures", textures);
    meta.AddProperty(prop.GetProperty());
}

static IDiagnostics::Ptr ImportTexturesToTemplate(ImportContext& context, META_NS::IMetadata& meta)
{
    auto arr = context.GetOptArray("textures");
    if (arr.empty()) {
        return nullptr;
    }
    EnsureTexturesProperty(meta);
    auto textures = meta.GetArrayProperty<META_NS::IObject::Ptr>("Textures", META_NS::MetadataQuery::EXISTING);
    if (!textures) {
        return nullptr;
    }
    ErrorHandler h(context);
    const size_t count = arr.size();
    for (size_t i = 0; i < count && i < textures->GetSize(); ++i) {
        if (auto err = ImportTextureEntry(context, arr[i], textures->GetValueAt(i)); h.Handle(err)) {
            return err;
        }
    }
    return h;
}

ImportResult ImportMaterial::Import(ImportContext& context)
{
    auto trace = context.Trace("Importing material");
    auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(SCENE_NS::ClassId::MaterialTemplate);
    if (!obj) {
        return ImportResult{context.CreateDiagnostics("Failed to create material template")};
    }
    auto meta = interface_cast<META_NS::IMetadata>(obj.get());
    if (!meta) {
        return ImportResult{context.CreateDiagnostics("Material template has no metadata")};
    }

    ErrorHandler h(context);
    if (auto err = ImportMaterialScalars(context, *meta); h.Handle(err)) {
        return ImportResult{err};
    }
    if (auto err = ImportRenderSortToTemplate(context, *meta); h.Handle(err)) {
        return ImportResult{err};
    }
    if (auto err = ImportMaterialShaders(context, *meta); h.Handle(err)) {
        return ImportResult{err};
    }
    if (auto err = ImportTexturesToTemplate(context, *meta); h.Handle(err)) {
        return ImportResult{err};
    }

    ImportResult result{obj};
    result.error = h;
    return result;
}

SCENE_IMP_END_NAMESPACE()
