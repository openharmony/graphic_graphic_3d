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
#else
#include <node_api.h>
#endif

#include "function_context.h"

namespace NapiApi {

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
