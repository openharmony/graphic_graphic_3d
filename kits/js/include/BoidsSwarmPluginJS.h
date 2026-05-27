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

#ifndef BOIDS_SWARM_PLUGIN_JS_H
#define BOIDS_SWARM_PLUGIN_JS_H

#include "BaseObjectJS.h"
#include "napi/weak_ref.h"

class BoidsSwarmWorldJS final : public BaseObject {
public:
    static constexpr uint32_t ID = 0XB01D000D;

    static void Init(napi_env env, napi_value exports);

    BoidsSwarmWorldJS(napi_env env, napi_callback_info info);
    ~BoidsSwarmWorldJS() override;

    void* GetInstanceImpl(uint32_t id) override;

    napi_value AddBoidsSimComponent(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx);
    napi_value SetBoidsSimComponent(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx);
    napi_value AddBoidsSimGravityComponent(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx);
    napi_value SetBoidsSimGravityComponent(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx);
    napi_value AddBoidsSimRepulsionComponent(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx);
    napi_value SetBoidsSimRepulsionComponent(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx);

    napi_value GetBoidsSimComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value GetBoidsSimGravityComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value GetBoidsSimRepulsionComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value GetBoidsSimStateComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    napi_value RemoveBoidsSimComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value RemoveBoidsSimGravityComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value RemoveBoidsSimRepulsionComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx);

private:
    napi_value AddComponent(
        NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx, BASE_NS::string_view compName);
    napi_value SetComponent(
        NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx, BASE_NS::string_view compName);
    napi_value GetComponent(NapiApi::FunctionContext<NapiApi::Object>& ctx, BASE_NS::string_view compName);
    napi_value RemoveComponent(
        NapiApi::FunctionContext<NapiApi::Object>& ctx, BASE_NS::string_view compName, const BASE_NS::Uid& uid);

    napi_value AddTarget(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object, BASE_NS::string>& ctx);

private:
    napi_value Play(NapiApi::FunctionContext<>& ctx);
    napi_value Pause(NapiApi::FunctionContext<>& ctx);
    napi_value Stop(NapiApi::FunctionContext<>& ctx);
    napi_value IsPlaying(NapiApi::FunctionContext<>& ctx);
    napi_value GetPlaySpeed(NapiApi::FunctionContext<>& ctx);
    void SetPlaySpeed(NapiApi::FunctionContext<float>& ctx);
    napi_value GetTimeStepSec(NapiApi::FunctionContext<>& ctx);
    void SetTimeStepSec(NapiApi::FunctionContext<float>& ctx);
    napi_value GetAxisMask(NapiApi::FunctionContext<>& ctx);
    void SetAxisMask(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value GetVelocitySmoothingFactor(NapiApi::FunctionContext<>& ctx);
    void SetVelocitySmoothingFactor(NapiApi::FunctionContext<float>& ctx);

    void DisposeNative(void*) override;

    NapiApi::WeakObjectRef scene_;
};

class BoidsSwarmPluginJS final : public BaseObject {
public:
    static constexpr uint32_t ID = 0xB01D5161;

    static void Init(napi_env env, napi_value exports);

    BoidsSwarmPluginJS(napi_env env, napi_callback_info info);
    ~BoidsSwarmPluginJS() override;

    void* GetInstanceImpl(uint32_t id) override;

    static napi_value GetDefaultBoidsSimWorld(NapiApi::FunctionContext<NapiApi::Object>& ctx);

private:
    void DisposeNative(void*) override;
};

#endif  // BOIDS_SWARM_PLUGIN_JS_H
