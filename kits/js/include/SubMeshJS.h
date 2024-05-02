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

#ifndef OHOS_RENDER_3D_SUBMESH_JS_H
#define OHOS_RENDER_3D_SUBMESH_JS_H
#include <meta/interface/intf_object.h>

#include "BaseObjectJS.h"
#include "Vec3Proxy.h"

class SubMeshJS : public BaseObject<SubMeshJS> {
public:
    static constexpr uint32_t ID = 50;
    static void Init(napi_env env, napi_value exports);
    SubMeshJS(napi_env, napi_callback_info);
    ~SubMeshJS() override;
    virtual void* GetInstanceImpl(uint32_t id) override;

private:
    void DisposeNative() override;
    napi_value GetName(NapiApi::FunctionContext<>& ctx);
    void SetName(NapiApi::FunctionContext<BASE_NS::string>& ctx);
    napi_value GetAABB(NapiApi::FunctionContext<>& ctx);

    napi_value GetMaterial(NapiApi::FunctionContext<>& ctx);
    void SetMaterial(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    BASE_NS::unique_ptr<Vec3Proxy> aabbMin_, aabbMax_;
    NapiApi::StrongRef scene_;
};
#endif // OHOS_RENDER_3D_SUBMESH_JS_H