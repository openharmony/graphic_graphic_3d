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

#include "environment.h"

#include <scene/interface/intf_environment.h>
#include <scene/interface/resource/types.h>

#include <meta/api/metadata_util.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/property/construct_property.h>

#include "../import_helpers.h"

SCENE_IMP_BEGIN_NAMESPACE()

namespace {
// clang-format off
static constexpr NamedValue<SCENE_NS::EnvBackgroundType> BACKGROUND_TABLE[] = {
    { "none",            SCENE_NS::EnvBackgroundType::NONE },
    { "image",           SCENE_NS::EnvBackgroundType::IMAGE },
    { "cubemap",         SCENE_NS::EnvBackgroundType::CUBEMAP },
    { "equirectangular", SCENE_NS::EnvBackgroundType::EQUIRECTANGULAR },
};
// clang-format on
}  // namespace

static OptValue<BASE_NS::Math::Vec3> ParseVec3Entry(ImportContext& context, const CORE_NS::json::value& v)
{
    if (!v.is_array() || v.array_.size() != 3) {
        return OptValue<BASE_NS::Math::Vec3>{
            context.CreateDiagnostics("Invalid irradianceCoefficients entry: expected vec3 array")};
    }
    if (!v.array_[0].is_number() || !v.array_[1].is_number() || !v.array_[2].is_number()) {
        return OptValue<BASE_NS::Math::Vec3>{
            context.CreateDiagnostics("Invalid irradianceCoefficients entry: expected numbers")};
    }
    return OptValue<BASE_NS::Math::Vec3>{BASE_NS::Math::Vec3{
        v.array_[0].as_number<float>(), v.array_[1].as_number<float>(), v.array_[2].as_number<float>()}};
}

static IDiagnostics::Ptr ImportIrradianceCoefficients(ImportContext& context, META_NS::IMetadata& meta)
{
    auto arr = context.GetOptArray("irradianceCoefficients");
    if (arr.empty()) {
        return nullptr;
    }
    ErrorHandler h(context);
    BASE_NS::vector<BASE_NS::Math::Vec3> coefficients;
    coefficients.reserve(arr.size());
    for (auto&& v : arr) {
        if (auto entry = ParseVec3Entry(context, v); h.HandleOptValue(entry)) {
            if (entry.error) {
                return entry.error;
            }
            coefficients.push_back(entry.GetValue());
        }
    }
    auto p = META_NS::ConstructArrayProperty<BASE_NS::Math::Vec3>("IrradianceCoefficients");
    meta.AddProperty(p.GetProperty());
    p->SetValue(coefficients);
    return h;
}

static IDiagnostics::Ptr ImportEnvironmentFactors(ImportContext& context, META_NS::IMetadata& meta, ErrorHandler& h)
{
    if (auto value = GetOptVec4(context, "indirectDiffuseFactor"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty(meta, "IndirectDiffuseFactor", value.GetValue());
    }
    if (auto value = GetOptVec4(context, "indirectSpecularFactor"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty(meta, "IndirectSpecularFactor", value.GetValue());
    }
    if (auto value = GetOptVec4(context, "envMapFactor"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty(meta, "EnvMapFactor", value.GetValue());
    }
    return nullptr;
}

static IDiagnostics::Ptr ImportEnvironmentRadianceAndEnvImage(
    ImportContext& context, const META_NS::IObject::Ptr& obj, META_NS::IMetadata& meta, ErrorHandler& h)
{
    if (auto err = ImportImageProperty(context, obj, "radianceImage", "RadianceImage"); h.Handle(err)) {
        return err;
    }
    if (auto value = GetOptUInt(context, "radianceCubemapMipCount"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty(meta, "RadianceCubemapMipCount", static_cast<uint32_t>(value.GetValue()));
    }
    if (auto err = ImportImageProperty(context, obj, "environmentImage", "EnvironmentImage"); h.Handle(err)) {
        return err;
    }
    if (auto value = GetOptFloat(context, "environmentMapLodLevel"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty(meta, "EnvironmentMapLodLevel", value.GetValue());
    }
    return nullptr;
}

static IDiagnostics::Ptr ImportEnvironmentRotation(ImportContext& context, META_NS::IMetadata& meta, ErrorHandler& h)
{
    if (auto value = GetOptQuat(context, "environmentRotation"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty(meta, "EnvironmentRotation", value.GetValue());
    }
    return nullptr;
}

ImportResult ImportEnvironment::Import(ImportContext& context)
{
    auto trace = context.Trace("Importing environment");
    auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(SCENE_NS::ClassId::EnvironmentTemplate);
    if (!obj) {
        return ImportResult{context.CreateDiagnostics("Failed to create environment template")};
    }
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    if (!meta) {
        return ImportResult{context.CreateDiagnostics("Environment template has no metadata")};
    }

    ErrorHandler h(context);
    if (auto err = ImportResourceName(context, *meta); h.Handle(err)) {
        return ImportResult{err};
    }
    if (auto err = ImportEnumProperty(context, *meta, "background", "Background", BACKGROUND_TABLE); h.Handle(err)) {
        return ImportResult{err};
    }
    if (auto err = ImportEnvironmentFactors(context, *meta, h)) {
        return ImportResult{err};
    }
    if (auto err = ImportEnvironmentRadianceAndEnvImage(context, obj, *meta, h)) {
        return ImportResult{err};
    }
    if (auto err = ImportIrradianceCoefficients(context, *meta); h.Handle(err)) {
        return ImportResult{err};
    }
    if (auto err = ImportEnvironmentRotation(context, *meta, h)) {
        return ImportResult{err};
    }

    ImportResult result{obj};
    result.error = h;
    return result;
}

SCENE_IMP_END_NAMESPACE()
