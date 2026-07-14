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

#ifndef SCENE_IMP_SRC_IMPORT_HELPERS_H
#define SCENE_IMP_SRC_IMPORT_HELPERS_H

#include <optional>
#include <scene/interface/intf_material.h>
#include <scene/interface/template/intf_template_options.h>

#include <base/math/matrix.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/resources/intf_resource_manager.h>

#include <meta/api/util.h>
#include <meta/base/time_span.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/property/construct_property.h>

#include "config.h"

SCENE_IMP_BEGIN_NAMESPACE()

class ImportContext;

template<typename Type>
struct OptValue {
    OptValue() = default;
    explicit OptValue(Type v) : value(BASE_NS::move(v))
    {}
    explicit OptValue(IDiagnostics::Ptr e) : error(BASE_NS::move(e))
    {}

    std::optional<Type> value{};
    IDiagnostics::Ptr error;

    Type GetValue() const
    {
        CORE_ASSERT(value);
        return *value;
    }
};

OptValue<BASE_NS::string> GetOptString(ImportContext&, BASE_NS::string_view name);
OptValue<float> GetOptFloat(ImportContext&, BASE_NS::string_view name);
OptValue<uint64_t> GetOptUInt(ImportContext&, BASE_NS::string_view name);
OptValue<int64_t> GetOptInt(ImportContext&, BASE_NS::string_view name);
OptValue<bool> GetOptBool(ImportContext&, BASE_NS::string_view name);
OptValue<BASE_NS::Math::Vec2> GetOptVec2(ImportContext&, BASE_NS::string_view name);
OptValue<BASE_NS::Math::Vec3> GetOptVec3(ImportContext&, BASE_NS::string_view name);
OptValue<BASE_NS::Math::Vec4> GetOptVec4(ImportContext&, BASE_NS::string_view name);
OptValue<BASE_NS::Color> GetOptColor(ImportContext&, BASE_NS::string_view name);
OptValue<BASE_NS::Math::Quat> GetOptQuat(ImportContext&, BASE_NS::string_view name);
OptValue<BASE_NS::Math::UVec2> GetOptUVec2(ImportContext&, BASE_NS::string_view name);
OptValue<BASE_NS::Math::UVec3> GetOptUVec3(ImportContext&, BASE_NS::string_view name);
OptValue<BASE_NS::Math::UVec4> GetOptUVec4(ImportContext&, BASE_NS::string_view name);
OptValue<BASE_NS::Math::IVec2> GetOptIVec2(ImportContext&, BASE_NS::string_view name);
OptValue<BASE_NS::Math::IVec3> GetOptIVec3(ImportContext&, BASE_NS::string_view name);
OptValue<BASE_NS::Math::IVec4> GetOptIVec4(ImportContext&, BASE_NS::string_view name);
OptValue<BASE_NS::Math::Mat4X4> GetOptMat4x4(ImportContext&, BASE_NS::string_view name);
OptValue<META_NS::TimeSpan> GetOptTimeSpan(ImportContext&, BASE_NS::string_view name);
OptValue<META_NS::ObjectId> GetOptObjectId(ImportContext&, BASE_NS::string_view name);

OptValue<CORE_NS::ResourceId> GetOptResourceId(ImportContext& context);
OptValue<CORE_NS::ResourceId> GetOptResourceId(ImportContext& context, BASE_NS::string_view name);

// --- Table-driven value lookup helpers ---
template<typename T>
struct NamedValue {
    BASE_NS::string_view name;
    T value;
};

template<typename T, size_t N>
std::optional<T> LookupValue(const NamedValue<T> (&table)[N], BASE_NS::string_view name)
{
    for (const auto& entry : table) {
        if (entry.name == name) {
            return entry.value;
        }
    }
    return {};
}

// Marker bit set on objects, properties, and attachments that originated
// from a template application, so the node template updater can tell
// template-introduced state apart from scene-local edits.
using SCENE_NS::IMPORTED_FROM_TEMPLATE_BIT;

// Construct a property of the given type and name, attach it to `meta`, and
// set its value. Used by resource handlers to populate typed-template metadata.
template<typename T>
inline void AddSetProperty(META_NS::IMetadata& meta, BASE_NS::string_view name, const T& value)
{
    auto prop = META_NS::ConstructProperty<T>(name);
    meta.AddProperty(prop.GetProperty());
    META_NS::SetValue<T>(prop.GetProperty(), value);
}

struct ImportContextParameters;

CORE_NS::IResource::Ptr ResolveDeferredResource(const ImportContextParameters& params,
    const SCENE_NS::IRenderContext::Ptr& renderContext, const CORE_NS::ResourceId& resourceId);

// Read the optional top-level "name" string and, when present, attach it as
// the resource template's INamed "Name" property. Shared by every resource
// handler that builds a template implementing META_NS::INamed.
IDiagnostics::Ptr ImportResourceName(ImportContext& context, META_NS::IMetadata& meta);

IDiagnostics::Ptr ImportImageProperty(ImportContext& context, const META_NS::IObject::Ptr& metaOwner,
    BASE_NS::string_view jsonName, BASE_NS::string_view propName);

// Read an optional resource reference from JSON field `jsonName` and store it on `meta` as a
// property named `propName` of type CORE_NS::ResourceId. No resolution or instantiation
// happens at import time — the apply paths in src/template/ resolve the ResourceId against
// the live scene's resource manager when the template is applied. Returns nullptr (no
// diagnostic) if the JSON field is absent; surfaces parse errors otherwise.
IDiagnostics::Ptr AddResourceIdProperty(
    ImportContext& context, META_NS::IMetadata& meta, BASE_NS::string_view jsonName, BASE_NS::string_view propName);

/// Parse the optional `renderSort` child object on `context`. Returns an
/// OptValue: present when the field was set in JSON (with the parsed struct as
/// the value), error when parsing failed, or empty when the field was absent.
/// Used by both material and mesh importers — the `types.schema.json#/$defs/
/// renderSort` shape is shared.
OptValue<SCENE_NS::RenderSort> ImportOptRenderSort(ImportContext& context);

IDiagnostics::Ptr ImportEnvironmentProperty(
    ImportContext& context, const META_NS::IProperty::Ptr& target, BASE_NS::string_view jsonName);

IDiagnostics::Ptr ImportPostProcessProperty(
    ImportContext& context, const META_NS::IProperty::Ptr& target, BASE_NS::string_view jsonName);

BASE_NS::string ResolveFilename(ImportContext& context, BASE_NS::string_view resourceIndex);
ImportResult LoadFile(ImportContext& context, BASE_NS::string_view file, BASE_NS::string_view type);

SCENE_IMP_END_NAMESPACE()

#endif