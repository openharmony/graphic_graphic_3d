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

#ifndef OHOS_RENDER_3D_ANIMATION_JS_H
#define OHOS_RENDER_3D_ANIMATION_JS_H

#include "BaseObjectJS.h"
#include "SceneResourceImpl.h"

class ThreadSafeCallback {
public:
    // can be called from any thread.
    void Release()
    {
        napi_status status;
        if (termfun) {
            status =
                napi_release_threadsafe_function(termfun, napi_threadsafe_function_release_mode::napi_tsfn_release);
            termfun = nullptr;
        }
    }
    void Trigger()
    {
        napi_status status;
        if (termfun) {
            status =
                napi_call_threadsafe_function(termfun, nullptr, napi_threadsafe_function_call_mode::napi_tsfn_blocking);
        }
    }

protected:
    virtual void Finalize(napi_env) = 0;
    virtual void Invoked(napi_env) = 0;
    ThreadSafeCallback(napi_env env, const char* n)
    {
        napi_status status;
        napi_value name;
        napi_create_string_latin1(env, n, NAPI_AUTO_LENGTH, &name);
        status = napi_create_threadsafe_function(env, nullptr, nullptr, name, 1, 1, this, &ThreadSafeCallback::Final,
            this, &ThreadSafeCallback::Call, &termfun);
    }
    static void Call(napi_env env, napi_value js_callback, void* context, void* inData)
    {
        ThreadSafeCallback* data = (ThreadSafeCallback*)context;
        data->Invoked(env);
    }
    static void Final(napi_env env, void* finalize_data, void* finalize_hint)
    {
        ThreadSafeCallback* data = (ThreadSafeCallback*)finalize_data;
        data->Finalize(env);
        delete data;
    };
    virtual ~ThreadSafeCallback()
    {
        // Called by final on JS thread.
        Release();
    }
    napi_threadsafe_function termfun { nullptr };
};

class AnimationJS : public BaseObject<AnimationJS>, public SceneResourceImpl {
public:
    static constexpr uint32_t ID = 70;
    static void Init(napi_env env, napi_value exports);
    AnimationJS(napi_env, napi_callback_info);
    ~AnimationJS() override;
    virtual void* GetInstanceImpl(uint32_t id) override;

private:
    napi_value GetEnabled(NapiApi::FunctionContext<>& ctx);
    void SetEnabled(NapiApi::FunctionContext<bool>& ctx);
    napi_value GetDuration(NapiApi::FunctionContext<>& ctx);
    napi_value GetRunning(NapiApi::FunctionContext<>& ctx);
    napi_value GetProgress(NapiApi::FunctionContext<>& ctx);

    napi_value OnFinished(NapiApi::FunctionContext<NapiApi::Function>& ctx);
    napi_value OnStarted(NapiApi::FunctionContext<NapiApi::Function>& ctx);

    napi_value Pause(NapiApi::FunctionContext<>& ctx);
    napi_value Restart(NapiApi::FunctionContext<>& ctx);
    napi_value Seek(NapiApi::FunctionContext<float>& ctx);

    napi_value Start(NapiApi::FunctionContext<>& ctx);
    napi_value Stop(NapiApi::FunctionContext<>& ctx);
    napi_value Finish(NapiApi::FunctionContext<>& ctx);

    void DisposeNative() override;
    void Finalize(napi_env env) override;

    META_NS::IEvent::Token OnFinishedToken_ { 0 };
    META_NS::IEvent::Token OnStartedToken_ { 0 };

    // we don't actually own these two, as lifetime is controlled by napi_threadsafe_function
    ThreadSafeCallback* OnStartedCB_ { nullptr };
    ThreadSafeCallback* OnFinishedCB_ { nullptr };
    NapiApi::WeakRef scene_;
};
#endif // OHOS_RENDER_3D_ANIMATION_JS_H
