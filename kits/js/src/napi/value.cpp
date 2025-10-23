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

#include "value.h"

#include <optional>

#include "array.h"
#include "function.h"
#include "object.h"

namespace NapiApi {

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
    if constexpr (BASE_NS::is_same_v<Type, bool>) {
        napi_get_boolean(env_, v, &value_);
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
    if constexpr (BASE_NS::is_same_v<type, std::optional<bool>>) {
        bool innerValue;
        status = napi_get_value_bool(env_, value_, &innerValue);
        if (status == napi_ok) {
            value = innerValue;
        } else {
            value.reset();
        }
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

template class Value<BASE_NS::string>;
template class Value<bool>;
template class Value<std::optional<bool>>;
template class Value<float>;
template class Value<double>;
template class Value<uint32_t>;
template class Value<int32_t>;
template class Value<int64_t>;
template class Value<uint64_t>;
template class Value<NapiApi::Object>;
template class Value<NapiApi::Function>;
template class Value<NapiApi::Array>;

} // namespace NapiApi
