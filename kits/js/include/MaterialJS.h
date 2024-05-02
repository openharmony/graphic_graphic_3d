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

#ifndef OHOS_RENDER_3D_MATERIAL_JS_H
#define OHOS_RENDER_3D_MATERIAL_JS_H
#include <meta/interface/intf_object.h>

#include "BaseObjectJS.h"
#include "NodeJS.h"

class BaseMaterial : public SceneResourceImpl {
public:
    static constexpr uint32_t ID = 30;
    enum MaterialType { SHADER = 1 };
    static void Init(const char* name, napi_env env, napi_value exports, napi_callback ctor,
        BASE_NS::vector<napi_property_descriptor>& props);
    BaseMaterial(MaterialType lt);
    ~BaseMaterial() override;

protected:
    void* GetInstanceImpl(uint32_t);
    void DisposeNative(TrueRootObject*);

private:
    napi_value GetMaterialType(NapiApi::FunctionContext<>& ctx);
    MaterialType materialType_;
};

class ShaderMaterialJS : BaseObject<ShaderMaterialJS>, BaseMaterial {
public:
    static constexpr uint32_t ID = 31;
    static void Init(napi_env env, napi_value exports);
    ShaderMaterialJS(napi_env, napi_callback_info);
    ~ShaderMaterialJS() override;

private:
    void* GetInstanceImpl(uint32_t) override;
    void DisposeNative() override;
    void Finalize(napi_env env) override;

    napi_value GetColorShader(NapiApi::FunctionContext<>& ctx);
    void SetColorShader(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    META_NS::IObject::Ptr shaderBind_;

    NapiApi::StrongRef shader_;
};
#endif // OHOS_RENDER_3D_MESH_JS_H