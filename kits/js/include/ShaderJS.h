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

#ifndef SHADER_JS_H
#define SHADER_JS_H
#include <scene/interface/intf_material.h>

#include <base/containers/unordered_map.h>

#include "BaseObjectJS.h"
#include "PropertyProxy.h"
#include "SceneResourceImpl.h"

class ShaderJS : BaseObject, SceneResourceImpl {
public:
    static constexpr uint32_t ID = 40;
    static void Init(napi_env env, napi_value exports);
    ShaderJS(napi_env, napi_callback_info);
    ~ShaderJS() override;
    void* GetInstanceImpl(uint32_t) override;

private:
    void DisposeNative(void*) override;
    void Finalize(napi_env env) override;
    void BindToMaterial(NapiApi::Object meJs, NapiApi::Object);
    void UnbindInputs();

    NapiApi::StrongRef inputs_;
    BASE_NS::unordered_map<BASE_NS::string, BASE_NS::shared_ptr<PropertyProxy>> proxies_;
};

#endif // _SHADER_JS_H
