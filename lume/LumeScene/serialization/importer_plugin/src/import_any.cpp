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

#include "import_any.h"

#include <core/json/json.h>

#include <meta/api/array_util.h>
#include <meta/base/type_traits.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/resource/intf_resource.h>

#include "import_context.h"
#include "import_helpers.h"
#include "numeric_any.h"
#include "objects/builtin.h"
#include "objects/property.h"
#include "util.h"

SCENE_IMP_BEGIN_NAMESPACE()

template<typename Type, typename Option, typename... Options>
static bool SetNumberValue(META_NS::IAny& any, const Type& v)
{
    if (META_NS::IsSetCompatible(any, META_NS::GetTypeId<Option>())) {
        if (IsValueInRange<Option>(v)) {
            if (any.SetValue(static_cast<Option>(v))) {
                return true;
            }
        }
    }
    if constexpr (sizeof...(Options)) {
        return SetNumberValue<Type, Options...>(any, v);
    }
    return false;
}

static IDiagnostics::Ptr SetArray(ImportContext& context, META_NS::IAny& any, const CORE_NS::json::value::array& array)
{
    auto arrany = interface_cast<META_NS::IArrayAny>(&any);
    if (!arrany) {
        CORE_LOG_E("Mismatch of property type");
        return context.CreateDiagnostics("Mismatch of property type");
    }
    if (auto item = any.Clone({META_NS::CloneValueType::DEFAULT_VALUE, META_NS::TypeIdRole::ITEM})) {
        ErrorHandler h(context);
        for (auto&& v : array) {
            if (auto err = SetAnyValue(context, *item, v); h.Handle(err)) {
                return err;
            } else if (!err) {
                arrany->InsertAnyAt(-1, *item);
            }
        }
        return h;
    }
    CORE_LOG_E("Failed to import array property (element type mismatch or missing 'values' array)");
    return context.CreateDiagnostics(
        "Failed to import array property (element type mismatch or missing 'values' array)");
}

IDiagnostics::Ptr ReadObjectIdent(ImportContext& context, ObjectIdent& out)
{
    out.type = context.GetOptString("type");
    out.objectUid = GetOptObjectId(context, "objectUid");
    return out.objectUid.error;
}

IDiagnostics::Ptr MatchClassId(ImportContext& context, const META_NS::IObject& obj, const ObjectIdent& ident)
{
    if (!ident.objectUid.value) {
        return nullptr;
    }
    const auto& expected = *ident.objectUid.value;
    auto actual = obj.GetClassId();
    if (actual == expected) {
        return nullptr;
    }
    CORE_LOG_E("Annotated objectUid does not match live object class (annotated: %s, actual: %s)",
        expected.ToString().c_str(),
        actual.ToString().c_str());
    return context.CreateDiagnostics("Annotated objectUid '" + expected.ToString() +
                                     "' does not match live object class '" + actual.ToString() + "'");
}

ImportResult ImportObject(ImportContext& context)
{
    ErrorHandler h(context);
    ObjectIdent ident;
    if (auto err = ReadObjectIdent(context, ident); h.Handle(err)) {
        return ImportResult{err};
    }
    if (ident.type.empty() && !ident.objectUid.value) {
        CORE_LOG_E("Object missing both 'type' and 'objectUid'");
        return ImportResult{context.CreateDiagnostics("Object missing both 'type' and 'objectUid'")};
    }
    if (ident.objectUid.value) {
        auto obj = META_NS::GetObjectRegistry().Create(*ident.objectUid.value);
        if (!obj) {
            CORE_LOG_E("Failed to create attachment (id: %s)", ident.objectUid.value->ToString().c_str());
            return ImportResult{
                context.CreateDiagnostics("Failed to create attachment: " + ident.objectUid.value->ToString())};
        }
        if (auto err = ImportProperties(context, *obj); h.Handle(err)) {
            return ImportResult{err};
        }
        ImportResult result{obj};
        result.error = h;
        return result;
    }
    return context.ImportSubType(ident.type, context.GetJsonValue());
}

// A resourceId builtin can target a resource-pointer property (a material/image/shader/mesh
// slot). The raw id cannot be assigned to a pointer any, so resolve it through the resource
// manager and assign the live resource instead. `handled` is set when the builtin actually
// carried a valid resourceId, regardless of whether resolving it succeeded.
static IDiagnostics::Ptr SetResourceIdPointer(
    ImportContext& context, IBuiltinContainer& builtin, META_NS::IAny& any, bool& handled)
{
    handled = false;
    CORE_NS::ResourceId rid;
    auto src = builtin.GetAny();
    if (!src || !src->GetValue(rid) || !rid.IsValid()) {
        return nullptr;
    }
    handled = true;
    auto resource = ResolveDeferredResource(context.GetImportParameters(), context.GetRenderContext(), rid);
    if (!resource) {
        CORE_LOG_E("No such resource: %s", rid.ToString().c_str());
        return context.CreateDiagnostics("No such resource: " + rid.ToString());
    }
    if (!META_NS::SetPointer(any, interface_pointer_cast<CORE_NS::IInterface>(resource))) {
        CORE_LOG_E("Mismatch of property type (resourceId '%s')", rid.ToString().c_str());
        return context.CreateDiagnostics("Mismatch of property type (resourceId '" + rid.ToString() + "')");
    }
    return nullptr;
}

