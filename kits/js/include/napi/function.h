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

#ifndef NAPI_API_FUNCTION_H
#define NAPI_API_FUNCTION_H

#ifdef __OHOS_PLATFORM__
#include "napi/native_api.h"
#else
#include <node_api.h>
#endif

#include "env.h"

namespace NapiApi {

class Object;

// The args that will be passed to JavaScript functions (including constructors).
struct JsFuncArgs {
    JsFuncArgs() : argc(0), argv(nullptr) {}
    JsFuncArgs(napi_value single) : argc(1), argv(&singleStorage), singleStorage(single) {}
    JsFuncArgs(size_t argc, napi_value* argv) : argc(argc), argv(argv) {}
    template<size_t N>
    JsFuncArgs(napi_value (&argv)[N]) : argc(N), argv(argv)
    {}

    size_t argc;
    napi_value* argv;
    napi_value singleStorage { nullptr };
};

class Function {
public:
    Function() = default;
    Function(napi_env env, napi_value v);

    operator napi_value() const;
    NapiApi::Env Env() const;
    napi_env GetEnv() const;

    napi_value Invoke(const NapiApi::Object& thisJS, const JsFuncArgs& args = {}) const;

    bool IsDefined();
    bool IsNull();

private:
    NapiApi::Env env_ { nullptr };
    napi_value func_ { nullptr };
    napi_valuetype jstype { napi_undefined };
};

} // namespace NapiApi

#endif
