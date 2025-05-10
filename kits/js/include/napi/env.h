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

#ifndef NAPI_API_ENV_H
#define NAPI_API_ENV_H

#ifdef __OHOS_PLATFORM__
#include "napi/native_api.h"
#else
#include <node_api.h>
#endif

#include <base/containers/string.h>

namespace NapiApi {

class Env {
public:
    Env() = default;
    explicit Env(napi_env env);

    operator bool() const;
    operator napi_env() const;

    napi_value GetNull() const;
    napi_value GetUndefined() const;
    napi_value GetBoolean(bool value) const;
    napi_value GetString(const BASE_NS::string_view value) const;

    napi_value GetNumber(uint32_t value) const;
    napi_value GetNumber(int32_t value) const;
    napi_value GetNumber(uint64_t value) const;
    napi_value GetNumber(int64_t value) const;
    napi_value GetNumber(float value) const;
    napi_value GetNumber(double value) const;

private:
    napi_env env_ { nullptr };
};

} // namespace NapiApi

#endif
