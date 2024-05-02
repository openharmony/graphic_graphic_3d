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
#include <scene_plugin/api/camera_uid.h>
#include <scene_plugin/interface/intf_camera.h>
#include <scene_plugin/interface/intf_scene.h>

void* GeometryJS::GetInstanceImpl(uint32_t id)
{
    if (id == GeometryJS::ID) {
        return this;
    }
    return NodeImpl::GetInstanceImpl(id);
}
void GeometryJS::DisposeNative()
{
    // do nothing for now..
    CORE_LOG_F("GeometryJS::DisposeNative");
    if (auto camera = interface_pointer_cast<SCENE_NS::ICamera>(GetNativeObject())) {
        // reset the native object refs
        SetNativeObject(nullptr, false);
        SetNativeObject(nullptr, true);

        ExecSyncTask([cam = BASE_NS::move(camera)]() mutable {
            auto camnode = interface_pointer_cast<SCENE_NS::INode>(cam);
            if (camnode == nullptr) {
                return META_NS::IAny::Ptr {};
            }
            auto scene = camnode->GetScene();
            if (scene == nullptr) {
                return META_NS::IAny::Ptr {};
            }
            scene->DeactivateCamera(cam);
            scene->ReleaseNode(camnode);
            return META_NS::IAny::Ptr {};
        });
    }
    scene_.Reset();
}
void GeometryJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;
    NodeImpl::GetPropertyDescs(node_props);

    using namespace NapiApi;
    node_props.push_back(GetSetProperty<Object, GeometryJS, &GeometryJS::GetMesh, &GeometryJS::SetMesh>("mesh"));

    napi_value func;
    auto status = napi_define_class(env, "Geometry", NAPI_AUTO_LENGTH, BaseObject::ctor<GeometryJS>(), nullptr,
        node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    napi_get_instance_data(env, (void**)&mis);
    mis->StoreCtor("Geometry", func);
}

GeometryJS::GeometryJS(napi_env e, napi_callback_info i) : BaseObject<GeometryJS>(e, i), NodeImpl(NodeImpl::GEOMETRY)
{
    LOG_F("GeometryJS ++ ");
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    // java script call.. with arguments
    NapiApi::Object scene = fromJs.Arg<0>();
    if (!fromJs) {
        // okay internal create. we will receive the object after.
        return;
    }
    scene_ = scene;
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        CORE_LOG_F("INVALID SCENE!");
    }

    // Creating new object...  (so we must have scene etc)
    auto scn = GetNativeMeta<SCENE_NS::IScene>(scene);

    if (scn == nullptr) {
        CORE_LOG_F("Invalid scene for GeometryJS!");
        return;
    }
    // missing actual creation of geometry node. (refer to camerajs etc)
}
GeometryJS::~GeometryJS()
{
    LOG_F("GeometryJS -- ");
}

napi_value GeometryJS::GetMesh(NapiApi::FunctionContext<>& ctx)
{
    META_NS::IObject::Ptr mesh;
    if (auto geom = interface_pointer_cast<SCENE_NS::INode>(GetNativeObject())) {
        ExecSyncTask([geom, &mesh]() {
            mesh = interface_pointer_cast<META_NS::IObject>(geom->Mesh()->GetValue());
            return META_NS::IAny::Ptr {};
        });
    }

    if (!mesh) {
        // no parent.
        napi_value res;
        napi_get_null(ctx, &res);
        return res;
    }

    if (auto cached = FetchJsObj(mesh)) {
        // always return the same js object.
        return cached;
    }
    auto uid = mesh->GetClassId();
    auto name = mesh->GetClassName();
    // create new js object for the native node.
    NapiApi::Object argJS(ctx);
    napi_value args[] = { scene_.GetValue(), argJS };

    return CreateFromNativeInstance(ctx, mesh, false /*these are owned by the scene*/, BASE_NS::countof(args), args);
}

void GeometryJS::SetMesh(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
}