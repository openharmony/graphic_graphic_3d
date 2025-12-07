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

#ifndef SAMPLERJS_H
#define SAMPLERJS_H

#include <scene/interface/intf_texture.h>

#include "BaseObjectJS.h"


class SamplerJS final : public BaseObject {
public:
    static constexpr uint32_t ID = 210;
    static void Init(napi_env env, napi_value exports);
    static SCENE_NS::SamplerFilter ConvertToFilter(uint32_t value);
    static uint32_t ConvertFromFilter(SCENE_NS::SamplerFilter value);
    static SCENE_NS::SamplerAddressMode ConvertToAddressMode(uint32_t value);
    static uint32_t ConvertFromAddressMode(SCENE_NS::SamplerAddressMode value);

    static void RegisterEnums(NapiApi::Object exports);

    // Create a Sampler that is just a JS object. There is no SamplerJS wrapping or native ISampler backing.
    static napi_value CreateRawJsObject(napi_env env);

    SamplerJS(napi_env, napi_callback_info);
    ~SamplerJS() override;

    void* GetInstanceImpl(uint32_t) override;
private:
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);
    void DisposeNative(void*) override;
    void Finalize(napi_env env) override;

    SCENE_NS::ISampler::Ptr GetSampler() const;

    // JS properties
    // magFilter?: SamplerFilter;
    napi_value GetMagFilter(NapiApi::FunctionContext<>& ctx);
    void SetMagFilter(NapiApi::FunctionContext<uint32_t>& ctx);

    // minFilter?: SamplerFilter;
    napi_value GetMinFilter(NapiApi::FunctionContext<>& ctx);
    void SetMinFilter(NapiApi::FunctionContext<uint32_t>& ctx);

    // mipMapMode?: SamplerFilter;
    napi_value GetMipMapMode(NapiApi::FunctionContext<>& ctx);
    void SetMipMapMode(NapiApi::FunctionContext<uint32_t>& ctx);

    // addressModeU?: SamplerAddressMode;
    napi_value GetAddressModeU(NapiApi::FunctionContext<>& ctx);
    void SetAddressModeU(NapiApi::FunctionContext<uint32_t>& ctx);

    // addressModeV?: SamplerAddressMode;
    napi_value GetAddressModeV(NapiApi::FunctionContext<>& ctx);
    void SetAddressModeV(NapiApi::FunctionContext<uint32_t>& ctx);

    // addressModeW?: SamplerAddressMode;
    napi_value GetAddressModeW(NapiApi::FunctionContext<>& ctx);
    void SetAddressModeW(NapiApi::FunctionContext<uint32_t>& ctx);
};
#endif
