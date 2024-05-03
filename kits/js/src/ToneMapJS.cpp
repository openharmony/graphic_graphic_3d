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
#include <scene_plugin/api/camera.h> //for the classid...
#include <scene_plugin/api/node_uid.h>
#include <scene_plugin/interface/intf_ecs_scene.h>
#include <scene_plugin/interface/intf_node.h>
#include <scene_plugin/interface/intf_postprocess.h>
#include <scene_plugin/interface/intf_scene.h>

#include <render/intf_render_context.h>

#include "PostProcJS.h"
using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
using namespace SCENE_NS;
SCENE_NS::ITonemap::TonemapType ConvertTo(ToneMapJS::ToneMappingType typeI)
{
    SCENE_NS::ITonemap::TonemapType type;
    switch (typeI) {
        case ToneMapJS::ToneMappingType::ACES:
            type = SCENE_NS::ITonemap::TonemapType::ACES;
            break;
        case ToneMapJS::ToneMappingType::ACES_2020:
            type = SCENE_NS::ITonemap::TonemapType::ACES_2020;
            break;
        case ToneMapJS::ToneMappingType::FILMIC:
            type = SCENE_NS::ITonemap::TonemapType::FILMIC;
            break;
        default:
            // default from lowlev..
            type = ITonemap::TonemapType::ACES;
            break;
    }
    return type;
}
ToneMapJS::ToneMappingType ConvertFrom(SCENE_NS::ITonemap::TonemapType typeI)
{
    ToneMapJS::ToneMappingType type;
    switch (typeI) {
        case SCENE_NS::ITonemap::TonemapType::ACES:
            type = ToneMapJS::ToneMappingType::ACES;
            break;
        case SCENE_NS::ITonemap::TonemapType::ACES_2020:
            type = ToneMapJS::ToneMappingType::ACES_2020;
            break;
        case SCENE_NS::ITonemap::TonemapType::FILMIC:
            type = ToneMapJS::ToneMappingType ::FILMIC;
            break;
        default:
            // default from lowlev..
            type = ToneMapJS::ToneMappingType ::ACES;
            break;
    }
    return type;
}

SCENE_NS::ITonemap::TonemapType ConvertTo(uint32_t typeI)
{
    return ConvertTo(static_cast<ToneMapJS::ToneMappingType>(typeI));
}
void ToneMapJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> node_props;
    // clang-format off
    node_props.emplace_back(GetSetProperty<uint32_t, ToneMapJS, &ToneMapJS::GetType, &ToneMapJS::SetType>("type"));
    node_props.emplace_back(GetSetProperty<float, ToneMapJS, &ToneMapJS::GetExposure,
        &ToneMapJS::SetExposure>("exposure"));
    node_props.push_back(MakeTROMethod<NapiApi::FunctionContext<>, ToneMapJS, &ToneMapJS::Dispose>("destroy"));
    // clang-format on

    napi_value func;
    auto status = napi_define_class(env, "ToneMappingSettings", NAPI_AUTO_LENGTH, BaseObject::ctor<ToneMapJS>(),
        nullptr, node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    napi_get_instance_data(env, (void**)&mis);
    mis->StoreCtor("ToneMappingSettings", func);

    NapiApi::Object exp(env, exports);

    napi_value eType;
    napi_value v;
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
    LOG_F("ToneMapJS::Dispose");
    DisposeNative();
    return {};
}
void ToneMapJS::DisposeNative()
{
    if (!disposed_) {
        disposed_ = true;
        LOG_F("ToneMapJS::DisposeNative");
        if (auto tmp = interface_pointer_cast<SCENE_NS::ITonemap>(GetNativeObject())) {
            // reset the native object refs
            SetNativeObject(nullptr, false);
            SetNativeObject(nullptr, true);

            ExecSyncTask([scn = BASE_NS::move(tmp)]() { return META_NS::IAny::Ptr {}; });
        }
    }
}
void* ToneMapJS::GetInstanceImpl(uint32_t id)
{
    if (id == ToneMapJS::ID) {
        return this;
    }
    return nullptr;
}
void ToneMapJS::Finalize(napi_env env)
{
    // hmm.. do i need to do something BEFORE the object gets deleted..
    DisposeNative();
    BaseObject<ToneMapJS>::Finalize(env);
}

