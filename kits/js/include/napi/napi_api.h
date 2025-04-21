/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef NAPI_API
#define NAPI_API
#define NAPI_VERSION 8

#ifdef __OHOS_PLATFORM__
#include "napi/native_api.h"
#else
#include <node_api.h>
#endif

#include <meta/interface/intf_object.h>

#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>

#define LOG_F(...) CORE_LOG_F(__VA_ARGS__)
#define LOG_E(...) CORE_LOG_E(__VA_ARGS__)
#define LOG_V(...)
#define LOG_W(...)

namespace NapiApi {
class Env {
    napi_env env_ { nullptr };

public:
    Env() = default;
    explicit Env(napi_env env) : env_(env) {}
    operator bool() const
    {
        return (env_);
    }
    operator napi_env() const
    {
        return env_;
    }
    napi_value GetUndefined() const
    {
        if (!env_) {
            return {};
        }
        napi_value undefined;
        napi_get_undefined(env_, &undefined);
        return undefined;
    }
    napi_value GetNull() const
    {
        if (!env_) {
            return {};
        }
        napi_value null;
        napi_get_null(env_, &null);
        return null;
    }
    napi_value GetBoolean(bool value) const
    {
        if (!env_) {
            return {};
        }
        napi_value val;
        napi_get_boolean(env_, value, &val);
        return val;
    }
    napi_value GetNumber(uint32_t value) const
    {
        if (!env_) {
            return {};
        }
        napi_value val;
        napi_create_uint32(env_, value, &val);
        return val;
    }
    napi_value GetNumber(int32_t value) const
    {
        if (!env_) {
            return {};
        }
        napi_value val;
        napi_create_int32(env_, value, &val);
        return val;
    }
    napi_value GetNumber(uint64_t value) const
    {
        if (!env_) {
            return {};
        }
        napi_value val;
        napi_create_bigint_uint64(env_, value, &val);
        return val;
    }
    napi_value GetNumber(int64_t value) const
    {
        if (!env_) {
            return {};
        }
        napi_value val;
        napi_create_bigint_int64(env_, value, &val);
        return val;
    }
    napi_value GetNumber(float value) const
    {
        if (!env_) {
            return {};
        }
        napi_value val;
        napi_create_double(env_, value, &val);
        return val;
    }
    napi_value GetNumber(double value) const
    {
        if (!env_) {
            return {};
        }
        napi_value val;
        napi_create_double(env_, value, &val);
        return val;
    }
    napi_value GetString(const BASE_NS::string_view value) const
    {
        napi_value val;
        napi_create_string_utf8(env_, value.data(), value.length(), &val);
        return val;
    }
};
template<typename type>
bool ValidateType(napi_valuetype jstype, bool isArray);

template<typename type>
class Value {
    napi_env env_ { nullptr };
    napi_value value_ { nullptr };
    napi_valuetype jstype = napi_undefined;

public:
    using Type = type;
    Value() = default;
    Value(napi_env env, Type v)
    {
        Init(env, v);
    }

    void Init(napi_env env, Type v);

    Value(napi_env env, napi_value v) : env_(env)
    {
        if ((env == nullptr) || (v == nullptr)) {
            return;
        }
        // validate type
        napi_status status = napi_invalid_arg;
        status = napi_typeof(env_, v, &jstype);
        if (status != napi_ok) {
            // okay then failed.
            return;
        }
        bool isArray = false;
        napi_is_array(env_, v, &isArray);

        if (ValidateType<type>(jstype, isArray)) {
            value_ = v;
        } else {
            jstype = napi_undefined;
        }
    }
    bool IsValid() const
    {
        return (env_ && value_);
    }
    bool IsNull() const
    {
        return (napi_null == jstype);
    }
    bool IsDefined() const
    {
        return !IsUndefined();
    }
    bool IsUndefined() const
    {
        return (napi_undefined == jstype);
    }
    bool IsUndefinedOrNull() const
    {
        if (!IsValid()) {
            return true;
        }
        return ((napi_null == jstype) || (napi_undefined == jstype));
    }
    bool IsDefinedAndNotNull() const
    {
        return ((napi_null != jstype) && (napi_undefined != jstype));
    }
    type valueOrDefault(const type defaultValue = {});
    operator type()
    {
        return valueOrDefault();
    }
    napi_env GetEnv() const
    {
        return env_;
    }
    napi_value ToNapiValue() const
    {
        return value_;
    }
};
class Function;

class Object {
public:
    operator bool() const
    {
        if (!env_ || !object_) {
            return false;
        }
        return (napi_object == jstype);
    }
    bool IsDefined() const
    {
        return (napi_undefined != jstype);
    }
    bool IsNull() const
    {
        return (napi_null == jstype);
    }
    Object() = default;
    explicit Object(Function ctor);
    Object(Function ctor, size_t argc, napi_value args[]);
    explicit Object(napi_env env) : env_(env)
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
    Object(napi_env env, napi_value v) : env_(env), object_(v)
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

