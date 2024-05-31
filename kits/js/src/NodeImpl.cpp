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
#include <scene_plugin/api/camera_uid.h>
#include <scene_plugin/api/light_uid.h>
#include <scene_plugin/api/node_uid.h>
#include <scene_plugin/api/scene.h>
#include <scene_plugin/interface/intf_node.h>

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
#undef DECL_ENUM
    exports.Set("NodeType", NodeType);
}
NodeImpl::NodeImpl(NodeType type) : SceneResourceImpl(SceneResourceImpl::NODE), type_(type)
{
    LOG_F("NodeImpl ++");
}
NodeImpl::~NodeImpl()
{
    LOG_F("NodeImpl --");
}

void* NodeImpl::GetInstanceImpl(uint32_t id)
{
    if (id == NodeImpl::ID) {
        return this;
    }
    return SceneResourceImpl::GetInstanceImpl(id);
}

void NodeImpl::GetPropertyDescs(BASE_NS::vector<napi_property_descriptor>& props)
{
    using namespace NapiApi;
    // META_NS::IContainer;

    SceneResourceImpl::GetPropertyDescs(props);

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
    if (TrueRootObject* instance = GetThisRootObject(ctx)) {
        instance->DisposeNative();
    }
    scene_.Reset();
    return ctx.GetUndefined();
}
napi_value NodeImpl::GetLayerMaskEnabled(NapiApi::FunctionContext<uint32_t>& ctx)
{
    uint32_t bit = ctx.Arg<0>();
    bool enabled = false;
    if (auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx))) {
        uint64_t mask = 1ull << bit;
    ExecSyncTask([node, mask, &enabled]() {
            enabled = node->LayerMask()->GetValue() & mask;
        return META_NS::IAny::Ptr {};
        });
    }
    return ctx.GetBoolean(enabled);
}
napi_value NodeImpl::SetLayerMaskEnabled(NapiApi::FunctionContext<uint32_t, bool>& ctx)
{
    uint32_t bit = ctx.Arg<0>();
    bool enabled = ctx.Arg<1>();
    if (auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx))) {
    uint64_t mask = 1ull << bit;
    ExecSyncTask([node, enabled, mask]() {
            if (enabled) {
                node->LayerMask()->SetValue(node->LayerMask()->GetValue() | mask);
            } else {
                node->LayerMask()->SetValue(node->LayerMask()->GetValue() & ~mask);
            }
                return META_NS::IAny::Ptr {};
        });
    }
    return ctx.GetUndefined();
}

napi_value NodeImpl::GetNodeType(NapiApi::FunctionContext<>& ctx)
{
    uint32_t type = -1; // return -1 if the object does not exist anymore
    if (auto node = interface_cast<SCENE_NS::INode>(GetThisNativeObject(ctx))) {
        type = type_;
    }
    napi_value value;
    napi_status status = napi_create_uint32(ctx, type, &value);
    return value;
}

napi_value NodeImpl::GetLayerMask(NapiApi::FunctionContext<>& ctx)
{
    if (auto node = interface_cast<SCENE_NS::INode>(GetThisNativeObject(ctx))) {
        return ctx.This();
    }
    return ctx.GetUndefined();
}

napi_value NodeImpl::GetPath(NapiApi::FunctionContext<>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    BASE_NS::string path;
    if (node) {
        ExecSyncTask([node, &path]() {
            if (interface_cast<META_NS::IContainable>(node)->GetParent()) {
                path = node->Path()->GetValue();
            }
            return META_NS::IAny::Ptr {};
        });
    }
    napi_value value;
    napi_status status = napi_create_string_utf8(ctx, path.c_str(), path.length(), &value);
    return value;
}

napi_value NodeImpl::GetVisible(NapiApi::FunctionContext<>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    bool visible = false;
    if (node) {
        ExecSyncTask([node, &visible]() {
            visible = node->Visible()->GetValue();
            return META_NS::IAny::Ptr {};
        });
    }
    napi_value value;
    napi_status status = napi_get_boolean(ctx, visible, &value);
    return value;
}
void NodeImpl::SetVisible(NapiApi::FunctionContext<bool>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    if (node) {
        bool visible = ctx.Arg<0>();
        ExecSyncTask([node, visible]() {
            node->Visible()->SetValue(visible);
            return META_NS::IAny::Ptr {};
        });
    }
}

