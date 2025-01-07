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
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/intf_attach.h>
#include <scene/interface/intf_scene.h>

#include "SceneJS.h"

class OnCallJS : public ThreadSafeCallback {
    NapiApi::StrongRef jsThis_;
    NapiApi::StrongRef ref_;

public:
    OnCallJS(const char* name, NapiApi::Object jsThis, NapiApi::Function toCall)
        : ThreadSafeCallback(toCall.GetEnv(), name), jsThis_(jsThis), ref_(toCall.GetEnv(), toCall)
    {}
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
    DeclareGetSet(float, "speed", GetSpeed, SetSpeed);
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
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    NapiApi::Object meJs(fromJs.This());
    NapiApi::Object scene = fromJs.Arg<0>(); // access to owning scene...
    NapiApi::Object args = fromJs.Arg<1>();  // access to params.
    scene_ = { scene };
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        LOG_F("INVALID SCENE!");
    }

    auto* tro = scene.Native<TrueRootObject>();
    if (tro) {
        auto* sceneJS = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
        if (sceneJS) {
            sceneJS->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
        }
    }

    if (args) {
        using namespace META_NS;
        auto obj = GetNativeObjectParam<IObject>(args);
        if (auto a = interface_cast<IAnimation>(obj)) {
            // linking to an existing object.
            SetNativeObject(obj, false);
            StoreJsObj(obj, meJs);

            // check if there is a speed controller already.(and use that)
            auto attachments = interface_cast<META_NS::IAttach>(a)->GetAttachments();
            for (auto at : attachments) {
                if (interface_cast<AnimationModifiers::ISpeed>(at)) {
                    // yes.. (expect at most one)
                    speedModifier_ = interface_pointer_cast<AnimationModifiers::ISpeed>(at);
                    break;
                }
            }
        }
    }
}
void* AnimationJS::GetInstanceImpl(uint32_t id)
{
    if (id == AnimationJS::ID)
        return this;
    return SceneResourceImpl::GetInstanceImpl(id);
}

void AnimationJS::Finalize(napi_env env)
{
    DisposeNative(nullptr);
    BaseObject<AnimationJS>::Finalize(env);
}
AnimationJS::~AnimationJS()
{
    LOG_V("AnimationJS -- ");
    DisposeNative(nullptr);
}

void AnimationJS::DisposeNative(void*)
{
    // do nothing for now..
    if (!disposed_) {
        disposed_ = true;

        LOG_V("AnimationJS::DisposeNative");
        NapiApi::Object obj = scene_.GetObject();
        auto* tro = obj.Native<TrueRootObject>();
        if (tro) {
            SceneJS* sceneJS = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
            if (sceneJS) {
                sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
            }
        }
        scene_.Reset();

        // make sure we release postProc settings
        if (auto anim = interface_pointer_cast<META_NS::IAnimation>(GetNativeObject())) {
            // reset the native object refs
            SetNativeObject(nullptr, false);
            SetNativeObject(nullptr, true);
            // remove listeners.
            if (OnStartedToken_) {
                anim->OnStarted()->RemoveHandler(OnStartedToken_);
            }
            if (OnFinishedToken_) {
                anim->OnFinished()->RemoveHandler(OnFinishedToken_);
            }
            if (speedModifier_) {
                auto attach = interface_cast<META_NS::IAttach>(anim);
                if (attach) {
                    attach->Detach(speedModifier_);
                }
                speedModifier_.reset();
            }
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
napi_value AnimationJS::GetSpeed(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    float speed = 1.0;
    if (speedModifier_) {
        speed = speedModifier_->SpeedFactor()->GetValue();
    }

    return ctx.GetNumber(speed);
}

void AnimationJS::SetSpeed(NapiApi::FunctionContext<float>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    float speed = ctx.Arg<0>();
    if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
        using namespace META_NS;
        if (!speedModifier_) {
            speedModifier_ =
                GetObjectRegistry().Create<AnimationModifiers::ISpeed>(ClassId::SpeedAnimationModifier);
            interface_cast<IAttach>(a)->Attach(speedModifier_);
        }
        speedModifier_->SpeedFactor()->SetValue(speed);
    }
}

napi_value AnimationJS::GetEnabled(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    bool enabled { false };
    if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
        enabled = a->Enabled()->GetValue();
    }
    return ctx.GetBoolean(enabled);
}
void AnimationJS::SetEnabled(NapiApi::FunctionContext<bool>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    bool enabled = ctx.Arg<0>();
    if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
        a->Enabled()->SetValue(enabled);
    }
}
napi_value AnimationJS::GetDuration(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    float duration = 0.0;
    if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
        duration = a->TotalDuration()->GetValue().ToSecondsFloat();
    }

    return ctx.GetNumber(duration);
}

