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

#include <variant>

class ShaderJS final : BaseObject, public SceneResourceImpl {
public:
    static constexpr uint32_t ID = 40;
    static void Init(napi_env env, napi_value exports);
    ShaderJS(napi_env, napi_callback_info);
    ~ShaderJS() override;
    void* GetInstanceImpl(uint32_t) override;
    void DetachFromMaterial(NapiApi::Object me);
private:
    void DisposeNative(void*) override;
    void Finalize(napi_env env) override;
    SCENE_NS::IMaterial::Ptr PrepareBind(NapiApi::Object& inputs, NapiApi::Object& material, NapiApi::Object& meJs);
    void SetDefaults(NapiApi::Object& inputs, NapiApi::Object& material, SCENE_NS::IMaterial::Ptr& mat,
        BASE_NS::vector<napi_property_descriptor>& inputProps, META_NS::IMetadata::Ptr customProperties);
    void DefineProperties(NapiApi::Object& inputs, napi_env& e, BASE_NS::vector<napi_property_descriptor>& inputProps);
    void BindToMaterial(NapiApi::Object meJs, NapiApi::Object);
    void UnbindInputs(NapiApi::Object meJs);
    napi_value GetInputs(NapiApi::FunctionContext<>& ctx);
    napi_value SetInputs(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    using InputValueType = std::variant<std::monostate, BASE_NS::Math::Vec4, BASE_NS::Math::Vec3,
                                        BASE_NS::Math::Vec2, float, SCENE_NS::IImage::Ptr>;
    void SetInput(BASE_NS::string_view key, InputValueType& value);
    void InitInputs(NapiApi::Object meJs, NapiApi::Object);
    template<class type>
    static InputValueType ParseInputs(NapiApi::Object& obj, BASE_NS::string_view key);

    bool isInputsReflValid_ = {};
    NapiApi::StrongRef material_;
    NapiApi::StrongRef inputs_;
    BASE_NS::unordered_map<BASE_NS::string, BASE_NS::shared_ptr<PropertyProxy>> proxies_;
};

#endif // _SHADER_JS_H
