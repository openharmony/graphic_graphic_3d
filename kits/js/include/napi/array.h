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

#ifndef NAPI_API_ARRAY_H
#define NAPI_API_ARRAY_H

#ifdef __OHOS_PLATFORM__
#include "napi/native_api.h"
#else
#include <node_api.h>
#endif

#include <base/containers/string.h>

#include "value.h"

namespace NapiApi {

class Array {
public:
    Array() = default;
    Array(napi_env env, size_t count);
    Array(napi_env env, napi_value v);

    operator napi_value() const;
    napi_env GetEnv() const;

    size_t Count() const;
    napi_valuetype Type(size_t index) const;

    napi_value Get_value(size_t index) const;
    void Set_value(size_t index, napi_value v) const;

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

} // namespace NapiApi

#endif
