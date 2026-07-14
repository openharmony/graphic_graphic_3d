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

#include "property.h"

#include <core/json/json.h>

#include <meta/api/array_util.h>
#include <meta/api/metadata_util.h>
#include <meta/api/util.h>

#include "../import_any.h"
#include "../import_context.h"
#include "../import_helpers.h"
#include "../perf/cpu_perf_scope.h"
#include "../resolve_object.h"
#include "../util.h"

SCENE_IMP_BEGIN_NAMESPACE()

static IDiagnostics::Ptr SetPropertyValue(
    ImportContext& context, META_NS::IProperty& prop, const CORE_NS::json::value& value)
{
    auto trace = context.Trace("Importing property '" + prop.GetName() + "'");
    META_NS::PropertyLock l{&prop};
    if (auto any = l->GetValueAny().Clone()) {
        if (auto err = SetAnyValue(context, *any, value)) {
            return err;
        }
        if (context.IsTemplateContext()) {
            l->SetDefaultValueAny(*any);
            if (!META_NS::IsValueSet(prop)) {
                prop.ResetValue();
            }
        } else {
            prop.SetValue(*any);
        }
        return nullptr;
    }
    CORE_LOG_E("Failed to import property: '%s'", prop.GetName().c_str());
    return context.CreateDiagnostics("Failed to import property '" + prop.GetName() + "'");
}

static bool CanDeferObjectRef(ImportContext& context, const CORE_NS::json::value& value)
{
    if (!value.is_object() || !context.FindDeferredScope(ImportContext::DeferTier::Hierarchy)) {
        return false;
    }
    auto ncont = context.CreateContext(value);
    return ncont.GetOptString("type") == "objectRef";
}

static void DeferObjectRef(
    ImportContext& context, const META_NS::IProperty::Ptr& prop, const CORE_NS::json::value& value)
{
    auto ncont = context.CreateContext(value);
    auto path = BASE_NS::string(ncont.GetOptString("path"));

    // Create a full copy of the current context for use in the deferred action.
    // This preserves the original params (scene, object), resource group overrides,
    // template flag, and trace stack for correct error reporting. Clear the
    // deferred-scope stack since deferred actions should never register further
    // actions.
    auto deferCtx = context.CreateContext(CORE_NS::json::value{});
    deferCtx.ClearDeferredScopes();

    context.AddDeferredAction(ImportContext::DeferTier::Hierarchy,
        [deferCtx = BASE_NS::move(deferCtx), prop, path]() mutable -> IDiagnostics::Ptr {
            auto res = ResolveObject(deferCtx, deferCtx.GetImportParameters().object, path);
            if (!res) {
                return res.error;
            }
            META_NS::PropertyLock l{prop.get()};
            auto any = l->GetValueAny().Clone();
            if (!any || !META_NS::SetPointer(*any, interface_pointer_cast<CORE_NS::IInterface>(res.object))) {
                return deferCtx.CreateDiagnostics("objectRef: failed to set property");
            }
            if (deferCtx.IsTemplateContext()) {
                l->SetDefaultValueAny(*any);
                if (!META_NS::IsValueSet(*prop)) {
                    prop->ResetValue();
                }
            } else {
                prop->SetValue(*any);
            }
            return nullptr;
        });
}

// Query readonly from the wrapper object the property was looked up on.
// IProperty::GetOwner() is unreliable for forwarded properties — it returns
// the underlying component, not the wrapper, so its metadata may not carry
// the same readonly flag.
static bool IsReadOnly(const META_NS::IObject& object, const META_NS::IProperty& prop)
{
    if (auto meta = interface_cast<META_NS::IMetadata>(&object)) {
        return meta->GetMetadata(META_NS::MetadataType::PROPERTY, prop.GetName()).readOnly;
    }
    return false;
}

