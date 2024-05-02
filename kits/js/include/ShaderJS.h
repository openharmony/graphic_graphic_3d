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

#ifndef OHOS_RENDER_3D_SHADER_JS_H
#define OHOS_RENDER_3D_SHADER_JS_H

#include <scene_plugin/interface/intf_material.h>

#include "BaseObjectJS.h"
#include "PropertyProxy.h"
#include "SceneResourceImpl.h"

class Proxy {
public:
    Proxy() = default;
    virtual ~Proxy() = default;
    virtual void insertProp(BASE_NS::vector<napi_property_descriptor>& props) = 0;
};

class ShaderJS : BaseObject<ShaderJS>, SceneResourceImpl {
public:
    static constexpr uint32_t ID = 40;
    static void Init(napi_env env, napi_value exports);
    ShaderJS(napi_env, napi_callback_info);
    ~ShaderJS() override;
    void* GetInstanceImpl(uint32_t id) override;

private:
    void DisposeNative() override;
    void Finalize(napi_env env) override;
    void BindToMaterial(NapiApi::Object meJs, NapiApi::Object);

private:
    NapiApi::StrongRef inputs_;
    BASE_NS::vector<BASE_NS::shared_ptr<Proxy>> proxies_;
};

#endif // OHOS_RENDER_3D_SHADER_JS_H