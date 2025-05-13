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

#ifndef NAPI_API_STRONG_REF_H
#define NAPI_API_STRONG_REF_H

#ifdef __OHOS_PLATFORM__
#include "napi/native_api.h"
#else
#include <node_api.h>
#endif

#include "object.h"

namespace NapiApi {

class StrongRef {
public:
    StrongRef() = default;
    StrongRef(napi_env env, napi_value obj);
    StrongRef(napi_env env, napi_ref ref);
    explicit StrongRef(const NapiApi::Object& obj);
    explicit StrongRef(const NapiApi::Value<NapiApi::Object> objValue);
    StrongRef(const NapiApi::StrongRef& ref);
    StrongRef(NapiApi::StrongRef&& ref) noexcept;

    NapiApi::StrongRef& operator=(NapiApi::StrongRef&& ref) noexcept;
    NapiApi::StrongRef& operator=(const NapiApi::StrongRef& ref);

    ~StrongRef();

    void Reset();
    bool IsEmpty() const;
    napi_env GetEnv() const;
    napi_value GetValue() const;
    NapiApi::Object GetObject() const;
    uint32_t GetRefCount() const;

private:
    void Ref();
    napi_env env_ { nullptr };
    napi_ref ref_ { nullptr };
};

} // namespace NapiApi

#endif
