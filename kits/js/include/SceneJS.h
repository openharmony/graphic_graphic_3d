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

#ifndef SCENE_JS_H
#define SCENE_JS_H
#include <meta/api/threading/mutex.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_object.h>
#include <scene/interface/intf_bitmap.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_scene.h>

#include <base/containers/unordered_map.h>
#ifdef __SCENE_ADAPTER__
#include "scene_adapter/intf_scene_adapter.h"
#endif
#include "BaseObjectJS.h"
#include "MeshResourceJS.h"

class SceneJS : public BaseObject<SceneJS> {
public:
    static constexpr uint32_t ID = 4;
    static void Init(napi_env env, napi_value exports);
    static void RegisterEnums(NapiApi::Object exports);

    SceneJS(napi_env, napi_callback_info);
    ~SceneJS() override;
    void* GetInstanceImpl(uint32_t) override;
#ifdef __SCENE_ADAPTER__
    std::shared_ptr<OHOS::Render3D::ISceneAdapter> scene_ = nullptr;
#endif

    void StoreBitmap(BASE_NS::string_view uri, SCENE_NS::IBitmap::Ptr bitmap);
    SCENE_NS::IBitmap::Ptr FetchBitmap(BASE_NS::string_view uri);

    void DisposeHook(uintptr_t token, NapiApi::Object);
    void ReleaseDispose(uintptr_t token);

    void StrongDisposeHook(uintptr_t token, NapiApi::Object);
    void ReleaseStrongDispose(uintptr_t token);

private:
    static void AddScene(META_NS::IObjectRegistry* obr, SCENE_NS::IScene::Ptr scene);
    static void FlushScenes();
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);
    void DisposeNative(void*) override;
    void Finalize(napi_env env) override;
    // JS properties
    napi_value GetRoot(NapiApi::FunctionContext<>& ctx);

    napi_value GetAnimations(NapiApi::FunctionContext<>& ctx);

    napi_value GetEnvironment(NapiApi::FunctionContext<>& ctx);
    void SetEnvironment(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    napi_value GetRenderMode(NapiApi::FunctionContext<>& ctx);
    void SetRenderMode(NapiApi::FunctionContext<uint32_t>& ctx);

    // JS methods
    napi_value GetNode(NapiApi::FunctionContext<BASE_NS::string>& ctx);
    napi_value GetResourceFactory(NapiApi::FunctionContext<>& ctx);

    napi_value CreateCamera(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value CreateLight(NapiApi::FunctionContext<NapiApi::Object, uint32_t>& ctx);
    napi_value CreateNode(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value CreateTextNode(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value CreateEnvironment(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value CreateImage(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value CreateShader(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value CreateMaterial(NapiApi::FunctionContext<NapiApi::Object, uint32_t>& ctx);
    napi_value CreateScene(NapiApi::FunctionContext<>& ctx);

    napi_value CreateMeshResource(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx);
    napi_value CreateGeometry(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx);

    napi_value ImportNode(NapiApi::FunctionContext<BASE_NS::string, NapiApi::Object, NapiApi::Object>& ctx);
    napi_value ImportScene(NapiApi::FunctionContext<BASE_NS::string, NapiApi::Object, NapiApi::Object>& ctx);

    napi_value RenderFrame(NapiApi::FunctionContext<>& ctx);

    // static js method
    static napi_value Load(NapiApi::FunctionContext<>& ctx);

    // make a storage of all bitmaps..
    CORE_NS::Mutex mutex_;
    BASE_NS::unordered_map<BASE_NS::string, SCENE_NS::IBitmap::Ptr> bitmaps_;
    NapiApi::StrongRef environmentJS_;
    BASE_NS::unordered_map<uintptr_t, NapiApi::StrongRef> strongDisposables_;
    BASE_NS::unordered_map<uintptr_t, NapiApi::WeakRef> disposables_;
    napi_env env_;

public:
    NapiApi::Object CreateEnvironment(NapiApi::Object, NapiApi::Object);
};
#endif
