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
#ifndef CAMERA_JS_H
#define CAMERA_JS_H

#include <optional>

#include <meta/interface/intf_object.h>
#include <scene/interface/intf_raycast.h>

#include <base/containers/unordered_map.h>

#include "BaseObjectJS.h"
#include "ColorProxy.h"
#include "NodeImpl.h"

class CameraJS final : public BaseObject, public NodeImpl {
public:
    static constexpr uint32_t ID = 80;
    static void Init(napi_env env, napi_value exports);
    static void RegisterEnums(NapiApi::Object exports);

    CameraJS(napi_env, napi_callback_info);
    ~CameraJS() override;
    void* GetInstanceImpl(uint32_t) override;

    META_NS::IObject::Ptr CreateObject(const META_NS::ClassInfo&);
    void ReleaseObject(const META_NS::IObject::Ptr&);

private:
    void DisposeNative(void*) override;
    void Finalize(napi_env env) override;
    napi_value GetFov(NapiApi::FunctionContext<>& ctx);
    void SetFov(NapiApi::FunctionContext<float>& ctx);

    napi_value GetFar(NapiApi::FunctionContext<>& ctx);
    void SetFar(NapiApi::FunctionContext<float>& ctx);
    napi_value GetNear(NapiApi::FunctionContext<>& ctx);
    void SetNear(NapiApi::FunctionContext<float>& ctx);

    napi_value GetPostProcess(NapiApi::FunctionContext<>& ctx);
    void SetPostProcess(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    napi_value GetEnabled(NapiApi::FunctionContext<>& ctx);
    void SetEnabled(NapiApi::FunctionContext<bool>& ctx);

    napi_value GetMSAA(NapiApi::FunctionContext<>& ctx);
    void SetMSAA(NapiApi::FunctionContext<std::optional<bool>>& ctx);

    napi_value GetRenderingPipeline(NapiApi::FunctionContext<>& ctx);
    void SetRenderingPipeline(NapiApi::FunctionContext<uint32_t>& ctx);

    napi_value GetRenderTargetColorFormat(NapiApi::FunctionContext<>& ctx);
    void SetRenderTargetColorFormat(NapiApi::FunctionContext<uint32_t>& ctx);

    napi_value GetColor(NapiApi::FunctionContext<>& ctx);
    void SetColor(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    napi_value GetEffects(NapiApi::FunctionContext<>& ctx);

    napi_value WorldToScreen(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value ScreenToWorld(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    enum class ProjectionDirection { WORLD_TO_SCREEN, SCREEN_TO_WORLD };
    template<ProjectionDirection dir>
    napi_value ProjectCoords(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    napi_value Raycast(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx);

    napi_value GetProjectionMatrix(NapiApi::FunctionContext<>& ctx);
    napi_value GetViewMatrix(NapiApi::FunctionContext<>& ctx);

    template<typename CoordType>
    struct RaycastResources {
        CoordType nativeCoord;
        SCENE_NS::ICameraRayCast::Ptr raycastSelf;
        NapiApi::StrongRef scene;

        bool hasEverything { false };
        BASE_NS::string errorMsg;
    };
    template<typename CoordType>
    RaycastResources<CoordType> GetRaycastResources(const NapiApi::Object& jsCoord);

    BASE_NS::unique_ptr<ColorProxy> clearColor_;
    NapiApi::StrongRef postProc_;
    NapiApi::StrongRef effects_;

    BASE_NS::unordered_map<uintptr_t, META_NS::IObject::Ptr> resources_;

    bool msaaEnabled_ { false };
    bool clearColorEnabled_ { false };
};
#endif
