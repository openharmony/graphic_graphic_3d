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
#ifndef BLOOM_JS_H
#define BLOOM_JS_H
#include <napi_api.h>
#include <scene/interface/intf_scene.h>
class BloomConfiguration {
public:
    enum class Quality : uint32_t { LOW = 1, NORMAL = 2, HIGH = 3 };
    enum class Type : uint32_t { NORMAL = 1, HORIZONTAL = 2, VERTICAL = 3, BILATERAL = 4 };

    BloomConfiguration();
    ~BloomConfiguration();
    void SetFrom(NapiApi::Object obj);

    static BloomConfiguration* Unwrap(NapiApi::Object obj);

    NapiApi::StrongRef Wrap(NapiApi::Object obj);

    void SetTo(NapiApi::Object obj);
    void SetFrom(SCENE_NS::IBloom::Ptr bloom);
    void SetTo(SCENE_NS::IBloom::Ptr bloom);
    SCENE_NS::IPostProcess::Ptr GetPostProc();
    void SetPostProc(SCENE_NS::IPostProcess::Ptr postproc);

private:
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);
    void Detach();
    napi_value GetAmount(NapiApi::FunctionContext<>& ctx);
    void SetAmount(NapiApi::FunctionContext<float>& ctx);
    napi_value GetThresholdSoft(NapiApi::FunctionContext<>& ctx);
    void SetThresholdSoft(NapiApi::FunctionContext<float>& ctx);
    napi_value GetThresholdHard(NapiApi::FunctionContext<>& ctx);
    void SetThresholdHard(NapiApi::FunctionContext<float>& ctx);

    napi_value GetScatter(NapiApi::FunctionContext<>& ctx);
    void SetScatter(NapiApi::FunctionContext<float>& ctx);
    napi_value GetScaleFactor(NapiApi::FunctionContext<>& ctx);
    void SetScaleFactor(NapiApi::FunctionContext<float>& ctx);

    napi_value GetType(NapiApi::FunctionContext<>& ctx);
    void SetType(NapiApi::FunctionContext<uint32_t>& ctx);
    napi_value GetQuality(NapiApi::FunctionContext<>& ctx);
    void SetQuality(NapiApi::FunctionContext<uint32_t>& ctx);

    template<napi_value (BloomConfiguration::*F)(NapiApi::FunctionContext<>&)>
    static napi_value Method(napi_env env, napi_callback_info info);

    template<napi_value (BloomConfiguration::*F)(NapiApi::FunctionContext<>&)>
    static napi_value Getter(napi_env env, napi_callback_info info);
    template<typename Type, void (BloomConfiguration::*F)(NapiApi::FunctionContext<Type>&)>
    static inline napi_value Setter(napi_env env, napi_callback_info info);
    SCENE_NS::IPostProcess::Ptr postproc_; // postprocess object owning the bloom
    SCENE_NS::IBloom::Ptr bloom_;          // bloom object from postproc_
    napi_ref ref_ { nullptr };
    float thresholdHard_ { 1.0f };
    float thresholdSoft_ { 2.0f };
    float amountCoefficient_ { 0.25f };
    float scatter_ { 1.0f };
    float scaleFactor_ { 1.0f };
    BloomConfiguration::Type type_ { BloomConfiguration::Type::NORMAL };
    BloomConfiguration::Quality quality_ { BloomConfiguration::Quality::NORMAL };
    SCENE_NS::EffectQualityType GetQuality(BloomConfiguration::Quality bloomQualityType);
    BloomConfiguration::Quality SetQuality(SCENE_NS::EffectQualityType bloomQualityType);

    SCENE_NS::BloomType GetType(BloomConfiguration::Type bloomType);
    BloomConfiguration::Type SetType(SCENE_NS::BloomType bloomTypeIn);
};

#endif