napi_value NodeImpl::GetPosition(NapiApi::FunctionContext<>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    if (!node) {
        return ctx.GetUndefined();
    }
    if (posProxy_ == nullptr) {
        posProxy_ = BASE_NS::make_unique<Vec3Proxy>(ctx, node->Position());
    }
    return posProxy_ ? posProxy_->Value() : NapiApi::Value<NapiApi::Object>();
}

void NodeImpl::SetPosition(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    if (!node) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (posProxy_ == nullptr) {
        posProxy_ = BASE_NS::make_unique<Vec3Proxy>(ctx, node->Position());
    }
    posProxy_->SetValue(obj);
}

napi_value NodeImpl::GetScale(NapiApi::FunctionContext<>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    if (!node) {
        return ctx.GetUndefined();
    }
    if (sclProxy_ == nullptr) {
        sclProxy_ = BASE_NS::make_unique<Vec3Proxy>(ctx, node->Scale());
    }
    return sclProxy_ ? sclProxy_->Value() : NapiApi::Value<NapiApi::Object>();
}

void NodeImpl::SetScale(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    if (!node) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (sclProxy_ == nullptr) {
        sclProxy_ = BASE_NS::make_unique<Vec3Proxy>(ctx, node->Scale());
    }
    sclProxy_->SetValue(obj);
}

napi_value NodeImpl::GetRotation(NapiApi::FunctionContext<>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    if (!node) {
        return ctx.GetUndefined();
    }
    if (rotProxy_ == nullptr) {
        rotProxy_ = BASE_NS::make_unique<QuatProxy>(ctx, node->Rotation());
    }
    return rotProxy_ ? rotProxy_->Value() : NapiApi::Value<NapiApi::Object>();
}

void NodeImpl::SetRotation(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    if (!node) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (rotProxy_ == nullptr) {
        rotProxy_ = BASE_NS::make_unique<QuatProxy>(ctx, node->Rotation());
    }
    rotProxy_->SetValue(obj);
}

napi_value NodeImpl::GetParent(NapiApi::FunctionContext<>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    if (!node) {
        return ctx.GetNull();
    }
    META_NS::IObject::Ptr root;
    if (auto containable = interface_cast<META_NS::IContainable>(node)) {
        ExecSyncTask([containable, &root]() {
            root = interface_pointer_cast<META_NS::IObject>(containable->GetParent());
            return META_NS::IAny::Ptr {};
        });
    }

    if (!root) {
        // no parent.
        return ctx.GetNull();
    }

    if (auto cached = FetchJsObj(root)) {
        // always return the same js object.
        return cached;
    }
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        CORE_LOG_F("INVALID SCENE!");
    }

    // create new js object for the native node.
    NapiApi::Object argJS(ctx);
    napi_value args[] = { scene_.GetValue(), argJS };

    return CreateFromNativeInstance(ctx, root, false /*these are owned by the scene*/, BASE_NS::countof(args), args);
}

napi_value NodeImpl::GetChildContainer(NapiApi::FunctionContext<>& ctx)
{
    // make sure the child list is up to date.
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    if (node) {
        ExecSyncTask([node]() {
            node->BuildChildren(SCENE_NS::INode::BuildBehavior::NODE_BUILD_ONLY_DIRECT_CHILDREN);
            return META_NS::IAny::Ptr {};
        });
    }

    // Node implements Container<Node>
    return ctx.This();
}

// container implementation
bool SkipNode(SCENE_NS::INode::Ptr node)
{
    auto o = interface_cast<META_NS::IObject>(node);
    auto classid = o->GetClassId();
    // white list of nodes that are actual nodes..
    if ((classid == SCENE_NS::ClassId::Camera) || (classid == SCENE_NS::ClassId::Light) ||
        (classid == SCENE_NS::ClassId::Node)) {
        return false;
    }
    return true;
}
napi_value NodeImpl::GetChild(NapiApi::FunctionContext<uint32_t>& ctx)
{
    uint32_t index = ctx.Arg<0>();
    META_NS::IObject::Ptr child;
    auto metaobj = GetThisNativeObject(ctx);
    if (auto container = interface_cast<META_NS::IContainer>(metaobj)) {
        ExecSyncTask([container, index, &child]() {
            auto data = container->GetAll<SCENE_NS::INode>();
            int count = 0;
            for (auto d : data) {
                // Skip nodes that are not real "nodes"
                if (SkipNode(d)) {
                    continue;
                }
                if (count == index) {
                    child = interface_pointer_cast<META_NS::IObject>(d);
                    break;
                }
                count++;
            }
            return META_NS::IAny::Ptr {};
        });
    }
    if (!child) {
        // return null
        napi_value value;
        napi_get_null(ctx, &value);
        return value;
    }
    auto cached = FetchJsObj(child);
    if (!cached) {
        // was not cached yet, so recreate.
        NapiApi::Object argJS(ctx);
        auto scn = scene_.GetObject();
        if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
            CORE_LOG_F("INVALID SCENE!");
        }

        napi_value args[] = { scene_.GetValue(), argJS };

        cached =
            CreateFromNativeInstance(ctx, child, false /*these are owned by the scene*/, BASE_NS::countof(args), args);
    }
    return cached;
}

