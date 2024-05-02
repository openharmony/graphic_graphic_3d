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
#include "AnimationJS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/animation/intf_animation.h>
#include <scene_plugin/interface/intf_scene.h>
#include "SceneJS.h"

class OnCallJS : public ThreadSafeCallback {
    NapiApi::StrongRef jsThis_;
    NapiApi::StrongRef ref_;

public:
    OnCallJS(const char* name, napi_value jsThis, NapiApi::Function toCall) : ThreadSafeCallback(toCall.GetEnv(), name)
    {
        jsThis_ = { toCall.GetEnv(), jsThis };
        ref_ = { toCall.GetEnv(), toCall };
    }
    ~OnCallJS()
    {
        jsThis_.Reset();
        ref_.Reset();
        Release();
    }
    void Finalize(napi_env env)
    {
        jsThis_.Reset();
        ref_.Reset();
    }
    void Invoked(napi_env env)
    {
        napi_value res;
        napi_call_function(env, jsThis_.GetValue(), ref_.GetValue(), 0, nullptr, &res);
    }
};

void AnimationJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;
    SceneResourceImpl::GetPropertyDescs(node_props);
// Try out the helper macros.
// Declare NAPI_API_JS_NAME to simplify the registering.
#define NAPI_API_JS_NAME Animation

    DeclareGetSet(bool, "enabled", GetEnabled, SetEnabled);
    DeclareGet(float, "duration", GetDuration);
    DeclareGet(bool, "running", GetRunning);
    DeclareGet(float, "progress", GetProgress);
    DeclareMethod("pause", Pause);
    DeclareMethod("restart", Restart);
    DeclareMethod("seek", Seek, float);
    DeclareMethod("start", Start);
    DeclareMethod("stop", Stop);
    DeclareMethod("finish", Finish);
    DeclareMethod("onFinished", OnFinished, NapiApi::Function);
    DeclareMethod("onStarted", OnStarted, NapiApi::Function);
    DeclareClass();
#undef NAPI_API_JS_NAME
}

AnimationJS::AnimationJS(napi_env e, napi_callback_info i)
    : BaseObject<AnimationJS>(e, i), SceneResourceImpl(SceneResourceImpl::ANIMATION)
{
    NapiApi::FunctionContext<NapiApi::Object> fromJs(e, i);
    NapiApi::Object meJs(e, fromJs.This());
    NapiApi::Object scene = fromJs.Arg<0>(); // access to owning scene...
    scene_ = { scene };
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        CORE_LOG_F("INVALID SCENE!");
    }

    auto* tro = scene.Native<TrueRootObject>();
    auto* sceneJS = ((SceneJS*)tro->GetInstanceImpl(SceneJS::ID));
    sceneJS->DisposeHook((uintptr_t)&scene_, meJs);
}
void* AnimationJS::GetInstanceImpl(uint32_t id)
{
    if (id == AnimationJS::ID) {
        return this;
    }
    return SceneResourceImpl::GetInstanceImpl(id);
}

void AnimationJS::Finalize(napi_env env)
{
    DisposeNative();
    BaseObject<AnimationJS>::Finalize(env);
}
AnimationJS::~AnimationJS()
{
    LOG_F("AnimationJS -- ");
    DisposeNative();
}

void AnimationJS::DisposeNative()
{
    // do nothing for now..
    if (!disposed_) {
        disposed_ = true;

        LOG_F("AnimationJS::DisposeNative");
        NapiApi::Object obj = scene_.GetObject();
        auto* tro = obj.Native<TrueRootObject>();
        SceneJS* sceneJS;
        if (tro) {
            sceneJS = ((SceneJS*)tro->GetInstanceImpl(SceneJS::ID));
            sceneJS->ReleaseDispose((uintptr_t)&scene_);
        }
        scene_.Reset();

        // make sure we release postProc settings
        if (auto animation = interface_pointer_cast<META_NS::IAnimation>(GetNativeObject())) {
            // reset the native object refs
            SetNativeObject(nullptr, false);
            SetNativeObject(nullptr, true);
            ExecSyncTask([this, anim = BASE_NS::move(animation)]() -> META_NS::IAny::Ptr {
                // remove listeners.
                if (OnStartedToken_) {
                    anim->OnStarted()->RemoveHandler(OnStartedToken_);
                }
                if (OnFinishedToken_) {
                    anim->OnFinished()->RemoveHandler(OnFinishedToken_);
                }
                return {};
            });
            if (OnStartedCB_) {
                // does a delayed delete
                OnStartedCB_->Release();
                OnStartedCB_ = nullptr;
            }
            if (OnFinishedCB_) {
                // does a delayed delete
                OnFinishedCB_->Release();
                OnFinishedCB_ = nullptr;
            }
        }
    }
}

napi_value AnimationJS::GetEnabled(NapiApi::FunctionContext<>& ctx)
{
    bool enabled { false };
    if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
        ExecSyncTask([a, &enabled]() {
            if (a) {
                enabled = a->Enabled()->GetValue();
            }
            return META_NS::IAny::Ptr {};
        });
    }

    return ctx.GetBoolean(enabled);
}
void AnimationJS::SetEnabled(NapiApi::FunctionContext<bool>& ctx)
{
    bool enabled = ctx.Arg<0>();
    if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
        ExecSyncTask([a, enabled]() {
            a->Enabled()->SetValue(enabled);
            return META_NS::IAny::Ptr {};
        });
    }
}
napi_value AnimationJS::GetDuration(NapiApi::FunctionContext<>& ctx)
{
    float duration = 0.0;
    if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
        ExecSyncTask([a, &duration]() {
            if (a) {
                duration = a->TotalDuration()->GetValue().ToSecondsFloat();
            }
            return META_NS::IAny::Ptr {};
        });
    }

    return NapiApi::Value<float>(ctx, duration);
}

