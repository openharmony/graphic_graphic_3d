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
#include "NodeImpl.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_containable.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <napi_api.h>
#include <scene/interface/intf_scene.h>
#include <scene/ext/component_util.h>
#include <scene/interface/component_util.h>
#include <scene/interface/intf_layer.h>
#include <scene/interface/intf_light.h>

#include <3d/ecs/components/layer_component.h>

#include "BaseObjectJS.h"
#include "JsObjectCache.h"

// NodeContainerJS

template<typename FC, napi_value (NodeContainerJS::*F)(FC&)>
static inline napi_value ECMethodI(napi_env env, napi_callback_info info)
{
    FC fc(env, info);
    if (auto value = fc.RawThis()) {
        NodeContainerJS* me = nullptr;
        napi_unwrap(env, value, (void**)&me);
        if (me) {
            return (me->*F)(fc);
        }
    }
    return fc.GetUndefined();
};

template<typename FC, napi_value (NodeContainerJS::*F)(FC&)>
static inline napi_property_descriptor ECMethod(
    const char* const name, napi_property_attributes flags = napi_default_method)
{
    return napi_property_descriptor { name, nullptr, ECMethodI<FC, F>, nullptr, nullptr, nullptr, flags, nullptr };
}

void NodeContainerJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    napi_property_descriptor props[] = { ECMethod<FunctionContext<Object>, &NodeContainerJS::AppendChild>("append"),
        ECMethod<FunctionContext<Object, Object>, &NodeContainerJS::InsertChildAfter>("insertAfter"),
        ECMethod<FunctionContext<Object>, &NodeContainerJS::RemoveChild>("remove"),
        ECMethod<FunctionContext<uint32_t>, &NodeContainerJS::GetChild>("get"),
        ECMethod<FunctionContext<>, &NodeContainerJS::ClearChildren>("clear"),
        ECMethod<FunctionContext<>, &NodeContainerJS::GetCount>("count") };
    napi_callback ctor = [](napi_env env, napi_callback_info info) -> napi_value {
        napi_value thisVar = nullptr;
        // fetch the "this" from javascript.
        napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);
        // The NodeContainerJS constructor calls napi_wrap
        auto r = BASE_NS::make_unique<NodeContainerJS>(env, info);
        r.release();
        return thisVar;
    };

    napi_value func;
    auto status = napi_define_class(
        env, "NodeContainer", NAPI_AUTO_LENGTH, ctor, nullptr, sizeof(props) / sizeof(props[0]), props, &func);
    if (status != napi_ok) {
        LOG_E("export class failed in %s", __func__);
    }

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
    if (!mis) {
        return;
    }
    mis->StoreCtor("NodeContainer", func);
}

NodeContainerJS::NodeContainerJS(napi_env e, napi_callback_info i)
{
    LOG_V("NodeContainerJS ++");
    napi_value thisVar = nullptr;
    napi_get_cb_info(e, i, nullptr, nullptr, &thisVar, nullptr);
    napi_wrap(e, thisVar, reinterpret_cast<void*>((NodeContainerJS*)this), nullptr, nullptr, nullptr);

    if (auto fromJs = NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>(e, i)) {
        node_ = NapiApi::Object(fromJs.Arg<0>());
        scene_ = NapiApi::Object(fromJs.Arg<1>());
    }
}

NodeContainerJS::~NodeContainerJS()
{
    LOG_V("NodeContainerJS --");
    scene_.Reset();
    node_.Reset();
}

NodeImpl* NodeContainerJS::GetNode()
{
    auto object = node_.GetNapiObject();
    if (object.IsDefined()) {
        if (auto root = object.GetRoot()) {
            return static_cast<NodeImpl*>(root->GetInstanceImpl(NodeImpl::ID));    
        }
    }
    return nullptr;
}

napi_value NodeContainerJS::GetCount(NapiApi::FunctionContext<>& ctx)
{
    if (auto* node = GetNode()) {
        auto nodeCtx = ctx.SwitchJSTarget(node_.GetValue());
        return node->GetCount(nodeCtx);
    }
    return ctx.GetUndefined();
}

