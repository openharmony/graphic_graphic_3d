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

template<typename EnumType, size_t N>
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
    auto sort = ImportOptRenderSort(context);
    if (sort.error) {
        return sort.error;
    }
    if (sort.value) {
        auto prop = META_NS::ConstructProperty<SCENE_NS::RenderSort>("RenderSort");
        META_NS::SetValue<SCENE_NS::RenderSort>(prop.GetProperty(), *sort.value);
        meta.AddProperty(prop.GetProperty());
    }
    return nullptr;
}

static IDiagnostics::Ptr ImportMaterialShaders(ImportContext& context, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    if (!context.GetOptValue("materialShader").is_null()) {
        if (auto err = AddResourceIdProperty(context, meta, "materialShader", "MaterialShader"); h.Handle(err)) {
            return err;
        }
    }
    if (!context.GetOptValue("depthShader").is_null()) {
        if (auto err = AddResourceIdProperty(context, meta, "depthShader", "DepthShader"); h.Handle(err)) {
            return err;
        }
    }
    return h;
}

// Build a fresh, empty per-texture sub-object. Properties are added only when the
// JSON entry actually carries the corresponding field.
static META_NS::IObject::Ptr CreateEmptyTextureEntry()
{
    return META_NS::GetObjectRegistry().Create<META_NS::IObject>(META_NS::ClassId::Object);
}

