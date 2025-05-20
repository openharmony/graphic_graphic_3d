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

#include "ParamParsing.h"
#include "SceneJS.h"

void NodeJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;
    NodeImpl::GetPropertyDescs(node_props);

    napi_value func;
    auto status = napi_define_class(env, "Node", NAPI_AUTO_LENGTH, BaseObject::ctor<NodeJS>(), nullptr,
        node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    mis->StoreCtor("Node", func);
}

NodeJS::NodeJS(napi_env e, napi_callback_info i) : BaseObject(e, i), NodeImpl(NodeImpl::NODE)
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
        if (const auto sceneJS = scene.GetJsWrapper<SceneJS>()) {
            sceneJS->StrongDisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
        }
    }

    // java script call.. with arguments
    scene_ = fromJs.Arg<0>().valueOrDefault();
    auto scn = scene_.GetObject().GetNative<SCENE_NS::IScene>();
    if (scn == nullptr) {
        // hmm..
        LOG_F("Invalid scene for NodeJS!");
        return;
    }
    if (!GetNativeObject()) {
        LOG_E("Cannot finish creating a node: Native node object missing");
        assert(false);
        return;
    }

    auto sceneNodeParameters = NapiApi::Object { fromJs.Arg<1>() };
    if (const auto name = ExtractName(sceneNodeParameters); !name.empty()) {
        fromJs.This().Set("name", name);
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

void NodeJS::DisposeNative(void* sc)
{
    if (!disposed_) {
        LOG_V("NodeJS::DisposeNative");
        disposed_ = true;

        if (auto* sceneJS = static_cast<SceneJS*>(sc)) {
            sceneJS->ReleaseStrongDispose(reinterpret_cast<uintptr_t>(&scene_));
        }

        scene_.Reset();
    }
}
void NodeJS::Finalize(napi_env env)
{
    DisposeNative(scene_.GetObject().GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
}
