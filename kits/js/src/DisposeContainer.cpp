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
#include "core/log.h"

namespace {
void DestroyNapiObject(NapiApi::Object obj, napi_value& scene)
{
    if (obj) {
        NapiApi::Function func = obj.Get<NapiApi::Function>("destroy");
        if (func) {
            func.Invoke(obj, { scene });
        }
    }
}

template<typename T>
void DisposeFromList(BASE_NS::unordered_map<uintptr_t, T>& disposables, uintptr_t token, napi_value* scene)
{
    auto it = disposables.find(token);
    if (it != disposables.end()) {
        auto object = it->second.GetObject();
        it->second.Reset();
        if (scene) {
            DestroyNapiObject(object, *scene);
        }
        disposables.erase(it->first);
    }
}
} // end anon ns

void DisposeContainer::DisposeHook(uintptr_t token, NapiApi::Object obj)
{
    disposables_[token] = { obj };
}

void DisposeContainer::ReleaseDispose(uintptr_t token)
{
    ::DisposeFromList<NapiApi::WeakRef>(disposables_, token, nullptr);
}

void DisposeContainer::StrongDisposeHook(uintptr_t token, NapiApi::Object obj)
{
    strongDisposables_[token] = NapiApi::StrongRef(obj);
}

void DisposeContainer::ReleaseStrongDispose(uintptr_t token)
{
    ::DisposeFromList<NapiApi::StrongRef>(strongDisposables_, token, nullptr);
}

void DisposeContainer::Dispose(napi_env e, BASE_NS::array_view<const uintptr_t> strongs,
    BASE_NS::array_view<const uintptr_t> weaks, SceneJS* sc)
{
    LOG_V("Mass Dispose %zu %zu", strongs.size(), weaks.size());
    NapiApi::Object scen(e);
    napi_value tmp;
    napi_create_external(
        e, static_cast<void*>(sc),
        [](napi_env env, void* data, void* finalize_hint) {
            // do nothing.
        },
        nullptr, &tmp);
    scen.Set("SceneJS", tmp);
    napi_value scene = scen.ToNapiValue();
    bool disposedSelected = false;
    for (auto s : strongs) {
        disposedSelected = true;
        ::DisposeFromList<NapiApi::StrongRef>(strongDisposables_, s, &scene);
    }
    for (auto w : weaks) {
        disposedSelected = true;
        ::DisposeFromList<NapiApi::WeakRef>(disposables_, w, &scene);
    }
    // if no targets were defined, clear all
    while (!disposedSelected && !strongDisposables_.empty()) {
        auto size = strongDisposables_.size();
        auto obj = strongDisposables_.begin()->second.GetObject();
        ::DestroyNapiObject(obj, scene);
        if (size == strongDisposables_.size()) {
            if (obj) {
                CORE_LOG_W("S Dispose function didn't dispose.");
            }
            strongDisposables_.erase(strongDisposables_.begin());
        }
    }
    while (!disposedSelected && !disposables_.empty()) {
        auto obj = disposables_.begin()->second.GetObject();
        auto size = disposables_.size();
        ::DestroyNapiObject(obj, scene);
        if (size == disposables_.size()) {
            if (obj) {
                CORE_LOG_W("W Dispose function didn't dispose.");
            }
            disposables_.erase(disposables_.begin());
        }
    }
}
