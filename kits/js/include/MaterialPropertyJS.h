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
#ifndef _MATERIAL_PROPERTY_JS_H_
#define _MATERIAL_PROPERTY_JS_H_
#include <meta/interface/intf_object.h>
#include "Vec4Proxy.h"

#include "BaseObjectJS.h"

class MaterialPropertyJS : public BaseObject {
public:
    static constexpr uint32_t ID = 35;
    static void Init(napi_env env, napi_value exports);
    MaterialPropertyJS(napi_env, napi_callback_info);
    ~MaterialPropertyJS() override;

    static void SetFactor(Scene::ITexture::Ptr texture, NapiApi::Object factorJS);
    static void SetImage(Scene::ITexture::Ptr texture, NapiApi::Object imageJS);
    static void SetSampler(Scene::ITexture::Ptr texture, NapiApi::Object samplerJS);
private:
    void* GetInstanceImpl(uint32_t) override;
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);
    void DisposeNative(void*) override;
    void Finalize(napi_env env) override;

    NapiApi::StrongRef sampler_;

    napi_value GetImage(NapiApi::FunctionContext<>& ctx);
    void SetImage(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value GetFactor(NapiApi::FunctionContext<>& ctx);
    void SetFactor(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value GetSampler(NapiApi::FunctionContext<>& ctx);
    void SetSampler(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    BASE_NS::unique_ptr<Vec4Proxy> factorProxy_;
};
#endif