    template<class T>
    T* Native()
    {
        if (!env_ || !object_) {
            return nullptr;
        }
        T* me = nullptr;
        napi_unwrap(env_, object_, (void**)&me);
        return me;
    }
    void Set(const BASE_NS::string_view name, napi_value value)
    {
        if (!env_ || !object_) {
            return;
        }
        // could check if it is declared. and optionally add it. (now it just adds it if not declared)
        napi_set_named_property(env_, object_, BASE_NS::string(name).c_str(), value);
    }
    template<typename type>
    void Set(const BASE_NS::string_view name, NapiApi::Value<type> value)
    {
        Set(name, value.ToNapiValue());
    }

    void Set(const BASE_NS::string_view name, NapiApi::Object value)
    {
        Set(name, value.ToNapiValue());
    }

    void Set(const BASE_NS::string_view name, const BASE_NS::string_view v)
    {
        if (!env_ || !object_) {
            return;
        }
        napi_status status;
        napi_value value = MakeTempString(v);
        status = napi_set_named_property(env_, object_, BASE_NS::string(name).c_str(), value);
    }

    bool Has(const BASE_NS::string_view name)
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

    napi_value Get(const BASE_NS::string_view name)
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
    template<typename t>
    Value<t> Get(const BASE_NS::string_view name)
    {
        return Value<t>(env_, Get(name));
    }
    bool IsUndefined(const BASE_NS::string_view name)
    {
        auto tmp = GetNamedPropertyAndType(name);
        if (!tmp.success) {
            return true;
        }
        return (tmp.jstype == napi_undefined);
    }

    bool IsNull(const BASE_NS::string_view name)
    {
        auto tmp = GetNamedPropertyAndType(name);
        if (!tmp.success) {
            return true;
        }
        return (tmp.jstype == napi_null);
    }

    bool IsUndefinedOrNull(const BASE_NS::string_view name)
    {
        auto tmp = GetNamedPropertyAndType(name);
        if (!tmp.success) {
            return true;
        }
        return ((tmp.jstype == napi_null) || (tmp.jstype == napi_undefined));
    }

    napi_value ToNapiValue() const
    {
        return object_;
    }

    napi_env GetEnv() const
    {
        return env_;
    }

    // note this compares the "value of object_"
    bool IsSame(NapiApi::Object other)
    {
        return object_ == other.object_;
    }

