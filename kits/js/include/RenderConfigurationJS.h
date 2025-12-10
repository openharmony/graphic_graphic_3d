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
#ifndef RENDER_CONFIGURATION_JS_H
#define RENDER_CONFIGURATION_JS_H

#include <napi_api.h>
#include <scene/interface/intf_scene.h>
#include "Vec2Proxy.h"

class RenderConfiguration {
public:
    RenderConfiguration();
    ~RenderConfiguration();

    static RenderConfiguration* Unwrap(NapiApi::Object obj);
    NapiApi::StrongRef Wrap(NapiApi::Object obj);

    void SetFrom(napi_env env, SCENE_NS::IRenderConfiguration::Ptr rc);

private:
    void SetTo(NapiApi::Object obj);
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);
    void Detach();
    napi_value GetShadowResolution(NapiApi::FunctionContext<>& ctx);
    void SetShadowResolution(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    UVec2Proxy* GetResolutionProxy(napi_env env);
    napi_value GetShadowResolutionValue(napi_env env);

    SCENE_NS::IRenderConfiguration::Ptr rc_;
    BASE_NS::unique_ptr<UVec2Proxy> resolutionProxy_;
};

#endif // RENDER_CONFIGURATION_JS_H
