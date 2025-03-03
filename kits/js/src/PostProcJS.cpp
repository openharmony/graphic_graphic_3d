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
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>

#include <render/intf_render_context.h>

#include "BloomJS.h"
#include "CameraJS.h"
using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
using namespace SCENE_NS;

void PostProcJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> node_props;
    // clang-format off

    node_props.push_back(GetSetProperty<Object, PostProcJS, &PostProcJS::GetBloom, &PostProcJS::SetBloom>("bloom"));
    node_props.emplace_back(
        GetSetProperty<Object, PostProcJS, &PostProcJS::GetToneMapping, &PostProcJS::SetToneMapping>("toneMapping"));
    node_props.push_back(MakeTROMethod<NapiApi::FunctionContext<>, PostProcJS, &PostProcJS::Dispose>("destroy"));

    // clang-format on

    napi_value func;
    auto status = napi_define_class(env, "PostProcessSettings", NAPI_AUTO_LENGTH, BaseObject::ctor<PostProcJS>(),
        nullptr, node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    GetInstanceData(env, (void**)&mis);
    mis->StoreCtor("PostProcessSettings", func);
}

napi_value PostProcJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    LOG_V("PostProcJS::Dispose");
    // see if we have "scenejs" as ext (prefer one given as argument)
    napi_status stat;
    CameraJS* ptr { nullptr };
    NapiApi::FunctionContext<NapiApi::Object> args(ctx);
    if (args) {
        if (NapiApi::Object obj = args.Arg<0>()) {
            if (napi_value ext = obj.Get("CameraJS")) {
                stat = napi_get_value_external(ctx.GetEnv(), ext, (void**)&ptr);
            }
        }
    }
    if (!ptr) {
        NapiApi::Object obj = camera_.GetObject();
        if (obj) {
            auto* tro = obj.Native<TrueRootObject>();
            if (tro) {
                ptr = static_cast<CameraJS*>(tro->GetInstanceImpl(CameraJS::ID));
            }
        }
    }

    DisposeNative(ptr);
    return {};
}
void PostProcJS::DisposeNative(void* cam)
{
    if (cam == nullptr) {
        if (!disposed_) {
            LOG_F("PostProcJS::DisposeNative but argument NULL");
        }
        return;
    }
    if (!disposed_) {
        disposed_ = true;
        LOG_V("PostProcJS::DisposeNative");
        // make sure we release toneMap settings

        if (auto tmjs = toneMap_.GetObject()) {
            NapiApi::Function func = tmjs.Get<NapiApi::Function>("destroy");
            if (func) {
                func.Invoke(tmjs);
            }
        }
        toneMap_.Reset();

        if (auto bmjs = bloom_.GetObject()) {
            NapiApi::Function func = bmjs.Get<NapiApi::Function>("destroy");
            if (func) {
                func.Invoke(bmjs);
            }
        }
        bloom_.Reset();
        if (auto post = interface_pointer_cast<IPostProcess>(GetNativeObject())) {
            // reset the native object refs
            SetNativeObject(nullptr, false);
            SetNativeObject(nullptr, true);
            post->Tonemap()->SetValue(nullptr);
            auto cameraJS = camera_.GetObject();
            if (cameraJS) {
                auto* rootobject = cameraJS.Native<TrueRootObject>();
                CameraJS* cam = static_cast<CameraJS*>(rootobject);

                ExecSyncTask([cam, post = BASE_NS::move(post)]() {
                    cam->ReleaseObject(interface_pointer_cast<META_NS::IObject>(post));
                    return META_NS::IAny::Ptr {};
                });
            }
        }
    }
}
void* PostProcJS::GetInstanceImpl(uint32_t id)
{
    if (id == PostProcJS::ID)
        return this;
    return nullptr;
}
void PostProcJS::Finalize(napi_env env)
{
    // hmm.. do i need to do something BEFORE the object gets deleted..
    DisposeNative(nullptr);
    BaseObject<PostProcJS>::Finalize(env);
}

PostProcJS::PostProcJS(napi_env e, napi_callback_info i) : BaseObject<PostProcJS>(e, i)
{
    LOG_V("PostProcJS ++");
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
    if (rootobject == nullptr)  {
        LOG_E("rootobject is nullptr");
        return;
    }
    auto postproc = interface_pointer_cast<SCENE_NS::IPostProcess>(
        static_cast<CameraJS*>(rootobject)->CreateObject(SCENE_NS::ClassId::PostProcess));

    // create a postprocess object owned by CameraJS.

    // process constructor args..
    NapiApi::Object meJs(fromJs.This());
    // weak ref, as we expect to be owned by the camera.
    SetNativeObject(interface_pointer_cast<META_NS::IObject>(postproc), false);
    StoreJsObj(interface_pointer_cast<META_NS::IObject>(postproc), meJs);
    // now, based on parameters, create correct objects.
    if (NapiApi::Object args = fromJs.Arg<1>()) {
        if (auto prm = args.Get("toneMapping")) {
            // enable tonemap.
            napi_value innerArgs[] = {
                meJs.ToNapiValue(), // postprocess
                prm                 // tonemap settings
            };
            SCENE_NS::ITonemap::Ptr tone = postproc->Tonemap()->GetValue();
            MakeNativeObjectParam(e, tone, BASE_NS::countof(innerArgs), innerArgs);
            NapiApi::Object tonemapJS(
                GetJSConstructor(e, "ToneMappingSettings"), BASE_NS::countof(innerArgs), innerArgs);
            meJs.Set("toneMapping", tonemapJS);
        }
        if (auto prm = args.Get("bloom")) {
            meJs.Set("bloom", prm);
        }
    }
}

