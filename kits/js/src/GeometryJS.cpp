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
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_mesh.h>

#include "SceneJS.h"

void* GeometryJS::GetInstanceImpl(uint32_t id)
{
    if (id == GeometryJS::ID) {
        return this;
    }
    return NodeImpl::GetInstanceImpl(id);
}
void GeometryJS::DisposeNative(void*)
{
    if (!disposed_) {
        LOG_V("GeometryJS::DisposeNative");
        disposed_ = true;

        NapiApi::Object obj = scene_.GetObject();
        auto* tro = obj.Native<TrueRootObject>();
        if (tro) {
            SceneJS* sceneJS = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
            if (sceneJS) {
                sceneJS->ReleaseStrongDispose(reinterpret_cast<uintptr_t>(&scene_));
            }
        }

        if (auto node = interface_pointer_cast<SCENE_NS::INode>(GetNativeObject())) {
            // reset the native object refs
            SetNativeObject(nullptr, false);
            SetNativeObject(nullptr, true);

            if (auto scene = node->GetScene()) {
                scene->ReleaseNode(node);
            }
        }
        scene_.Reset();
    }
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
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        // okay internal create. we will receive the object after.
        return;
    }
    scene_ = fromJs.Arg<0>().valueOrDefault();
    auto scn = GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject());
    if (scn == nullptr) {
        // hmm..
        LOG_F("Invalid scene for GeometryJS!");
        return;
    }
    {
        // add the dispose hook to scene. (so that the geometry node is disposed when scene is disposed)
        NapiApi::Object meJs(fromJs.This());
        NapiApi::Object scene = fromJs.Arg<0>();
        auto* tro = scene.Native<TrueRootObject>();
        if (tro) {
            auto* sceneJS = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
            if (sceneJS) {
                sceneJS->StrongDisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
            }
        }
    }
}
GeometryJS::~GeometryJS()
{
    LOG_V("GeometryJS -- ");
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
