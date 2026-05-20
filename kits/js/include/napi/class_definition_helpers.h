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

#ifndef NAPI_API_CLASS_DEFINITION_HELPERS_H
#define NAPI_API_CLASS_DEFINITION_HELPERS_H

#ifdef __OHOS_PLATFORM__
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#else
#include <node_api.h>
#endif

#include "function_context.h"
#include "utils.h"

#include <string>

namespace NapiApi {

template<typename Type>
constexpr const char *GetTypeName()
{
    std::string_view full_name = __PRETTY_FUNCTION__;
    // GCC format: ... [with Type = MyNamespace::MyClass]
    // Clang format: ... [Type = MyNamespace::MyClass]
    size_t start = full_name.find("Type = ");
    if (start == std::string_view::npos) { return "Unknown"; }

    start += 7; // Skip the length of "Type = ", 7

    // Find the ending ']' or ';'
    size_t end = full_name.find_first_of("];", start);
    if (end == std::string_view::npos) { end = full_name.length(); }

    std::string_view className = full_name.substr(start, end - start);
    // If only the class name is desired, remove the namespace (e.g., MyNS::A -> A)
    if (const size_t last_colon = className.find_last_of(':'); last_colon != std::string_view::npos) {
        className = className.substr(last_colon + 1);
    }

    // __PRETTY_FUNCTION__ returns a string literal with static storage duration.
    // className.data() points to a substring of this literal, so the pointer is valid for the program's lifetime.
    return className.data();
}

inline napi_status WrapTaggedImpl(napi_env env, napi_value js_object, void* native_object,
    napi_finalize finalize_cb, void* finalize_hint, const napi_type_tag& type_tag, napi_ref* result)
{
#ifdef __OHOS_PLATFORM__
    return napi_wrap_s(env, js_object, native_object, finalize_cb, finalize_hint, &type_tag, result);
#else
    auto status = napi_wrap(env, js_object, native_object, finalize_cb, finalize_hint, result);
    if (status == napi_ok) {
        status = napi_type_tag_object(env, js_object, &type_tag);
        if (status != napi_ok && result != nullptr) {
            napi_delete_reference(env, *result);
            *result = nullptr;
        }
    }
    return status;
#endif
}

inline napi_status UnwrapTaggedImpl(napi_env env, napi_value js_object, const napi_type_tag& type_tag, void** result)
{
#ifdef __OHOS_PLATFORM__
    return napi_unwrap_s(env, js_object, &type_tag, result);
#else
    bool isTagged = false;
    auto status = napi_check_object_type_tag(env, js_object, &type_tag, &isTagged);
    if (status != napi_ok || !isTagged) {
        *result = nullptr;
        return status != napi_ok ? status : napi_invalid_arg;
    }
    return napi_unwrap(env, js_object, result);
#endif
}

template<typename Class>
inline bool WrapTagged(napi_env env, napi_value js_object, void* native_object,
    napi_finalize finalize_cb, void* finalize_hint, const napi_type_tag& type_tag, napi_ref* result)
{
    auto status = WrapTaggedImpl(env, js_object, native_object, finalize_cb, finalize_hint, type_tag, result);
    if (status != napi_ok) {
        const napi_extended_error_info* errorInfo = nullptr;
        napi_get_last_error_info(env, &errorInfo);
        LOG_W("WrapTagged<%s> failed: %s", GetTypeName<Class>(),
            errorInfo && errorInfo->error_message ? errorInfo->error_message : "unknown");
    }
    return status == napi_ok;
}

template<typename Class>
inline Class* UnwrapTagged(napi_env env, napi_value js_object, const napi_type_tag& type_tag)
{
    void* result = nullptr;
    auto status = UnwrapTaggedImpl(env, js_object, type_tag, &result);
    if (status != napi_ok) {
        const napi_extended_error_info* errorInfo = nullptr;
        napi_get_last_error_info(env, &errorInfo);
        LOG_W("UnwrapTagged<%s> failed: %s", GetTypeName<Class>(),
            errorInfo && errorInfo->error_message ? errorInfo->error_message : "unknown");
        return nullptr;
    }
    return static_cast<Class*>(result);
}

template<typename Object, napi_value (Object::*F)(NapiApi::FunctionContext<>&)>
static inline napi_value Getter(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext fc(env, info);
    if (fc && fc.This()) {
        if (auto scj = fc.This().template GetJsWrapper<Object>()) {
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
        if (auto scj = fc.This().template GetJsWrapper<Object>()) {
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
        if (auto scj = fc.This().template GetJsWrapper<Object>()) {
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

} // namespace NapiApi

#define NAPI_API_xs(s) NAPI_API_s(s)
#define NAPI_API_s(s) #s
#define NAPI_API_xcn(s) NAPI_API_cn(s)
#define NAPI_API_cn(s) s##JS
#define NAPI_API_JS_NAME_STRING NAPI_API_xs(NAPI_API_JS_NAME)
#define NAPI_API_CLASS_NAME NAPI_API_xcn(NAPI_API_JS_NAME)

// First #define NAPI_API_JS_NAME YourJsClassNameHere
// and then use these macros.
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
#define DeclareClass() {                                                                                    \
        napi_value func;                                                                                    \
        auto status = napi_define_class(env, NAPI_API_JS_NAME_STRING, NAPI_AUTO_LENGTH,                     \
            BaseObject::ctor<NAPI_API_CLASS_NAME>(), nullptr, node_props.size(), node_props.data(), &func); \
        NapiApi::MyInstanceState* mis;                                                                      \
        NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);                                           \
        if (mis) {                                                                                          \
            mis->StoreCtor(NAPI_API_JS_NAME_STRING, func);                                                  \
        }                                                                                                   \
    }

#endif