PostProcJS::~PostProcJS()
{
    LOG_V("PostProcJS --");
    DisposeNative(nullptr);
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
        if (tonemapJS.StrictEqual(currentlySet)) { // setting the exactly the same tonemap setting. do nothing.
            return;
        }
        // dispose the old bound object.. (actually should just convert to unbound state. and release the native object,
        // which may or may not get disposed.)
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
            ctx.This().ToNapiValue(),  // postproc..
            ctx.Arg<0>().ToNapiValue() // "javascript object for values"
        };
        NapiApi::Object res(GetJSConstructor(ctx.Env(), "ToneMappingSettings"), BASE_NS::countof(args), args);
        native = res.Native<TrueRootObject>();
        tonemapJS = res;

        // creating a new js object does have the side effect of not syncing modifications to input object to affect.
        // you always need to GET a bound one from post proc to control it..
        //
        // we COULD forcefully bind to the input object in this case.
        // but not sure what possible side effects would that have later on.
    } else {
        tonemap = interface_pointer_cast<SCENE_NS::ITonemap>(native->GetNativeObject());
        postproc->Tonemap()->SetValue(tonemap);
    }
    toneMap_ = NapiApi::StrongRef(tonemapJS); // take ownership of the object.
}

napi_value PostProcJS::GetToneMapping(NapiApi::FunctionContext<>& ctx)
{
    if (auto postproc = interface_cast<SCENE_NS::IPostProcess>(GetNativeObject())) {
        SCENE_NS::ITonemap::Ptr tone = postproc->Tonemap()->GetValue();
        if (!tone->Enabled()->GetValue()) {
            tone.reset();
        }
        if (!tone) {
            return ctx.GetUndefined();
        }
        auto obj = interface_pointer_cast<META_NS::IObject>(tone);

        if (auto cached = FetchJsObj(obj)) {
            // always return the same js object.
            return cached.ToNapiValue();
        }

        napi_value args[] = {
            ctx.This().ToNapiValue() // postproc..
        };
        MakeNativeObjectParam(ctx.GetEnv(), tone, BASE_NS::countof(args), args);
        auto tonemapJS = CreateFromNativeInstance(ctx.GetEnv(), obj, false, BASE_NS::countof(args), args);
        toneMap_ = NapiApi::StrongRef(tonemapJS); // take ownership of the object.
        return tonemapJS.ToNapiValue();
    }
    toneMap_.Reset();
    return ctx.GetUndefined();
}
napi_value PostProcJS::GetBloom(NapiApi::FunctionContext<>& ctx)
{
    if (!bloom_.IsEmpty()) {
        // okay return the existing one.
        return bloom_.GetValue();
    }

    BloomConfiguration* data = nullptr;
    if (auto postproc = interface_pointer_cast<SCENE_NS::IPostProcess>(GetNativeObject())) {
        auto bloom = postproc->Bloom()->GetValue();
        if (bloom->Enabled()->GetValue()) {
            data = new BloomConfiguration();
            data->SetFrom(bloom);
            data->SetPostProc(postproc);
        }
    }

    if (data == nullptr) {
        return ctx.GetUndefined();
    }

    NapiApi::Env env(ctx.GetEnv());
    NapiApi::Object obj(env);
    bloom_ = BASE_NS::move(data->Wrap(obj));
    return bloom_.GetValue();
}

void PostProcJS::SetBloom(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto postproc = interface_pointer_cast<SCENE_NS::IPostProcess>(GetNativeObject());
    if (!postproc) {
        return;
    }
    BloomConfiguration* settings = nullptr;
    NapiApi::Env env(ctx.GetEnv());
    NapiApi::Object inputObj = ctx.Arg<0>();
    bool enabled = false;
    if (inputObj) {
        enabled = true;
        settings = BloomConfiguration::Unwrap(inputObj);
        // is it wrapped?
        if (settings) {
            auto boundpost = settings->GetPostProc();
            if ((boundpost) && (boundpost != postproc)) {
                // silently fail, we can not use settings object from another postprocess object yet.
                LOG_F("Tried to attach a bloom setting object that was already bound to another one");
            } else {
                // has wrapper, but no native postproc. (or the bound one is the same)
                // so we can use it.
                settings->SetPostProc(postproc);
                // save the reference..
                bloom_ = BASE_NS::move(NapiApi::StrongRef(inputObj));
            }
        } else {
            settings = new BloomConfiguration();
            settings->SetFrom(inputObj);
            settings->SetPostProc(postproc);
            bloom_ = settings->Wrap(inputObj);
        }
    } else {
        // disabling bloom.. get current, and unwrap it.
        auto oldObj = bloom_.GetObject();
        settings = BloomConfiguration::Unwrap(oldObj);
        if (settings) {
            // detaches native and javascript object.
            settings->SetPostProc(nullptr); // detach from object
        }
        // release our reference to the current bloom settings (JS)
        bloom_.Reset();
        enabled = false;
    }
    if (SCENE_NS::IBloom::Ptr bloom = postproc->Bloom()->GetValue()) {
        bloom->Enabled()->SetValue(enabled);
        postproc->Bloom()->SetValue(bloom);
    }
}