ToneMapJS::ToneMapJS(napi_env e, napi_callback_info i) : BaseObject<ToneMapJS>(e, i)
{
    LOG_F("ToneMapJS ++");
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        // no arguments. so internal create.
        // expecting caller to finish
        return;
    }
    // postprocess that we bind to..
    NapiApi::Object postProcJS = fromJs.Arg<0>();
    auto postproc = GetNativeMeta<SCENE_NS::IPostProcess>(postProcJS);
    NapiApi::Object toneMapArgs = fromJs.Arg<1>();
    // now, based on parameters, initialize the object
    // so it is a tonemap
    float exposure = toneMapArgs.Get<float>("exposure").valueOrDefault(0.7);
    SCENE_NS::ITonemap::TonemapType type =
        ConvertTo(toneMapArgs.Get<uint32_t>("type").valueOrDefault(ToneMappingType::ACES));

    auto tonemap = GetNativeObjectParam<SCENE_NS::ITonemap>(toneMapArgs);

    ExecSyncTask([&tonemap, exposure, type, postproc]() -> META_NS::IAny::Ptr {
        if (!tonemap) {
            tonemap = META_NS::GetObjectRegistry().Create<SCENE_NS::ITonemap>(SCENE_NS::ClassId::Tonemap);
        }
        tonemap->Type()->SetValue(type);
        tonemap->Exposure()->SetValue(exposure);
        tonemap->Enabled()->SetValue(true);
        postproc->Tonemap()->SetValue(tonemap);
        return {};
    });
    auto obj = interface_pointer_cast<META_NS::IObject>(tonemap);
    // process constructor args..
    NapiApi::Object meJs(e, fromJs.This());
    // weak ref, due to the ToneMap class being owned by the postprocess.
    SetNativeObject(obj, false);
    StoreJsObj(obj, meJs);
}

ToneMapJS::~ToneMapJS()
{
    LOG_F("ToneMapJS --");
    DisposeNative();
    if (!GetNativeObject()) {
        return;
    }
}

napi_value ToneMapJS::GetType(NapiApi::FunctionContext<>& ctx)
{
    SCENE_NS::ITonemap::TonemapType type = SCENE_NS::ITonemap::TonemapType::ACES; // default
    if (auto toneMap = interface_cast<SCENE_NS::ITonemap>(GetNativeObject())) {
        ExecSyncTask([toneMap, &type]() {
            type = toneMap->Type()->GetValue();
            return META_NS::IAny::Ptr {};
        });
    }

    auto typeI = ConvertFrom(type);
    napi_value value;
    napi_status status = napi_create_uint32(ctx, static_cast<uint32_t>(typeI), &value);
    return value;
}
void ToneMapJS::SetType(NapiApi::FunctionContext<uint32_t>& ctx)
{
    auto type = ConvertTo(ctx.Arg<0>());

    if (auto toneMap = interface_cast<SCENE_NS::ITonemap>(GetNativeObject())) {
        ExecSyncTask([toneMap, type]() {
            toneMap->Type()->SetValue(type);
            return META_NS::IAny::Ptr {};
        });
    }
}

napi_value ToneMapJS::GetExposure(NapiApi::FunctionContext<>& ctx)
{
    float exp = 0.0;
    if (auto toneMap = interface_cast<SCENE_NS::ITonemap>(GetNativeObject())) {
        ExecSyncTask([toneMap, &exp]() {
            exp = toneMap->Exposure()->GetValue();
            return META_NS::IAny::Ptr {};
        });
    }

    napi_value value;
    napi_status status = napi_create_double(ctx, exp, &value);
    return value;
}

void ToneMapJS::SetExposure(NapiApi::FunctionContext<float>& ctx)
{
    float exp = ctx.Arg<0>();
    if (auto toneMap = interface_cast<SCENE_NS::ITonemap>(GetNativeObject())) {
        ExecSyncTask([toneMap, exp]() {
            toneMap->Exposure()->SetValue(exp);
            return META_NS::IAny::Ptr {};
        });
    }
}
