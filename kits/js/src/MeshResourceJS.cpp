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

#include "MeshResourceJS.h"

#include <scene/interface/intf_create_mesh.h>
#include <scene/interface/intf_mesh_resource.h>
#include <scene/interface/intf_scene.h>

#include "JsObjectCache.h"
#include "SceneJS.h"

MeshResourceJS::MeshResourceJS(napi_env e, napi_callback_info i)
    : BaseObject(e, i), SceneResourceImpl(SceneResourceType::MESH_RESOURCE)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> ctx(e, i);
    // As long as our native object is a dummy interface without a backing implementation, we're only concerned about
    // arg validity and not about native object existence.
    if (!ctx) {
        LOG_E("Cannot finish creating a mesh resource: Invalid args given");
        assert(false);
        return;
    }

    scene_ = ctx.Arg<0>().valueOrDefault();
    // Add the dispose hook to scene so that the MeshResourceJS is disposed when scene is disposed.
    if (const auto sceneJS = scene_.GetJsWrapper<SceneJS>()) {
        sceneJS->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), ctx.This());
    } else if (const auto renderContextJS = scene_.GetJsWrapper<RenderContextJS>()) {
        renderContextJS->GetResources()->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), ctx.This());
    }

    auto resourceParams = NapiApi::Object { ctx.Arg<1>() };
    if (auto nameParam = resourceParams.Get<BASE_NS::string>("name"); nameParam.IsDefined()) {
        ctx.This().Set("name", nameParam);
    }

    GeometryDefinition::GeometryDefinition* geomDef {};
    napi_get_value_external(e, resourceParams.Get("GeometryDefinition"), (void**)&geomDef);
    geometryDefinition_.reset(geomDef);
}

void MeshResourceJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;
    SceneResourceImpl::GetPropertyDescs(node_props);

#define NAPI_API_JS_NAME MeshResource
    DeclareClass();
#undef NAPI_API_JS_NAME
}

void* MeshResourceJS::GetInstanceImpl(uint32_t id)
{
    if (id == MeshResourceJS::ID) {
        return static_cast<MeshResourceJS*>(this);
    }
    if (auto ret = SceneResourceImpl::GetInstanceImpl(id)) {
        return ret;
    }
    return BaseObject::GetInstanceImpl(id);
}

SCENE_NS::IMesh::Ptr MeshResourceJS::CreateMesh(const SCENE_NS::IScene::Ptr& scene)
{
    if (!scene || !geometryDefinition_) {
        CORE_LOG_E("Trying to create Scene mesh without scene");
        return {};
    }

    const auto meshCreator = scene->CreateObject<SCENE_NS::ICreateMesh>(SCENE_NS::ClassId::MeshCreator).GetResult();
    // Name and material aren't set here. Name is set in the constructor. Material needs to be manually set later.
    auto meshConfig = SCENE_NS::MeshConfig {};
    return geometryDefinition_->CreateMesh(meshCreator, meshConfig);
}

void MeshResourceJS::DisposeNative(void* /*scene*/)
{
    if (disposed_) {
        return;
    }
    disposed_ = true;
    if (auto native = GetNativeObject<META_NS::IObject>()) {
        DetachJsObj(native, "_JSW");
        UnsetNativeObject();
    }
    geometryDefinition_.reset();

    if (const auto sceneJS = scene_.GetJsWrapper<SceneJS>()) {
        sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
    }

    if (const auto renderContextJS = scene_.GetJsWrapper<RenderContextJS>()) {
        renderContextJS->GetResources()->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
    }

    scene_.Reset();
}

void MeshResourceJS::Finalize(napi_env env)
{
    DisposeNative(scene_.GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
}
