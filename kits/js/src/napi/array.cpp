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

#include "array.h"

namespace NapiApi {

Array::Array(napi_env env, size_t count) : env_(env)
{
    napi_create_array_with_length(env, count, &array_);
}

Array::Array(napi_env env, napi_value v)
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

Array::operator napi_value() const
{
    return array_;
}

napi_env Array::GetEnv() const
{
    return env_;
}

size_t Array::Count() const
{
    uint32_t size = 0;
    napi_get_array_length(env_, array_, &size);
    return size;
}

napi_valuetype Array::Type(size_t index) const
{
    napi_value element;
    napi_get_element(env_, array_, index, &element);
    napi_valuetype jstype;
    napi_status status = napi_invalid_arg;
    status = napi_typeof(env_, element, &jstype);
    return jstype;
}

napi_value Array::Get_value(size_t index) const
{
    napi_value result;
    napi_get_element(env_, array_, index, &result);
    return result;
}

void Array::Set_value(size_t index, napi_value v) const
{
    napi_set_element(env_, array_, index, v);
}

} // namespace NapiApi
