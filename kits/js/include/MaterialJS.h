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

#include <optional>

#include <meta/interface/intf_object.h>
#include <meta/api/property/property_event_handler.h>
#include <scene/interface/intf_material.h>

#include "BaseObjectJS.h"
#include "SceneResourceImpl.h"
#include "PropertyProxy.h"

class BaseMaterial : public SceneResourceImpl {
public:
    static constexpr uint32_t ID = 30;
    enum MaterialType { SHADER = 1, METALLIC_ROUGHNESS = 2, UNLIT = 3, OCCLUSION = 4 };
    enum CullMode { NONE = 0, FRONT = 1, BACK = 2 };
    enum PolygonMode { FILL = 0, LINE = 1, POINT = 2 };
    static void Init(const char* name, napi_env env, napi_value exports, napi_callback ctor,
        BASE_NS::vector<napi_property_descriptor>& props);
    static void InitEnumType(napi_env env, napi_value exports);
    void* GetInstanceImpl(uint32_t) override;
protected:
    BaseMaterial(MaterialType lt);
    ~BaseMaterial() override;

    void DisposeNative(BaseObject*);

    MaterialType materialType_;

    void ShaderChanged(SCENE_NS::IShader::Ptr& shader);

private:
    napi_value GetMaterialType(NapiApi::FunctionContext<>& ctx);
    napi_value GetShadowReceiver(NapiApi::FunctionContext<>& ctx);
    void SetShadowReceiver(NapiApi::FunctionContext<std::optional<bool>>& ctx);
    napi_value GetCullMode(NapiApi::FunctionContext<>& ctx);
    void SetCullMode(NapiApi::FunctionContext<std::optional<uint32_t>>& ctx);
    napi_value GetPolygonMode(NapiApi::FunctionContext<>& ctx);
    void SetPolygonMode(NapiApi::FunctionContext<std::optional<uint32_t>>& ctx);
    napi_value GetBlend(NapiApi::FunctionContext<>& ctx);
    void SetBlend(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value GetAlphaCutoff(NapiApi::FunctionContext<>& ctx);
    void SetAlphaCutoff(NapiApi::FunctionContext<std::optional<float>>& ctx);
    napi_value GetRenderSort(NapiApi::FunctionContext<>& ctx);
    void SetRenderSort(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    SCENE_NS::IShader::Ptr GetMaterialShader(const NapiApi::Object& me) const;

    struct BlendJS {
        NapiApi::Object GetObject()
        {
            return objectProxy.GetObject();
        }
        NapiApi::StrongRef objectProxy;
        BASE_NS::shared_ptr<PropertyProxy> enabledProxy;
    };
    BlendJS blend_;
    META_NS::PropertyChangedEventHandler shaderChanged_;

    void StoreDefaultCullMode(const SCENE_NS::IShader::Ptr& shader);
    std::optional<SCENE_NS::CullModeFlags> defaultCullMode_;
};

class MaterialJS final : public BaseObject, public BaseMaterial {
public:
    static constexpr uint32_t ID = 31;
    static void Init(napi_env env, napi_value exports);
    MaterialJS(napi_env, napi_callback_info);
    ~MaterialJS() override;

    // This currently returns the scene object this material is tied to. By current design this mirrors the
    // MaterialComponent from ecs but could in the future be moved under RenderContext as well. If this is done later
    // this needs to be changed accordingly. This is mainly used by ShaderJS which is not tied to a scene to access the
    // material's scene when it is bound to it.
    NapiApi::Object GetSceneObject();
    void* GetInstanceImpl(uint32_t) override;
private:
    void DisposeNative(void*) override;
    void Finalize(napi_env env) override;

    napi_value GetColorShader(NapiApi::FunctionContext<>& ctx);
    bool EarlyExitSet(NapiApi::Object& shaderJS, SCENE_NS::IMaterial::Ptr& material);
    void SetColorShader(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    template<size_t Index>
    napi_value GetMaterialProperty(NapiApi::FunctionContext<>& ctx);
    template<size_t Index>
    void SetMaterialProperty(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    META_NS::IObject::Ptr shaderBind_;
    NapiApi::StrongRef shader_;
};

#endif
