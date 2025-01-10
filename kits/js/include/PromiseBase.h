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

#ifndef PROMISE_BASE_H
#define PROMISE_BASE_H

#include <napi_api.h>

/**
 * @brief Provide an async state with JavaScript promise semantics.
 *
 * Usage example:
 * struct Promise : public PromiseBase {
 * using PromiseBase::PromiseBase;
 *     NapiApi::StrongRef arg;
 *     bool SetResult() override {
 *         result_ = DoSomethingWith(arg);
 *     }
 * };
 * auto promise = new Promise();
 * promise->arg = someVariable;
 * auto jsValue = promise->ToNapiValue();   // Extract the JS promise before settling.
 * promise->SettleLater();                  // After a settle call, do not touch the promise object.
 * return jsValue;
 *
 * @note An instance must be dynamically allocated (into a raw pointer) and created in the main thread. Call one of
 *       SettleLater, SettleNow or Reject to settle the promise. After settling, the object will delete itself.
 */
class PromiseBase {
public:
    /**
     * @brief Call only with new, in the main thread that is local to the given napi_env.
     */
    explicit PromiseBase(napi_env env);
    virtual ~PromiseBase();

    /**
     * @brief Get the JavaScript promise object.
     */
    napi_value ToNapiValue() const;

    /**
     * @brief Settle the promise later, in the JS thread, and delete this object.
     * @note Call only from the engine thread. After calling, do not use this object.
     */
    void SettleLater();

    /**
     * @brief Settle the promise immediately and delete this object.
     * @note After calling, do not use this object.
     */
    void SettleNow();

    /**
     * @brief Settle the promise immediately without fulfilling it and delete this object.
     * @note After calling, do not use this object.
     */
    void Reject();

protected:
    /**
     * @brief Set the result of the promise.
     * @return True on success. If false, the promise will be rejected.
     * @note SettleLater will run this in the JS thread; SettleNow in the same thread where it was called.
     */
    virtual bool SetResult() = 0;
    napi_value result_ { nullptr };
    napi_env env_ { nullptr };

private:
    void Resolve();
    void CreateJsPromise();
    void CreateSettleFunc();
    void ReleaseSettleFunc();
    void Abort();

    static inline void SettleCB(napi_env env, napi_value js_callback, void* context, void* inData);
    static inline void FinalizeCB(napi_env env, void* finalize_data, void* finalize_hint);

    napi_deferred deferred_ { nullptr };
    napi_value promise_ { nullptr };
    napi_threadsafe_function settleLaterFunc_ { nullptr };
};

#endif