    // This API represents the invocation of the Strict Equality algorithm as defined in Section 7.2.14 of the
    // ECMAScript Language Specification.
    bool StrictEqual(napi_value other)
    {
        bool equal;
        if (env_) {
            // this is undefined. so return false.
            return false;
        }
        napi_strict_equals(env_, object_, other, &equal);
        // make it a bit more strict by checking wrapping
        void *l = nullptr;
        void *r = nullptr;
        napi_unwrap(env_, object_, &l);
        napi_unwrap(env_, other, &r);
        if (l || r) {
            if (l == r) {
                if (!equal) {
                    CORE_LOG_F("napi_strict_equals returns false, but wrapped native objects are same");
                }
                return true;
            } else {
                if (equal) {
                    CORE_LOG_F("napi_strict_equals returns true, but wrapped native objects are different");
                }
                return false;
            }
        }
        return equal;
    }
    bool StrictEqual(NapiApi::Object other)
    {
        return StrictEqual(other.object_);
    }
    void AddProperty(const napi_property_descriptor desc)
    {
        if (!env_ || !object_) {
            return;
        }
        napi_status status = napi_define_properties(env_, object_, 1, &desc);
    }
    bool DeleteProperty(const BASE_NS::string_view name)
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

protected:
    struct NameAndType {
        bool success { false };
        napi_value res { nullptr };
        napi_valuetype jstype { napi_undefined };
    };
    NameAndType GetNamedPropertyAndType(const BASE_NS::string_view name)
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
    napi_value MakeTempString(const BASE_NS::string_view v)
    {
        return env_.GetString(v);
    }

private:
    napi_valuetype jstype = napi_undefined;
    NapiApi::Env env_ { nullptr };
    napi_value object_ { nullptr };
};

template<typename... Types>
class FunctionContext {
public:
    template<typename First, typename... Rest>
    inline bool validate(size_t index)
    {
        napi_valuetype jstype;
        napi_status status = napi_invalid_arg;
        status = napi_typeof(env_, args[index], &jstype);
        bool isArray = false;
        napi_is_array(env_, args[index], &isArray);

        bool ret = ValidateType<First>(jstype, isArray);
        if (ret) {
            if constexpr (sizeof...(Rest) == 0) {
                return true;
            }
            if constexpr (sizeof...(Rest) > 0) {
                return validate<Rest...>(index + 1);
            }
        }
        return false;
    }
    template<typename... ot>
    FunctionContext(FunctionContext<ot...> other) : FunctionContext(other.GetEnv(), other.GetInfo())
    {}
    FunctionContext(const napi_env env, const napi_callback_info info)
    {
        if ((!env) || (!info)) {
            return;
        }
        napi_status status { napi_ok };
        status = napi_get_cb_info(env, info, &argCount_, nullptr, &jsThis_, &data_);
        if (status != napi_ok) {
            return;
        }
        env_ = NapiApi::Env(env);
        info_ = info;
        if constexpr (sizeof...(Types) > 0) {
            // validate arg count first
            if (argc != argCount_) {
                // non matching arg count. fail
                jsThis_ = nullptr;
                data_ = nullptr;
                info_ = nullptr;
                return;
            }

            // get the arguments
            status = napi_get_cb_info(env, info, &argCount_, args, nullptr, nullptr);
            if (!validate<Types...>(0)) {
                // non matching types in context!
                jsThis_ = nullptr;
                data_ = nullptr;
                info_ = nullptr;
                return;
            }
        }
    }
    operator bool() const
    {
        return (env_ && info_);
    }
    void* GetData() const
    {
        return data_;
    }
    NapiApi::Env Env() const
    {
        return env_;
    }
    napi_env GetEnv() const
    {
        return env_;
    }
    napi_callback_info GetInfo() const
    {
        return info_;
    }

    NapiApi::Object This()
    {
        return { env_, jsThis_ };
    }
    napi_value value(size_t index)
    {
        if (index < argc) {
            return args[index];
        }
        return nullptr;
    }

    template<size_t I, typename T, typename... TypesI>
    struct GetTypeImpl {
        using type = typename GetTypeImpl<I - 1, TypesI...>::type;
    };
    template<typename T, typename... TypesI>
    struct GetTypeImpl<0, T, TypesI...> {
        using type = T;
    };

    template<size_t index>
    auto Arg()
    {
        if constexpr (sizeof...(Types) > 0) {
            if constexpr (index < sizeof...(Types)) {
                return Value<typename GetTypeImpl<index, Types...>::type> { env_, args[index] };
            }
            if constexpr (index >= sizeof...(Types)) {
                static_assert(index < sizeof...(Types), "Index out of range !");
                return Value<void*>((napi_env) nullptr, (void*)nullptr);
            }
        }
        if constexpr (sizeof...(Types) == 0) {
            return;
        }
    }

    size_t ArgCount() const
    {
        return argCount_;
    }

