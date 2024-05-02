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

#ifndef OHOS_RENDER_3D_TONEMAPJS_H
#define OHOS_RENDER_3D_TONEMAPJS_H
#include "BaseObjectJS.h"

class ToneMapJS : public BaseObject<ToneMapJS> {
public:
    static constexpr uint32_t ID = 60;
    static void Init(napi_env env, napi_value exports);

    enum ToneMappingType {
        NOT_SET = -1,
        ACES = 0,
        ACES_2020 = 1,
        FILMIC = 2,
    };
    ToneMapJS(napi_env, napi_callback_info);
    ~ToneMapJS() override;

private:
    void* GetInstanceImpl(uint32_t) override;
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);
    void DisposeNative() override;
    void Finalize(napi_env env) override;
    // JS properties
    napi_value GetType(NapiApi::FunctionContext<>& ctx);
    void SetType(NapiApi::FunctionContext<uint32_t>& ctx);

    napi_value GetExposure(NapiApi::FunctionContext<>& ctx);
    void SetExposure(NapiApi::FunctionContext<float>& ctx);
};
#endif // OHOS_RENDER_3D_TONEMAPJS_H