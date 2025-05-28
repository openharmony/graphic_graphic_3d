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
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_mesh_resource.h>
#include <scene/interface/intf_scene.h>

#include "MeshResourceJS.h"
#include "ParamParsing.h"
#include "SceneJS.h"
#include "geometry_definition/GeometryDefinition.h"

void* GeometryJS::GetInstanceImpl(uint32_t id)
{
    if (id == GeometryJS::ID)
        return this;
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
        UnsetNativeObject();
    }
    scene_.Reset();
}
void GeometryJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;
    NodeImpl::GetPropertyDescs(node_props);

    using namespace NapiApi;
    node_props.push_back(GetProperty<Object, GeometryJS, &GeometryJS::GetMesh>("mesh"));
    node_props.push_back(GetProperty<Object, GeometryJS, &GeometryJS::GetMorpher>("morpher"));

    napi_value func;
    auto status = napi_define_class(env, "Geometry", NAPI_AUTO_LENGTH, BaseObject::ctor<GeometryJS>(), nullptr,
        node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor("Geometry", func);
    }
}

GeometryJS::GeometryJS(napi_env e, napi_callback_info i) : BaseObject(e, i), NodeImpl(NodeImpl::GEOMETRY)
{
    LOG_V("GeometryJS ++ ");

    if (auto ctx = NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> { e, i }) {
        Construct(e, ctx.This(), ctx.Arg<0>(), ctx.Arg<1>());
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
    DisposeNative(scene_.GetObject().GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
}

void GeometryJS::Construct(
    napi_env env, NapiApi::Object meJs, NapiApi::Object scene, NapiApi::Object sceneNodeParameters)
{
    if (!scene.GetNative<SCENE_NS::IScene>()) {
        LOG_W("Can't construct GeometryJS: Scene has gone missing");
        return;
    }
    scene_ = NapiApi::WeakRef { env, scene.ToNapiValue() };

    // Add the dispose hook to scene so that the Geometry node is disposed when scene is disposed.
    if (const auto sceneJS = scene.GetJsWrapper<SceneJS>()) {
        sceneJS->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
    }

    if (const auto name = ExtractName(sceneNodeParameters); !name.empty()) {
        meJs.Set("name", name);
    }
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

    NapiApi::Env env(ctx.Env());
    NapiApi::Object argJS(env);
    napi_value args[] = { scene_.GetValue(), argJS.ToNapiValue() };
    return CreateFromNativeInstance(env, "Mesh", mesh, PtrType::WEAK, args, "_JSWMesh").ToNapiValue();
}

napi_value GeometryJS::GetMorpher(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    META_NS::IObject::Ptr node = GetNativeObject();
    auto access = interface_cast<SCENE_NS::IMorphAccess>(node);

    if (!access || !access->Morpher()) {
        // no morpher.
        return ctx.GetNull();
    }
    
    SCENE_NS::IMorpher::Ptr morpher = META_NS::GetValue(access->Morpher());
    NapiApi::Env env(ctx.Env());
    NapiApi::Object argJS(env);
    napi_value args[] = { scene_.GetValue(), argJS.ToNapiValue() };
    return CreateFromNativeInstance(env, "Morpher", morpher, PtrType::WEAK, args, "_JSWMorpher")
        .ToNapiValue();
}
