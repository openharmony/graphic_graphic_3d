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
        size_t providedArgCount = 0;
        status = napi_get_cb_info(env, info, &providedArgCount, nullptr, &jsThis_, &data_);
        if (status != napi_ok) {
            return;
        }
        // Get as many args as were provided, despite how many were requested.
        args_.resize(providedArgCount);

        status = napi_get_cb_info(env, info, &providedArgCount, args_.data(), nullptr, nullptr);
        if (status != napi_ok) {
            return;
        }
        env_ = NapiApi::Env(env);
        info_ = info;

        if constexpr (sizeof...(RequestedArgs) > 0) {
            // validate arg count first
            const bool exactOk = argMode == ArgCount::EXACT && requestedArgCount_ == args_.size();
            const bool partialOk = argMode == ArgCount::PARTIAL && requestedArgCount_ <= args_.size();
            if ((!exactOk && !partialOk) || !validate<RequestedArgs...>(0)) {
                jsThis_ = nullptr;
                data_ = nullptr;
                info_ = nullptr;
                args_.clear();
                return;
            }
        }
    }

    template<typename... Other>
    FunctionContext(const FunctionContext<Other...>& other)
        : jsThis_(other.RawThis()), data_(other.GetData()), args_(other.GetArgs()), env_(other.GetEnv()),
          info_(other.GetInfo())
    {
        if constexpr (sizeof...(RequestedArgs) > 0) {
            const bool exactOk = requestedArgCount_ == args_.size();
            if (!exactOk || !validate<RequestedArgs...>(0)) {
                jsThis_ = nullptr;
                data_ = nullptr;
                info_ = nullptr;
                args_.clear();
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

    napi_value RawThis() const
    {
        return jsThis_;
    }

    BASE_NS::vector<napi_value> GetArgs() const
    {
        return args_;
    }

    // Get any provided arg (even past the requested ones) if it exists, else return nullptr.
    napi_value Value(size_t index)
    {
        if (index < args_.size()) {
            return args_[index];
        }
        return nullptr;
    }

    // Reassign any provided arg (even past the requested ones) if it exists. Return true for success.
    bool SetValue(size_t index, napi_value newValue)
    {
        if (index < args_.size()) {
            args_[index] = newValue;
            return true;
        }
        return false;
    }

    template<size_t I, typename T, typename... TypesI>
    struct GetTypeImpl {
        using type = typename GetTypeImpl<I - 1, TypesI...>::type;
    };

    template<typename T, typename... TypesI>
    struct GetTypeImpl<0, T, TypesI...> {
        using type = T;
    };

    // Get a requested arg. For a truthy FunctionContext, the value will be valid; for a falsy, invalid.
    template<size_t index>
    auto Arg()
    {
        static_assert(index < sizeof...(RequestedArgs), "Index out of range!");
        using Type = typename GetTypeImpl<index, RequestedArgs...>::type;
        return NapiApi::Value<Type> { env_, Value(index) };
    }

    size_t ArgCount() const
    {
        return args_.size();
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

    auto SwitchJSTarget(napi_value target)
    {
        FunctionContext<RequestedArgs...> ctx(*this);
        ctx.jsThis_ = target;
        return ctx;
    }

private:
    template<typename First, typename... Rest>
    inline bool validate(size_t index)
    {
        napi_valuetype jstype;
        napi_status status = napi_invalid_arg;
        status = napi_typeof(env_, args_[index], &jstype);
        if (status != napi_ok) {
            return false;
        }
        bool isArray = false;
        napi_is_array(env_, args_[index], &isArray);

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
    // How many args we want to validate and access with Arg.
    const size_t requestedArgCount_ { sizeof...(RequestedArgs) };
    BASE_NS::vector<napi_value> args_;
    NapiApi::Env env_ { nullptr };
    napi_callback_info info_ { nullptr };
};

} // namespace NapiApi

#endif
