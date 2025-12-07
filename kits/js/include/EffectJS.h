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

#ifndef EFFECTJS_H
#define EFFECTJS_H

#include "BaseObjectJS.h"
#include "PropertyProxy.h"

#include <base/containers/unordered_map.h>
#include <scene/interface/intf_effect.h>

class EffectsContainerJS {
public:
    static void Init(napi_env env, napi_value exports);
    EffectsContainerJS(napi_env, napi_callback_info);
    virtual ~EffectsContainerJS();

private:
    META_NS::ArrayProperty<SCENE_NS::IEffect::Ptr> GetEffects() const;
    napi_value GetCount(NapiApi::FunctionContext<>& ctx);
    napi_value GetChild(NapiApi::FunctionContext<uint32_t>& ctx);

    napi_value ClearChildren(NapiApi::FunctionContext<>& ctx);
    napi_value InsertChildAfter(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx);
    napi_value AppendChild(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value RemoveChild(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    NapiApi::WeakObjectRef camera_;
    NapiApi::WeakObjectRef scene_;
};

class EffectJS : public BaseObject {
public:
    static constexpr uint32_t ID = 1024;
    static void Init(napi_env env, napi_value exports);
    static SCENE_NS::IEffect::Ptr CreateEffectInstance();

    EffectJS(napi_env, napi_callback_info);
    ~EffectJS() override;
    void* GetInstanceImpl(uint32_t) override;

    napi_value GetEnabled(NapiApi::FunctionContext<>& ctx);
    void SetEnabled(NapiApi::FunctionContext<bool>& ctx);
    napi_value GetEffectId(NapiApi::FunctionContext<>& ctx);

private:
    SCENE_NS::IEffect::Ptr GetEffect(NapiApi::Object o) const;
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);
    void DisposeNative(void *) override;
    void Finalize(napi_env env) override;

    void AddProperties(NapiApi::Object meJs, const META_NS::IObject::Ptr& obj);
    NapiApi::WeakObjectRef scene_;
    BASE_NS::unordered_map<BASE_NS::string, BASE_NS::shared_ptr<PropertyProxy>> proxies_;
};
#endif