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

#ifndef OHOS_RENDER_3D_LIGHT_JS_H
#define OHOS_RENDER_3D_LIGHT_JS_H
#include <meta/interface/intf_object.h>

#include "BaseObjectJS.h"
#include "ColorProxy.h"
#include "NodeJS.h"

class BaseLight : public NodeImpl {
public:
    static constexpr uint32_t ID = 10;
    enum LightType {
        /**
         * Directional light.
         */
        DIRECTIONAL = 1,
        /**
         * Spotlight.
         */
        SPOT = 2,
        /**
         * Point light.
         */
        POINT = 3,
    };
    static void Init(const char* name, napi_env env, napi_value exports,
        BASE_NS::vector<napi_property_descriptor>& node_props, napi_callback ctor);
    BaseLight(LightType lt);
    ~BaseLight() override;
    static void RegisterEnums(NapiApi::Object exports);

protected:
    void Create(napi_env e, napi_callback_info i);
    void* GetInstanceImpl(uint32_t id);
    void DisposeNative(TrueRootObject*);
    void Finalize(napi_env env, TrueRootObject*);

private:
    napi_value GetlightType(NapiApi::FunctionContext<>& ctx);
    napi_value GetEnabled(NapiApi::FunctionContext<>& ctx);
    void SetEnabled(NapiApi::FunctionContext<bool>& ctx);

    napi_value GetColor(NapiApi::FunctionContext<>& ctx);
    void SetColor(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    napi_value GetShadowEnabled(NapiApi::FunctionContext<>& ctx);
    void SetShadowEnabled(NapiApi::FunctionContext<bool>& ctx);

    napi_value GetIntensity(NapiApi::FunctionContext<>& ctx);
    void SetIntensity(NapiApi::FunctionContext<float>& ctx);

    LightType lightType_;
    BASE_NS::unique_ptr<ColorProxy> colorProxy_;
};

class SpotLightJS : BaseObject<SpotLightJS>, BaseLight {
public:
    static constexpr uint32_t ID = 11;
    static void Init(napi_env env, napi_value exports);
    SpotLightJS(napi_env, napi_callback_info);

private:
    void* GetInstanceImpl(uint32_t id);
    void DisposeNative();
    void Finalize(napi_env env);
};

class DirectionalLightJS : BaseObject<DirectionalLightJS>, BaseLight {
public:
    static constexpr uint32_t ID = 12;
    static void Init(napi_env env, napi_value exports);
    DirectionalLightJS(napi_env, napi_callback_info);

private:
    void* GetInstanceImpl(uint32_t id);
    void DisposeNative();
    void Finalize(napi_env env);
    napi_value GetNear(NapiApi::FunctionContext<>& ctx);
    void SetNear(NapiApi::FunctionContext<float>& ctx);
};
class PointLightJS : BaseObject<PointLightJS>, BaseLight {
public:
    static constexpr uint32_t ID = 13;
    static void Init(napi_env env, napi_value exports);
    PointLightJS(napi_env, napi_callback_info);

private:
    void* GetInstanceImpl(uint32_t id);
    void DisposeNative();
    void Finalize(napi_env env);
};

#endif // OHOS_RENDER_3D_LIGHT_JS_H
