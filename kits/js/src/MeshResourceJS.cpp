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

#include "SceneJS.h"

MeshResourceJS::MeshResourceJS(napi_env e, napi_callback_info i)
    : BaseObject<MeshResourceJS>(e, i), SceneResourceImpl(SceneResourceType::MESH_RESOURCE)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    NapiApi::Object meJs(fromJs.This());

    if (!fromJs) {
        // We except this was created internally and that things will be initialized later.
        return;
    }
    scene_ = fromJs.Arg<0>().valueOrDefault();
    // Add the dispose hook to scene so that the MeshResourceJS is disposed when scene is disposed.
    if (auto sceneJS = GetJsWrapper<SceneJS>(scene_.GetObject())) {
        sceneJS->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), fromJs.This());
    }

    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        LOG_F("INVALID SCENE!");
    }

    auto resourceParams = NapiApi::Object { fromJs.Arg<1>() };
    auto nativeObj = GetNativeObjectParam<META_NS::IObject>(resourceParams);
    if (!nativeObj) {
        nativeObj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(SCENE_NS::ClassId::MeshResource);
    }

    // TODO: Name remains undefined. This has no effect. There are prop problems with other SceneResourceImpls as well.
    auto name = BASE_NS::string {};
    if (auto nameParam = resourceParams.Get<BASE_NS::string>("name"); nameParam.IsDefined()) {
        name = nameParam;
    } else if (nativeObj) {
        name = nativeObj->GetName();
    }
    if (!name.empty()) {
        meJs.Set("name", name);
    }

    SetNativeObject(nativeObj, false);
    StoreJsObj(nativeObj, meJs);

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
    return (id == MeshResourceJS::ID) ? this : SceneResourceImpl::GetInstanceImpl(id);
}

SCENE_NS::IMesh::Ptr MeshResourceJS::CreateMesh()
{
    auto scene = GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject());
    if (!scene || !geometryDefinition_) {
        return {};
    }

    const auto meshCreator = scene->CreateObject<SCENE_NS::ICreateMesh>(SCENE_NS::ClassId::MeshCreator).GetResult();
    // Name and material aren't set here. Name is set in the constructor. Material needs to be manually set later.
    auto meshConfig = SCENE_NS::MeshConfig {};
    return geometryDefinition_->CreateMesh(meshCreator, meshConfig);
}

void MeshResourceJS::DisposeNative(void* scene)
{
    if (disposed_) {
        return;
    }
    disposed_ = true;
    if (auto node = interface_pointer_cast<SCENE_NS::IMeshResource>(GetNativeObject())) {
        // reset the native object refs
        SetNativeObject(nullptr, false);
        SetNativeObject(nullptr, true);
    }
    geometryDefinition_.reset();

    if (auto* sceneJS = static_cast<SceneJS*>(scene)) {
        sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
    }

    scene_.Reset();
}

void MeshResourceJS::Finalize(napi_env env)
{
    DisposeNative(GetJsWrapper<SceneJS>(scene_.GetObject()));
    BaseObject::Finalize(env);
}