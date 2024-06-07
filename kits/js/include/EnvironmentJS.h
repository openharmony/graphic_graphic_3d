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

#ifndef OHOS_RENDER_3D_ENVIRONMENTJS_H
#define OHOS_RENDER_3D_ENVIRONMENTJS_H
#include "BaseObjectJS.h"
#include "SceneResourceImpl.h"
#include "Vec4Proxy.h"
class EnvironmentJS : public BaseObject<EnvironmentJS>, SceneResourceImpl {
public:
    static constexpr uint32_t ID = 90;
    static void Init(napi_env env, napi_value exports);

    EnvironmentJS(napi_env, napi_callback_info);
    ~EnvironmentJS() override;
    void* GetInstanceImpl(uint32_t id) override;

private:
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);
    void DisposeNative() override;
    void Finalize(napi_env env) override;
    // JS properties
    enum EnvironmentBackgroundType {
        /**
         * The background is none.
         *
         * @syscap SystemCapability.ArkUi.Graphics3D
         * @since 12
         */
        BACKGROUND_NONE = 0,

        /**
         * The background is image.
         *
         * @syscap SystemCapability.ArkUi.Graphics3D
         * @since 12
         */
        BACKGROUND_IMAGE = 1,

        /**
         * The background is cubmap.
         *
         * @syscap SystemCapability.ArkUi.Graphics3D
         * @since 12
         */
        BACKGROUND_CUBEMAP = 2,

        /**
         * The background is equirectangular.
         *
         * @syscap SystemCapability.ArkUi.Graphics3D
         * @since 12
         */
        BACKGROUND_EQUIRECTANGULAR = 3,
    };

    napi_value GetBackgroundType(NapiApi::FunctionContext<>& ctx);
    void SetBackgroundType(NapiApi::FunctionContext<uint32_t>& ctx);
    napi_value GetEnvironmentImage(NapiApi::FunctionContext<>& ctx);
    void SetEnvironmentImage(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value GetRadianceImage(NapiApi::FunctionContext<>& ctx);
    void SetRadianceImage(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value GetIrradianceCoefficients(NapiApi::FunctionContext<>& ctx);
    void SetIrradianceCoefficients(NapiApi::FunctionContext<NapiApi::Array>& ctx);

    napi_value GetIndirectDiffuseFactor(NapiApi::FunctionContext<>& ctx);
    void SetIndirectDiffuseFactor(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    napi_value GetIndirectSpecularFactor(NapiApi::FunctionContext<>& ctx);
    void SetIndirectSpecularFactor(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    napi_value GetEnvironmentMapFactor(NapiApi::FunctionContext<>& ctx);
    void SetEnvironmentMapFactor(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    BASE_NS::unique_ptr<Vec4Proxy> diffuseFactor_;
    BASE_NS::unique_ptr<Vec4Proxy> specularFactor_;
    BASE_NS::unique_ptr<Vec4Proxy> environmentFactor_;

    // weak reference to the owning scene.
};
#endif // OHOS_RENDER_3D_ENVIRONMENTJS_H