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

#include "DisposeContainer.h"

void DisposeContainer::DisposeHook(uintptr_t token, NapiApi::Object obj)
{
    disposables_[token] = { obj };
}

void DisposeContainer::ReleaseDispose(uintptr_t token)
{
    auto it = disposables_.find(token);
    if (it != disposables_.end()) {
        it->second.Reset();
        disposables_.erase(it->first);
    }
}

void DisposeContainer::StrongDisposeHook(uintptr_t token, NapiApi::Object obj)
{
    strongDisposables_[token] = NapiApi::StrongRef(obj);
}

void DisposeContainer::ReleaseStrongDispose(uintptr_t token)
{
    auto it = strongDisposables_.find(token);
    if (it != strongDisposables_.end()) {
        it->second.Reset();
        strongDisposables_.erase(it->first);
    }
}

void DisposeContainer::DisposeAll(napi_env e)
{
    NapiApi::Object scen(e);
    napi_value tmp;
    napi_create_external(
        e, static_cast<void*>(this),
        [](napi_env env, void* data, void* finalize_hint) {
            // do nothing.
        },
        nullptr, &tmp);
    scen.Set("DisposeContainer", tmp);
    napi_value scene = scen.ToNapiValue();

    // dispose all cameras/env/etcs.
    while (!strongDisposables_.empty()) {
        auto it = strongDisposables_.begin();
        auto token = it->first;
        auto env = it->second.GetObject();
        if (env) {
            auto size = strongDisposables_.size();
            NapiApi::Function func = env.Get<NapiApi::Function>("destroy");
            if (func) {
                func.Invoke(env, 1, &scene);
            }

            if (size == strongDisposables_.size()) {
                LOG_E("Dispose function didn't dispose.");
                strongDisposables_.erase(strongDisposables_.begin());
            }
        } else {
            strongDisposables_.erase(strongDisposables_.begin());
        }
    }

    // dispose
    while (!disposables_.empty()) {
        auto env = disposables_.begin()->second.GetObject();
        if (env) {
            auto size = disposables_.size();
            NapiApi::Function func = env.Get<NapiApi::Function>("destroy");
            if (func) {
                func.Invoke(env, 1, &scene);
            }
            if (size == disposables_.size()) {
                LOG_E("Weak ref dispose function didn't dispose.");
                disposables_.erase(disposables_.begin());
            }
        } else {
            disposables_.erase(disposables_.begin());
        }
    }
}

