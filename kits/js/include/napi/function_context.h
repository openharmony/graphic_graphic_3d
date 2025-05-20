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

#ifndef NAPI_API_FUNCTION_CONTEXT_H
#define NAPI_API_FUNCTION_CONTEXT_H

#ifdef __OHOS_PLATFORM__
#include "napi/native_api.h"
#else
#include <node_api.h>
#endif

#include <base/containers/string.h>

#include "env.h"
#include "object.h"
#include "utils.h"
#include "value.h"

namespace NapiApi {

// Should the requested arg count match the provided count or do we allow partial requests.
enum class ArgCount { EXACT, PARTIAL };

template<typename... RequestedArgs>
class FunctionContext {
public:
    FunctionContext(const napi_env env, const napi_callback_info info, ArgCount argMode = ArgCount::EXACT)
    {
        if ((!env) || (!info)) {
            return;
        }
        napi_status status { napi_ok };
        status = napi_get_cb_info(env, info, &providedArgCount_, nullptr, &jsThis_, &data_);
        if (status != napi_ok) {
            return;
        }
        env_ = NapiApi::Env(env);
        info_ = info;
        if constexpr (sizeof...(RequestedArgs) > 0) {
            // validate arg count first
            const bool exactOk = argMode == ArgCount::EXACT && requestedArgCount_ == providedArgCount_;
            const bool partialOk = argMode == ArgCount::PARTIAL && requestedArgCount_ <= providedArgCount_;
            if (!exactOk && !partialOk) {
                jsThis_ = nullptr;
                data_ = nullptr;
                info_ = nullptr;
                return;
            }

            // get the arguments
            auto getThisManyArgs = requestedArgCount_; // The napi call needs a non-const.
            status = napi_get_cb_info(env, info, &getThisManyArgs, args, nullptr, nullptr);
            if (!validate<RequestedArgs...>(0)) {
                // non matching types in context!
                jsThis_ = nullptr;
                data_ = nullptr;
                info_ = nullptr;
                return;
            }
        }
    }

    template<typename... Other>
    FunctionContext(const FunctionContext<Other...>& other) : FunctionContext(other.GetEnv(), other.GetInfo())
    {}

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
        if (index < requestedArgCount_) {
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
        if constexpr (sizeof...(RequestedArgs) > 0) {
            if constexpr (index < sizeof...(RequestedArgs)) {
                return NapiApi::Value<typename GetTypeImpl<index, RequestedArgs...>::type> { env_, args[index] };
            }
            if constexpr (index >= sizeof...(RequestedArgs)) {
                static_assert(index < sizeof...(RequestedArgs), "Index out of range !");
                return NapiApi::Value<void*>((napi_env) nullptr, (void*)nullptr);
            }
        }
        if constexpr (sizeof...(RequestedArgs) == 0) {
            return;
        }
    }

    size_t ArgCount() const
    {
        return providedArgCount_;
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
    template<typename First, typename... Rest>
    inline bool validate(size_t index)
    {
        napi_valuetype jstype;
        napi_status status = napi_invalid_arg;
        status = napi_typeof(env_, args[index], &jstype);
        bool isArray = false;
        napi_is_array(env_, args[index], &isArray);

        bool ret = NapiApi::ValidateType<First>(jstype, isArray);
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

    napi_value jsThis_ { nullptr };
    void* data_ { nullptr };
    // How many args we want to access from the JS function call.
    const size_t requestedArgCount_ { sizeof...(RequestedArgs) };
    napi_value args[sizeof...(RequestedArgs) + 1] {};
    NapiApi::Env env_ { nullptr };
    napi_callback_info info_ { nullptr };
    // How many args actually were in the JS function call.
    size_t providedArgCount_ { 0 };
};

} // namespace NapiApi

#endif
