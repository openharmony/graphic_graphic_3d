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

#include <scene_plugin/interface/intf_node.h>
#include <scene_plugin/interface/intf_scene.h>

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
    LOG_F("NodeJS ++");

    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        // no arguments. so internal create.
        // expecting caller to finish initialization
        return;
    }
    // java script call.. with arguments
    NapiApi::Object scene = fromJs.Arg<0>();
    scene_ = scene;
    auto scn = GetNativeMeta<SCENE_NS::IScene>(scene);

    if (scn == nullptr) {
        CORE_LOG_F("Invalid scene for NodeJS!");
        return;
    }
    NapiApi::Object args = fromJs.Arg<1>();

    auto obj = GetNativeObjectParam<META_NS::IObject>(args);
    if (obj) {
        NapiApi::Object meJs(e, fromJs.This());
        StoreJsObj(obj, meJs);
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

    if (path) {
        // create using path
        nodePath = path.valueOrDefault("");
    } else if (name) {
        // use the name as path (creates under root)
        nodePath = name.valueOrDefault("");
    } else {
        // no name or path defined should this just fail?
    }

    // Create actual node object.
    SCENE_NS::INode::Ptr node;
    ExecSyncTask([scn, nodePath, &node]() {
        node = scn->CreateNode<SCENE_NS::INode>(nodePath, true);
        return META_NS::IAny::Ptr {};
    });

    SetNativeObject(interface_pointer_cast<META_NS::IObject>(node), false);
    node.reset();
    NapiApi::Object meJs(e, fromJs.This());
    StoreJsObj(GetNativeObject(), meJs);

    if (name) {
        // set the name of the object. if we were given one
        meJs.Set("name", name);
    }
}
NodeJS::~NodeJS()
{
    LOG_F("NodeJS --");
}
void* NodeJS::GetInstanceImpl(uint32_t id)
{
    if (id == NodeJS::ID) {
        return this;
    }
    return NodeImpl::GetInstanceImpl(id);
}

void NodeJS::DisposeNative()
{
    LOG_F("NodeJS::DisposeNative");
    scene_.Reset();
}
