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

#include "PostProcJS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/property_events.h>
#include <scene_plugin/api/camera.h> //for the classid...
#include <scene_plugin/api/node_uid.h>
#include <scene_plugin/interface/intf_ecs_scene.h>
#include <scene_plugin/interface/intf_node.h>
#include <scene_plugin/interface/intf_scene.h>

#include <render/intf_render_context.h>

#include "CameraJS.h"
using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
using namespace SCENE_NS;

void PostProcJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> node_props;
    // clang-format off

    node_props.push_back(GetSetProperty<bool, PostProcJS, &PostProcJS::GetBloom, &PostProcJS::SetBloom>("bloom"));
    node_props.emplace_back(GetSetProperty<Object, PostProcJS, &PostProcJS::GetToneMapping,
        &PostProcJS::SetToneMapping>("toneMapping"));
    node_props.push_back(MakeTROMethod<NapiApi::FunctionContext<>, PostProcJS, &PostProcJS::Dispose>("destroy"));

    // clang-format on

    napi_value func;
    auto status = napi_define_class(env, "PostProcessSettings", NAPI_AUTO_LENGTH, BaseObject::ctor<PostProcJS>(),
        nullptr, node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    napi_get_instance_data(env, (void**)&mis);
    mis->StoreCtor("PostProcessSettings", func);
}

napi_value PostProcJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    LOG_F("PostProcJS::Dispose");
    DisposeNative();
    return {};
}
void PostProcJS::DisposeNative()
{
    if (!disposed_) {
        disposed_ = true;
        LOG_F("PostProcJS::DisposeNative");
        // make sure we release toneMap settings

        auto tmjs = toneMap_.GetObject();
        if (tmjs) {
            NapiApi::Function func = tmjs.Get<NapiApi::Function>("destroy");
            if (func) {
                func.Invoke(tmjs);
            }
        }
        toneMap_.Reset();

        if (auto post = interface_pointer_cast<IPostProcess>(GetNativeObject())) {
            // reset the native object refs
            SetNativeObject(nullptr, false);
            SetNativeObject(nullptr, true);

            auto cameraJS = camera_.GetObject();
            if (cameraJS) {
                auto* rootobject = cameraJS.Native<TrueRootObject>();
                CameraJS* cam = (CameraJS*)(rootobject);

                ExecSyncTask([cam, post = BASE_NS::move(post)]() {
                    cam->ReleaseObject(META_NS::interface_pointer_cast<META_NS::IObject>(post));
                    post->Tonemap()->SetValue(nullptr);
                    return META_NS::IAny::Ptr {};
                });
            }
        }
    }
}
void* PostProcJS::GetInstanceImpl(uint32_t id)
{
    if (id == PostProcJS::ID) {
        return this;
    }
    return nullptr;
}
void PostProcJS::Finalize(napi_env env)
{
    // need to do something BEFORE the object gets deleted..
    DisposeNative();
    BaseObject<PostProcJS>::Finalize(env);
}

PostProcJS::PostProcJS(napi_env e, napi_callback_info i) : BaseObject<PostProcJS>(e, i)
{
    LOG_F("PostProcJS ++");
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        // no arguments. so internal create.
        // expecting caller to finish
        return;
    }

    // camera that we bind to..
    NapiApi::Object cameraJS = fromJs.Arg<0>();
    camera_ = { cameraJS };
    auto* rootobject = cameraJS.Native<TrueRootObject>();
    auto postproc = interface_pointer_cast<SCENE_NS::IPostProcess>(
        ((CameraJS*)(rootobject))->CreateObject(SCENE_NS::ClassId::PostProcess));

    // create a postprocess object owned by CameraJS.

    // process constructor args..
    NapiApi::Object meJs(e, fromJs.This());
    // weak ref, as we expect to be owned by the camera.
    SetNativeObject(interface_pointer_cast<META_NS::IObject>(postproc), false);
    StoreJsObj(interface_pointer_cast<META_NS::IObject>(postproc), meJs);
    // now, based on parameters, create correct objects.
    if (NapiApi::Object args = fromJs.Arg<1>()) {
        if (auto prm = args.Get("toneMapping")) {
            // enable tonemap.
            napi_value args[] = {
                meJs, // postprocess
                prm   // tonemap settings
            };
            SCENE_NS::ITonemap::Ptr tone;
            ExecSyncTask([postproc, &tone]() {
                tone = postproc->Tonemap()->GetValue();
                return META_NS::IAny::Ptr {};
            });
            MakeNativeObjectParam(e, tone, BASE_NS::countof(args), args);
            NapiApi::Object tonemapJS(GetJSConstructor(e, "ToneMappingSettings"), BASE_NS::countof(args), args);
            meJs.Set("toneMapping", tonemapJS);
        }
        if (auto prm = args.Get("bloom")) {
            ExecSyncTask([postproc]() -> META_NS::IAny::Ptr {
                SCENE_NS::IBloom::Ptr bloom = postproc->Bloom()->GetValue();
                bloom->Enabled()->SetValue(true);
                return {};
            });
        }
    }
}

