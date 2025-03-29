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
#include "GeometryJS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_create_mesh.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_mesh_resource.h>
#include <scene/interface/intf_scene.h>

#include "MeshResourceJS.h"
#include "SceneJS.h"
#include "geometry_definition/CubeJS.h"
#include "geometry_definition/CustomJS.h"
#include "geometry_definition/PlaneJS.h"
#include "geometry_definition/SphereJS.h"

void* GeometryJS::GetInstanceImpl(uint32_t id)
{
    if (id == GeometryJS::ID) {
        return this;
    }
    return NodeImpl::GetInstanceImpl(id);
}
void GeometryJS::DisposeNative(void* scn)
{
    if (disposed_) {
        return;
    }
    LOG_V("GeometryJS::DisposeNative");
    disposed_ = true;

    SceneJS* sceneJS = static_cast<SceneJS*>(scn);
    if (sceneJS) {
        sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
    }

    if (auto node = interface_pointer_cast<SCENE_NS::INode>(GetNativeObject())) {
        if (!IsAttached()) {
            if (auto access = interface_pointer_cast<SCENE_NS::IMeshAccess>(node)) {
                access->SetMesh(nullptr).Wait();
            }
            if (auto scene = node->GetScene()) {
                scene->RemoveNode(BASE_NS::move(node)).Wait();
            }
        }
        // reset the native object refs
        SetNativeObject(nullptr, false);
        SetNativeObject(nullptr, true);
    }
    scene_.Reset();
}
void GeometryJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;
    NodeImpl::GetPropertyDescs(node_props);

    using namespace NapiApi;
    node_props.push_back(GetProperty<Object, GeometryJS, &GeometryJS::GetMesh>("mesh"));

    napi_value func;
    auto status = napi_define_class(env, "Geometry", NAPI_AUTO_LENGTH, BaseObject::ctor<GeometryJS>(), nullptr,
        node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    GetInstanceData(env, (void**)&mis);
    mis->StoreCtor("Geometry", func);
}

GeometryJS::GeometryJS(napi_env e, napi_callback_info i) : BaseObject<GeometryJS>(e, i), NodeImpl(NodeImpl::GEOMETRY)
{
    LOG_V("GeometryJS ++ ");

    // Resolve overload with 2 or 3 given args.
    if (auto ctx = NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> { e, i }) {
        Construct(e, ctx.This(), ctx.Arg<0>(), ctx.Arg<1>());
    } else if (auto ctx = NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object, NapiApi::Object> { e, i }) {
        // Manual creation with an extra arg.
        if (Construct(e, ctx.This(), ctx.Arg<0>(), ctx.Arg<1>()) == ConstructionState::LACKS_NATIVE) {
            CreateNativeObject(e, ctx.This(), ctx.Arg<1>(), ctx.Arg<2>()); // 2: index
        }
    } else {
        LOG_E("Bad args given for GeometryJS constructor");
        return;
    }
}
GeometryJS::~GeometryJS()
{
    LOG_V("GeometryJS -- ");
}

void GeometryJS::Finalize(napi_env env)
{
    DisposeNative(GetJsWrapper<SceneJS>(scene_.GetObject()));
    BaseObject::Finalize(env);
}
GeometryJS::ConstructionState GeometryJS::Construct(
    napi_env env, NapiApi::Object meJs, NapiApi::Object scene, NapiApi::Object sceneNodeParameters)
{
    if (!GetNativeMeta<SCENE_NS::IScene>(scene)) {
        LOG_F("Invalid scene for GeometryJS!");
        return ConstructionState::FAILED;
    }
    scene_ = NapiApi::WeakRef { env, scene.ToNapiValue() };

    // Add the dispose hook to scene so that the Geometry node is disposed when scene is disposed.
    if (auto sceneJS = GetJsWrapper<SceneJS>(scene)) {
        sceneJS->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
    }

    if (const auto name = sceneNodeParameters.Get<BASE_NS::string>("name"); name.IsValid()) {
        meJs.Set("name", name);
    }

    auto nativeObject = GetNativeObjectParam<META_NS::IObject>(sceneNodeParameters);
    if (nativeObject) {
        StoreJsObj(nativeObject, meJs);
        return ConstructionState::FINISHED;
    }
    return ConstructionState::LACKS_NATIVE;
}

