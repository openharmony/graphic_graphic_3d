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

#include "shadow_configuration/PCFConfigJS.h"
#include <napi_api.h>

namespace ShadowConfiguration {

PCFConfigJS::PCFConfigJS(float radius, int32_t count) : SoftShadowConfigJS(), radius_(radius), count_(count) {}

void PCFConfigJS::Init(napi_env env, napi_value exports)
{
    napi_value defaultType = nullptr;
    napi_value defaultRadius = nullptr;
    napi_value defaultCount = nullptr;
    napi_create_uint32(env, static_cast<uint32_t>(ShadowAlgorithmType::PCF), &defaultType);
    napi_create_double(env, DEFAULT_RADIUS, &defaultRadius);
    napi_create_int32(env, DEFAULT_COUNT, &defaultCount);

    const auto props = BASE_NS::vector<napi_property_descriptor> {
        // clang-format off
        { "shadowAlgorithmType", nullptr, nullptr, nullptr, nullptr, defaultType, napi_default_jsproperty, nullptr },
        { "shadowSampleRadius", nullptr, nullptr, nullptr, nullptr, defaultRadius, napi_default_jsproperty, nullptr },
        { "shadowSampleCount", nullptr, nullptr, nullptr, nullptr, defaultCount, napi_default_jsproperty, nullptr },
        // clang-format on
    };

    napi_value jsClassCtor = nullptr;
    auto ctor = [](napi_env env, napi_callback_info info) -> napi_value {
        return NapiApi::FunctionContext(env, info).This().ToNapiValue();
    };
    napi_define_class(env, "PCFConfig", NAPI_AUTO_LENGTH, ctor, nullptr, props.size(), props.data(), &jsClassCtor);
    NapiApi::MyInstanceState* mis {};
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor("PCFConfig", jsClassCtor);
    }
    NapiApi::Object { env, exports }.Set("PCFConfig", jsClassCtor);
}

SoftShadowConfigJS* PCFConfigJS::FromJs(NapiApi::Object& jsPCFConfig)
{
    float radius = jsPCFConfig.Get<float>("shadowSampleRadius").valueOrDefault();
    int32_t count = jsPCFConfig.Get<int32_t>("shadowSampleCount").valueOrDefault();

    if (!std::isfinite(radius) || radius < 0.0f || count < 0) {
        LOG_E("Unable to create PCFConfigJS: Invalid shadowSampleRadius or shadowSampleCount given");
        return {};
    } else {
        return new PCFConfigJS(radius, count);
    }
}

NapiApi::StrongRef PCFConfigJS::Wrap(NapiApi::Object obj)
{
    auto dtor = [](napi_env env, void* nativeObject, void* finalize) {
        PCFConfigJS* ptr = static_cast<PCFConfigJS*>(nativeObject);
        delete ptr;
    };
    napi_wrap(obj.GetEnv(), obj.ToNapiValue(), this, dtor, nullptr, nullptr);

    // clang-format off
    napi_property_descriptor methods[] = {
        { "shadowAlgorithmType", nullptr, nullptr, Getter<PCFConfigJS, &PCFConfigJS::GetType>,
            nullptr, nullptr, napi_default, this },
        { "shadowSampleRadius", nullptr, nullptr, Getter<PCFConfigJS, &PCFConfigJS::GetRadius>,
            Setter<PCFConfigJS, float, &PCFConfigJS::SetRadius>, nullptr, napi_default, this },
        { "shadowSampleCount", nullptr, nullptr, Getter<PCFConfigJS, &PCFConfigJS::GetCount>,
            Setter<PCFConfigJS, int32_t, &PCFConfigJS::SetCount>, nullptr, napi_default, this },
    };
    // clang-format on
    napi_define_properties(obj.GetEnv(), obj.ToNapiValue(), sizeof(methods) / sizeof(methods[0]), methods);
    return NapiApi::StrongRef(obj);
}

napi_value PCFConfigJS::GetType(NapiApi::FunctionContext<>& ctx)
{
    return ctx.GetNumber(static_cast<uint32_t>(ShadowAlgorithmType::PCF));
}

napi_value PCFConfigJS::GetRadius(NapiApi::FunctionContext<>& ctx)
{
    if (isRadiusUndefined_) {
        return ctx.GetUndefined();
    }
    return ctx.GetNumber(radius_);
}

void PCFConfigJS::SetRadius(NapiApi::FunctionContext<float>& ctx)
{
    auto rc = GetRenderConfiguration();
    if (!rc) {
        LOG_E("no rc return it");
        return;
    }

    auto arg0 = ctx.Arg<0>();
    if (arg0.IsUndefinedOrNull()) {
        LOG_E("Undefined shadow sample radius given, lume engine back to default value, js value as undefined.");
        rc->VariablePcfRadius()->SetValue(DEFAULT_RADIUS);
        isRadiusUndefined_ = true;
        return;
    }

    float radius = arg0.valueOrDefault();
    if (!std::isfinite(radius) || radius < 0.0f) {
        LOG_E("Invalid shadow sample radius given, return it.");
        return;
    } else {
        rc->VariablePcfRadius()->SetValue(radius);
        isRadiusUndefined_ = false;
        radius_ = radius;
    }
}

napi_value PCFConfigJS::GetCount(NapiApi::FunctionContext<>& ctx)
{
    if (isCountUndefined_) {
        return ctx.GetUndefined();
    }
    return ctx.GetNumber(count_);
}

void PCFConfigJS::SetCount(NapiApi::FunctionContext<int32_t>& ctx)
{
    auto rc = GetRenderConfiguration();
    if (!rc) {
        LOG_E("no rc return it");
        return;
    }

    auto arg0 = ctx.Arg<0>();
    if (arg0.IsUndefinedOrNull()) {
        LOG_E("Undefined shadow sample count given, lume engine back to default value, js value as undefined.");
        rc->VariablePcfSampleCount()->SetValue(DEFAULT_COUNT);
        isCountUndefined_ = true;
        return;
    }

    int32_t count = arg0.valueOrDefault();
    if (count < 0) {
        LOG_E("Invalid shadow sample count given, return it.");
        return;
    } else {
        rc->VariablePcfSampleCount()->SetValue(count);
        isCountUndefined_ = false;
        count_ = count;
    }
}

SCENE_NS::IRenderConfiguration::Ptr PCFConfigJS::GetRenderConfiguration() const
{
    return renderConfiguration_.lock();
}

void PCFConfigJS::SetRenderConfiguration(SCENE_NS::IRenderConfiguration::Ptr rc)
{
    if (!rc) {
        LOG_E("no rc return it");
        return;
    }
    rc->ShadowType()->SetValue(SCENE_NS::SceneShadowType::VARIABLE_PCF);
    rc->VariablePcfRadius()->SetValue(radius_);
    rc->VariablePcfSampleCount()->SetValue(count_);

    renderConfiguration_ = rc;
}

} // namespace ShadowConfiguration
