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

#include "shadow_configuration/ShadowConfiguration.h"
#include "shadow_configuration/PCFConfigJS.h"
#include <napi_api.h>

namespace ShadowConfiguration {

void RegisterEnums(NapiApi::Object exports)
{
    napi_value v;
    NapiApi::Object shadowAlgorithmType(exports.GetEnv());

#define DECL_ENUM(enu, x)                                                                    \
    {                                                                                        \
        napi_create_uint32(enu.GetEnv(), static_cast<uint32_t>(ShadowAlgorithmType::x), &v); \
        enu.Set(#x, v);                                                                      \
    }                                                                                        \
    DECL_ENUM(shadowAlgorithmType, PCF);
#undef DECL_ENUM

    exports.Set("ShadowAlgorithmType", shadowAlgorithmType);
}

BASE_NS::unique_ptr<SoftShadowConfigJS> SoftShadowConfigJS::FromJs(NapiApi::Object jsShadowConfig)
{
    SoftShadowConfigJS* result = nullptr;
    if (auto value = jsShadowConfig.Get<uint32_t>("shadowAlgorithmType"); value.IsValid()) {
        uint32_t type = value;
        if (type == static_cast<uint32_t>(ShadowAlgorithmType::PCF)) {
            result = PCFConfigJS::FromJs(jsShadowConfig);
        } else {
            LOG_E("Invalid Soft Shadow Config given: unsupported shadowAlgorithmType");
        }
    } else {
        LOG_E("Invalid Soft Shadow Config given: shadowAlgorithmType missing");
    }
    return BASE_NS::unique_ptr<SoftShadowConfigJS> { result };
}

} // namespace ShadowConfiguration
