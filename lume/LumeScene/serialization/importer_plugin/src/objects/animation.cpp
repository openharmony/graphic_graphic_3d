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

#include "animation.h"

#include <scene/interface/resource/types.h>

#include <meta/api/metadata_util.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/property/construct_array_property.h>

#include "../import_any.h"
#include "../import_context.h"
#include "../import_helpers.h"
#include "../resolve_object.h"
#include "curve.h"

SCENE_IMP_BEGIN_NAMESPACE()

// Import IAnimation::Name (string)
static IDiagnostics::Ptr ImportAnimationName(ImportContext& context, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    if (auto value = GetOptString(context, "name"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty(meta, "Name", value.GetValue());
    }
    return h;
}

// Import IAnimation::Enabled (bool)
static IDiagnostics::Ptr ImportAnimationEnabled(ImportContext& context, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    if (auto value = GetOptBool(context, "enabled"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty(meta, "Enabled", value.GetValue());
    }
    return h;
}

// Import ILoop::LoopCount (int32_t)
static IDiagnostics::Ptr ImportAnimationLoop(ImportContext& context, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    if (auto value = GetOptInt(context, "loop"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty(meta, "LoopCount", static_cast<int32_t>(value.GetValue()));
    }
    return h;
}

// Import ISpeed::SpeedFactor (float)
static IDiagnostics::Ptr ImportAnimationSpeed(ImportContext& context, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    if (auto value = GetOptFloat(context, "speed"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty(meta, "SpeedFactor", value.GetValue());
    }
    return h;
}

// Import IAnimation::Curve (IObject::Ptr via built-in type or custom objectUid)
static IDiagnostics::Ptr ImportAnimationCurve(ImportContext& context, META_NS::IMetadata& meta)
{
    auto curveObj = context.GetOptObject("curve");
    if (curveObj.empty()) {
        return nullptr;
    }
    auto curveContext = context.CreateContext(CORE_NS::json::value(BASE_NS::move(curveObj)));
    auto result = ImportCurve(curveContext);
    if (!result) {
        return result.error;
    }
    AddSetProperty(meta, "Curve", result.object);
    return result.error;
}

// Import ITimedAnimation::Duration (float, milliseconds)
static IDiagnostics::Ptr ImportTrackDuration(ImportContext& context, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    if (auto value = GetOptFloat(context, "duration"); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        AddSetProperty(meta, "DurationMs", value.GetValue());
    }
    return h;
}

// Resolve the deferred animation property path against the scene root.
// The hierarchy is built only after the index has loaded, so resolution
// runs at end of `ImportScene` via the Hierarchy-tier deferred scope.
static IDiagnostics::Ptr ResolveDeferredTrackProperty(
    ImportContext& context, META_NS::IMetadata& meta, BASE_NS::string path)
{
    auto scene = context.GetImportParameters().scene;
    if (!scene) {
        return nullptr;
    }
    auto root = interface_pointer_cast<META_NS::IObject>(scene->GetRootNode().GetResult());
    auto result = ResolveObject(context, root, path);
    if (!result) {
        return nullptr;
    }
    auto resolved = interface_pointer_cast<META_NS::IProperty>(result.object);
    auto existing = meta.GetProperty<META_NS::IProperty::WeakPtr>("Property", META_NS::MetadataQuery::EXISTING);
    if (existing && META_NS::GetValue(existing).lock()) {
        return nullptr;
    }
    AddSetProperty(meta, "Property", META_NS::IProperty::WeakPtr(resolved));
    return nullptr;
}

// Import target property path (string). Resolution against the scene hierarchy
// is deferred to end of scene import via the Hierarchy tier; the import-time
// step only stores `PropertyPath`. The deferred action will populate `Property`
// once the rootNode hierarchy is built.
static IDiagnostics::Ptr ImportTrackProperty(ImportContext& context, META_NS::IMetadata& meta)
{
    ErrorHandler h(context);
    auto value = GetOptString(context, "property");
    if (!h.HandleOptValue(value) || value.error) {
        return value.error ? value.error : IDiagnostics::Ptr{};
    }
    auto path = value.GetValue();
    AddSetProperty(meta, "PropertyPath", path);
    auto inst = interface_cast<META_NS::IObjectInstance>(&meta);
    auto metaPtr = inst ? inst->GetSelf<META_NS::IMetadata>() : nullptr;
    auto deferCtx = context.CreateContext(CORE_NS::json::value{});
    deferCtx.ClearDeferredScopes();
    context.AddDeferredAction(ImportContext::DeferTier::Hierarchy,
        [deferCtx = BASE_NS::move(deferCtx), metaPtr, path]() mutable -> IDiagnostics::Ptr {
            return metaPtr ? ResolveDeferredTrackProperty(deferCtx, *metaPtr, path) : nullptr;
        });
    return h;
}

// Import ITrackAnimation::Timestamps (array of float)
static IDiagnostics::Ptr ImportTimestamps(ImportContext& context, META_NS::IMetadata& meta)
{
    auto arr = context.GetOptArray("timestamps");
    if (arr.empty()) {
        return nullptr;
    }
    ErrorHandler h(context);
    BASE_NS::vector<float> timestamps;
    timestamps.reserve(arr.size());
    for (auto&& v : arr) {
        if (!v.is_number()) {
            auto err = context.CreateDiagnostics("Invalid timestamps entry: expected number");
            if (h.Handle(err)) {
                return err;
            }
        } else {
            timestamps.push_back(v.as_number<float>());
        }
    }
    auto p = META_NS::ConstructArrayProperty<float>("Timestamps", timestamps);
    meta.AddProperty(p.GetProperty());
    return h;
}

// Import keyframe values (array of IAny::Ptr)
static IDiagnostics::Ptr ImportKeyframes(ImportContext& context, META_NS::IMetadata& meta)
{
    auto arr = context.GetOptArray("keyframes");
    if (arr.empty()) {
        return nullptr;
    }
    ErrorHandler h(context);
    BASE_NS::vector<META_NS::IAny::Ptr> keyframes;
    keyframes.reserve(arr.size());
    for (auto&& v : arr) {
        META_NS::IAny::Ptr any;
        if (auto err = GetAnyValue(context, v, any); h.Handle(err)) {
            return err;
        } else {
            keyframes.push_back(BASE_NS::move(any));
        }
    }
    auto p = META_NS::ConstructArrayProperty<META_NS::IAny::Ptr>("KeyframeValues", keyframes);
    meta.AddProperty(p.GetProperty());
    return h;
}

// Import a single keyframe curve entry.
static ImportResult ImportKeyframeCurveEntry(ImportContext& context, const CORE_NS::json::value& v)
{
    auto curveContext = context.CreateContext(v);
    return ImportCurve(curveContext);
}

// Import ITrackAnimation::KeyframeCurves (array of IObject::Ptr)
static IDiagnostics::Ptr ImportKeyframeCurves(ImportContext& context, META_NS::IMetadata& meta)
{
    auto arr = context.GetOptArray("keyframeCurves");
    if (arr.empty()) {
        return nullptr;
    }
    ErrorHandler h(context);
    BASE_NS::vector<META_NS::IObject::Ptr> curves;
    curves.reserve(arr.size());
    for (auto&& v : arr) {
        if (v.is_null()) {
            curves.push_back(nullptr);
        } else if (v.is_object()) {
            auto result = ImportKeyframeCurveEntry(context, v);
            if (!result && h.Handle(result.error)) {
                return result.error;
            } else if (result) {
                curves.push_back(BASE_NS::move(result.object));
            }
        } else {
            auto err = context.CreateDiagnostics("Invalid keyframeCurves entry: expected object or null");
            if (h.Handle(err)) {
                return err;
            }
        }
    }
    auto p = META_NS::ConstructArrayProperty<META_NS::IObject::Ptr>("KeyframeCurves", curves);
    meta.AddProperty(p.GetProperty());
    return h;
}

// Import a single IAny::Ptr value from a JSON field (e.g. "from", "to")
static IDiagnostics::Ptr ImportAnyField(
    ImportContext& context, META_NS::IMetadata& meta, BASE_NS::string_view jsonName, BASE_NS::string_view propName)
{
    auto val = context.GetOptValue(jsonName);
    if (val.is_null()) {
        return nullptr;
    }
    META_NS::IAny::Ptr any;
    if (auto err = GetAnyValue(context, val, any)) {
        return err;
    }
    AddSetProperty(meta, propName, BASE_NS::move(any));
    return nullptr;
}

static bool IsTrackAnimation(BASE_NS::string_view type)
{
    return type == "trackAnimation";
}

static bool IsKeyframeAnimation(BASE_NS::string_view type)
{
    return type == "keyframeAnimation";
}

static bool IsTimedPropertyAnimation(BASE_NS::string_view type)
{
    return type == "trackAnimation" || type == "keyframeAnimation";
}

static bool IsContainerAnimation(BASE_NS::string_view type)
{
    return type == "sequentialAnimation" || type == "parallelAnimation";
}

struct DeferredChildEntry {
    size_t index{};
    CORE_NS::ResourceId rid;
};

static IDiagnostics::Ptr ResolveChildAnimation(const META_NS::IObject::Ptr& owner, size_t idx,
    const CORE_NS::ResourceId& rid, const ImportContextParameters& params,
    const SCENE_NS::IRenderContext::Ptr& renderCtx)
{
    auto resource = ResolveDeferredResource(params, renderCtx, rid);
    if (!resource) {
        CORE_LOG_W("Failed to resolve child animation: %s", rid.ToString().c_str());
        return nullptr;
    }
    auto m = interface_cast<META_NS::IMetadata>(owner);
    if (!m) {
        return nullptr;
    }
    auto prop = m->GetArrayProperty<META_NS::IObject::Ptr>("Animations", META_NS::MetadataQuery::EXISTING);
    if (prop) {
        prop->SetValueAt(idx, interface_pointer_cast<META_NS::IObject>(resource));
    }
    return nullptr;
}

static void RegisterDeferredChildResolution(
    ImportContext& context, const META_NS::IObject::Ptr& owner, const BASE_NS::vector<DeferredChildEntry>& entries)
{
    auto params = context.GetImportParameters();
    auto renderCtx = context.GetRenderContext();
    for (auto&& entry : entries) {
        auto idx = entry.index;
        auto rid = entry.rid;
        context.AddDeferredAction(ImportContext::DeferTier::Resource, [owner, idx, rid, params, renderCtx]() {
            return ResolveChildAnimation(owner, idx, rid, params, renderCtx);
        });
    }
}

// Parse a single child animation entry from the JSON array
static IDiagnostics::Ptr ParseChildEntry(ImportContext& context, const CORE_NS::json::value& v, ErrorHandler& h,
    BASE_NS::vector<META_NS::IObject::Ptr>& children, BASE_NS::vector<DeferredChildEntry>& deferred)
{
    if (!v.is_object()) {
        auto err = context.CreateDiagnostics("Invalid animations entry: expected object");
        if (h.Handle(err)) {
            return err;
        }
        return nullptr;
    }
    auto childContext = context.CreateContext(v);
    auto type = childContext.GetOptString("type");
    if (type.empty()) {
        auto err = context.CreateDiagnostics("Child animation missing 'type' field");
        if (h.Handle(err)) {
            return err;
        }
    } else if (type == "resourceId") {
        auto rid = GetOptResourceId(childContext);
        if (rid.error && h.Handle(rid.error)) {
            return rid.error;
        }
        if (rid.value) {
            deferred.push_back({children.size(), *rid.value});
            children.push_back(nullptr);
        }
    } else {
        auto result = childContext.ImportSubType(type, childContext.GetJsonValue());
        if (!result && h.Handle(result.error)) {
            return result.error;
        }
        if (result) {
            children.push_back(BASE_NS::move(result.object));
        }
    }
    return nullptr;
}

// Import child animations array for sequential/parallel containers
static IDiagnostics::Ptr ImportChildAnimations(
    ImportContext& context, META_NS::IMetadata& meta, const META_NS::IObject::Ptr& owner)
{
    auto arr = context.GetOptArray("animations");
    if (arr.empty()) {
        return nullptr;
    }
    ErrorHandler h(context);
    BASE_NS::vector<META_NS::IObject::Ptr> children;
    children.reserve(arr.size());
    BASE_NS::vector<DeferredChildEntry> deferred;

    for (auto&& v : arr) {
        if (auto err = ParseChildEntry(context, v, h, children, deferred)) {
            return err;
        }
    }
    auto p = META_NS::ConstructArrayProperty<META_NS::IObject::Ptr>("Animations", children);
    meta.AddProperty(p.GetProperty());

    RegisterDeferredChildResolution(context, owner, deferred);
    return h;
}

// Base properties shared by every animation type, plus the curve for timed animations.
static IDiagnostics::Ptr ImportBaseAnimationFields(
    ImportContext& context, META_NS::IMetadata& meta, BASE_NS::string_view animationType, ErrorHandler& h)
{
    if (auto err = ImportAnimationName(context, meta); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportAnimationEnabled(context, meta); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportAnimationLoop(context, meta); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportAnimationSpeed(context, meta); h.Handle(err)) {
        return err;
    }
    if (IsTimedPropertyAnimation(animationType)) {
        if (auto err = ImportAnimationCurve(context, meta); h.Handle(err)) {
            return err;
        }
    }
    return nullptr;
}

static IDiagnostics::Ptr ImportTrackAnimationFields(ImportContext& context, META_NS::IMetadata& meta, ErrorHandler& h)
{
    if (auto err = ImportTrackDuration(context, meta); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportTrackProperty(context, meta); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportTimestamps(context, meta); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportKeyframes(context, meta); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportKeyframeCurves(context, meta); h.Handle(err)) {
        return err;
    }
    return nullptr;
}

static IDiagnostics::Ptr ImportKeyframeAnimationFields(
    ImportContext& context, META_NS::IMetadata& meta, ErrorHandler& h)
{
    if (auto err = ImportTrackDuration(context, meta); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportTrackProperty(context, meta); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportAnyField(context, meta, "from", "From"); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportAnyField(context, meta, "to", "To"); h.Handle(err)) {
        return err;
    }
    return nullptr;
}

ImportResult ImportAnimation::Import(ImportContext& context)
{
    auto animationType = context.GetOptString("type");
    auto trace = context.Trace("Importing animation (type: '" + animationType + "')");
    auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(SCENE_NS::ClassId::AnimationTemplate);
    if (!obj) {
        return ImportResult{context.CreateDiagnostics("Failed to create animation template")};
    }
    auto meta = interface_cast<META_NS::IMetadata>(obj.get());
    if (!meta) {
        return ImportResult{context.CreateDiagnostics("Animation template has no metadata")};
    }

    AddSetProperty(*meta, "AnimationType", BASE_NS::string(animationType));

    ErrorHandler h(context);
    if (auto err = ImportBaseAnimationFields(context, *meta, animationType, h)) {
        return ImportResult{err};
    }
    if (IsTrackAnimation(animationType)) {
        if (auto err = ImportTrackAnimationFields(context, *meta, h)) {
            return ImportResult{err};
        }
    }
    if (IsKeyframeAnimation(animationType)) {
        if (auto err = ImportKeyframeAnimationFields(context, *meta, h)) {
            return ImportResult{err};
        }
    }
    if (IsContainerAnimation(animationType)) {
        if (auto err = ImportChildAnimations(context, *meta, obj); h.Handle(err)) {
            return ImportResult{err};
        }
    }

    ImportResult result{obj};
    result.error = h;
    return result;
}

SCENE_IMP_END_NAMESPACE()
