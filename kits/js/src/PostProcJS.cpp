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

#include <cmath>
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
#include "JsObjectCache.h"
#include "ToneMapJS.h"
using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
using namespace SCENE_NS;

void PostProcJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> node_props;

    node_props.push_back(GetSetProperty<Object, PostProcJS, &PostProcJS::GetBloom, &PostProcJS::SetBloom>("bloom"));
    node_props.emplace_back(
        GetSetProperty<Object, PostProcJS, &PostProcJS::GetToneMapping, &PostProcJS::SetToneMapping>("toneMapping"));
    node_props.emplace_back(
        GetSetProperty<Object, PostProcJS, &PostProcJS::GetVignette, &PostProcJS::SetVignette>("vignette"));
    node_props.emplace_back(
        GetSetProperty<Object, PostProcJS, &PostProcJS::GetColorFringe, &PostProcJS::SetColorFringe>("colorFringe"));
    node_props.push_back(MakeTROMethod<NapiApi::FunctionContext<>, PostProcJS, &PostProcJS::Dispose>("destroy"));

    napi_value func;
    auto status = napi_define_class(env, "PostProcessSettings", NAPI_AUTO_LENGTH, BaseObject::ctor<PostProcJS>(),
        nullptr, node_props.size(), node_props.data(), &func);
    if (status != napi_ok) {
        LOG_E("export class failed in %s", __func__);
    }

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor("PostProcessSettings", func);
    }
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
        ptr = camera_.GetJsWrapper<CameraJS>();
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
        DisposeBridge(this);
        LOG_V("PostProcJS::DisposeNative");
        if (auto native = GetNativeObject<META_NS::IObject>()) {
            DetachJsObj(native, "_JSW");
        }
        // make sure we release toneMap settings

        // Detach this wrapper and its members tonemap and bloom from the native objects.
        if (auto tmjs = toneMap_.GetObject()) {
            tmjs.Invoke("destroy", {});
        }
        toneMap_.Reset();

        if (auto bmjs = bloom_.GetObject()) {
            bmjs.Invoke("destroy", {});
        }
        bloom_.Reset();
        if (auto post = interface_pointer_cast<IPostProcess>(GetNativeObject())) {
            UnsetNativeObject();
            if (const auto cam = camera_.GetJsWrapper<CameraJS>()) {
                ExecSyncTask([cam, post = BASE_NS::move(post)]() {
                    cam->ReleaseObject(interface_pointer_cast<META_NS::IObject>(post));
                    return META_NS::IAny::Ptr {};
                });
            }
        }
        camera_.Reset();
        vignetteProxy_.Reset();
    }
}
void* PostProcJS::GetInstanceImpl(uint32_t id)
{
    if (id == PostProcJS::ID) {
        return static_cast<PostProcJS*>(this);
    }
    return BaseObject::GetInstanceImpl(id);
}
void PostProcJS::Finalize(napi_env env)
{
    DisposeNative(camera_.GetJsWrapper<CameraJS>());
    BaseObject::Finalize(env);
    FinalizeBridge(this);
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
    AddBridge("PostProcJS", meJs);
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
    DestroyBridge(this);
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
    NapiApi::Object argObj = ctx.Arg<0>();

    if (auto currentlySet = toneMap_.GetObject()) {
        if (currentlySet.StrictEqual(argObj)) {
            return;
        }
        if (const auto wrapper = currentlySet.GetJsWrapper<ToneMapJS>()) {
            // Free the old tonemap so it isn't bound to the changes happening in the new one.
            wrapper->UnbindFromNative();
        }
    }
    auto target = postproc->Tonemap()->GetValue();
    if (!target) {
        toneMap_.Reset();
        return;
    }

    if (const auto wrapper = argObj.GetJsWrapper<ToneMapJS>()) {
        // Design shortcoming: suppose we have two cameras in ArkTS code: cam1 and cam2.
        // If we extract the tonemap: const tonemap = cam1.toneMapping; and assign it also to cam2,
        // then tonemap.exposure = 2; affects only cam2, as we bind it to cam2's native on the following line.
        // We could copy ObjectProxy usage from vignette and color fringe to mitigate (but not solve) this.
        wrapper->BindToNative(target);
        META_NS::SetValue(target->Enabled(), true);
        toneMap_ = NapiApi::StrongRef(argObj);
    } else if (argObj) {
        // The arg is a raw JS object.
        napi_value args[] = { argObj.ToNapiValue() };
        auto tonemapJS = CreateFromNativeInstance(ctx.GetEnv(), target, PtrType::WEAK, args);
        // ToneMapJS constructor sets native properties from the given args only once.
        // After first creation, the same cached wrapper is used, so the constructor isn't being run.
        tonemapJS.Set<uint32_t>("type", argObj.Get<uint32_t>("type"));
        tonemapJS.Set<float>("exposure", argObj.Get<float>("exposure"));
        // If using a cached wrapper, we have just unbound it above. Rebind to sync changes.
        auto jsWrapper = tonemapJS.GetJsWrapper<ToneMapJS>();
        if (!jsWrapper) {
            return;
        }
        jsWrapper->BindToNative(target);
        META_NS::SetValue(target->Enabled(), true);
        toneMap_ = NapiApi::StrongRef(tonemapJS);
    } else {
        // The arg is undefined.
        META_NS::SetValue(target->Enabled(), false);
        toneMap_.Reset();
    }
}

