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

#ifndef RENDERCONTEXTJS_H
#define RENDERCONTEXTJS_H

#include <base/containers/unordered_map.h>

#include <meta/api/threading/mutex.h>

#include <scene/interface/resource/intf_render_resource_manager.h>

#ifdef __SCENE_ADAPTER__
#include "scene_adapter/intf_scene_adapter.h"
#endif

#include "BaseObjectJS.h"
#include "DisposeContainer.h"

struct GlobalResources;

class RenderResources {
public:
    RenderResources(napi_env env);
    ~RenderResources();

    void DisposeHook(uintptr_t token, NapiApi::Object);
    void ReleaseDispose(uintptr_t token);

    void StrongDisposeHook(uintptr_t token, NapiApi::Object);
    void ReleaseStrongDispose(uintptr_t token);

    void StoreBitmap(BASE_NS::string_view uri, SCENE_NS::IBitmap::Ptr bitmap);
    SCENE_NS::IBitmap::Ptr FetchBitmap(BASE_NS::string_view uri);

private:
    CORE_NS::Mutex mutex_;
    napi_env env_;
    BASE_NS::unordered_map<BASE_NS::string, SCENE_NS::IBitmap::Ptr> bitmaps_;
    DisposeContainer disposeContainer_;
};

class RenderContextJS : public BaseObject {
public:
    static constexpr uint32_t ID = 201;
    static void Init(napi_env env, napi_value exports);
    static void RegisterEnums(NapiApi::Object exports);

    RenderContextJS(napi_env, napi_callback_info);
    ~RenderContextJS() override;
    void* GetInstanceImpl(uint32_t) override;

    BASE_NS::shared_ptr<RenderResources> GetResources() const;

    static napi_value GetDefaultContext(napi_env env);

private:
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);
    void DisposeNative(void* id) override;
    void Finalize(napi_env env) override;

public:
    napi_value GetResourceFactory(NapiApi::FunctionContext<>& ctx);
    napi_value LoadPlugin(NapiApi::FunctionContext<BASE_NS::string>& ctx);

    // create shader
    napi_value CreateShader(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    // create image
    napi_value CreateImage(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    // create mesh
    napi_value CreateMeshResource(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx);
    // create sampler
    // create scene
    // Register shader Path
    napi_value RegisterResourcePath(NapiApi::FunctionContext<BASE_NS::string, BASE_NS::string>& ctx);

private:
    CORE_NS::Mutex mutex_;
    napi_env env_;
    SCENE_NS::IRenderResourceManager::Ptr renderManager_;
    BASE_NS::shared_ptr<GlobalResources> globalResources_;
    mutable BASE_NS::weak_ptr<RenderResources> resources_;
};

#endif // RENDERCONTEXTJS_H