    napi_value GetUndefined()
    {
        return env_.GetUndefined();
    }
    napi_value GetNull()
    {
        return env_.GetNull();
    }
    napi_value GetBoolean(bool value)
    {
        return env_.GetBoolean(value);
    }
    napi_value GetNumber(int32_t value)
    {
        return env_.GetNumber(value);
    }
    napi_value GetNumber(uint32_t value)
    {
        return env_.GetNumber(value);
    }
    napi_value GetNumber(float value)
    {
        return env_.GetNumber(value);
    }
    napi_value GetNumber(double value)
    {
        return env_.GetNumber(value);
    }
    napi_value GetString(const BASE_NS::string_view value)
    {
        return env_.GetString(value);
    }

private:
    napi_value jsThis_ { nullptr };
    void* data_ { nullptr };
    const size_t argc { sizeof...(Types) };
    napi_value args[sizeof...(Types) + 1] {};
    NapiApi::Env env_ { nullptr };
    napi_callback_info info_ { nullptr };
    size_t argCount_ { 0 };
};

class Array {
public:
    Array() = default;
    Array(napi_env env, size_t count) : env_(env)
    {
        napi_create_array_with_length(env, count, &array_);
    }
    Array(napi_env env, napi_value v)
    {
        napi_valuetype jstype;
        napi_typeof(env, v, &jstype);
        if (jstype != napi_object) {
            return;
        }
        bool isArray = false;
        napi_is_array(env, v, &isArray);
        if (!isArray) {
            return;
        }
        env_ = env;
        array_ = v;
    }

    operator napi_value() const
    {
        return array_;
    }

    napi_env GetEnv() const
    {
        return env_;
    }

    size_t Count() const
    {
        uint32_t size;
        napi_get_array_length(env_, array_, &size);
        return size;
    }
    void Set_value(size_t index, napi_value v) const
    {
        napi_set_element(env_, array_, index, v);
    }

    napi_value Get_value(size_t index) const
    {
        napi_value result;
        napi_get_element(env_, array_, index, &result);
        return result;
    }
    napi_valuetype Type(size_t index) const
    {
        napi_value element;
        napi_get_element(env_, array_, index, &element);
        napi_valuetype jstype;
        napi_status status = napi_invalid_arg;
        status = napi_typeof(env_, element, &jstype);
        return jstype;
    }
    template<typename T>
    Value<T> Get(size_t index) const
    {
        return Value<T> { env_, Get_value(index) };
    }
    template<typename T>
    void Set(size_t index, T t) const
    {
        Set_value(index, Value<T>(env_, t).ToNapiValue());
    }

private:
    napi_env env_ { nullptr };
    napi_value array_ { nullptr };
};

class Function {
public:
    bool IsDefined()
    {
        return (napi_undefined != jstype);
    }
    bool IsNull()
    {
        return (napi_null == jstype);
    }
    Function() = default;
    Function(napi_env env, napi_value v) : env_(env), func_(v)
    {
        if (env_) {
            napi_typeof(env_, v, &jstype);
        }
        if (jstype != napi_function) {
            // value was not a function
            env_ = {};
            func_ = nullptr;
        }
    }
    operator napi_value() const
    {
        return func_;
    }

    NapiApi::Env Env() const
    {
        return env_;
    }
    napi_env GetEnv() const
    {
        return env_;
    }
    napi_value Invoke(NapiApi::Object thisJS, size_t argc = 0, napi_value* argv = nullptr) const
    {
        if (!env_) {
            return nullptr;
        }
        if (!func_) {
            return env_.GetUndefined();
        }
        napi_value res;
        napi_call_function(env_, thisJS.ToNapiValue(), func_, argc, argv, &res);
        return res;
    }

private:
    NapiApi::Env env_ { nullptr };
    napi_value func_ { nullptr };
    napi_valuetype jstype = napi_undefined;
};

class MyInstanceState {
public:
    MyInstanceState(napi_env env, napi_value obj) : env_(env)
    {
        napi_create_reference(env_, obj, 1, &ref_);
    }
    MyInstanceState(NapiApi::Object obj) : MyInstanceState(obj.GetEnv(), obj.ToNapiValue()) {}
    ~MyInstanceState()
    {
        uint32_t res;
        napi_reference_unref(env_, ref_, &res);
    }
    napi_value getRef()
    {
        napi_value tmp;
        napi_get_reference_value(env_, ref_, &tmp);
        return tmp;
    }

