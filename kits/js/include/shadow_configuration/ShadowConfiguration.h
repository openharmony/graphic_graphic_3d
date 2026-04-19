/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef SHADOW_CONFIGURATION_SOFT_SHADOW_CONFIG_JS_H
#define SHADOW_CONFIGURATION_SOFT_SHADOW_CONFIG_JS_H

#include <napi_api.h>
#include <scene/interface/intf_scene.h>

namespace ShadowConfiguration {

enum class ShadowAlgorithmType : uint32_t { PCF = 0 };
void RegisterEnums(NapiApi::Object exports);

template<typename Class, napi_value (Class::*F)(NapiApi::FunctionContext<>&)>
napi_value Getter(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext fc(env, info);
    if (fc && fc.This()) {
        Class* impl = nullptr;
        napi_unwrap(fc.GetEnv(), fc.This().ToNapiValue(), (void**)&impl);
        if (impl) {
            return (impl->*F)(fc);
        }
    }
    return fc.GetUndefined();
}

template<typename Class, typename Type, void (Class::*F)(NapiApi::FunctionContext<Type>&)>
inline napi_value Setter(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext<Type> fc(env, info);
    if (fc && fc.This()) {
        Class* impl = nullptr;
        napi_unwrap(fc.GetEnv(), fc.This().ToNapiValue(), (void**)&impl);
        if (impl) {
            (impl->*F)(fc);
        }
    }
    return fc.GetUndefined();
};

class SoftShadowConfigJS {
public:
    virtual ~SoftShadowConfigJS() = default;
    static BASE_NS::unique_ptr<SoftShadowConfigJS> FromJs(NapiApi::Object jsShadowConfig);
    virtual SCENE_NS::IRenderConfiguration::Ptr GetRenderConfiguration() const = 0;
    virtual void SetRenderConfiguration(SCENE_NS::IRenderConfiguration::Ptr rc) = 0;
    virtual NapiApi::StrongRef Wrap(NapiApi::Object obj) = 0;

protected:
    SoftShadowConfigJS() = default;
    SCENE_NS::IRenderConfiguration::WeakPtr renderConfiguration_;
};

} // namespace ShadowConfiguration

#endif
