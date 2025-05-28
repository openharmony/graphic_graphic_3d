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

#include "ToneMapJS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/property_events.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_scene.h>

#include <render/intf_render_context.h>

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
using namespace SCENE_NS;
SCENE_NS::TonemapType ToNative(ToneMapJS::ToneMappingType typeI)
{
    SCENE_NS::TonemapType type;
    switch (typeI) {
        case ToneMapJS::ToneMappingType::ACES:
            type = SCENE_NS::TonemapType::ACES;
            break;
        case ToneMapJS::ToneMappingType::ACES_2020:
            type = SCENE_NS::TonemapType::ACES_2020;
            break;
        case ToneMapJS::ToneMappingType::FILMIC:
            type = SCENE_NS::TonemapType::FILMIC;
            break;
        default:
            // default from lowlev..
            type = SCENE_NS::TonemapType::ACES;
            break;
    }
    return type;
}
ToneMapJS::ToneMappingType ToJs(SCENE_NS::TonemapType typeI)
{
    ToneMapJS::ToneMappingType type;
    switch (typeI) {
        case SCENE_NS::TonemapType::ACES:
            type = ToneMapJS::ToneMappingType::ACES;
            break;
        case SCENE_NS::TonemapType::ACES_2020:
            type = ToneMapJS::ToneMappingType::ACES_2020;
            break;
        case SCENE_NS::TonemapType::FILMIC:
            type = ToneMapJS::ToneMappingType ::FILMIC;
            break;
        default:
            // default from lowlev..
            type = ToneMapJS::ToneMappingType ::ACES;
            break;
    }
    return type;
}

SCENE_NS::TonemapType ToneMapJS::ToNativeType(uint32_t typeI)
{
    return ToNative(static_cast<ToneMapJS::ToneMappingType>(typeI));
}

void ToneMapJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> node_props;
    // clang-format off
    node_props.emplace_back(GetSetProperty<uint32_t, ToneMapJS, &ToneMapJS::GetType, &ToneMapJS::SetType>("type"));
    node_props.emplace_back(
        GetSetProperty<float, ToneMapJS, &ToneMapJS::GetExposure, &ToneMapJS::SetExposure>("exposure"));
    node_props.push_back(MakeTROMethod<NapiApi::FunctionContext<>, ToneMapJS, &ToneMapJS::Dispose>("destroy"));
    // clang-format on

    napi_value func;
    auto status = napi_define_class(env, "ToneMappingSettings", NAPI_AUTO_LENGTH, BaseObject::ctor<ToneMapJS>(),
        nullptr, node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor("ToneMappingSettings", func);
    }

    NapiApi::Object exp(env, exports);

    napi_value eType, v;
    napi_create_object(env, &eType);
#define DECL_ENUM(enu, x)                            \
    napi_create_uint32(env, ToneMappingType::x, &v); \
    napi_set_named_property(env, enu, #x, v);

    DECL_ENUM(eType, ACES);
    DECL_ENUM(eType, ACES_2020);
    DECL_ENUM(eType, FILMIC);
#undef DECL_ENUM
    exp.Set("ToneMappingType", eType);
}

napi_value ToneMapJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    LOG_V("ToneMapJS::Dispose");
    DisposeNative(nullptr);
    return {};
}
void ToneMapJS::DisposeNative(void*)
{
    if (!disposed_) {
        disposed_ = true;
        LOG_V("ToneMapJS::DisposeNative");
        UnsetNativeObject();
    }
}
void ToneMapJS::UnbindFromNative()
{
    // We don't use a property proxy, so update members manually.
    if (auto toneMap = GetNativeObject<SCENE_NS::ITonemap>()) {
        type_ = ToJs(toneMap->Type()->GetValue());
        exposure_ = toneMap->Exposure()->GetValue();
    }
    UnsetNativeObject();
}

void ToneMapJS::BindToNative(SCENE_NS::ITonemap::Ptr native)
{
    SetNativeObject(interface_pointer_cast<META_NS::IObject>(native), PtrType::WEAK);
    if (auto toneMap = GetNativeObject<SCENE_NS::ITonemap>()) {
        toneMap->Type()->SetValue(ToNative(type_));
        toneMap->Exposure()->SetValue(exposure_);
    }
}

void* ToneMapJS::GetInstanceImpl(uint32_t id)
{
    if (id == ToneMapJS::ID)
        return this;
    return nullptr;
}
void ToneMapJS::Finalize(napi_env env)
{
    DisposeNative(nullptr);
    BaseObject::Finalize(env);
}

ToneMapJS::ToneMapJS(napi_env e, napi_callback_info i)
    : BaseObject(e, i), type_(DEFAULT_TYPE), exposure_(DEFAULT_EXPOSURE)
{
    LOG_V("ToneMapJS ++");
    auto tonemap = GetNativeObject<SCENE_NS::ITonemap>();
    if (!tonemap) {
        LOG_E("Cannot finish creating a tonemap: Native tonemap object missing");
        assert(false);
        return;
    }

    // We accept 0 and 1 arg calls. If we get 0, do nothing so original tonemap stays unchanged.
    NapiApi::FunctionContext<NapiApi::Object> ctx(e, i);
    if (!ctx) {
        if (ctx.ArgCount() != 0) {
            LOG_E("Cannot finish creating a tonemap: Invalid args given");
            assert(false);
        }
        return;
    }

    NapiApi::Object toneMapArgs = ctx.Arg<0>();
    exposure_ = toneMapArgs.Get<float>("exposure").valueOrDefault(exposure_);
    type_ = static_cast<ToneMappingType>(toneMapArgs.Get<uint32_t>("type").valueOrDefault(type_));
    tonemap->Type()->SetValue(ToNative(type_));
    tonemap->Exposure()->SetValue(exposure_);
    tonemap->Enabled()->SetValue(true);
}

ToneMapJS::~ToneMapJS()
{
    LOG_V("ToneMapJS --");
    DisposeNative(nullptr);
    if (!GetNativeObject()) {
        return;
    }
}

napi_value ToneMapJS::GetType(NapiApi::FunctionContext<>& ctx)
{
    if (auto toneMap = interface_cast<SCENE_NS::ITonemap>(GetNativeObject())) {
        type_ = ToJs(toneMap->Type()->GetValue());
    }

    return ctx.GetNumber(static_cast<uint32_t>(type_));
}

void ToneMapJS::SetType(NapiApi::FunctionContext<uint32_t>& ctx)
{
    const auto nativeType = ToNativeType(ctx.Arg<0>());
    type_ = ToJs(nativeType);

    if (auto toneMap = interface_cast<SCENE_NS::ITonemap>(GetNativeObject())) {
        toneMap->Type()->SetValue(nativeType);
    }
}

napi_value ToneMapJS::GetExposure(NapiApi::FunctionContext<>& ctx)
{
    if (auto toneMap = interface_cast<SCENE_NS::ITonemap>(GetNativeObject())) {
        exposure_ = toneMap->Exposure()->GetValue();
    }

    return ctx.GetNumber(exposure_);
}

void ToneMapJS::SetExposure(NapiApi::FunctionContext<float>& ctx)
{
    exposure_ = ctx.Arg<0>();
    if (auto toneMap = interface_cast<SCENE_NS::ITonemap>(GetNativeObject())) {
        toneMap->Exposure()->SetValue(exposure_);
    }
}
