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

#ifndef PROMISE_H
#define PROMISE_H

#include <napi_api.h>

/**
 * @brief Wrap napi promise handling.
 */
class Promise {
public:
    /**
     * @brief Whether to resolve or reject when settling a promise.
     */
    enum class Action : char { RESOLVE, REJECT };

    /**
     * @brief Create an object that wraps a JS promise.
     * @param env The napi environment.
     * @note Call only from the JS thread.
     */
    explicit Promise(napi_env env);

    /**
     * @brief Resolve the promise.
     * @param result The result of the promise.
     * @return This settled promise as a napi_value.
     * @note Call only from the JS thread. Further calls to Resolve, Reject or Settle will have no effect.
     */
    napi_value Resolve(napi_value result);

    /**
     * @brief Reject the promise with a reason for rejecting.
     * @param reason Why the promise was rejected. An error message to set as result.
     * @return This settled promise as a napi_value.
     * @note Call only from the JS thread. Further calls to Resolve, Reject or Settle will have no effect.
     */
    napi_value Reject(const BASE_NS::string& reason);

    /**
     * @brief Settle the promise.
     * @param result The result of the promise. Should be a reason string when rejecting.
     * @param action Whether to resolve or reject the promise.
     * @return This settled promise as a napi_value.
     * @note Call only from the JS thread. Further calls to Resolve, Reject or Settle will have no effect.
     */
    napi_value Settle(napi_value result, Action action);

    /**
     * @brief Get the napi environment.
     */
    napi_env Env() const;

    /**
     * @brief Return the promise as a napi_value.
     */
    napi_value ToNapiValue() const;

    /**
     * @brief Return the promise as a napi_value.
     */
    operator napi_value() const;

private:
    napi_env env_ { nullptr };
    napi_deferred deferred_ { nullptr };
    napi_value promise_ { nullptr };
    bool alreadySettled_ { false };
};

#endif
