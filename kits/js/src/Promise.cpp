/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "Promise.h"

#include <napi_api.h>

Promise::Promise(napi_env env) : env_(env)
{
    napi_create_promise(env, &deferred_, &promise_);
}

napi_value Promise::Resolve(napi_value result)
{
    return Settle(result, Action::RESOLVE);
}

napi_value Promise::Reject(const BASE_NS::string& reason)
{
    LOG_E("%s", reason.c_str());
    return Settle(NapiApi::Env { env_ }.GetString(reason), Action::REJECT);
}

napi_value Promise::Settle(napi_value result, Action action)
{
    if (alreadySettled_) {
        LOG_E("Trying to settle a promise that has already been settled");
        return promise_;
    }
    alreadySettled_ = true;

    if (!result) {
        napi_get_undefined(env_, &result);
    }
#if __OHOS__
    // use the common workaround to make the promise resolve/reject async.
    struct tmp {
        Action action;
        napi_ref ref;
        napi_deferred def;
        napi_async_work asyncWork;
    };
    tmp* ctx = new tmp;
    napi_create_reference(env_, result, 1, &ctx->ref);
    ctx->action = action;
    ctx->def = deferred_;
    deferred_ = nullptr;
    napi_value resourceName = nullptr;
    napi_create_string_latin1(env_, "PromiseAsync", NAPI_AUTO_LENGTH, &resourceName);
    if (auto status = napi_create_async_work(
        env_, nullptr, resourceName, [](napi_env env, void* data) {},
        [](napi_env env, napi_status status, void* data) {
            tmp* ctx = (tmp*)data;
            napi_value result;
            napi_get_reference_value(env, ctx->ref, &result);
            if (ctx->action == Action::RESOLVE) {
                napi_resolve_deferred(env, ctx->def, result);
            } else {
                napi_reject_deferred(env, ctx->def, result);
            }
            napi_delete_async_work(env, ctx->asyncWork);
            napi_delete_reference(env, ctx->ref);
            delete ctx;
        },
        (void*)ctx, &ctx->asyncWork); status != napi_ok) {
        napi_delete_reference(env_, ctx->ref);
        delete ctx;
    } else if (auto status = napi_queue_async_work(env_, ctx->asyncWork); status != napi_ok) {
        napi_delete_reference(env_, ctx->ref);
        napi_delete_async_work(env_, ctx->asyncWork);
        delete ctx;
    }
#else
    // on node.js the promise resolve is always asynchronous.
    if (action == Action::RESOLVE) {
        napi_resolve_deferred(env_, deferred_, result);
    } else {
        napi_reject_deferred(env_, deferred_, result);
    }
#endif
    return promise_;
}

napi_env Promise::Env() const
{
    return env_;
}

napi_value Promise::ToNapiValue() const
{
    return promise_;
}

Promise::operator napi_value() const
{
    return promise_;
}