napi_value PostProcJS::GetToneMapping(NapiApi::FunctionContext<>& ctx)
{
    if (auto postproc = interface_cast<SCENE_NS::IPostProcess>(GetNativeObject())) {
        SCENE_NS::ITonemap::Ptr tone = META_NS::GetValue(postproc->Tonemap());
        if ((!tone) || (!META_NS::GetValue(tone->Enabled(), false))) {
            // no tonemap object or tonemap disabled.
            return ctx.GetUndefined();
        }
        if (const auto toneMap = toneMap_.GetObject()) {
            return toneMap.ToNapiValue();
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
        // Poke postProc so that the changes to bloom are updated.
    }
}

napi_value PostProcJS::GetVignette(NapiApi::FunctionContext<>& ctx)
{
    auto vignette = SCENE_NS::IVignette::Ptr {};
    if (const auto postProcess = ctx.This().GetNative<SCENE_NS::IPostProcess>()) {
        vignette = postProcess->Vignette()->GetValue();
    }
    if (!vignette || !vignette->Enabled()->GetValue()) {
        return ctx.GetUndefined();
    }

    if (!vignetteProxy_.GetObject()) {
        const auto env = ctx.GetEnv();
        SetupVignetteProxy(env, vignette);
    }
    return vignetteProxy_.GetObject().ToNapiValue();
}

void PostProcJS::SetVignette(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto vignette = SCENE_NS::IVignette::Ptr {};
    if (const auto postProcess = ctx.This().GetNative<SCENE_NS::IPostProcess>()) {
        vignette = postProcess->Vignette()->GetValue();
    }
    if (!vignette) {
        LOG_E("Unable to set new vignette: Native vignette missing");
        assert(false);
        return;
    }

    if (auto newVignette = NapiApi::Object { ctx.Arg<0>() }) {
        if (!vignetteProxy_.GetObject()) {
            SetupVignetteProxy(ctx.GetEnv(), vignette);
        }
        vignetteProxy_.ReplaceObject(newVignette);
    } else {
        // Setting to undefined means to not disable, but rather use defaults.
        vignetteProxy_.Reset();
        vignette->Power()->Reset();
        vignette->Coefficient()->Reset();
    }
    // Due to backwards-compatibility requirements in the TS user-facing side, the default is to enable.
    vignette->Enabled()->SetValue(true);
}

void PostProcJS::SetupVignetteProxy(napi_env env, SCENE_NS::IVignette::Ptr vignette)
{
#define VIGNETTE_LOG_OFFSET (5.562684646268003e-309)
    // -log2(VIGNETTE_LOG_OFFSET) is roughly 1024.
    const auto roundnessToNative = [vignette](napi_env env, napi_value jsVal) {
        if (auto value = NapiApi::Value<double>(env, jsVal); value.IsValid()) {
            constexpr auto offset = VIGNETTE_LOG_OFFSET;
            auto nativeValue = BASE_NS::Math::max(double { value }, 0.0);
            nativeValue = BASE_NS::Math::clamp(-std::log2(nativeValue + offset), 0.0, 1024.0);
            jsVal = NapiApi::Env { env }.GetNumber(nativeValue);
        } else if (value.IsUndefined()) {
            vignette->Coefficient()->Reset();
        }
        return jsVal;
    };
    const auto roundnessToJs = [](napi_env env, napi_value jsVal) {
        if (auto value = NapiApi::Value<double>(env, jsVal); value.IsValid()) {
            constexpr auto offset = VIGNETTE_LOG_OFFSET;
            auto nativeValue = double { value };
            nativeValue = BASE_NS::Math::clamp01(BASE_NS::Math::pow(2, -nativeValue) - offset);
            jsVal = NapiApi::Env { env }.GetNumber(nativeValue);
        }
        return jsVal;
    };

    const auto intensityToNative = [vignette](napi_env env, napi_value jsVal) {
        if (auto value = NapiApi::Value<double>(env, jsVal); value.IsUndefined()) {
            vignette->Power()->Reset();
        }
        return jsVal;
    };
#undef VIGNETTE_LOG_OFFSET
    vignetteProxy_.Init(env, {
                                 { "roundness", vignette->Coefficient(), roundnessToNative, roundnessToJs },
                                 { "intensity", vignette->Power(), intensityToNative },
                             });
}

napi_value PostProcJS::GetColorFringe(NapiApi::FunctionContext<>& ctx)
{
    auto colorFringe = SCENE_NS::IColorFringe::Ptr {};
    if (const auto postProcess = ctx.This().GetNative<SCENE_NS::IPostProcess>()) {
        colorFringe = postProcess->ColorFringe()->GetValue();
    }
    if (!colorFringe || !colorFringe->Enabled()->GetValue()) {
        return ctx.GetUndefined();
    }

    if (!colorFringeProxy_.GetObject()) {
        const auto env = ctx.GetEnv();
        SetupColorFringeProxy(env, colorFringe);
    }
    return colorFringeProxy_.GetObject().ToNapiValue();
}

void PostProcJS::SetColorFringe(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto colorFringe = SCENE_NS::IColorFringe::Ptr {};
    if (const auto postProcess = ctx.This().GetNative<SCENE_NS::IPostProcess>()) {
        colorFringe = postProcess->ColorFringe()->GetValue();
    }
    if (!colorFringe) {
        LOG_E("Unable to set new color fringe: Native color fringe missing");
        assert(false);
        return;
    }

    if (auto newColorFringe = NapiApi::Object { ctx.Arg<0>() }) {
        if (!colorFringeProxy_.GetObject()) {
            SetupColorFringeProxy(ctx.GetEnv(), colorFringe);
        }
        colorFringeProxy_.ReplaceObject(newColorFringe);
        colorFringe->Enabled()->SetValue(true);
    } else {
        colorFringeProxy_.Reset();
        colorFringe->DistanceCoefficient()->Reset();
        colorFringe->Enabled()->Reset();
    }
}

void PostProcJS::SetupColorFringeProxy(napi_env env, SCENE_NS::IColorFringe::Ptr colorFringe)
{
#define COLOR_FRINGE_FACTOR (10)
    const auto scaleToNative = [colorFringe](napi_env env, napi_value jsVal) {
        if (auto value = NapiApi::Value<double>(env, jsVal); value.IsValid()) {
            auto nativeValue = double { value } * COLOR_FRINGE_FACTOR;
            jsVal = NapiApi::Env { env }.GetNumber(nativeValue);
        } else if (value.IsUndefined()) {
            colorFringe->DistanceCoefficient()->Reset();
        }
        return jsVal;
    };
    const auto scaleToJs = [](napi_env env, napi_value jsVal) {
        if (auto value = NapiApi::Value<double>(env, jsVal); value.IsValid()) {
            auto nativeValue = double { value } / COLOR_FRINGE_FACTOR;
            jsVal = NapiApi::Env { env }.GetNumber(nativeValue);
        }
        return jsVal;
    };
#undef COLOR_FRINGE_FACTOR
    colorFringeProxy_.Init(env, { { "intensity", colorFringe->DistanceCoefficient(), scaleToNative, scaleToJs } });
}
