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
    if (id == NodeImpl::ID)
        return this;
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
    props.push_back(TROGetSetProperty<bool, NodeImpl, &NodeImpl::GetChildContainer, &NodeImpl::SetVisible>("children"));
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
    props.push_back(MakeTROMethod<FunctionContext<Object>, NodeImpl, &NodeImpl::AppendChild>("append"));
    props.push_back(
        MakeTROMethod<FunctionContext<Object, Object>, NodeImpl, &NodeImpl::InsertChildAfter>("insertAfter"));
    props.push_back(MakeTROMethod<FunctionContext<Object>, NodeImpl, &NodeImpl::RemoveChild>("remove"));
    props.push_back(MakeTROMethod<FunctionContext<uint32_t>, NodeImpl, &NodeImpl::GetChild>("get"));
    props.push_back(MakeTROMethod<FunctionContext<>, NodeImpl, &NodeImpl::ClearChildren>("clear"));
    props.push_back(MakeTROMethod<FunctionContext<>, NodeImpl, &NodeImpl::GetCount>("count"));
}
napi_value NodeImpl::Dispose(NapiApi::FunctionContext<>& ctx)
{
    // Dispose of the native object. (makes the js object invalid)
    posProxy_.reset();
    sclProxy_.reset();
    rotProxy_.reset();
    SceneResourceImpl::Dispose(ctx);
    scene_.Reset();
    return ctx.GetUndefined();
}
napi_value NodeImpl::GetLayerMaskEnabled(NapiApi::FunctionContext<uint32_t>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    uint32_t bit = ctx.Arg<0>();
    bool enabled = true;
    if (auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx))) {
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
    bool enabled = ctx.Arg<1>();
    if (auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx))) {
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

napi_value NodeImpl::GetNodeType(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    uint32_t type = -1; // return -1 if the object does not exist anymore
    if (auto node = interface_cast<SCENE_NS::INode>(GetThisNativeObject(ctx))) {
        type = type_;
    }
    return ctx.GetNumber(type);
}

napi_value NodeImpl::GetLayerMask(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    if (auto node = interface_cast<SCENE_NS::INode>(GetThisNativeObject(ctx))) {
        return ctx.This().ToNapiValue();
    }
    return ctx.GetUndefined();
}

napi_value NodeImpl::GetNodeName(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    BASE_NS::string name;
    auto native = GetThisNativeObject(ctx);
    auto object = interface_pointer_cast<META_NS::IObject>(native);
    auto node = interface_pointer_cast<SCENE_NS::INode>(native);
    if (!object || !node) {
        LOG_E("NodeImpl not a node! %p %p %p", native.get(), object.get(), node.get());
        return ctx.GetUndefined();
    }
    name = object->GetName();

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
    if (auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx))) {
        node->SetName(name).Wait();
    } else {
        LOG_W("renaming resource not implemented, trying to name (%d) to '%s'", type_, name.c_str());
    }
}

napi_value NodeImpl::GetPath(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
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
    NapiApi::Object obj = scene_.GetObject();
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

    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
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

    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
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

    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
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

    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
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

    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
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

    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
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

    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
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

    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
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

    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    if (!node) {
        return ctx.GetNull();
    }

    auto parent = node->GetParent().GetResult();
    if (!parent) {
        // no parent. (root node)
        return ctx.GetNull();
    }

    if (auto cached = FetchJsObj(parent)) {
        // always return the same js object.
        return cached.ToNapiValue();
    }
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        LOG_F("INVALID SCENE!");
    }

    // create new js object for the native node.
    NapiApi::Env env(ctx.GetEnv());
    NapiApi::Object argJS(env);
    napi_value args[] = { scene_.GetValue(), argJS.ToNapiValue() };

    auto js = CreateFromNativeInstance(env, interface_pointer_cast<META_NS::IObject>(parent),
        false /*these are owned by the scene*/, BASE_NS::countof(args), args);
    if (auto nm = GetJsWrapper<NodeImpl>(js)) {
        nm->Attached(true);
    }
    return js.ToNapiValue();
}

