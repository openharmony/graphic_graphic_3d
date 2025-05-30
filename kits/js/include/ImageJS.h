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
#ifndef IMAGEJS_H
#define IMAGEJS_H
#include "BaseObjectJS.h"
#include "SceneResourceImpl.h"
#include "RenderContextJS.h"

class ImageJS : public BaseObject, SceneResourceImpl {
public:
    static constexpr uint32_t ID = 110;
    static void Init(napi_env env, napi_value exports);

    ImageJS(napi_env, napi_callback_info);
    ~ImageJS() override;
    void* GetInstanceImpl(uint32_t) override;

private:
    void DisposeNative(void*) override;
    void Finalize(napi_env env) override;
    // JS properties
    napi_value GetWidth(NapiApi::FunctionContext<>& ctx);
    napi_value GetHeight(NapiApi::FunctionContext<>& ctx);

private:
    BASE_NS::shared_ptr<RenderResources> resources_;
};

#endif