void GeometryJS::CreateNativeObject(
    napi_env env, NapiApi::Object meJs, NapiApi::Object sceneNodeParameters, NapiApi::Object meshResourceParam)
{
    auto name = BASE_NS::string {};
    auto path = BASE_NS::string {};
    if (auto param = sceneNodeParameters.Get("name")) {
        name = NapiApi::Value<BASE_NS::string>(env, param).valueOrDefault("");
    }
    if (auto param = sceneNodeParameters.Get("path")) {
        path = NapiApi::Value<BASE_NS::string>(env, param).valueOrDefault("");
    }

    auto nodePath = BASE_NS::string {};
    if (!path.empty()) {
        nodePath = path;
    } else if (!name.empty()) {
        // Use the name as path. Create the node directly under root.
        nodePath = name;
    }

    auto scene = GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject());
    // Node creation can fail e.g. due to a bad path. Then we're going to have a null Geometry object.
    auto meshNode = scene->CreateNode(nodePath, SCENE_NS::ClassId::MeshNode).GetResult();
    if (auto access = interface_pointer_cast<SCENE_NS::IMeshAccess>(meshNode)) {
        const auto resource = static_cast<MeshResourceJS*>(meshResourceParam.Native<TrueRootObject>());
        const auto mesh = CreateMesh(env, resource);
        access->SetMesh(mesh).GetResult();
    }
    // Always set regardless of success.
    SetNativeObject(interface_pointer_cast<META_NS::IObject>(meshNode), false);
    StoreJsObj(GetNativeObject(), meJs);
}

napi_value GeometryJS::GetMesh(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    META_NS::IObject::Ptr mesh = GetNativeObject();
    if (!interface_cast<SCENE_NS::IMesh>(mesh)) {
        mesh = nullptr;
    }

    if (!mesh) {
        // no mesh.
        return ctx.GetNull();
    }

    if (auto cached = FetchJsObj(mesh, "_JSWMesh")) {
        // always return the same js object.
        return cached.ToNapiValue();
    }
    // create new js object for the native node.
    NapiApi::Env env(ctx.Env());
    NapiApi::Object argJS(env);
    napi_value args[] = { scene_.GetValue(), argJS.ToNapiValue() };

    MakeNativeObjectParam(env, mesh, BASE_NS::countof(args), args);

    auto nodeJS = CreateJsObj(env, "Mesh", mesh, false, BASE_NS::countof(args), args);
    if (!nodeJS) {
        LOG_E("Could not create JSObject for Class %s", "Mesh");
        return {};
    }
    return StoreJsObj(mesh, nodeJS, "_JSWMesh").ToNapiValue();
}

SCENE_NS::IMesh::Ptr GeometryJS::CreateMesh(napi_env env, MeshResourceJS* meshResource)
{
    auto mesh = SCENE_NS::IMesh::Ptr {};
    if (!meshResource) {
        return mesh;
    }
    auto scene = GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject());
    const auto tro = meshResource->GetGeometryDefinition().GetObject().Native<TrueRootObject>();
    if (!scene || !tro) {
        return mesh;
    }

    const auto meshCreator = scene->CreateObject<SCENE_NS::ICreateMesh>(SCENE_NS::ClassId::MeshCreator).GetResult();
    // Name and material aren't set here. Name is set in the constructor. Material needs to be manually set later.
    auto meshConfig = SCENE_NS::MeshConfig {};
    using namespace GeometryDefinition;
    if (const auto cube = static_cast<CubeJS*>(tro->GetInstanceImpl(CubeJS::ID))) {
        const auto size { cube->GetSize() };
        mesh = meshCreator->CreateCube(meshConfig, size.x, size.y, size.z).GetResult();
    } else if (const auto plane = static_cast<PlaneJS*>(tro->GetInstanceImpl(PlaneJS::ID))) {
        const auto size { plane->GetSize() };
        mesh = meshCreator->CreatePlane(meshConfig, size.x, size.y).GetResult();
    } else if (const auto sphere = static_cast<SphereJS*>(tro->GetInstanceImpl(SphereJS::ID))) {
        const auto segmentCount { sphere->GetSegmentCount() };
        mesh = meshCreator->CreateSphere({}, sphere->GetRadius(), segmentCount, segmentCount).GetResult();
    } else if (const auto custom = static_cast<CustomJS*>(tro->GetInstanceImpl(CustomJS::ID))) {
        mesh = meshCreator->Create(meshConfig, custom->ToNative()).GetResult();
    } else {
        LOG_E("Unknown geometry type for mesh creation");
    }
    return mesh;
}
