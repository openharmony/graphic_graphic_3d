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
#include <meta/api/util.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/property_events.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>

#include <render/intf_render_context.h>

#include "BloomJS.h"
#include "CameraJS.h"
#include "ToneMapJS.h"
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
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
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
        ptr = camera_.GetObject().GetJsWrapper<CameraJS>();
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

        // Detach this wrapper and its members tonemap and bloom from the native objects.
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
            UnsetNativeObject();
            if (const auto cam = camera_.GetObject().GetJsWrapper<CameraJS>()) {
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
    DisposeNative(camera_.GetObject().GetJsWrapper<CameraJS>());
    BaseObject::Finalize(env);
}

PostProcJS::PostProcJS(napi_env e, napi_callback_info i) : BaseObject(e, i)
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

    auto postproc = GetNativeObject<SCENE_NS::IPostProcess>();
    if (!postproc) {
        LOG_E("Error creating PostProcessSettings: No native object given");
        assert(false);
        return;
    }

    // process constructor args..
    NapiApi::Object meJs(fromJs.This());
    // now, based on parameters, create correct objects.
    if (NapiApi::Object args = fromJs.Arg<1>()) {
        if (auto prm = args.Get("toneMapping")) {
            // enable tonemap.
            napi_value innerArgs[] = { prm };
            const auto tone = postproc->Tonemap()->GetValue();
            // The tonemap is a part of the PostProcess object, so we own it. TonemapJS get only a weak ref.
            const auto tonemapJS = CreateFromNativeInstance(e, tone, PtrType::WEAK, innerArgs);
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
        if (tonemapJS.StrictEqual(currentlySet)) {
            return;
        }
        if (const auto wrapper = currentlySet.GetJsWrapper<ToneMapJS>()) {
            // Free the old tonemap so it isn't bound to the changes happening in the new one.
            wrapper->UnbindFromNative();
        }
    }
    auto target = postproc->Tonemap()->GetValue();
    if (!target) {
        return;
    }


    // TODO: If the new JS tonemap is bound to a native tonemap, we have this problem:
    // let sharedTonemap = cam1.postprocess.tonemap;
    // cam2.postprocess.tonemap = sharedTonemap;
    // sharedTonemap.exposure = 0.5;  // Should set the tonemap of both cameras but won't.
    auto source = tonemapJS.GetNative<SCENE_NS::ITonemap>();
    if (tonemapJS.IsNull()) {
        META_NS::SetValue(target->Enabled(), false);
    } else {
        const auto newType = tonemapJS.Get<uint32_t>("type").valueOrDefault(ToneMapJS::DEFAULT_TYPE);
        const auto newExposure = tonemapJS.Get<float>("exposure").valueOrDefault(ToneMapJS::DEFAULT_EXPOSURE);
        META_NS::SetValue(target->Type(), ToneMapJS::ToNativeType(newType));
        META_NS::SetValue(target->Exposure(), newExposure);
        META_NS::SetValue(target->Enabled(), true);
    }
    toneMap_ = NapiApi::StrongRef(tonemapJS);
}

napi_value PostProcJS::GetToneMapping(NapiApi::FunctionContext<>& ctx)
{
    if (auto postproc = interface_cast<SCENE_NS::IPostProcess>(GetNativeObject())) {
        SCENE_NS::ITonemap::Ptr tone = META_NS::GetValue(postproc->Tonemap());
        if ((!tone) || (!META_NS::GetValue(tone->Enabled(), false))) {
            // no tonemap object or tonemap disabled.
            return ctx.GetUndefined();
        }
        auto tonemapJS = CreateFromNativeInstance(ctx.GetEnv(), tone, PtrType::WEAK, {});
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
    auto postproc = GetNativeObject<SCENE_NS::IPostProcess>();
    if (!postproc) {
        return;
    }

    bool enabled = false;
    if (const auto bloomJs = NapiApi::Object { ctx.Arg<0>() }) {
        enabled = true;
        // is it wrapped?
        if (auto wrapper = BloomConfiguration::Unwrap(bloomJs)) {
            if (const auto newPostProc = wrapper->GetPostProc(); newPostProc != postproc) {
                // Can't use the wrapper of another postprocess object yet.
                LOG_F("Tried to attach a bloom wrapper object that was already bound to another one");
                return;
            }
            // The new bloom is wrapped, but has no native postproc (or we are already its native).
            wrapper->SetPostProc(postproc);
            bloom_ = NapiApi::StrongRef(bloomJs);
        } else {
            wrapper = new BloomConfiguration();
            wrapper->SetFrom(bloomJs);
            wrapper->SetPostProc(postproc);
            bloom_ = wrapper->Wrap(bloomJs);
        }
    } else {
        // disabling bloom.. get current, and unwrap it.
        auto oldBloom = bloom_.GetObject();
        if (auto oldWrapper = BloomConfiguration::Unwrap(oldBloom)) {
            // detaches native and javascript object.
            oldWrapper->SetPostProc(nullptr); // detach from object
        }
        // release our reference to the current bloom settings (JS)
        bloom_.Reset();
    }

    if (auto bloom = postproc->Bloom()->GetValue()) {
        bloom->Enabled()->SetValue(enabled);
        // TODO: Poke postProc so that the changes to bloom are updated.
    }
}
