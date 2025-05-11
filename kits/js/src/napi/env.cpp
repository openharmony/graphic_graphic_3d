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

#include "env.h"

namespace NapiApi {

Env::Env(napi_env env) : env_(env) {}

Env::operator bool() const
{
    return (env_);
}

Env::operator napi_env() const
{
    return env_;
}

napi_value Env::GetNull() const
{
    if (!env_) {
        return {};
    }
    napi_value null;
    napi_get_null(env_, &null);
    return null;
}

napi_value Env::GetUndefined() const
{
    if (!env_) {
        return {};
    }
    napi_value undefined;
    napi_get_undefined(env_, &undefined);
    return undefined;
}

napi_value Env::GetBoolean(bool value) const
{
    if (!env_) {
        return {};
    }
    napi_value val;
    napi_get_boolean(env_, value, &val);
    return val;
}

napi_value Env::GetString(const BASE_NS::string_view value) const
{
    napi_value val;
    napi_create_string_utf8(env_, value.data(), value.length(), &val);
    return val;
}

napi_value Env::GetNumber(uint32_t value) const
{
    if (!env_) {
        return {};
    }
    napi_value val;
    napi_create_uint32(env_, value, &val);
    return val;
}

napi_value Env::GetNumber(int32_t value) const
{
    if (!env_) {
        return {};
    }
    napi_value val;
    napi_create_int32(env_, value, &val);
    return val;
}

napi_value Env::GetNumber(uint64_t value) const
{
    if (!env_) {
        return {};
    }
    napi_value val;
    napi_create_bigint_uint64(env_, value, &val);
    return val;
}

napi_value Env::GetNumber(int64_t value) const
{
    if (!env_) {
        return {};
    }
    napi_value val;
    napi_create_bigint_int64(env_, value, &val);
    return val;
}

napi_value Env::GetNumber(float value) const
{
    if (!env_) {
        return {};
    }
    napi_value val;
    napi_create_double(env_, value, &val);
    return val;
}

napi_value Env::GetNumber(double value) const
{
    if (!env_) {
        return {};
    }
    napi_value val;
    napi_create_double(env_, value, &val);
    return val;
}

} // namespace NapiApi