napi_value AnimationJS::GetRunning(NapiApi::FunctionContext<>& ctx)
{
    bool running { false };
    if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
        ExecSyncTask([a, &running]() {
            if (a) {
                running = a->Running()->GetValue();
            }
            return META_NS::IAny::Ptr {};
        });
    }

    return ctx.GetBoolean(running);
}
napi_value AnimationJS::GetProgress(NapiApi::FunctionContext<>& ctx)
{
    float progress = 0.0;
    if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
        ExecSyncTask([a, &progress]() {
            if (a) {
                progress = a->Progress()->GetValue();
            }
            return META_NS::IAny::Ptr {};
        });
    }

    return NapiApi::Value<float>(ctx, progress);
}

napi_value AnimationJS::OnFinished(NapiApi::FunctionContext<NapiApi::Function>& ctx)
{
    auto func = ctx.Arg<0>();
    // do we have existing callback?
    if (OnFinishedCB_) {
        // stop listening ...
        if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
            ExecSyncTask([this, a]() -> META_NS::IAny::Ptr {
                a->OnFinished()->RemoveHandler(OnFinishedToken_);
                OnFinishedToken_ = 0;
                return {};
            });
        }
        // ... and release it
        OnFinishedCB_->Release();
    }
    // do we have a new callback?
    if (func) {
        // create handler...
        OnFinishedCB_ = new OnCallJS("OnFinished", ctx.This(), func);
        // ... and start listening
        if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
            ExecSyncTask([this, a]() -> META_NS::IAny::Ptr {
                OnFinishedToken_ = a->OnFinished()->AddHandler(
                    META_NS::MakeCallback<META_NS::IOnChanged>(OnFinishedCB_, &OnCallJS::Trigger));
                return {};
            });
        }
    }
    return ctx.GetUndefined();
}

napi_value AnimationJS::OnStarted(NapiApi::FunctionContext<NapiApi::Function>& ctx)
{
    auto func = ctx.Arg<0>();
    // do we have existing callback?
    if (OnStartedCB_) {
        // stop listening ...
        if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
            ExecSyncTask([this, a]() -> META_NS::IAny::Ptr {
                a->OnStarted()->RemoveHandler(OnStartedToken_);
                OnStartedToken_ = 0;
                return {};
            });
        }
        // ... and release it
        OnStartedCB_->Release();
    }
    // do we have a new callback?
    if (func) {
        // create handler...
        OnStartedCB_ = new OnCallJS("OnStart", ctx.This(), func);
        // ... and start listening
        if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
            ExecSyncTask([this, a]() -> META_NS::IAny::Ptr {
                OnStartedToken_ = a->OnStarted()->AddHandler(
                    META_NS::MakeCallback<META_NS::IOnChanged>(OnStartedCB_, &OnCallJS::Trigger));
                return {};
            });
        }
    }
    return ctx.GetUndefined();
}

napi_value AnimationJS::Pause(NapiApi::FunctionContext<>& ctx)
{
    if (auto a = interface_cast<META_NS::IStartableAnimation>(GetNativeObject())) {
        ExecSyncTask([a]() {
            a->Pause();
            return META_NS::IAny::Ptr {};
        });
    }
    return ctx.GetUndefined();
}
napi_value AnimationJS::Restart(NapiApi::FunctionContext<>& ctx)
{
    if (auto a = interface_cast<META_NS::IStartableAnimation>(GetNativeObject())) {
        ExecSyncTask([a]() {
            a->Restart();
            return META_NS::IAny::Ptr {};
        });
    }
    return ctx.GetUndefined();
}
napi_value AnimationJS::Seek(NapiApi::FunctionContext<float>& ctx)
{
    float pos = ctx.Arg<0>();
    if (auto a = interface_cast<META_NS::IStartableAnimation>(GetNativeObject())) {
        ExecSyncTask([a, pos]() {
            a->Seek(pos);
            return META_NS::IAny::Ptr {};
        });
    }
    return ctx.GetUndefined();
}
napi_value AnimationJS::Start(NapiApi::FunctionContext<>& ctx)
{
    if (auto a = interface_cast<META_NS::IStartableAnimation>(GetNativeObject())) {
        ExecSyncTask([a]() {
            a->Start();
            return META_NS::IAny::Ptr {};
        });
    }
    return ctx.GetUndefined();
}

napi_value AnimationJS::Stop(NapiApi::FunctionContext<>& ctx)
{
    if (auto a = interface_cast<META_NS::IStartableAnimation>(GetNativeObject())) {
        ExecSyncTask([a]() {
            a->Stop();
            return META_NS::IAny::Ptr {};
        });
    }
    return ctx.GetUndefined();
}
napi_value AnimationJS::Finish(NapiApi::FunctionContext<>& ctx)
{
    if (auto a = interface_cast<META_NS::IStartableAnimation>(GetNativeObject())) {
        ExecSyncTask([a]() {
            a->Finish();
            return META_NS::IAny::Ptr {};
        });
    }
    return ctx.GetUndefined();
}