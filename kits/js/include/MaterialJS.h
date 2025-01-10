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
#ifndef MATERIAL_JS_H
#define MATERIAL_JS_H
#include <meta/interface/intf_object.h>

#include "BaseObjectJS.h"
#include "SceneResourceImpl.h"

class BaseMaterial : public SceneResourceImpl {
public:
    static constexpr uint32_t ID = 30;
    enum MaterialType { SHADER = 1, METALLIC_ROUGHNESS = 2 };
    enum CullMode { NONE = 0, FRONT = 1, BACK = 2 };
    static void Init(const char* name, napi_env env, napi_value exports, napi_callback ctor,
        BASE_NS::vector<napi_property_descriptor>& props);
    BaseMaterial(MaterialType lt);
    ~BaseMaterial() override;

protected:
    void* GetInstanceImpl(uint32_t);
    void DisposeNative(TrueRootObject*);

private:
    napi_value GetMaterialType(NapiApi::FunctionContext<>& ctx);
    napi_value GetShadowReceiver(NapiApi::FunctionContext<>& ctx);
    void SetShadowReceiver(NapiApi::FunctionContext<bool>& ctx);
    napi_value GetCullMode(NapiApi::FunctionContext<>& ctx);
    void SetCullMode(NapiApi::FunctionContext<uint32_t>& ctx);
    napi_value GetBlend(NapiApi::FunctionContext<>& ctx);
    void SetBlend(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value GetAlphaCutoff(NapiApi::FunctionContext<>& ctx);
    void SetAlphaCutoff(NapiApi::FunctionContext<float>& ctx);
    napi_value GetRenderSort(NapiApi::FunctionContext<>& ctx);
    void SetRenderSort(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    MaterialType materialType_;
    NapiApi::StrongRef blend_;
    NapiApi::StrongRef renderSort_;
};

class ShaderMaterialJS : BaseObject<ShaderMaterialJS>, BaseMaterial {
public:
    static constexpr uint32_t ID = 31;
    static void Init(napi_env env, napi_value exports);
    ShaderMaterialJS(napi_env, napi_callback_info);
    ~ShaderMaterialJS() override;

private:
    void* GetInstanceImpl(uint32_t) override;
    void DisposeNative(void*) override;
    void Finalize(napi_env env) override;

    napi_value GetColorShader(NapiApi::FunctionContext<>& ctx);
    void SetColorShader(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    META_NS::IObject::Ptr shaderBind_;

    NapiApi::StrongRef shader_;
};
#endif