napi_value AnimationJS::GetRunning(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    bool running { false };
    if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
        running = a->Running()->GetValue();
    }
    return ctx.GetBoolean(running);
}
napi_value AnimationJS::GetProgress(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    float progress = 0.0;
    if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
        progress = a->Progress()->GetValue();
    }
    return ctx.GetNumber(progress);
}

napi_value AnimationJS::OnFinished(NapiApi::FunctionContext<NapiApi::Function>& ctx)
{
    auto func = ctx.Arg<0>();
    // do we have existing callback?
    if (OnFinishedCB_) {
        // stop listening ...
        if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
            a->OnFinished()->RemoveHandler(OnFinishedToken_);
            OnFinishedToken_ = 0;
        }
        // ... and release it
        OnFinishedCB_->Release();
    }
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    // do we have a new callback?
    if (func.IsDefinedAndNotNull()) {
        // create handler...
        OnFinishedCB_ = new OnCallJS("OnFinished", ctx.This(), func);
        // ... and start listening
        if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
            OnFinishedToken_ = a->OnFinished()->AddHandler(
                META_NS::MakeCallback<META_NS::IOnChanged>(OnFinishedCB_, &OnCallJS::Trigger));
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
            a->OnStarted()->RemoveHandler(OnStartedToken_);
            OnStartedToken_ = 0;
        }
        // ... and release it
        OnStartedCB_->Release();
    }
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    // do we have a new callback?
    if (func.IsDefinedAndNotNull()) {
        // create handler...
        OnStartedCB_ = new OnCallJS("OnStart", ctx.This(), func);
        // ... and start listening
        if (auto a = interface_cast<META_NS::IAnimation>(GetNativeObject())) {
            OnStartedToken_ = a->OnStarted()->AddHandler(
                META_NS::MakeCallback<META_NS::IOnChanged>(OnStartedCB_, &OnCallJS::Trigger));
        }
    }
    return ctx.GetUndefined();
}

napi_value AnimationJS::Pause(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    if (auto a = interface_cast<META_NS::IStartableAnimation>(GetNativeObject())) {
        a->Pause();
    }
    return ctx.GetUndefined();
}
napi_value AnimationJS::Restart(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    if (auto a = interface_cast<META_NS::IStartableAnimation>(GetNativeObject())) {
        a->Restart();
    }
    return ctx.GetUndefined();
}
napi_value AnimationJS::Seek(NapiApi::FunctionContext<float>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    float pos = ctx.Arg<0>();
    if (auto a = interface_cast<META_NS::IStartableAnimation>(GetNativeObject())) {
        a->Seek(pos);
    }
    return ctx.GetUndefined();
}
napi_value AnimationJS::Start(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    if (auto a = interface_cast<META_NS::IStartableAnimation>(GetNativeObject())) {
        a->Start();
    }
    return ctx.GetUndefined();
}

napi_value AnimationJS::Stop(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    if (auto a = interface_cast<META_NS::IStartableAnimation>(GetNativeObject())) {
        a->Stop();
    }
    return ctx.GetUndefined();
}
napi_value AnimationJS::Finish(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    if (auto a = interface_cast<META_NS::IStartableAnimation>(GetNativeObject())) {
        a->Finish();
    }
    return ctx.GetUndefined();
}
