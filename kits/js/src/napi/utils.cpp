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

#if __OHOS_PLATFORM__
// need to use a workaround
// as napi_set_instance_data/napi_get_instance_data
// seems to be implemented incorrectly in a way
// that shares single instance data and env for ALL modules.
#define USE_WORKAROUND 1
#else
#define USE_WORKAROUND 0
#endif

#include <optional>

#if USE_WORKAROUND
#include <base/containers/unordered_map.h>
#endif

#include "array.h"
#include "function.h"
#include "object.h"
#include "utils.h"

namespace NapiApi {

MyInstanceState::MyInstanceState(napi_env env, napi_value obj) : env_(env)
{
    napi_create_reference(env_, obj, 1, &ref_);
}

MyInstanceState::~MyInstanceState()
{
    uint32_t res;
    napi_reference_unref(env_, ref_, &res);
}

napi_value MyInstanceState::GetRef()
{
    napi_value tmp;
    napi_get_reference_value(env_, ref_, &tmp);
    return tmp;
}
#if USE_WORKAROUND
namespace {
// workaround os bug
class workaround {
public:
    workaround() = default;
    workaround(const workaround&) = delete;
    workaround(workaround&& other) noexcept
    {
        cb = other.cb;
        hint = other.hint;
        data = other.data;
        env = other.env;
        other.cb = nullptr;
        other.hint = nullptr;
        other.data = nullptr;
        other.env = nullptr;
    }
    workaround& operator=(const workaround& other) = delete;
    workaround& operator=(workaround&& other) noexcept
    {
        cb = other.cb;
        hint = other.hint;
        data = other.data;
        env = other.env;
        other.cb = nullptr;
        other.hint = nullptr;
        other.data = nullptr;
        other.env = nullptr;
        return *this;
    }
    ~workaround()
    {
        if (cb) {
            cb(env, data, hint);
        }
    }
    workaround(napi_env e, void* d, napi_finalize c, void* h) : env(e), data(d), cb(c), hint(h) {};
    napi_finalize cb { nullptr };
    void* hint { nullptr };
    void* data { nullptr };
    napi_env env { nullptr };
};
BASE_NS::unordered_map<uintptr_t, workaround> workaround_data;
}; // namespace
#endif
void MyInstanceState::GetInstance(napi_env env, void** data)
{
    if (!data || !env) {
        return;
    }
#if USE_WORKAROUND
    // workaround os bug
    if (!workaround_data.contains((uintptr_t)env)) {
        *data = nullptr;
        return;
    }
    *data = workaround_data[(uintptr_t)env].data;
#else
    napi_get_instance_data(env, data);
#endif
}
void MyInstanceState::SetInstance(napi_env env, void* data, napi_finalize finalize_cb, void* finalize_hint)
{
    if (!env) {
        return;
    }
#if USE_WORKAROUND
    // workaround os bug
    if (workaround_data.contains((uintptr_t)env)) {
        auto& wd = workaround_data[(uintptr_t)env];
        wd.env = env;
        wd.data = data;
        wd.cb = finalize_cb;
        wd.hint = finalize_hint;
        return;
    }
    workaround_data.insert({ (uintptr_t)env, { env, data, finalize_cb, finalize_hint } });
#else
    napi_set_instance_data(env, data, finalize_cb, finalize_hint);
#endif
}

void MyInstanceState::StoreCtor(BASE_NS::string_view name, napi_value ctor)
{
    NapiApi::Object exp(env_, GetRef());
    exp.Set(name, ctor);
}

napi_value MyInstanceState::FetchCtor(BASE_NS::string_view name)
{
    NapiApi::Object exp(env_, GetRef());
    return exp.Get(name);
}

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
    if constexpr (BASE_NS::is_same_v<type, std::optional<bool>>) {
        if (jstype == napi_boolean) {
            return true;
        }

        if (jstype == napi_undefined) {
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
        // allow undefined and null also
        if (jstype == napi_undefined) {
            return true;
        }
        if (jstype == napi_null) {
            return true;
        }
    }
    return false;
}

template bool ValidateType<BASE_NS::string>(napi_valuetype, bool);
template bool ValidateType<bool>(napi_valuetype, bool);
template bool ValidateType<std::optional<bool>>(napi_valuetype, bool);
template bool ValidateType<float>(napi_valuetype, bool);
template bool ValidateType<double>(napi_valuetype, bool);
template bool ValidateType<uint32_t>(napi_valuetype, bool);
template bool ValidateType<int32_t>(napi_valuetype, bool);
template bool ValidateType<int64_t>(napi_valuetype, bool);
template bool ValidateType<uint64_t>(napi_valuetype, bool);
template bool ValidateType<NapiApi::Object>(napi_valuetype, bool);
template bool ValidateType<NapiApi::Function>(napi_valuetype, bool);
template bool ValidateType<NapiApi::Array>(napi_valuetype, bool);

} // namespace NapiApi
