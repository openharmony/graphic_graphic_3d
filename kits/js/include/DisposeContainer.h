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

#ifndef DISPOSE_CONTAINER_H
#define DISPOSE_CONTAINER_H

#include <base/containers/unordered_map.h>

#include "napi_api.h"

class DisposeContainer {
public:
    void DisposeHook(uintptr_t token, NapiApi::Object obj);
    void ReleaseDispose(uintptr_t token);

    void StrongDisposeHook(uintptr_t token, NapiApi::Object obj);
    void ReleaseStrongDispose(uintptr_t token);

    void DisposeAll(napi_env env);

private:
    BASE_NS::unordered_map<uintptr_t, NapiApi::StrongRef> strongDisposables_;
    BASE_NS::unordered_map<uintptr_t, NapiApi::WeakRef> disposables_;
};
#endif // DISPOSE_CONTAINER_H
