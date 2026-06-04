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

#ifndef SHADOW_CONFIGURATION_PCF_CONFIG_JS_H
#define SHADOW_CONFIGURATION_PCF_CONFIG_JS_H

#include "shadow_configuration/ShadowConfiguration.h"
#include <optional>

namespace ShadowConfiguration {

class PCFConfigJS : public SoftShadowConfigJS {
public:
    ~PCFConfigJS() override = default;
    static SoftShadowConfigJS* FromJs(NapiApi::Object& jsPCFConfig);
    SCENE_NS::IRenderConfiguration::Ptr GetRenderConfiguration() const override;
    void SetRenderConfiguration(SCENE_NS::IRenderConfiguration::Ptr rc) override;
    NapiApi::StrongRef Wrap(NapiApi::Object obj) override;

    static constexpr float DEFAULT_RADIUS = 5.0f;
    static constexpr int32_t DEFAULT_COUNT = 16;
    static void Init(napi_env env, napi_value exports);

private:
    explicit PCFConfigJS(float radius, int32_t count);
    float radius_{DEFAULT_RADIUS};
    int32_t count_{DEFAULT_COUNT};

    bool isRadiusUndefined_{false};
    bool isCountUndefined_{false};

    napi_value GetType(NapiApi::FunctionContext<>& ctx);
    napi_value GetRadius(NapiApi::FunctionContext<>& ctx);
    napi_value GetCount(NapiApi::FunctionContext<>& ctx);

    void SetDefaultShadowSampleCount();
    void SetDefaultShadowSampleRadius();

    void SetRadius(float radius);
    void SetCount(int32_t count);
};

}  // namespace ShadowConfiguration

#endif
