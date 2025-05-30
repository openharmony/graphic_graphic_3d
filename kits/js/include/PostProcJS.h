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

#ifndef POSTPROCJS_H
#define POSTPROCJS_H
#include "BaseObjectJS.h"

class PostProcJS : public BaseObject {
public:
    static constexpr uint32_t ID = 20;
    static void Init(napi_env env, napi_value exports);

    PostProcJS(napi_env, napi_callback_info);
    ~PostProcJS() override;
    void* GetInstanceImpl(uint32_t) override;

private:
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);
    void DisposeNative(void*) override;
    void Finalize(napi_env env) override;

    // JS properties
    napi_value GetToneMapping(NapiApi::FunctionContext<>& ctx);
    void SetToneMapping(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value GetBloom(NapiApi::FunctionContext<>& ctx);
    void SetBloom(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    NapiApi::StrongRef toneMap_; // keep a strong ref..

    NapiApi::WeakRef camera_; // weak ref to owning camera.
    NapiApi::StrongRef bloom_; // keep a strong ref..
};
#endif