PostProcJS::~PostProcJS()
{
    LOG_F("PostProcJS --");
    DisposeNative();
    if (!GetNativeObject()) {
        return;
    }
}

void PostProcJS::SetToneMapping(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto postproc = interface_cast<SCENE_NS::IPostProcess>(GetNativeObject());
    if (!postproc) {
        // not possible.
        return;
    }
    NapiApi::Object tonemapJS = ctx.Arg<0>();

    if (auto currentlySet = toneMap_.GetObject()) {
        if ((napi_value)currentlySet == (napi_value)tonemapJS) {
            // setting the exactly the same tonemap setting. do nothing.
            return;
        }
        // dispose the old bound object..
        NapiApi::Function func = currentlySet.Get<NapiApi::Function>("destroy");
        if (func) {
            func.Invoke(currentlySet);
        }
        toneMap_.Reset();
    }

    TrueRootObject* native { nullptr };
    SCENE_NS::ITonemap::Ptr tonemap;
    // does the input parameter already have a bridge..
    native = tonemapJS.Native<TrueRootObject>();
    if (!native) {
        // nope.. so create a new bridge object based on the input.
        napi_value args[] = {
            ctx.This(), // postproc..
            ctx.Arg<0>() // "javascript object for values"
        };
        NapiApi::Object res(GetJSConstructor(ctx, "ToneMappingSettings"), BASE_NS::countof(args), args);
        native = res.Native<TrueRootObject>();
        tonemapJS = res;
    } else {
        tonemap = interface_pointer_cast<SCENE_NS::ITonemap>(native->GetNativeObject());
        ExecSyncTask([postproc, tonemap]() {
            postproc->Tonemap()->SetValue(tonemap);
            return META_NS::IAny::Ptr {};
        });
    }
    toneMap_ = { ctx, tonemapJS }; // take ownership of the object.
}

napi_value PostProcJS::GetToneMapping(NapiApi::FunctionContext<>& ctx)
{
    if (auto postproc = interface_cast<SCENE_NS::IPostProcess>(GetNativeObject())) {
        SCENE_NS::ITonemap::Ptr tone;
        ExecSyncTask([postproc, &tone]() {
            tone = postproc->Tonemap()->GetValue();
            return META_NS::IAny::Ptr {};
        });
        auto obj = interface_pointer_cast<META_NS::IObject>(tone);

        if (auto cached = FetchJsObj(obj)) {
            // always return the same js object.
            return cached;
        }

        napi_value args[] = {
            ctx.This() // postproc..
        };
        MakeNativeObjectParam(ctx, tone, BASE_NS::countof(args), args);
        napi_value tonemapJS = CreateFromNativeInstance(ctx, obj, false, BASE_NS::countof(args), args);
        toneMap_ = { ctx, tonemapJS }; // take ownership of the object.
        return tonemapJS;
    }
    toneMap_.Reset();
    return ctx.GetUndefined();
}

napi_value PostProcJS::GetBloom(NapiApi::FunctionContext<>& ctx)
{
    bool enabled = false;
    if (auto postproc = interface_pointer_cast<SCENE_NS::IPostProcess>(GetNativeObject())) {
        ExecSyncTask([postproc, &enabled]() {
            SCENE_NS::IBloom::Ptr bloom = postproc->Bloom()->GetValue();
            enabled = bloom->Enabled()->GetValue();
            return META_NS::IAny::Ptr {};
        });
    }

    napi_value value;
    napi_status status = napi_get_boolean(ctx, enabled, &value);
    return value;
}

void PostProcJS::SetBloom(NapiApi::FunctionContext<bool>& ctx)
{
    bool enable = ctx.Arg<0>();
    if (auto postproc = interface_pointer_cast<SCENE_NS::IPostProcess>(GetNativeObject())) {
        ExecSyncTask([postproc, enable]() {
            SCENE_NS::IBloom::Ptr bloom = postproc->Bloom()->GetValue();
            bloom->Enabled()->SetValue(enable);
            return META_NS::IAny::Ptr {};
        });
    }
}
