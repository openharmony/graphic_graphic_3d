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

#include "PromiseBase.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <napi_api.h>

static constexpr BASE_NS::Uid JS_RELEASE_THREAD { "3784fa96-b25b-4e9c-bbf1-e897d36f73af" };

PromiseBase::PromiseBase(napi_env env) : env_(env)
{
    CreateJsPromise();
    CreateSettleFunc();
}

PromiseBase::~PromiseBase() {}

napi_value PromiseBase::ToNapiValue() const
{
    return promise_;
}

void PromiseBase::SettleLater()
{
    auto task = META_NS::MakeCallback<META_NS::ITaskQueueTask>(BASE_NS::move([this]() {
        napi_call_threadsafe_function(
            settleLaterFunc_, nullptr, napi_threadsafe_function_call_mode::napi_tsfn_blocking);
        return false;
    }));
    // An extra task in JS_RELEASE_THREAD to trigger this is required to work around an issue
    // where SettleLater is called *in* an eventhandler.
    // There are rare cases where napi_release_threadsafe_function waits for threadsafe function completion
    // and the "JS function" is waiting for the engine thread (which is blocked releasing the function)
    META_NS::GetTaskQueueRegistry().GetTaskQueue(JS_RELEASE_THREAD)->AddTask(task);
}

void PromiseBase::SettleNow()
{
    if (SetResult()) {
        Resolve();
    } else {
        Reject();
    }
    ReleaseSettleFunc();
}

void PromiseBase::Reject()
{
    if (!result_) {
        napi_get_undefined(env_, &result_);
    }
    napi_reject_deferred(env_, deferred_, result_);
    ReleaseSettleFunc();
}

void PromiseBase::Resolve()
{
    if (!result_) {
        napi_get_undefined(env_, &result_);
    }
    napi_resolve_deferred(env_, deferred_, result_);
}

void PromiseBase::CreateJsPromise()
{
    auto status = napi_create_promise(env_, &deferred_, &promise_);
    if (status != napi_ok) {
        CORE_LOG_F("Failed to create promise");
        Abort();
    }
}

void PromiseBase::CreateSettleFunc()
{
    napi_value name;
    napi_create_string_latin1(env_, "async_promise_settle_func", NAPI_AUTO_LENGTH, &name);
    auto settleContext = this;
    auto finalizerDataArg = this;
    auto status = napi_create_threadsafe_function(
        env_, nullptr, nullptr, name, 1, 1, finalizerDataArg, FinalizeCB, settleContext, SettleCB, &settleLaterFunc_);
    if (status != napi_ok) {
        CORE_LOG_F("Failed to create a threadsafe JS function");
        Abort();
        return;
    }
    napi_ref_threadsafe_function(env_, settleLaterFunc_);
}

void PromiseBase::ReleaseSettleFunc()
{
    // The finalizer of the settle later function deletes this promise object.
    napi_unref_threadsafe_function(env_, settleLaterFunc_);
    napi_release_threadsafe_function(settleLaterFunc_, napi_threadsafe_function_release_mode::napi_tsfn_release);
}

void PromiseBase::Abort()
{
    const napi_extended_error_info* errorInfo { nullptr };
    napi_get_last_error_info(env_, &errorInfo);
    auto originalMessage = errorInfo->error_message;
    bool exceptionPending { false };
    napi_is_exception_pending(env_, &exceptionPending);
    if (exceptionPending) {
        // Not allowed to call most napi functions, so can't clean up.
        deferred_ = nullptr;
        promise_ = nullptr;
    } else {
        napi_get_undefined(env_, &result_);
        if (deferred_) {
            napi_reject_deferred(env_, deferred_, result_);
        }
        auto message = (originalMessage != nullptr) ? originalMessage : "Error initializing Promise";
        napi_throw_error(env_, nullptr, message);
    }
    settleLaterFunc_ = nullptr;
}

void PromiseBase::SettleCB(napi_env env, napi_value js_callback, void* context, void* inData)
{
    // This is run in the JS thread, so be careful with the calls to engine.
    PromiseBase* self = (PromiseBase*)context;
    self->SettleNow();
}

void PromiseBase::FinalizeCB(napi_env env, void* finalize_data, void* finalize_hint)
{
    PromiseBase* self = (PromiseBase*)finalize_data;
    delete self;
}
