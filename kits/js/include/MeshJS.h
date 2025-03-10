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
#ifndef MESH_JS_H
#define MESH_JS_H
#include <meta/interface/intf_object.h>
#include <scene/interface/intf_mesh.h>

#include "BaseObjectJS.h"
#include "SceneResourceImpl.h"

class MeshJS : public BaseObject<MeshJS>, SceneResourceImpl {
public:
    static constexpr uint32_t ID = 120;
    static void Init(napi_env env, napi_value exports);
    MeshJS(napi_env, napi_callback_info);
    ~MeshJS() override;
    virtual void* GetInstanceImpl(uint32_t) override;

    // Update the cached submesh at the index and sync all submeshes to scene. Return true for success.
    bool UpdateSubmesh(uint32_t index, SCENE_NS::ISubMesh::Ptr newSubmesh);

private:
    void DisposeNative(void*) override;
    napi_value GetSubmesh(NapiApi::FunctionContext<>& ctx);
    napi_value GetAABB(NapiApi::FunctionContext<>& ctx);

    napi_value GetMaterialOverride(NapiApi::FunctionContext<>& ctx);
    void SetMaterialOverride(NapiApi::FunctionContext<NapiApi::Object>& ctx);

    BASE_NS::vector<SCENE_NS::ISubMesh::Ptr> subs_;
};
#endif