napi_value NodeContainerJS::GetChild(NapiApi::FunctionContext<uint32_t>& ctx)
{
    if (auto* node = GetNode()) {
        auto nodeCtx = ctx.SwitchJSTarget(node_.GetValue());
        return node->GetChild(nodeCtx);
    }
    return ctx.GetUndefined();
}

napi_value NodeContainerJS::ClearChildren(NapiApi::FunctionContext<>& ctx)
{
    if (auto* node = GetNode()) {
        auto nodeCtx = ctx.SwitchJSTarget(node_.GetValue());
        return node->ClearChildren(nodeCtx);
    }
    return ctx.GetUndefined();
}

napi_value NodeContainerJS::InsertChildAfter(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    if (auto* node = GetNode()) {
        auto nodeCtx = ctx.SwitchJSTarget(node_.GetValue());
        return node->InsertChildAfter(nodeCtx);
    }
    return ctx.GetUndefined();
}

napi_value NodeContainerJS::AppendChild(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (auto* node = GetNode()) {
        auto nodeCtx = ctx.SwitchJSTarget(node_.GetValue());
        return node->AppendChild(nodeCtx);
    }
    return ctx.GetUndefined();
}

napi_value NodeContainerJS::RemoveChild(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (auto* node = GetNode()) {
        auto nodeCtx = ctx.SwitchJSTarget(node_.GetValue());
        return node->RemoveChild(nodeCtx);
    }
    return ctx.GetUndefined();
}

// NodeImpl