napi_value NodeImpl::GetChildContainer(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    // Node implements Container<Node>
    return ctx.This().ToNapiValue();
}

napi_value NodeImpl::GetChild(NapiApi::FunctionContext<uint32_t>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    uint32_t index = ctx.Arg<0>();
    META_NS::IObject::Ptr child;
    if (auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx))) {
        auto children = node->GetChildren().GetResult();
        if (index < children.size()) {
            child = interface_pointer_cast<META_NS::IObject>(children[index]);
        }
    }
    if (!child) {
        // return null
        return ctx.GetNull();
    }
    auto cached = FetchJsObj(child);
    if (!cached) {
        // was not cached yet, so recreate.
        NapiApi::Env env(ctx.GetEnv());
        NapiApi::Object argJS(env);
        auto scn = scene_.GetObject();
        if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
            LOG_F("INVALID SCENE!");
        }

        napi_value args[] = { scene_.GetValue(), argJS.ToNapiValue() };

        cached =
            CreateFromNativeInstance(env, child, false /*these are owned by the scene*/, BASE_NS::countof(args), args);
    }
    if (auto nm = GetJsWrapper<NodeImpl>(cached)) {
        nm->Attached(true);
    }
    return cached.ToNapiValue();
}

napi_value NodeImpl::GetCount(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    uint32_t count = 0;
    if (auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx))) {
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

    auto childNode = GetNativeMeta<SCENE_NS::INode>(childJS);
    if (!childNode) {
        return ctx.GetUndefined();
    }

    auto metaobj = GetThisNativeObject(ctx);
    if (auto parent = interface_cast<SCENE_NS::INode>(metaobj)) {
        if (auto nm = GetJsWrapper<NodeImpl>(childJS)) {
            nm->Attached(true);
        }
        parent->AddChild(childNode);
        childNode->Enabled()->SetValue(true);
    }

    // make the js object keep a weak ref again (scene keeps the native object alive)
    // (or move ownership back from SceneJS? and remove dispose hook?)
    if (auto tro = GetRootObject(childJS)) {
        if (auto native = tro->GetNativeObject()) {
            tro->SetNativeObject(nullptr, true);
            tro->SetNativeObject(native, false);
            native.reset();
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
        // okay. Invalid arg error?
        return ctx.GetUndefined();
    }

    NapiApi::Object childJS = arg0;
    auto childNode = GetNativeMeta<SCENE_NS::INode>(childJS);
    if (!childNode) {
        return ctx.GetUndefined();
    }

    auto arg1 = ctx.Arg<1>();
    SCENE_NS::INode::Ptr siblingNode;
    if (arg1.IsDefinedAndNotNull()) {
        NapiApi::Object siblingJS = arg1;
        siblingNode = GetNativeMeta<SCENE_NS::INode>(siblingJS);
    }

    auto metaobj = GetThisNativeObject(ctx);
    if (auto parent = interface_cast<SCENE_NS::INode>(metaobj)) {
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
        if (auto nm = GetJsWrapper<NodeImpl>(childJS)) {
            nm->Attached(true);
        }
        parent->AddChild(childNode, index).GetResult();
        childNode->Enabled()->SetValue(true);
    }

    // make the js object keep a weak ref again (scene keeps the native object alive)
    // (or move ownership back from SceneJS? and remove dispose hook?)
    if (auto tro = GetRootObject(childJS)) {
        if (auto native = tro->GetNativeObject()) {
            tro->SetNativeObject(nullptr, true);
            tro->SetNativeObject(native, false);
            native.reset();
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

    auto childNode = GetNativeMeta<SCENE_NS::INode>(childJS);
    if (!childNode) {
        return ctx.GetUndefined();
    }

    // make the js object keep a strong ref.
    // (or give SceneJS ownership? and add dispose hook?)
    if (auto tro = GetRootObject(childJS)) {
        if (auto native = tro->GetNativeObject()) {
            tro->SetNativeObject(nullptr, false);
            tro->SetNativeObject(native, true);
            native.reset();
        }
    }

    auto metaobj = GetThisNativeObject(ctx);
    if (auto parent = interface_cast<SCENE_NS::INode>(metaobj)) {
        if (auto nm = GetJsWrapper<NodeImpl>(childJS)) {
            nm->Attached(false);
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

    auto metaobj = GetThisNativeObject(ctx);
    BASE_NS::vector<SCENE_NS::INode::Ptr> removedNodes;
    if (auto parent = interface_cast<SCENE_NS::INode>(metaobj)) {
        for (auto node : parent->GetChildren().GetResult()) {
            if (auto childJS = FetchJsObj(node)) {
                if (auto nm = GetJsWrapper<NodeImpl>(childJS)) {
                    nm->Attached(false);
                }
            }
            parent->RemoveChild(node).GetResult();
            node->Enabled()->SetValue(false);
            removedNodes.emplace_back(BASE_NS::move(node));
        }

        for (auto node : removedNodes) {
            if (auto cached = FetchJsObj(node)) {
                if (auto tro = GetRootObject(cached)) {
                    if (auto native = tro->GetNativeObject()) {
                        tro->SetNativeObject(nullptr, false);
                        tro->SetNativeObject(native, true);
                        native.reset();
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
        return ctx.GetUndefined();
    }

    BASE_NS::string path = ctx.Arg<0>();
    auto meta = GetThisNativeObject(ctx);
    if (!meta) {
        return ctx.GetNull();
    }

    META_NS::IObject::Ptr child;
    if (auto node = interface_pointer_cast<SCENE_NS::INode>(meta)) {
        BASE_NS::string childPath = node->GetPath().GetResult() + "/" + path;
        child = node->GetScene()->FindNode<META_NS::IObject>(childPath).GetResult();
    }

    if (!child) {
        // no such child.
        return ctx.GetNull();
    }

    if (auto cached = FetchJsObj(child)) {
        // always return the same js object.
        return cached.ToNapiValue();
    }

    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        LOG_F("INVALID SCENE!");
    }

    // create new js object for the native node.
    NapiApi::Env env(ctx.GetEnv());
    NapiApi::Object argJS(env);
    napi_value args[] = { scene_.GetValue(), argJS.ToNapiValue() };

    auto js =
        CreateFromNativeInstance(env, child, false /*these are owned by the scene*/, BASE_NS::countof(args), args);
    if (auto nm = GetJsWrapper<NodeImpl>(js)) {
        nm->Attached(true);
    }
    return js.ToNapiValue();
}
napi_value NodeImpl::GetComponent(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    BASE_NS::string name = ctx.Arg<0>();
    auto meta = GetThisNativeObject(ctx);
    if (!meta) {
        return ctx.GetNull();
    }

    if (auto att = interface_pointer_cast<META_NS::IAttach>(meta)) {
        if (auto cont = att->GetAttachmentContainer(false)) {
            if (auto comp = cont->FindByName<SCENE_NS::IComponent>(name)) {
                auto obj = interface_pointer_cast<META_NS::IObject>(comp);
                if (auto cached = FetchJsObj(obj)) {
                    return cached.ToNapiValue();
                }

                NapiApi::Env env(ctx.GetEnv());

                NapiApi::Object argJS(env);
                napi_value args[] = { scene_.GetValue(), argJS.ToNapiValue() };
                auto argc = BASE_NS::countof(args);
                MakeNativeObjectParam(env, obj, argc, args);
                auto result = CreateJsObj(env, "SceneComponent", obj, false, argc, args);
                return StoreJsObj(obj, result).ToNapiValue();
            }
        }
    }
    return ctx.GetUndefined();
}

bool NodeImpl::IsAttached()
{
    return attached_;
}
void NodeImpl::Attached(bool attached)
{
    attached_ = attached;
}