    void StoreCtor(BASE_NS::string_view name, napi_value ctor)
    {
        NapiApi::Object exp(env_, getRef());
        exp.Set(name, ctor);
    }
    napi_value FetchCtor(BASE_NS::string_view name)
    {
        NapiApi::Object exp(env_, getRef());
        return exp.Get(name);
    }

private:
    napi_ref ref_;
    napi_env env_;
};
template<typename type>
bool ValidateType(napi_valuetype jstype, bool isArray)
{
    /*
      napi_undefined,
      napi_null,
      napi_symbol,
      napi_function,
      napi_external,
      napi_bigint,
    */

    if constexpr (BASE_NS::is_same_v<type, BASE_NS::string>) {
        if (jstype == napi_string) {
            return true;
        }
    }
    if constexpr (BASE_NS::is_same_v<type, bool>) {
        if (jstype == napi_boolean) {
            return true;
        }
    }
    // yup..
    if constexpr (BASE_NS::is_same_v<type, float>) {
        if (jstype == napi_number) {
            return true;
        }
    }
    if constexpr (BASE_NS::is_same_v<type, double>) {
        if (jstype == napi_number) {
            return true;
        }
    }
    if constexpr (BASE_NS::is_same_v<type, uint32_t>) {
        if (jstype == napi_number) {
            return true;
        }
    }
    if constexpr (BASE_NS::is_same_v<type, int32_t>) {
        if (jstype == napi_number) {
            return true;
        }
    }
    if constexpr (BASE_NS::is_same_v<type, int64_t>) {
        if (jstype == napi_number) {
            return true;
        }
    }
    if constexpr (BASE_NS::is_same_v<type, uint64_t>) {
        if (jstype == napi_number) {
            return true;
        }
    }
    if constexpr (BASE_NS::is_same_v<type, NapiApi::Object>) {
        if (jstype == napi_object) {
            return true;
        }
        // allow undefined and null also
        if (jstype == napi_undefined) {
            return true;
        }
        if (jstype == napi_null) {
            return true;
        }
    }
    if constexpr (BASE_NS::is_same_v<type, NapiApi::Array>) {
        if (jstype == napi_object) {
            return isArray;
        }
    }
    if constexpr (BASE_NS::is_same_v<type, NapiApi::Function>) {
        if (jstype == napi_function) {
            return true;
        }
    }
    return false;
}

template<typename type>
type NapiApi::Value<type>::valueOrDefault(const type defaultValue)
{
    if (!value_) {
        return defaultValue;
    }
    napi_status status = napi_invalid_arg;
    type value {};
    if constexpr (BASE_NS::is_same_v<type, BASE_NS::string>) {
        size_t length;
        status = napi_get_value_string_utf8(env_, value_, nullptr, 0, &length);
        if (status != napi_ok) {
            // return default if failed.
            return defaultValue;
        }
        value.reserve(length + 1);
        value.resize(length);
        status = napi_get_value_string_utf8(env_, value_, value.data(), length + 1, &length);
    }
    if constexpr (BASE_NS::is_same_v<type, bool>) {
        status = napi_get_value_bool(env_, value_, &value);
    }
    if constexpr (BASE_NS::is_same_v<type, float>) {
        double tmp;
        status = napi_get_value_double(env_, value_, &tmp);
        value = tmp;
    }
    if constexpr (BASE_NS::is_same_v<type, double>) {
        status = napi_get_value_double(env_, value_, &value);
    }
    if constexpr (BASE_NS::is_same_v<type, uint32_t>) {
        status = napi_get_value_uint32(env_, value_, &value);
    }
    if constexpr (BASE_NS::is_same_v<type, int32_t>) {
        status = napi_get_value_int32(env_, value_, &value);
    }
    if constexpr (BASE_NS::is_same_v<type, int64_t>) {
        status = napi_get_value_int64(env_, value_, &value);
    }
    if constexpr (BASE_NS::is_same_v<type, uint64_t>) {
        int64_t tmp;
        status = napi_get_value_int64(env_, value_, &tmp);
        value = static_cast<uint64_t>(tmp);
    }
    if constexpr (BASE_NS::is_same_v<type, NapiApi::Object>) {
        status = napi_ok;
        value = NapiApi::Object(env_, value_);
    }
    if constexpr (BASE_NS::is_same_v<type, NapiApi::Function>) {
        status = napi_ok;
        value = NapiApi::Function(env_, value_);
    }
    if constexpr (BASE_NS::is_same_v<type, NapiApi::Array>) {
        status = napi_ok;
        value = NapiApi::Array(env_, value_);
    }
    if (status != napi_ok) {
        // return default if failed.
        return defaultValue;
    }
    return value;
}

inline Object::Object(Function ctor) : Object(ctor, 0, nullptr)
{
}

inline Object::Object(Function ctor, size_t argc, napi_value args[])
{
    env_ = ctor.Env();
    napi_new_instance(env_, ctor, argc, args, &object_);
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

template<typename Object, napi_value (Object::*F)(NapiApi::FunctionContext<>&)>
static inline napi_value Getter(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext fc(env, info);
    if (fc && fc.This()) {
        if (auto scj = fc.This().template Native<Object>()) {
            if (auto ret = (scj->*F)(fc)) {
                return ret;
            }
        }
    }
    return fc.GetUndefined();
};

template<typename Type, typename Object, void (Object::*F)(NapiApi::FunctionContext<Type>&)>
static inline napi_value Setter(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext<Type> fc(env, info);
    if (fc && fc.This()) {
        if (auto scj = fc.This().template Native<Object>()) {
            (scj->*F)(fc);
        }
    }
    return fc.GetUndefined();
};

template<typename FC, typename Object, napi_value (Object::*F)(FC&)>
static inline napi_value MethodI(napi_env env, napi_callback_info info)
{
    FC fc(env, info);
    if (fc && fc.This()) {
        if (auto scj = fc.This().template Native<Object>()) {
            return (scj->*F)(fc);
        }
    }
    return fc.GetUndefined();
};

template<typename FC, typename Object, napi_value (Object::*F)(FC&)>
static inline napi_property_descriptor Method(
    const char* const name, napi_property_attributes flags = napi_default_method)
{
    return napi_property_descriptor { name, nullptr, MethodI<FC, Object, F>, nullptr, nullptr, nullptr, flags,
        nullptr };
}

template<typename Type, typename Object, void (Object::*F2)(NapiApi::FunctionContext<Type>&)>
static inline napi_property_descriptor SetProperty(
    const char* const name, napi_property_attributes flags = napi_default_jsproperty)
{
    static_assert(F2 != nullptr);
    return napi_property_descriptor { name, nullptr, nullptr, nullptr, Setter<Type, Object, F2>, nullptr, flags,
        nullptr };
}

template<typename Type, typename Object, napi_value (Object::*F)(NapiApi::FunctionContext<>&)>
static inline napi_property_descriptor GetProperty(
    const char* const name, napi_property_attributes flags = napi_default_jsproperty)
{
    static_assert(F != nullptr);
    return napi_property_descriptor { name, nullptr, nullptr, Getter<Object, F>, nullptr, nullptr, flags, nullptr };
}

template<typename Type, typename Object, napi_value (Object::*F)(NapiApi::FunctionContext<>&),
    void (Object::*F2)(NapiApi::FunctionContext<Type>&)>
static inline napi_property_descriptor GetSetProperty(
    const char* const name, napi_property_attributes flags = napi_default_jsproperty)
{
    static_assert(F != nullptr);
    static_assert(F2 != nullptr);
    return napi_property_descriptor { name, nullptr, nullptr, Getter<Object, F>, Setter<Type, Object, F2>, nullptr,
        flags, nullptr };
}
class WeakRef {
public:
    WeakRef() = default;
    WeakRef(NapiApi::WeakRef&& ref) noexcept
    {
        env_ = ref.env_;
        ref_ = ref.ref_;
        ref.env_ = nullptr;
        ref.ref_ = nullptr;
    }

    WeakRef(const NapiApi::WeakRef& ref)
    {
        if (!ref.IsEmpty()) {
            napi_status stat;
            env_ = ref.env_;
            stat = napi_create_reference(env_, ref.GetValue(), 0, &ref_);
        }
    }
    NapiApi::WeakRef& operator=(NapiApi::WeakRef&& ref) noexcept
    {
        env_ = ref.env_;
        ref_ = ref.ref_;
        ref.env_ = nullptr;
        ref.ref_ = nullptr;
        return *this;
    }
    NapiApi::WeakRef& operator=(const NapiApi::WeakRef& ref)
    {
        if (&ref != this) {
            if (!ref.IsEmpty()) {
                napi_status stat;
                // unh just create a new one..
                env_ = ref.env_;
                stat = napi_create_reference(env_, ref.GetValue(), 0, &ref_);
            }
        }
        return *this;
    }
    WeakRef(NapiApi::Object obj)
    {
        env_ = obj.GetEnv();
        napi_create_reference(env_, obj.ToNapiValue(), 0, &ref_);
    }
    WeakRef(napi_env env, napi_value obj)
    {
        env_ = env;
        napi_create_reference(env_, obj, 0, &ref_);
    }
    ~WeakRef()
    {
        Reset();
    }
    bool IsEmpty() const
    {
        if (env_ && ref_) {
            // possibly actually check the ref?
            return false;
        }
        return true;
    }
    NapiApi::Object GetObject()
    {
        if (env_ && ref_) {
            napi_value value;
            napi_get_reference_value(env_, ref_, &value);
            return NapiApi::Object(env_, value);
        }
        return {};
    }
    napi_env GetEnv()
    {
        return env_;
    }
    napi_value GetValue() const
    {
        if (env_ && ref_) {
            napi_value value;
            napi_get_reference_value(env_, ref_, &value);
            return value;
        }
        return {};
    }
    void Reset()
    {
        if (env_ && ref_) {
            napi_delete_reference(env_, ref_);
        }
        env_ = nullptr;
        ref_ = nullptr;
    }

private:
    napi_env env_ { nullptr };
    napi_ref ref_ { nullptr };
};

class StrongRef {
public:
    StrongRef() = default;
    uint32_t GetRefCount()
    {
        // this is racy, but for debug use "fine"
        if (!IsEmpty()) {
            uint32_t cnt;
            napi_reference_ref(env_, ref_, &cnt);
            napi_reference_unref(env_, ref_, &cnt);
            return cnt;
        }
        return 0;
    }
    StrongRef(NapiApi::StrongRef&& ref) noexcept
    {
        if (!ref.IsEmpty()) {
            env_ = ref.env_;
            ref_ = ref.ref_;
            ref.env_ = nullptr;
            ref.ref_ = nullptr;
        }
    }
    StrongRef(const NapiApi::StrongRef& ref)
    {
        if (!ref.IsEmpty()) {
            env_ = ref.env_;
            ref_ = ref.ref_;
            Ref();
        }
    }
    NapiApi::StrongRef& operator=(NapiApi::StrongRef&& ref) noexcept
    {
        if (&ref != this) {
            Reset();
            env_ = ref.env_;
            ref_ = ref.ref_;
            ref.env_ = nullptr;
            ref.ref_ = nullptr;
        }
        return *this;
    }
    NapiApi::StrongRef& operator=(const NapiApi::StrongRef& ref)
    {
        if (&ref != this) {
            Reset();
            if (!ref.IsEmpty()) {
                env_ = ref.env_;
                ref_ = ref.ref_;
                Ref();
            }
        }
        return *this;
    }

    StrongRef(napi_env env, napi_ref ref)
    {
        if (env && ref) {
            env_ = env;
            ref_ = ref;
            Ref();
        }
    }

    explicit StrongRef(const NapiApi::Object obj)
    {
        if (obj) {
            env_ = obj.GetEnv();
            napi_create_reference(env_, obj.ToNapiValue(), 1, &ref_);
        }
    }
    explicit StrongRef(const NapiApi::Value<NapiApi::Object> objValue)
        : StrongRef(objValue.GetEnv(), objValue.ToNapiValue())
    {}

    StrongRef(napi_env env, napi_value obj)
    {
        if (env && obj) {
            env_ = env;
            napi_create_reference(env_, obj, 1, &ref_);
        }
    }
    ~StrongRef()
    {
        if (!IsEmpty()) {
            Reset();
        }
    }
    bool IsEmpty() const
    {
        return !(env_ && ref_);
    }
    NapiApi::Object GetObject() const
    {
        return NapiApi::Object(env_, (napi_value)GetValue());
    }
    napi_env GetEnv() const
    {
        return env_;
    }
    napi_value GetValue() const
    {
        if (IsEmpty()) {
            return {};
        }
        napi_value value;
        napi_get_reference_value(env_, ref_, &value);
        return value;
    }
    void Reset()
    {
        if (IsEmpty()) {
            return;
        }
        uint32_t cnt;
        napi_status stat = napi_reference_unref(env_, ref_, &cnt);
        if (stat != napi_ok) {
            return;
        }
        if (cnt == 0) {
            // that was the last reference.
            napi_delete_reference(env_, ref_);
        }
        env_ = nullptr;
        ref_ = nullptr;
    }

private:
    napi_env env_ { nullptr };
    napi_ref ref_ { nullptr };
    void Ref()
    {
        napi_status stat;
        uint32_t cnt;
        stat = napi_reference_ref(env_, ref_, &cnt);
    }
};

template<typename T>
void Value<T>::Init(napi_env env, Type v)
{
    if (env == nullptr) {
        return;
    }
    env_ = env;
    if constexpr (BASE_NS::is_same_v<Type, float>) {
        napi_create_double(env_, v, &value_);
    }
    if constexpr (BASE_NS::is_same_v<Type, double>) {
        napi_create_double(env_, v, &value_);
    }
    if constexpr (BASE_NS::is_same_v<Type, uint32_t>) {
        napi_create_uint32(env_, v, &value_);
    }
    if constexpr (BASE_NS::is_same_v<Type, int32_t>) {
        napi_create_int32(env_, v, &value_);
    }
    if constexpr (BASE_NS::is_same_v<Type, int64_t>) {
        napi_create_int64(env_, v, &value_);
    }
    if constexpr (BASE_NS::is_same_v<Type, uint64_t>) {
        int64_t tmp = static_cast<int64_t>(v);
        napi_create_int64(env_, tmp, &value_);
    }
    if constexpr (BASE_NS::is_same_v<Type, NapiApi::Object>) {
        value_ = v.ToNapiValue();
    }
}

} // namespace NapiApi

NapiApi::Object FetchJsObj(const META_NS::IObject::Ptr& obj, BASE_NS::string_view name = "_JSW");
template<typename t>
NapiApi::Object FetchJsObj(const t& obj, BASE_NS::string_view name = "_JSW")
{
    return FetchJsObj(interface_pointer_cast<META_NS::IObject>(obj));
}
// creates a new reference to jsobj. returns napi_value from reference.
NapiApi::Object StoreJsObj(const META_NS::IObject::Ptr& obj, NapiApi::Object jsobj, BASE_NS::string_view name = "_JSW");
NapiApi::Function GetJSConstructor(napi_env env, const BASE_NS::string_view jsName);

// extracts the uri from "string" or "Resource"
BASE_NS::string FetchResourceOrUri(napi_env e, napi_value arg);
BASE_NS::string FetchResourceOrUri(NapiApi::FunctionContext<>& ctx);

// Instance data
napi_status SetInstanceData(napi_env env, void* data, napi_finalize finalizeCb, void* finalizeHint);
napi_status GetInstanceData(napi_env env, void** data);

// little helper macros

// declare NAPI_API_JS_NAME ...
#define NAPI_API_xs(s) NAPI_API_s(s)
#define NAPI_API_s(s) #s
#define NAPI_API_xcn(s) NAPI_API_cn(s)
#define NAPI_API_cn(s) s##JS
#define NAPI_API_JS_NAME_STRING NAPI_API_xs(NAPI_API_JS_NAME)
#define NAPI_API_CLASS_NAME NAPI_API_xcn(NAPI_API_JS_NAME)

#define DeclareGet(type, name, getter) \
    node_props.push_back(NapiApi::GetProperty<type, NAPI_API_CLASS_NAME, &NAPI_API_CLASS_NAME::getter>(name));
#define DeclareSet(type, name, setter) \
    node_props.push_back(NapiApi::SetProperty<type, NAPI_API_CLASS_NAME, &NAPI_API_CLASS_NAME::setter>(name));
#define DeclareGetSet(type, name, getter, setter)                                                         \
    node_props.push_back(NapiApi::GetSetProperty<type, NAPI_API_CLASS_NAME, &NAPI_API_CLASS_NAME::getter, \
        &NAPI_API_CLASS_NAME::setter>(name));
#define DeclareMethod(name, function, ...)                                                                           \
    node_props.push_back(                                                                                            \
        NapiApi::Method<NapiApi::FunctionContext<__VA_ARGS__>, NAPI_API_CLASS_NAME, &NAPI_API_CLASS_NAME::function>( \
            name));
#define DeclareClass()                                                                                      \
    {                                                                                                       \
        napi_value func;                                                                                    \
        auto status = napi_define_class(env, NAPI_API_JS_NAME_STRING, NAPI_AUTO_LENGTH,                     \
            BaseObject::ctor<NAPI_API_CLASS_NAME>(), nullptr, node_props.size(), node_props.data(), &func); \
        NapiApi::MyInstanceState* mis;                                                                      \
        GetInstanceData(env, (void**)&mis);                                                                 \
        mis->StoreCtor(NAPI_API_JS_NAME_STRING, func);                                                      \
    }

#endif
