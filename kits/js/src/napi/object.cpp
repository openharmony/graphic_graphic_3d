/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "object.h"

#include <base/containers/string.h>

#include "TrueRootObject.h"
#include "function.h"
#include "utils.h"

namespace NapiApi {

Object::Object(napi_env env) : env_(env)
{
    if (!env_) {
        return;
    }
    napi_create_object(env, &object_);
    if (object_) {
        napi_typeof(env, object_, &jstype);
    }
    if (jstype != napi_object) {
        // value was not an object!
        env_ = {};
        object_ = nullptr;
        jstype = napi_undefined;
    }
}

Object::Object(napi_env env, napi_value v) : env_(env), object_(v)
{
    if (!env_) {
        return;
    }
    if (v) {
        napi_typeof(env_, v, &jstype);
    }
    if (jstype != napi_object) {
        // value was not an object!
        env_ = {};
        object_ = nullptr;
        jstype = napi_undefined;
    }
}

Object::Object(napi_env env, BASE_NS::string_view className, const JsFuncArgs& args) : env_(env)
{
    NapiApi::MyInstanceState* mis {};
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    const auto ctor = NapiApi::Function(env, mis->FetchCtor(className));

    napi_new_instance(env_, ctor, args.argc, args.argv, &object_);
    if (object_) {
        napi_typeof(env_, object_, &jstype);
    }
    if (jstype != napi_object) {
        // value was not an object!
        env_ = {};
        object_ = nullptr;
        jstype = napi_undefined;
    }
}

napi_value Object::ToNapiValue() const
{
    return object_;
}

napi_env Object::GetEnv() const
{
    return env_;
}

TrueRootObject* Object::GetRoot() const
{
    return GetInterface<TrueRootObject>();
}

META_NS::IObject::Ptr Object::GetNative() const
{
    if (auto tro = GetRoot()) {
        return tro->GetNativeObject();
    }
    return {};
}

napi_value Object::Get(const BASE_NS::string_view name)
{
    auto tmp = GetNamedPropertyAndType(name);
    if (tmp.jstype == napi_null) {
        return nullptr;
    }
    if (tmp.jstype == napi_undefined) {
        return nullptr;
    }
    return tmp.res;
}

void Object::Set(const BASE_NS::string_view name, napi_value value)
{
    if (!env_ || !object_) {
        return;
    }
    // could check if it is declared. and optionally add it. (now it just adds it if not declared)
    napi_set_named_property(env_, object_, BASE_NS::string(name).c_str(), value);
}

void Object::Set(const BASE_NS::string_view name, const Object& value)
{
    Set(name, value.ToNapiValue());
}

void Object::Set(const BASE_NS::string_view name, const BASE_NS::string_view v)
{
    if (!env_ || !object_) {
        return;
    }
    napi_status status;
    napi_value value = MakeTempString(v);
    status = napi_set_named_property(env_, object_, BASE_NS::string(name).c_str(), value);
}

Object::operator bool() const
{
    if (!env_ || !object_) {
        return false;
    }
    return (napi_object == jstype);
}

bool Object::Has(const BASE_NS::string_view name)
{
    if (!env_ || !object_) {
        return false;
    }
    bool res;
    napi_value key = MakeTempString(name);
    napi_status status = napi_has_property(env_, object_, key, &res);
    if (napi_ok != status) {
        return false;
    }
    return res;
}

bool Object::IsNull() const
{
    return (napi_null == jstype);
}

bool Object::IsNull(const BASE_NS::string_view name)
{
    auto tmp = GetNamedPropertyAndType(name);
    if (!tmp.success) {
        return true;
    }
    return (tmp.jstype == napi_null);
}

bool Object::IsDefined() const
{
    return (napi_undefined != jstype);
}

bool Object::IsUndefined(const BASE_NS::string_view name)
{
    auto tmp = GetNamedPropertyAndType(name);
    if (!tmp.success) {
        return true;
    }
    return (tmp.jstype == napi_undefined);
}

bool Object::IsUndefinedOrNull(const BASE_NS::string_view name)
{
    auto tmp = GetNamedPropertyAndType(name);
    if (!tmp.success) {
        return true;
    }
    return ((tmp.jstype == napi_null) || (tmp.jstype == napi_undefined));
}

bool Object::IsSame(const Object& other) const
{
    return object_ == other.object_;
}

bool Object::StrictEqual(const Object& other) const
{
    if (!env_) {
        CORE_LOG_F("Unitialized object, can't compare strict equality");
        assert(false);
        return false;
    }

    bool equal = false;
    napi_strict_equals(env_, object_, other.object_, &equal);
    return equal;
}

void Object::AddProperty(const napi_property_descriptor desc)
{
    if (!env_ || !object_) {
        return;
    }
    napi_status status = napi_define_properties(env_, object_, 1, &desc);
}

bool Object::DeleteProperty(const BASE_NS::string_view name)
{
    if (!env_ || !object_) {
        return false;
    }
    // remove property from object.
    napi_status status;
    bool result { false };
    napi_value key = MakeTempString(name);
    status = napi_delete_property(env_, object_, key, &result);
    return result;
}

Object::NameAndType Object::GetNamedPropertyAndType(const BASE_NS::string_view name)
{
    napi_value res;
    napi_status status;
    napi_valuetype jstype;
    if (!env_ || !object_) {
        return { false, nullptr, napi_undefined };
    }
    status = napi_get_named_property(env_, object_, BASE_NS::string(name).c_str(), &res);
    if ((!res) || (napi_ok != status)) {
        return { false, nullptr, napi_undefined };
    }
    napi_typeof(env_, res, &jstype);
    return { true, res, jstype };
}

napi_value Object::MakeTempString(const BASE_NS::string_view v)
{
    return env_.GetString(v);
}

} // namespace NapiApi
