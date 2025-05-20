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
    if (action == Action::RESOLVE) {
        napi_resolve_deferred(env_, deferred_, result);
    } else {
        napi_reject_deferred(env_, deferred_, result);
    }
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
