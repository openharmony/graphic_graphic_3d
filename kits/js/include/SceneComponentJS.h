/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef SCENECOMPONENTJS_H
#define SCENECOMPONENTJS_H

#include "BaseObjectJS.h"
#include "PropertyProxy.h"

#include <base/containers/unordered_map.h>

class SceneComponentJS final : public BaseObject {
public:
    static constexpr uint32_t ID = 200;
    static void Init(napi_env env, napi_value exports);

    SceneComponentJS(napi_env, napi_callback_info);
    ~SceneComponentJS() override;
    void* GetInstanceImpl(uint32_t) override;

private:
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);
    void DisposeNative(void *) override;
    void Finalize(napi_env env) override;

    void AddProperties(NapiApi::Object meJs, const META_NS::IObject::Ptr& obj);
    NapiApi::StrongRef jsProps_;
    NapiApi::WeakObjectRef scene_;
    BASE_NS::unordered_map<BASE_NS::string, BASE_NS::shared_ptr<PropertyProxy>> proxies_;
};
#endif