static IDiagnostics::Ptr SetObject(ImportContext& context, META_NS::IAny& any)
{
    auto res = ImportObject(context);
    if (!res) {
        return res.error;
    }
    if (auto object = interface_cast<IBuiltinContainer>(res.object)) {
        if (object->SetToAny(any)) {
            return res.error;
        }
        // SetToAny fails when a resourceId builtin targets a resource-pointer property:
        // resolve the id to a live resource and assign that instead.
        bool handled = false;
        if (auto err = SetResourceIdPointer(context, *object, any, handled); handled) {
            return err;
        }
    } else if (META_NS::SetPointer(any, interface_pointer_cast<CORE_NS::IInterface>(res.object))) {
        return res.error;
    }
    CORE_LOG_E("Mismatch of property type");
    return context.CreateDiagnostics("Mismatch of property type");
}

IDiagnostics::Ptr SetAnyValue(ImportContext& context, META_NS::IAny& any, const CORE_NS::json::value& value)
{
    if (value.is_string()) {
        if (any.SetValue(CORE_NS::json::unescape(value.string_))) {
            return nullptr;
        }
    } else if (value.is_boolean()) {
        if (any.SetValue(value.boolean_)) {
            return nullptr;
        }
    } else if (value.is_signed_int()) {
        if (SetNumberValue<int64_t, int64_t, int32_t, int16_t, int8_t, double, float>(any, value.signed_)) {
            return nullptr;
        }
    } else if (value.is_unsigned_int()) {
        if (SetNumberValue<uint64_t,
                uint64_t,
                uint32_t,
                uint16_t,
                uint8_t,
                int64_t,
                int32_t,
                int16_t,
                int8_t,
                double,
                float>(any, value.unsigned_)) {
            return nullptr;
        }
    } else if (value.is_floating_point()) {
        if (SetNumberValue<double, double, float>(any, value.float_)) {
            return nullptr;
        }
    } else if (value.is_null()) {
        if (META_NS::SetPointer(any, BASE_NS::shared_ptr<CORE_NS::IInterface>{})) {
            return nullptr;
        }
    } else if (value.is_array()) {
        return SetArray(context, any, value.array_);
    } else if (value.is_object()) {
        auto ncont = context.CreateContext(value);
        return SetObject(ncont, any);
    }
    CORE_LOG_E("Mismatch of property type");
    return context.CreateDiagnostics("Mismatch of property type");
}

static IDiagnostics::Ptr GetArray(
    ImportContext& context, const CORE_NS::json::value::array& array, META_NS::IAny::Ptr& result)
{
    ErrorHandler h(context);
    BASE_NS::vector<META_NS::IAny::Ptr> items;
    items.reserve(array.size());
    for (auto&& v : array) {
        META_NS::IAny::Ptr item;
        if (auto err = GetAnyValue(context, v, item); h.Handle(err)) {
            return err;
        } else {
            items.push_back(BASE_NS::move(item));
        }
    }
    result = META_NS::ConstructAny<META_NS::IAny::Ptr>(BASE_NS::move(items));
    return h;
}

static IDiagnostics::Ptr GetObject(ImportContext& context, META_NS::IAny::Ptr& result)
{
    auto res = ImportObject(context);
    if (!res) {
        return res.error;
    }
    if (auto bc = interface_cast<IBuiltinContainer>(res.object)) {
        result = bc->GetAny();
    } else {
        result = META_NS::ConstructAny<META_NS::IObject::Ptr>(BASE_NS::move(res.object));
    }
    return res.error;
}

IDiagnostics::Ptr GetAnyValue(ImportContext& context, const CORE_NS::json::value& value, META_NS::IAny::Ptr& result)
{
    if (value.is_string()) {
        result = META_NS::ConstructAny<BASE_NS::string>(BASE_NS::string(value.string_));
    } else if (value.is_boolean()) {
        result = META_NS::ConstructAny<bool>(value.boolean_);
    } else if (value.is_signed_int()) {
        result = MakeSignedIntAny(value.signed_);
    } else if (value.is_unsigned_int()) {
        result = MakeUnsignedIntAny(value.unsigned_);
    } else if (value.is_floating_point()) {
        result = MakeFloatingAny(value.float_);
    } else if (value.is_array()) {
        return GetArray(context, value.array_, result);
    } else if (value.is_object()) {
        auto ncont = context.CreateContext(value);
        return GetObject(ncont, result);
    }
    // null → result stays null, no error
    return nullptr;
}

SCENE_IMP_END_NAMESPACE()
