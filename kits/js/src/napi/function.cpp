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

#include "function.h"

#include "env.h"
#include "object.h"

namespace NapiApi {

Function::Function(napi_env env, napi_value v) : env_(env), func_(v)
{
    if (env_) {
        napi_typeof(env_, v, &jstype);
    }
    if (jstype != napi_function) {
        // value was not a function
        env_ = {};
        func_ = nullptr;
    }
}

Function::operator napi_value() const
{
    return func_;
}

NapiApi::Env Function::Env() const
{
    return env_;
}

napi_env Function::GetEnv() const
{
    return env_;
}

napi_value Function::Invoke(const NapiApi::Object& thisJS, const JsFuncArgs& args) const
{
    if (!env_) {
        return nullptr;
    }
    auto res = env_.GetUndefined();
    if (func_) {
        napi_call_function(env_, thisJS.ToNapiValue(), func_, args.argc, args.argv, &res);
    }
    return res;
}

bool Function::IsDefined()
{
    return (napi_undefined != jstype);
}

bool Function::IsNull()
{
    return (napi_null == jstype);
}

} // namespace NapiApi
