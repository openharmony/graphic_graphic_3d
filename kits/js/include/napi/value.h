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

#ifndef NAPI_API_VALUE_H
#define NAPI_API_VALUE_H

#ifdef __OHOS_PLATFORM__
#include "napi/native_api.h"
#else
#include <node_api.h>
#endif

#include <base/containers/string.h>
#include <base/containers/type_traits.h>

#include "utils.h"

namespace NapiApi {

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

        if (NapiApi::ValidateType<type>(jstype, isArray)) {
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

    type valueOrDefault(const type defaultValue = {});
};

} // namespace NapiApi

#endif
