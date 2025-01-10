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

#include "NodeJS.h"

#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>

#include "SceneJS.h"
void NodeJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;
    NodeImpl::GetPropertyDescs(node_props);

    napi_value func;
    auto status = napi_define_class(env, "Node", NAPI_AUTO_LENGTH, BaseObject::ctor<NodeJS>(), nullptr,
        node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    napi_get_instance_data(env, (void**)&mis);
    mis->StoreCtor("Node", func);
}

NodeJS::NodeJS(napi_env e, napi_callback_info i) : BaseObject<NodeJS>(e, i), NodeImpl(NodeImpl::NODE)
{
    LOG_V("NodeJS ++");

    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        // no arguments. so internal create.
        // expecting caller to finish initialization
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

    // java script call.. with arguments
    scene_ = fromJs.Arg<0>().valueOrDefault();
    auto scn = GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject());
    if (scn == nullptr) {
        // hmm..
        LOG_F("Invalid scene for NodeJS!");
        return;
    }
    NapiApi::Object args = fromJs.Arg<1>();

    auto obj = GetNativeObjectParam<META_NS::IObject>(args);
    if (obj) {
        StoreJsObj(obj, fromJs.This());
        return;
    }

    // collect parameters
    NapiApi::Value<BASE_NS::string> name;
    NapiApi::Value<BASE_NS::string> path;
    if (auto prm = args.Get("name")) {
        name = NapiApi::Value<BASE_NS::string>(e, prm);
    }
    if (auto prm = args.Get("path")) {
        path = NapiApi::Value<BASE_NS::string>(e, prm);
    }

    BASE_NS::string nodePath;

    if (path.IsDefined()) {
        // create using path
        nodePath = path.valueOrDefault("");
    } else if (name.IsDefined()) {
        // use the name as path (creates under root)
        nodePath = name.valueOrDefault("");
    }

    // Create actual node object.
    SCENE_NS::INode::Ptr node = scn->CreateNode(nodePath).GetResult();

    SetNativeObject(interface_pointer_cast<META_NS::IObject>(node), false);
    node.reset();
    NapiApi::Object meJs(fromJs.This());
    StoreJsObj(GetNativeObject(), meJs);

    if (name.IsDefined()) {
        // set the name of the object. if we were given one
        meJs.Set("name", name);
    }
}
NodeJS::~NodeJS()
{
    LOG_V("NodeJS --");
}
void* NodeJS::GetInstanceImpl(uint32_t id)
{
    if (id == NodeJS::ID)
        return this;
    return NodeImpl::GetInstanceImpl(id);
}

void NodeJS::DisposeNative(void*)
{
    if (!disposed_) {
        LOG_V("NodeJS::DisposeNative");
        disposed_ = true;

        NapiApi::Object obj = scene_.GetObject();
        auto* tro = obj.Native<TrueRootObject>();
        if (tro) {
            SceneJS* sceneJS = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
            if (sceneJS) {
                sceneJS->ReleaseStrongDispose(reinterpret_cast<uintptr_t>(&scene_));
            }
        }

        scene_.Reset();
    }
}