// A read-only object-pointer property (the RenderConfiguration pattern) cannot be
// assigned. Recurse into the object it already points to and import the nested
// "properties" onto it. `objectUid` is verified against the live object's class id.
static IDiagnostics::Ptr ImportReadOnlyProperty(
    ImportContext& context, const META_NS::IProperty::Ptr& prop, const CORE_NS::json::value& value)
{
    const auto& name = prop->GetName();
    if (!value.is_object()) {
        CORE_LOG_E("Read-only property '%s' expects a nested object value", name.c_str());
        return context.CreateDiagnostics("Read-only property '" + name + "' expects a nested object value");
    }
    auto ncont = context.CreateContext(value);
    ObjectIdent ident;
    if (auto err = ReadObjectIdent(ncont, ident)) {
        return err;
    }
    auto obj = META_NS::GetPointer<META_NS::IObject>(prop);
    if (!obj) {
        CORE_LOG_E("Cannot modify read-only property '%s' of non-object type", name.c_str());
        return context.CreateDiagnostics("Cannot modify read-only property '" + name + "' of non-object type");
    }
    if (auto err = MatchClassId(ncont, *obj, ident)) {
        return err;
    }
    return ImportProperties(ncont, *obj);
}

static IDiagnostics::Ptr ImportProperty(
    ImportContext& context, META_NS::IObject& object, const BASE_NS::string& name, const CORE_NS::json::value& value)
{
    auto meta = interface_cast<META_NS::IMetadata>(&object);
    if (!meta) {
        CORE_LOG_E("Object does not implement IMetadata (object: '%s', property: '%s')",
            object.GetName().c_str(),
            name.c_str());
        return context.CreateDiagnostics(
            "Object does not implement IMetadata (object: '" + object.GetName() + "', property: '" + name + "')");
    }
    auto prop = meta->GetProperty(name);
    if (!prop) {
        CORE_LOG_E("No such property '%s' on object '%s'", name.c_str(), object.GetName().c_str());
        return context.CreateDiagnostics("No such property '" + name + "' on object '" + object.GetName() + "'");
    }
    if (IsReadOnly(object, *prop)) {
        return ImportReadOnlyProperty(context, prop, value);
    }
    if (auto err = SetPropertyValue(context, *prop, value)) {
        if (CanDeferObjectRef(context, value)) {
            DeferObjectRef(context, prop, value);
            return nullptr;
        }
        return err;
    }
    return nullptr;
}

IDiagnostics::Ptr ImportProperty(ImportContext& context, META_NS::IObject& object)
{
    BASE_NS::string name;
    if (auto err = context.GetRequiredString("name", name)) {
        return err;
    }
    auto value = context.GetOptValue("value");
    if (!value) {
        CORE_LOG_E("Invalid or missing value for property: %s", name.c_str());
        return context.CreateDiagnostics("Invalid or missing value for property: " + name);
    }
    return ImportProperty(context, object, name, value);
}

IDiagnostics::Ptr ImportProperty(
    ImportContext& context, META_NS::IObject& object, const META_NS::IProperty::Ptr& property)
{
    auto value = context.GetOptValue("value");
    if (!value) {
        CORE_LOG_E("Invalid or missing value for property: %s", property->GetName().c_str());
        return context.CreateDiagnostics("Invalid or missing value for property: " + property->GetName());
    }
    if (IsReadOnly(object, *property)) {
        return ImportReadOnlyProperty(context, property, value);
    }
    if (auto err = SetPropertyValue(context, *property, value)) {
        if (CanDeferObjectRef(context, value)) {
            DeferObjectRef(context, property, value);
            return nullptr;
        }
        return err;
    }
    return nullptr;
}

IDiagnostics::Ptr ImportProperties(ImportContext& context, META_NS::IObject& object)
{
    SCENE_IMP_CPU_PERF_SCOPE("Import", "Properties");
    ErrorHandler h(context);
    auto props = context.GetOptArray("properties");
    for (size_t i = 0; i < props.size(); ++i) {
        auto ncont = context.CreateContext(props[i]);
        auto entryTrace = ncont.TraceIndex("properties", i);
        if (auto err = ImportProperty(ncont, object); h.Handle(err)) {
            return err;
        }
    }
    return h;
}

SCENE_IMP_END_NAMESPACE()