napi_value NodeImpl::GetCount(NapiApi::FunctionContext<>& ctx)
{
    uint32_t count = 0;
    auto metaobj = GetThisNativeObject(ctx);
    if (auto container = interface_cast<META_NS::IContainer>(metaobj)) {
        ExecSyncTask([container, &count]() {
            auto data = container->GetAll<SCENE_NS::INode>();
            for (auto d : data) {
                // Skip nodes that are not real "nodes"
                if (SkipNode(d)) {
                    continue;
                }
                count++;
            }
            return META_NS::IAny::Ptr {};
        });
    }
    napi_value value;
    napi_status nstatus = napi_create_uint32(ctx, count, &value);
    return value;
}
napi_value NodeImpl::AppendChild(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    NapiApi::Object childJS = ctx.Arg<0>();
    if ((napi_value)childJS == nullptr) {
        // okay. Invalid arg error?
        return ctx.GetUndefined();
    }
    auto childNode = GetNativeMeta<SCENE_NS::INode>(childJS);
    if (!childNode) {
        return ctx.GetUndefined();
    }

    auto metaobj = GetThisNativeObject(ctx);
    if (auto container = interface_cast<META_NS::IContainer>(metaobj)) {
        ExecSyncTask([container, childNode]() {
            container->Add(childNode);
            childNode->Visible()->SetValue(true);
            return META_NS::IAny::Ptr {};
        });
    }

    // make the js object keep a weak ref again (scene keeps the native object alive)
    // (or move ownership back from SceneJS? and remove dispose hook?)
    if (auto tro = GetRootObject(ctx, childJS)) {
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
    NapiApi::Object childJS = ctx.Arg<0>();
    NapiApi::Object siblingJS = ctx.Arg<1>();

    if ((napi_value)childJS == nullptr) {
        // okay. Invalid arg error?
        return ctx.GetUndefined();
    }
    auto childNode = GetNativeMeta<SCENE_NS::INode>(childJS);
    if (!childNode) {
        return ctx.GetUndefined();
    }
    SCENE_NS::INode::Ptr siblingNode;
    if (siblingJS) {
        siblingNode = GetNativeMeta<SCENE_NS::INode>(siblingJS);
    }

    auto metaobj = GetThisNativeObject(ctx);
    if (auto container = interface_cast<META_NS::IContainer>(metaobj)) {
        ExecSyncTask([container, childNode, siblingNode]() {
            if (siblingNode) {
                auto data = container->GetAll<SCENE_NS::INode>();
                int64_t index = 0;
                for (auto d : data) {
                    index++;
                    if (d == siblingNode) {
                        // insert "after" the hit
                        container->Insert(index, childNode);
                        childNode->Visible()->SetValue(true);
                        return META_NS::IAny::Ptr {};
                    }
                }
                // did not find the sibling.. do nothing? add first? add last?
                // for now add last..
                container->Add(childNode);
                childNode->Visible()->SetValue(true);
                return META_NS::IAny::Ptr {};
            }
            // insert as first..
            container->Insert(0, childNode);
            childNode->Visible()->SetValue(true);
            return META_NS::IAny::Ptr {};
        });
    }

    // make the js object keep a weak ref again (scene keeps the native object alive)
    // (or move ownership back from SceneJS? and remove dispose hook?)
    if (auto tro = GetRootObject(ctx, childJS)) {
        if (auto native = tro->GetNativeObject()) {
            tro->SetNativeObject(nullptr, true);
            tro->SetNativeObject(native, false);
            native.reset();
        }
    }

    return ctx.GetUndefined();
}

void NodeImpl::ResetNativeObj(NapiApi::FunctionContext<>& ctx, NapiApi::Object& obj)
{
    if (auto tro = GetRootObject(ctx, obj)) {
        if (auto native = tro->GetNativeObject()) {
            tro->SetNativeObject(nullptr, false);
            tro->SetNativeObject(native, true);
            native.reset();
        }
    }
}

napi_value NodeImpl::RemoveChild(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    // okay. just detach from parent. (make parent invalid)
    // and disable it.
    NapiApi::Object childJS = ctx.Arg<0>();
    if ((napi_value)childJS == nullptr) {
        // okay. Invalid arg error?
        return ctx.GetUndefined();
    }
    auto childNode = GetNativeMeta<SCENE_NS::INode>(childJS);
    if (!childNode) {
        return ctx.GetUndefined();
    }

    // make the js object keep a strong ref.
    // (or give SceneJS ownership? and add dispose hook?)
    if (auto tro = GetRootObject(ctx, childJS)) {
        if (auto native = tro->GetNativeObject()) {
            tro->SetNativeObject(nullptr, false);
            tro->SetNativeObject(native, true);
            native.reset();
        }
    }

    auto metaobj = GetThisNativeObject(ctx);
    if (auto container = interface_cast<META_NS::IContainer>(metaobj)) {
        ExecSyncTask([container, childNode]() {
            container->Remove(childNode);
            childNode->Visible()->SetValue(false);
            return META_NS::IAny::Ptr {};
        });
    }
    return ctx.GetUndefined();
}

napi_value NodeImpl::ClearChildren(NapiApi::FunctionContext<>& ctx)
{
    auto metaobj = GetThisNativeObject(ctx);
    BASE_NS::vector<SCENE_NS::INode::Ptr> removedNodes;
    if (auto container = interface_cast<META_NS::IContainer>(metaobj)) {
        ExecSyncTask([container, &removedNodes]() {
            auto tmp = container->GetAll();
            for (auto t : tmp) {
                if (auto node = interface_pointer_cast<SCENE_NS::INode>(t)) {
                    if (SkipNode(node)) {
                        continue;
                    }
                    container->Remove(node);
                    node->Visible()->SetValue(false);
                    removedNodes.emplace_back(BASE_NS::move(node));
                }
            }
            return META_NS::IAny::Ptr {};
        });
        for (auto node : removedNodes) {
            if (auto cached = FetchJsObj(node)) {
                ResetNativeObj(ctx, cached);
            }
        }
    }
    return ctx.GetUndefined();
}

SCENE_NS::INode::Ptr recurse_children(const SCENE_NS::INode::Ptr startNode, BASE_NS::string_view path)
{
    SCENE_NS::INode::Ptr node = startNode;
    while (node != nullptr) {
        node->BuildChildren(SCENE_NS::INode::BuildBehavior::NODE_BUILD_ONLY_DIRECT_CHILDREN);
        // see if
        if (auto container = interface_cast<META_NS::IContainer>(node)) {
            auto pos = path.find('/', 0);
            BASE_NS::string_view step = path.substr(0, pos);
            node = interface_pointer_cast<SCENE_NS::INode>(container->FindByName(step));
            if (node) {
                if (pos == BASE_NS::string_view::npos) {
                    return node;
                }
                path = path.substr(pos + 1);
            }
        }
    }
    return {};
}
napi_value NodeImpl::GetNodeByPath(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    BASE_NS::string path = ctx.Arg<0>();
    auto meta = GetThisNativeObject(ctx);
    if (!meta) {
        return ctx.GetNull();
    }

    META_NS::IObject::Ptr child;
    if (auto node = interface_pointer_cast<SCENE_NS::INode>(meta)) {
        ExecSyncTask([node, &child, path]() {
            // make sure the child objects exist
            child = interface_pointer_cast<META_NS::IObject>(recurse_children(node, path));
            return META_NS::IAny::Ptr {};
        });
    }

    if (!child) {
        // no such child.
        return ctx.GetNull();
    }

    if (auto cached = FetchJsObj(child)) {
        // always return the same js object.
        return cached;
    }

    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        CORE_LOG_F("INVALID SCENE!");
    }

    // create new js object for the native node.
    NapiApi::Object argJS(ctx);
    napi_value args[] = { scene_.GetValue(), argJS };

    return CreateFromNativeInstance(ctx, child, false /*these are owned by the scene*/, BASE_NS::countof(args), args);
}