void NodeImpl::RegisterEnums(NapiApi::Object exports)
{
    napi_value v;
    NapiApi::Object NodeType(exports.GetEnv());
#define DECL_ENUM(enu, x)                                            \
    {                                                                \
        napi_create_uint32(enu.GetEnv(), NodeImpl::NodeType::x, &v); \
        enu.Set(#x, v);                                              \
    }
    DECL_ENUM(NodeType, NODE);
    DECL_ENUM(NodeType, GEOMETRY);
    DECL_ENUM(NodeType, CAMERA);
    DECL_ENUM(NodeType, LIGHT);
    DECL_ENUM(NodeType, TEXT);
    DECL_ENUM(NodeType, CUSTOM);
#undef DECL_ENUM
    exports.Set("NodeType", NodeType);
}
NodeImpl::NodeImpl(NodeType type) : SceneResourceImpl(SceneResourceImpl::NODE), type_(type)
{
    LOG_V("NodeImpl ++");
}
NodeImpl::~NodeImpl()
{
    LOG_V("NodeImpl --");
    posProxy_.reset();
    sclProxy_.reset();
    rotProxy_.reset();
}
void* NodeImpl::GetInstanceImpl(uint32_t id)
{
    if (id == NodeImpl::ID) {
        return static_cast<NodeImpl*>(this);
    }
    return SceneResourceImpl::GetInstanceImpl(id);
}
void NodeImpl::GetPropertyDescs(BASE_NS::vector<napi_property_descriptor>& props)
{
    using namespace NapiApi;
    // META_NS::IContainer;

    SceneResourceImpl::GetPropertyDescs(props);

    // override "name" from sceneresource
    for (auto it = props.begin(); it != props.end(); it++) {
        if (BASE_NS::string_view("name").compare(it->utf8name) == 0) {
            props.erase(it);
            break;
        }
    }
    props.push_back(
        TROGetSetProperty<BASE_NS::string, NodeImpl, &NodeImpl::GetNodeName, &NodeImpl::SetNodeName>("name"));

    // node

    props.push_back(TROGetProperty<BASE_NS::string, NodeImpl, &NodeImpl::GetPath>("path"));
    props.push_back(TROGetSetProperty<Object, NodeImpl, &NodeImpl::GetPosition, &NodeImpl::SetPosition>("position"));
    props.push_back(TROGetSetProperty<Object, NodeImpl, &NodeImpl::GetRotation, &NodeImpl::SetRotation>("rotation"));
    props.push_back(TROGetSetProperty<Object, NodeImpl, &NodeImpl::GetScale, &NodeImpl::SetScale>("scale"));
    props.push_back(TROGetProperty<BASE_NS::string, NodeImpl, &NodeImpl::GetParent>("parent"));
    props.push_back(TROGetSetProperty<bool, NodeImpl, &NodeImpl::GetVisible, &NodeImpl::SetVisible>("visible"));
    props.push_back(TROGetProperty<bool, NodeImpl, &NodeImpl::GetChildContainer>("children"));
    props.push_back(TROGetProperty<BASE_NS::string, NodeImpl, &NodeImpl::GetNodeType>("nodeType"));
    props.push_back(TROGetProperty<BASE_NS::string, NodeImpl, &NodeImpl::GetLayerMask>("layerMask"));

    // node methods
    props.push_back(MakeTROMethod<FunctionContext<>, NodeImpl, &NodeImpl::Dispose>("destroy"));
    props.push_back(
        MakeTROMethod<FunctionContext<BASE_NS::string>, NodeImpl, &NodeImpl::GetNodeByPath>("getNodeByPath"));
    props.push_back(
        MakeTROMethod<FunctionContext<BASE_NS::string>, NodeImpl, &NodeImpl::GetComponent>("getComponent"));

    // layermask methods.
    props.push_back(MakeTROMethod<FunctionContext<uint32_t>, NodeImpl, &NodeImpl::GetLayerMaskEnabled>("getEnabled"));
    props.push_back(
        MakeTROMethod<FunctionContext<uint32_t, bool>, NodeImpl, &NodeImpl::SetLayerMaskEnabled>("setEnabled"));

    // container
#ifndef NODE_CONTAINER_WORKS
     //Should not be needed anymore as NodeContainerJS handles container interface
    props.push_back(MakeTROMethod<FunctionContext<Object>, NodeImpl, &NodeImpl::AppendChild>("append"));
    props.push_back(
        MakeTROMethod<FunctionContext<Object, Object>, NodeImpl, &NodeImpl::InsertChildAfter>("insertAfter"));
    props.push_back(MakeTROMethod<FunctionContext<Object>, NodeImpl, &NodeImpl::RemoveChild>("remove"));
    props.push_back(MakeTROMethod<FunctionContext<uint32_t>, NodeImpl, &NodeImpl::GetChild>("get"));
    props.push_back(MakeTROMethod<FunctionContext<>, NodeImpl, &NodeImpl::ClearChildren>("clear"));
    props.push_back(MakeTROMethod<FunctionContext<>, NodeImpl, &NodeImpl::GetCount>("count"));
#endif	

    // Internal methods
    props.push_back(
        MakeTROMethod<FunctionContext<uint32_t>, NodeImpl, &NodeImpl::SetNodeTypeInternal>("setNodeTypeInternal"));
}
napi_value NodeImpl::Dispose(NapiApi::FunctionContext<>& ctx)
{
    // Dispose of the native object. (makes the js object invalid)
    children_.Reset();
    posProxy_.reset();
    sclProxy_.reset();
    rotProxy_.reset();
    SceneResourceImpl::Dispose(ctx);
    scene_.Reset();
    return ctx.GetUndefined();
}

constexpr uint32_t MAX_LAYER_BIT = 63; // Max bit index for uint64_t bitmask

napi_value NodeImpl::GetLayerMaskEnabled(NapiApi::FunctionContext<uint32_t>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    uint32_t bit = ctx.Arg<0>();
    bool enabled = true;
    if (bit > MAX_LAYER_BIT) {
        return ctx.GetBoolean(false);
    }
    if (auto node = ctx.This().GetNative<SCENE_NS::INode>()) {
        uint64_t mask = 1ull << bit;
        if (auto comp = SCENE_NS::GetComponent<SCENE_NS::ILayer>(node)) {
            enabled = comp->LayerMask()->GetValue() & mask;
        }
    }
    return ctx.GetBoolean(enabled);
}
napi_value NodeImpl::SetLayerMaskEnabled(NapiApi::FunctionContext<uint32_t, bool>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    uint32_t bit = ctx.Arg<0>();
    if (bit > MAX_LAYER_BIT) {
        return ctx.GetUndefined();
    }
    bool enabled = ctx.Arg<1>();
    if (auto node = ctx.This().GetNative<SCENE_NS::INode>()) {
        if (!SCENE_NS::GetComponent<SCENE_NS::ILayer>(node)) {
            SCENE_NS::AddComponent<CORE3D_NS::ILayerComponentManager>(node);
        }
        if (auto layer = SCENE_NS::GetComponent<SCENE_NS::ILayer>(node)) {
            uint64_t mask = 1ull << bit;
            if (enabled) {
                layer->LayerMask()->SetValue(layer->LayerMask()->GetValue() | mask);
            } else {
                layer->LayerMask()->SetValue(layer->LayerMask()->GetValue() & ~mask);
            }
        }
    }
    return ctx.GetUndefined();
}

napi_value NodeImpl::SetNodeTypeInternal(NapiApi::FunctionContext<uint32_t>& ctx)
{
    if (!validateSceneRef()) {
        return {};
    }

    if (ctx.This().GetNative<SCENE_NS::INode>()) {
        type_ = static_cast<NodeType>(ctx.Arg<0>().valueOrDefault());
    }

    return {};
}

napi_value NodeImpl::GetNodeType(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    uint32_t type = -1; // return -1 if the object does not exist anymore
    if (ctx.This().GetNative<SCENE_NS::INode>()) {
        type = type_;
    }
    return ctx.GetNumber(type);
}

napi_value NodeImpl::GetLayerMask(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    if (const auto nodeJs = ctx.This(); nodeJs.GetNative<SCENE_NS::INode>()) {
        return nodeJs.ToNapiValue();
    }
    return ctx.GetUndefined();
}

napi_value NodeImpl::GetNodeName(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    BASE_NS::string name;
    auto native = ctx.This().GetNative();
    auto node = interface_pointer_cast<SCENE_NS::INode>(native);
    if (!node) {
        LOG_E("NodeImpl not a node!");
        return ctx.GetUndefined();
    }
    name = native->GetName();

    // LEGACY COMPATIBILITY start
    auto parent = node->GetParent().GetResult();
    if (!parent) {
        //parentless node, "root"
        if (name.empty()) {
            // no name, and no parent.
            // so return the legacy default name
            name = "rootNode_";
        }
    }
    // LEGACY COMPATIBILITY end

    return ctx.GetString(name);
}
void NodeImpl::SetNodeName(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    BASE_NS::string name = ctx.Arg<0>();
    if (auto node = ctx.This().GetNative<META_NS::INamed>()) {
        node->Name()->SetValue(name);
    } else {
        LOG_W("renaming resource not implemented, trying to name (%d) to '%s'", type_, name.c_str());
    }
}

napi_value NodeImpl::GetPath(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto node = ctx.This().GetNative<SCENE_NS::INode>();
    if (!node) {
        // node does not exist anymore?
        return ctx.GetUndefined();
    }

    auto parent = node->GetParent().GetResult();
    if (!parent) {
        // parentless nodes are "root", and root nodes path is ofcourse empty.
        return ctx.GetString("");
    }
    BASE_NS::string path = node->GetPath().GetResult();

    // LEGACY COMPATIBILITY start
    // get root from scene..
    NapiApi::Object obj = scene_.GetNapiObject();
    NapiApi::Object root = obj.Get<NapiApi::Object>("root");
    // get the name of the root..
    BASE_NS::string rootName = root.Get<BASE_NS::string>("name");

    // remove node name from path.
    path = path.substr(0, path.find_last_of('/') + 1);
    // make sure root node name is there.. (hack)

    // see if root name is in the path.
    auto pos = path.find(rootName);
    if (pos == BASE_NS::string::npos) {
        // rootname missing from path.
        path.insert(1, rootName.c_str());
    }
    // LEGACY COMPATIBILITY end
    return ctx.GetString(path);
}

napi_value NodeImpl::GetVisible(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto node = ctx.This().GetNative<SCENE_NS::INode>();
    bool visible = false;
    if (node) {
        visible = node->Enabled()->GetValue();
    }
    return ctx.GetBoolean(visible);
}
void NodeImpl::SetVisible(NapiApi::FunctionContext<bool>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    auto node = ctx.This().GetNative<SCENE_NS::INode>();
    if (node) {
        bool visible = ctx.Arg<0>();
        node->Enabled()->SetValue(visible);
    }
}

napi_value NodeImpl::GetPosition(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto node = ctx.This().GetNative<SCENE_NS::INode>();
    if (!node) {
        return ctx.GetUndefined();
    }
    if (posProxy_ == nullptr) {
        posProxy_ = BASE_NS::make_unique<Vec3Proxy>(ctx.Env(), node->Position());
    }
    return posProxy_->Value();
}

void NodeImpl::SetPosition(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    auto node = ctx.This().GetNative<SCENE_NS::INode>();
    if (!node) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (posProxy_ == nullptr) {
        posProxy_ = BASE_NS::make_unique<Vec3Proxy>(ctx.Env(), node->Position());
    }
    posProxy_->SetValue(obj);
}

napi_value NodeImpl::GetScale(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto node = ctx.This().GetNative<SCENE_NS::INode>();
    if (!node) {
        return ctx.GetUndefined();
    }
    if (sclProxy_ == nullptr) {
        sclProxy_ = BASE_NS::make_unique<Vec3Proxy>(ctx.Env(), node->Scale());
    }
    return sclProxy_->Value();
}

void NodeImpl::SetScale(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    auto node = ctx.This().GetNative<SCENE_NS::INode>();
    if (!node) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (sclProxy_ == nullptr) {
        sclProxy_ = BASE_NS::make_unique<Vec3Proxy>(ctx.Env(), node->Scale());
    }
    sclProxy_->SetValue(obj);
}

napi_value NodeImpl::GetRotation(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto node = ctx.This().GetNative<SCENE_NS::INode>();
    if (!node) {
        return ctx.GetUndefined();
    }
    if (rotProxy_ == nullptr) {
        rotProxy_ = BASE_NS::make_unique<QuatProxy>(ctx.Env(), node->Rotation());
    }
    return rotProxy_->Value();
}

void NodeImpl::SetRotation(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    auto node = ctx.This().GetNative<SCENE_NS::INode>();
    if (!node) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (rotProxy_ == nullptr) {
        rotProxy_ = BASE_NS::make_unique<QuatProxy>(ctx.Env(), node->Rotation());
    }
    rotProxy_->SetValue(obj);
}

napi_value NodeImpl::GetParent(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto node = ctx.This().GetNative<SCENE_NS::INode>();
    if (!node) {
        return ctx.GetNull();
    }

    auto parent = node->GetParent().GetResult();
    if (!parent) {
        // no parent. (root node)
        return ctx.GetNull();
    }

    NapiApi::Env env(ctx.GetEnv());
    NapiApi::Object argJS(env);
    napi_value args[] = { scene_.GetValue(), argJS.ToNapiValue() };

    // These are owned by the scene, so store only a weak reference.
    auto js = CreateFromNativeInstance(env, parent, PtrType::WEAK, args);
    if (auto wrapper = js.GetJsWrapper<NodeImpl>()) {
        wrapper->Attached(true);
    }
    return js.ToNapiValue();
}

napi_value NodeImpl::GetChildContainer(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
#ifndef NODE_CONTAINER_WORKS
    return ctx.This().ToNapiValue();
#else
    if (children_.IsEmpty()) {
        auto meJs = ctx.This();
        if (auto me = meJs.GetNative<SCENE_NS::INode>()) {
            NapiApi::Env env(ctx.Env());
            napi_value args[] = { meJs.ToNapiValue(), scene_.GetValue() };
            children_ = NapiApi::StrongRef { NapiApi::Object { env, "NodeContainer", args } };
        }
    }
    return children_.GetValue();
#endif
}

napi_value NodeImpl::GetChild(NapiApi::FunctionContext<uint32_t>& ctx)
{
    if (!validateSceneRef()) {
        LOG_F("INVALID SCENE!");
        return ctx.GetUndefined();
    }

    uint32_t index = ctx.Arg<0>();
    if (auto node = ctx.This().GetNative<SCENE_NS::INode>()) {
        auto children = node->GetChildren().GetResult();
        if (index < children.size()) {
            const auto env = ctx.GetEnv();
            auto argJS = NapiApi::Object { env };
            napi_value args[] = { scene_.GetValue(), argJS.ToNapiValue() };
            // These are owned by the scene, so store only weak reference.
            auto js = CreateFromNativeInstance(env, children[index], PtrType::WEAK, args);
            if (auto wrapper = js.GetJsWrapper<NodeImpl>()) {
                wrapper->Attached(true);
            }
            return js.ToNapiValue();
        }
    }
    return ctx.GetNull();
}

napi_value NodeImpl::GetCount(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    uint32_t count = 0;
    if (auto node = ctx.This().GetNative<SCENE_NS::INode>()) {
        count = node->GetChildren().GetResult().size();
    }
    return ctx.GetNumber(count);
}
napi_value NodeImpl::AppendChild(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    auto arg0 = ctx.Arg<0>();
    if (arg0.IsUndefinedOrNull()) {
        // okay. Invalid arg error?
        return ctx.GetUndefined();
    }

    NapiApi::Object childJS = arg0;
    auto childNode = childJS.GetNative<SCENE_NS::INode>();
    if (!childNode) {
        return ctx.GetUndefined();
    }
    if (auto parent = ctx.This().GetNative<SCENE_NS::INode>()) {
        if (childNode->GetScene() != parent->GetScene()) {
            LOG_E("!!!!!!! Cannot add child from another scene");
            return ctx.GetUndefined();
        }

       if (auto wrapper = childJS.GetJsWrapper<NodeImpl>()) {
            wrapper->Attached(true);
        }
        parent->AddChild(childNode);
        childNode->Enabled()->SetValue(true);
    }

    // make the js object keep a weak ref again (scene keeps the native object alive)
    // (or move ownership back from SceneJS? and remove dispose hook?)
    if (auto tro = childJS.GetRoot()) {
        if (auto native = tro->GetNativeObject()) {
            tro->SetNativeObject(native, PtrType::WEAK);
        }
    }

    return ctx.GetUndefined();
}
napi_value NodeImpl::InsertChildAfter(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    auto arg0 = ctx.Arg<0>();
    if (arg0.IsUndefinedOrNull()) {
        return ctx.GetUndefined();
    }
    NapiApi::Object childJS = arg0;
    auto childNode = childJS.GetNative<SCENE_NS::INode>();
    if (!childNode) {
        return ctx.GetUndefined();
    }
    auto arg1 = ctx.Arg<1>();
    SCENE_NS::INode::Ptr siblingNode;
    if (arg1.IsDefinedAndNotNull()) {
        NapiApi::Object siblingJS = arg1;
        siblingNode = siblingJS.GetNative<SCENE_NS::INode>();
    }
    if (auto parent = ctx.This().GetNative<SCENE_NS::INode>()) {
        if (childNode->GetScene() != parent->GetScene()) {
            LOG_E("Cannot add child from another scene");
            return ctx.GetUndefined();
        }
        size_t index = 0;
        if (siblingNode) {
            auto data = parent->GetChildren().GetResult();
            for (auto d : data) {
                index++;
                if (d == siblingNode) {
                    break;
                }
            }
        }
        if (auto wrapper = childJS.GetJsWrapper<NodeImpl>()) {
            wrapper->Attached(true);
        }
        parent->AddChild(childNode, index).GetResult();
        childNode->Enabled()->SetValue(true);
    }
    // make the js object keep a weak ref again (scene keeps the native object alive)
    if (auto tro = childJS.GetRoot()) {
        if (auto native = tro->GetNativeObject()) {
            tro->SetNativeObject(native, PtrType::WEAK);
        }
    }
    return ctx.GetUndefined();
}

napi_value NodeImpl::RemoveChild(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    // okay. just detach from parent. (make parent invalid)
    // and disable it.
    auto arg0 = ctx.Arg<0>();
    if (arg0.IsUndefinedOrNull()) {
        // okay. Invalid arg error?
        return ctx.GetUndefined();
    }
    NapiApi::Object childJS = arg0;

    auto childNode = childJS.GetNative<SCENE_NS::INode>();
    if (!childNode) {
        return ctx.GetUndefined();
    }

    // make the js object keep a strong ref.
    // (or give SceneJS ownership? and add dispose hook?)
    if (auto tro = childJS.GetRoot()) {
        if (auto native = tro->GetNativeObject()) {
            tro->SetNativeObject(native, PtrType::STRONG);
        }
    }

    if (auto parent = ctx.This().GetNative<SCENE_NS::INode>()) {
        if (childNode->GetScene() != parent->GetScene()) {
            LOG_E("!!!!!!! Cannot add child from another scene");
            return ctx.GetUndefined();
        }
        if (auto wrapper = childJS.GetJsWrapper<NodeImpl>()) {
            wrapper->Attached(false);
        }
        parent->RemoveChild(childNode).GetResult();
        childNode->Enabled()->SetValue(false);
    }
    return ctx.GetUndefined();
}

napi_value NodeImpl::ClearChildren(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    BASE_NS::vector<SCENE_NS::INode::Ptr> removedNodes;
    if (auto parent = ctx.This().GetNative<SCENE_NS::INode>()) {
        for (auto node : parent->GetChildren().GetResult()) {
            if (auto childJS = FetchJsObj(node)) {
                if (auto wrapper = childJS.GetJsWrapper<NodeImpl>()) {
                    wrapper->Attached(false);
                }
            }
            parent->RemoveChild(node).GetResult();
            node->Enabled()->SetValue(false);
            removedNodes.emplace_back(BASE_NS::move(node));
        }

        for (auto node : removedNodes) {
            if (auto cached = FetchJsObj(node)) {
                if (auto tro = cached.GetRoot()) {
                    if (auto native = tro->GetNativeObject()) {
                        tro->SetNativeObject(native, PtrType::STRONG);
                    }
                }
            }
        }
    }
    return ctx.GetUndefined();
}

napi_value NodeImpl::GetNodeByPath(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    if (!validateSceneRef()) {
        LOG_F("INVALID SCENE!");
        return ctx.GetUndefined();
    }

    BASE_NS::string path = ctx.Arg<0>();
    if (auto node = ctx.This().GetNative<SCENE_NS::INode>()) {
        BASE_NS::string childPath = node->GetPath().GetResult() + "/" + path;
        const auto child = node->GetScene()->FindNode(childPath).GetResult();
        if (!child) {
            CORE_LOG_E("Child is null, path is %s", path.c_str());
            return ctx.GetNull();
        }
        const auto env = ctx.GetEnv();
        auto argJS = NapiApi::Object { env };
        napi_value args[] = { scene_.GetValue(), argJS.ToNapiValue() };
        // These are owned by the scene, so store only weak reference.
        auto js = CreateFromNativeInstance(env, child, PtrType::WEAK, args);
        if (js.GetJsWrapper<NodeImpl>()) {
            js.GetJsWrapper<NodeImpl>()->Attached(true);
            return js.ToNapiValue();
        }
    }
    LOG_E("This node is invalid, path is %s", path.c_str());
    return ctx.GetNull();
}

napi_value NodeImpl::GetComponent(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    BASE_NS::string name = ctx.Arg<0>();
    auto meta = ctx.This().GetNative();
    if (!meta) {
        return ctx.GetNull();
    }

    if (auto att = interface_pointer_cast<META_NS::IAttach>(meta)) {
        if (auto cont = att->GetAttachmentContainer(false)) {
            if (auto comp = cont->FindByName<SCENE_NS::IComponent>(name)) {
                NapiApi::Env env(ctx.GetEnv());
                NapiApi::Object argJS(env);
                napi_value args[] = { scene_.GetValue(), argJS.ToNapiValue() };
                return CreateFromNativeInstance(env, comp, PtrType::WEAK, args).ToNapiValue();
            }
        }
    }
    return ctx.GetUndefined();
}

bool NodeImpl::IsAttached() const
{
    return attached_;
}
void NodeImpl::Attached(bool attached)
{
    attached_ = attached;
}

void CleanupNode(TrueRootObject* bo, bool isAttached)
{
    if (!bo) {
        LOG_E("#### CleanupNode: null root object");
        return;
    }
    auto node = bo->GetNativeObject<SCENE_NS::INode>();
    if (!node) {
        LOG_V("#### CleanupNode: null native object, attached %d", isAttached);
        return;
    }
    ExecSyncTask([remove = !isAttached, bo, node = BASE_NS::move(node)]() mutable {
        BASE_NS::weak_ptr wp = node;
        bo->UnsetNativeObject();
        if (auto scene = node->GetScene()) {
            if (remove) {
                // fully destroy the node
                scene->RemoveNode(BASE_NS::move(node)).Wait();
                node.reset();
            } else {
                // just release the cache
                scene->ReleaseNode(BASE_NS::move(node), false).Wait();
            }
        }
        return META_NS::IAny::Ptr {};
    });
}
