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

#ifndef NAPI_API_UTILS_H
#define NAPI_API_UTILS_H

#ifdef __OHOS_PLATFORM__
#include "napi/native_api.h"
#else
#include <node_api.h>
#endif

#include <core/log.h>

#define LOG_F(...) CORE_LOG_F(__VA_ARGS__)
#define LOG_E(...) CORE_LOG_E(__VA_ARGS__)

// Disable levels verbose and warning.
#define LOG_V(...)
#define LOG_W(...)

namespace NapiApi {

class MyInstanceState {
public:
    MyInstanceState(napi_env env, napi_value obj);
    ~MyInstanceState();

    static void SetInstance(napi_env env, void* data, napi_finalize finalize_cb, void* finalize_hint);
    static void GetInstance(napi_env env, void** data);
    napi_value GetRef();

    void StoreCtor(BASE_NS::string_view name, napi_value ctor);
    napi_value FetchCtor(BASE_NS::string_view name);

private:
    napi_ref ref_;
    napi_env env_;
};

template<typename type>
bool ValidateType(napi_valuetype jstype, bool isArray);

} // namespace NapiApi

#endif