static IDiagnostics::Ptr ImportTextureScalars(ImportContext& context, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    if (auto value = GetOptVec4(context, "factor"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty<BASE_NS::Math::Vec4>(meta, "Factor", value.GetValue());
    }
    if (auto value = GetOptVec2(context, "translation"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty<BASE_NS::Math::Vec2>(meta, "Translation", value.GetValue());
    }
    if (auto value = GetOptFloat(context, "rotation"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty<float>(meta, "Rotation", value.GetValue());
    }
    if (auto value = GetOptVec2(context, "scale"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty<BASE_NS::Math::Vec2>(meta, "Scale", value.GetValue());
    }
    return h;
}

static IDiagnostics::Ptr ImportTextureImage(ImportContext& context, META_NS::IMetadata& meta)
{
    if (!context.GetOptValue("image")) {
        return nullptr;
    }
    ErrorHandler h(context);
    // Store the resource id as a ResourceId-typed property; apply resolves against the
    // scene's resource manager. Resolution at apply time lets the override see resources
    // that don't exist yet at index-load time (e.g. a gltf-derived image that the node
    // template's own external child will register at instantiation).
    if (auto err = AddResourceIdProperty(context, meta, "image", "Image"); h.Handle(err)) {
        return err;
    }
    return h;
}

// Adds an enum property only when the JSON field is present, preserving
// sparseness so the apply step never touches members the JSON didn't declare.
template<typename EnumType, size_t N>
static IDiagnostics::Ptr ImportEnumPropertyIfPresent(ImportContext& context, BASE_NS::string_view jsonName,
    META_NS::IMetadata& meta, BASE_NS::string_view propName, const NamedValue<EnumType> (&table)[N])
{
    if (!context.GetOptValue(jsonName)) {
        return nullptr;
    }
    auto prop = META_NS::ConstructProperty<EnumType>(propName);
    meta.AddProperty(prop.GetProperty());
    return ImportEnumToProperty(context, jsonName, prop.GetProperty(), table);
}

static IDiagnostics::Ptr ImportSamplerToTemplate(ImportContext& context, META_NS::IMetadata& sampler)
{
    auto samplerObj = context.GetOptObject("sampler");
    if (samplerObj.empty()) {
        return nullptr;
    }
    auto sc = context.CreateContext(BASE_NS::move(samplerObj));
    ErrorHandler h(context);
    if (auto err = ImportEnumPropertyIfPresent(sc, "magFilter", sampler, "MagFilter", SAMPLER_FILTER_TABLE);
        h.Handle(err)) {
        return err;
    }
    if (auto err = ImportEnumPropertyIfPresent(sc, "minFilter", sampler, "MinFilter", SAMPLER_FILTER_TABLE);
        h.Handle(err)) {
        return err;
    }
    if (auto err = ImportEnumPropertyIfPresent(sc, "mipMapMode", sampler, "MipMapMode", SAMPLER_FILTER_TABLE);
        h.Handle(err)) {
        return err;
    }
    if (auto err = ImportEnumPropertyIfPresent(sc, "addressModeU", sampler, "AddressModeU", SAMPLER_ADDRESS_MODE_TABLE);
        h.Handle(err)) {
        return err;
    }
    if (auto err = ImportEnumPropertyIfPresent(sc, "addressModeV", sampler, "AddressModeV", SAMPLER_ADDRESS_MODE_TABLE);
        h.Handle(err)) {
        return err;
    }
    return ImportEnumPropertyIfPresent(sc, "addressModeW", sampler, "AddressModeW", SAMPLER_ADDRESS_MODE_TABLE);
}

static IDiagnostics::Ptr ImportTextureSamplerEntry(ImportContext& context, META_NS::IMetadata& meta)
{
    if (!context.GetOptValue("sampler")) {
        return nullptr;
    }
    ErrorHandler h(context);
    auto samplerObj = CreateEmptyTextureEntry();
    auto samplerMeta = interface_cast<META_NS::IMetadata>(samplerObj);
    if (!samplerMeta) {
        return nullptr;
    }
    if (auto err = ImportSamplerToTemplate(context, *samplerMeta); h.Handle(err)) {
        return err;
    }
    auto prop = META_NS::ConstructProperty<META_NS::IObject::Ptr>("Sampler", samplerObj);
    meta.AddProperty(prop.GetProperty());
    return h;
}

static IDiagnostics::Ptr ImportTextureEntryFields(ImportContext& context, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    if (auto err = ImportTextureScalars(context, meta); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportTextureImage(context, meta); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportTextureSamplerEntry(context, meta); h.Handle(err)) {
        return err;
    }
    return h;
}

static IDiagnostics::Ptr AppendNamedTextureEntry(ImportContext& context, META_NS::IMetadata& materialMeta,
    BASE_NS::string_view slotName, const CORE_NS::json::value& entryJson)
{
    if (!entryJson.is_object()) {
        return context.CreateDiagnostics(
            BASE_NS::string("Invalid textures entry for slot '") + slotName + "': expected object");
    }
    auto entry = CreateEmptyTextureEntry();
    auto entryMeta = interface_cast<META_NS::IMetadata>(entry);
    if (!entryMeta) {
        return nullptr;
    }
    AddSetProperty<BASE_NS::string>(*entryMeta, "Name", BASE_NS::string(slotName));

    auto entryContext = context.CreateContext(entryJson);
    ErrorHandler h(context);
    if (auto err = ImportTextureEntryFields(entryContext, *entryMeta); h.Handle(err)) {
        return err;
    }

    auto textures = materialMeta.GetArrayProperty<META_NS::IObject::Ptr>("Textures", META_NS::MetadataQuery::EXISTING);
    if (!textures) {
        BASE_NS::vector<META_NS::IObject::Ptr> empty;
        auto prop = META_NS::ConstructArrayProperty<META_NS::IObject::Ptr>("Textures", empty);
        materialMeta.AddProperty(prop.GetProperty());
        textures = materialMeta.GetArrayProperty<META_NS::IObject::Ptr>("Textures", META_NS::MetadataQuery::EXISTING);
    }
    if (textures) {
        textures->AddValue(entry);
    }
    return h;
}

static IDiagnostics::Ptr ImportTexturesToTemplate(ImportContext& context, META_NS::IMetadata& meta)
{
    auto obj = context.GetOptObject("textures");
    if (obj.empty()) {
        return nullptr;
    }
    ErrorHandler h(context);
    for (const auto& pair : obj) {
        BASE_NS::string slotName(pair.key);
        if (auto err = AppendNamedTextureEntry(context, meta, slotName, pair.value); h.Handle(err)) {
            return err;
        }
    }
    return h;
}

// Per-material graphics state. Stored as a sparse "Options" sub-object (only
// fields present in JSON get a property); applied to the material's bound
// shader at apply time.
static IDiagnostics::Ptr ImportMaterialOptions(ImportContext& context, META_NS::IMetadata& meta)
{
    auto optionsObj = context.GetOptObject("options");
    if (optionsObj.empty()) {
        return nullptr;
    }
    auto oc = context.CreateContext(BASE_NS::move(optionsObj));
    auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(META_NS::ClassId::Object);
    auto optMeta = interface_cast<META_NS::IMetadata>(obj);
    if (!optMeta) {
        return context.CreateDiagnostics("Failed to create material options object");
    }
    ErrorHandler h(context);
    if (auto err = ImportEnumPropertyIfPresent(oc, "cullMode", *optMeta, "CullMode", CULL_MODE_TABLE); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportEnumPropertyIfPresent(oc, "polygonMode", *optMeta, "PolygonMode", POLYGON_MODE_TABLE);
        h.Handle(err)) {
        return err;
    }
    if (auto value = GetOptBool(oc, "blend"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty(*optMeta, "Blend", value.GetValue());
    }
    auto prop = META_NS::ConstructProperty<META_NS::IObject::Ptr>("Options", obj);
    meta.AddProperty(prop.GetProperty());
    return h;
}

ImportResult ImportMaterial::Import(ImportContext& context)
{
    auto trace = context.Trace("Importing material");
    auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(SCENE_NS::ClassId::MaterialTemplate);
    if (!obj) {
        return ImportResult{context.CreateDiagnostics("Failed to create material template")};
    }
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    if (!meta) {
        return ImportResult{context.CreateDiagnostics("Material template has no metadata")};
    }

    auto typeProp = META_NS::ConstructProperty<SCENE_NS::MaterialType>("Type");
    META_NS::SetValue<SCENE_NS::MaterialType>(typeProp.GetProperty(), materialType_);
    meta->AddProperty(typeProp.GetProperty());

    ErrorHandler h(context);
    if (auto err = ImportResourceName(context, *meta); h.Handle(err)) {
        return ImportResult{err};
    }
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
    if (auto err = ImportMaterialOptions(context, *meta); h.Handle(err)) {
        return ImportResult{err};
    }

    ImportResult result{obj};
    result.error = h;
    return result;
}

SCENE_IMP_END_NAMESPACE()
