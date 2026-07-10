/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "node.h"

#include <scene/interface/intf_layer.h>

#include <meta/api/util.h>
#include <meta/interface/intf_attach.h>

#include "../import_helpers.h"
#include "attachment.h"
#include "external_node.h"
#include "flag_tables.h"
#include "property.h"

SCENE_IMP_BEGIN_NAMESPACE()

namespace {
IDiagnostics::Ptr ImportNodeFlags(ImportContext& context, const SCENE_NS::INode::Ptr& node)
{
    auto flags = context.GetOptArray("nodeFlags");
    if (flags.empty()) {
        return nullptr;
    }
    ErrorHandler h(context);
    auto bits = static_cast<SCENE_NS::NodeFlags>(0);
    if (auto err = ImportFlagsFromArray(context, flags, NODE_FLAGS_TABLE, "nodeFlags", bits); h.Handle(err)) {
        return err;
    }
    SetValue(context, node->NodeFlags(), bits);
    return h;
}

}  // namespace

IDiagnostics::Ptr ImportNodeBase::ImportLayerMask(ImportContext& context, const SCENE_NS::INode::Ptr& node)
{
    ErrorHandler h(context);
    auto value = GetOptUInt(context, "layerMask");
    if (!h.HandleOptValue(value)) {
        return h;
    }
    if (value.error) {
        return value.error;
    }
    if (auto layer = interface_cast<SCENE_NS::ILayer>(node)) {
        SetValue(context, layer->LayerMask(), value.GetValue());
    }
    return h;
}

IDiagnostics::Ptr ImportNodeBase::ImportEnabled(ImportContext& context, const SCENE_NS::INode::Ptr& node)
{
    ErrorHandler h(context);
    auto value = GetOptBool(context, "enabled");
    if (!h.HandleOptValue(value)) {
        return h;
    }
    if (value.error) {
        return value.error;
    }
    SetValue(context, node->Enabled(), value.GetValue());
    return h;
}

IDiagnostics::Ptr ImportNodeBase::ImportTransform(ImportContext& context, const SCENE_NS::INode::Ptr& node)
{
    auto trans = interface_cast<SCENE_NS::ITransform>(node);
    if (!trans) {
        auto name = context.GetOptString("name");
        CORE_LOG_E("Node does not implement ITransform (name: '%s')", name.c_str());
        return context.CreateDiagnostics("Node does not implement ITransform (name: '" + name + "')");
    }
    ErrorHandler h(context);
    if (auto pos = GetOptVec3(context, "position"); h.HandleOptValue(pos)) {
        if (pos.error) {
            return pos.error;
        }
        SetValue(context, trans->Position(), pos.GetValue());
    }
    if (auto scale = GetOptVec3(context, "scale"); h.HandleOptValue(scale)) {
        if (scale.error) {
            return scale.error;
        }
        SetValue(context, trans->Scale(), scale.GetValue());
    }
    if (auto rot = GetOptQuat(context, "rotation"); h.HandleOptValue(rot)) {
        if (rot.error) {
            return rot.error;
        }
        SetValue(context, trans->Rotation(), rot.GetValue());
    }
    return h;
}

IDiagnostics::Ptr ImportNodeBase::ImportComponent(ImportContext& context, const SCENE_NS::INode::Ptr& node)
{
    ErrorHandler h(context);
    auto name = GetOptString(context, "name");
    if (h.HandleOptValue(name)) {
        if (name.error) {
            return name.error;
        }
    }
    auto id = GetOptObjectId(context, "componentUid");
    if (h.HandleOptValue(id)) {
        if (id.error) {
            return id.error;
        }
    }
    if (!id.value && !name.value) {
        CORE_LOG_E("Component missing both 'name' and 'componentUid'");
        return context.CreateDiagnostics("Component missing both 'name' and 'componentUid'");
    }
    auto trace = context.Trace("Importing component [name=" + name.value.value_or(BASE_NS::string{}) +
                               ", uid=" + id.value.value_or(META_NS::ObjectId{}).ToString() + "]");
    SCENE_NS::IComponent::Ptr component;
    auto scene = node->GetScene();
    if (id.value) {
        component = META_NS::GetObjectRegistry().Create<SCENE_NS::IComponent>(*id.value);
        if (component) {
            if (auto attach = interface_cast<META_NS::IAttach>(node)) {
                attach->Attach(component);
            }
        }
    } else {
        component = scene->CreateComponent(node, name.GetValue()).GetResult();
    }
    if (!component) {
        auto compName = name.value.value_or(BASE_NS::string{});
        auto compUid = id.value.value_or(META_NS::ObjectId{}).ToString();
        CORE_LOG_E("Failed to create component (name: '%s', uid: %s)", compName.c_str(), compUid.c_str());
        return context.CreateDiagnostics("Failed to create component (name: '" + compName + "', uid: " + compUid + ")");
    }
    if (context.IsTemplateContext()) {
        META_NS::SetObjectFlags(component, IMPORTED_FROM_TEMPLATE_BIT, true);
    }
    auto componentObj = interface_pointer_cast<META_NS::IObject>(component);
    if (!componentObj) {
        return context.CreateDiagnostics("Component does not implement IObject");
    }
    if (auto err = ImportProperties(context, *componentObj); h.Handle(err)) {
        return err;
    }
    return h;
}

IDiagnostics::Ptr ImportNodeBase::ImportComponents(ImportContext& context, const SCENE_NS::INode::Ptr& node)
{
    ErrorHandler h(context);
    auto arr = context.GetOptArray("components");
    for (size_t i = 0; i < arr.size(); ++i) {
        auto ncont = context.CreateContext(arr[i]);
        auto entryTrace = ncont.TraceIndex("components", i);
        if (auto err = ImportComponent(ncont, node); h.Handle(err)) {
            return err;
        }
    }
    return h;
}

IDiagnostics::Ptr ImportNodeBase::ImportNodeAttributes(ImportContext& context, const SCENE_NS::INode::Ptr& node)
{
    ErrorHandler h(context);
    if (auto err = ImportTransform(context, node); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportComponents(context, node); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportNodeFlags(context, node); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportLayerMask(context, node); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportEnabled(context, node); h.Handle(err)) {
        return err;
    }
    if (auto err = ImportAttachments(context, interface_pointer_cast<META_NS::IObject>(node)); h.Handle(err)) {
        return err;
    }
    return h;
}

IDiagnostics::Ptr ImportNodeBase::ImportNodeCommon(ImportContext& context, const SCENE_NS::INode::Ptr& node)
{
    ErrorHandler h(context);
    if (auto err = ImportNodeAttributes(context, node); h.Handle(err)) {
        return err;
    }

    constexpr size_t MAX_NODE_DEPTH = 128;
    auto params = context.GetImportParameters();
    params.object = interface_pointer_cast<META_NS::IObject>(node);
    // Pin `importRoot` to the first node imported in a fresh subtree (scene
    // rootNode for normal imports, template content root for instantiations).
    // `/` paths in descendants anchor here. Once set we don't overwrite, so
    // nested templates keep the outer template's root unless they've been
    // entered via TInstantiator (which starts a fresh context).
    if (!params.importRoot) {
        params.importRoot = params.object;
    }
    if (params.nodeDepth >= MAX_NODE_DEPTH) {
        CORE_LOG_E("Maximum node hierarchy depth exceeded");
        return context.CreateDiagnostics("Maximum node hierarchy depth exceeded");
    }
    params.nodeDepth++;

    auto children = context.GetOptArray("children");
    for (size_t i = 0; i < children.size(); ++i) {
        auto childTrace = context.TraceIndex("children", i);
        if (auto res = context.ImportSubType("abstract-node", children[i], params); !res) {
            if (h.Handle(res.error)) {
                return res.error;
            }
        }
    }
    return h;
}

ImportResult ImportNodeBase::ImportNode(ImportContext& context, BASE_NS::string_view type)
{
    auto scene = context.GetImportParameters().scene;
    if (!scene) {
        CORE_LOG_E("Invalid state: scene not set when constructing node");
        return ImportResult{context.CreateDiagnostics("Invalid state: scene not set when constructing node")};
    }
    if (auto err = context.RequireString("type", type)) {
        return ImportResult{err};
    }
    auto name = context.GetOptString("name");
    auto trace = context.Trace("Importing node '" + name + "' with type '" + type + "'");
    SCENE_NS::INode::Ptr node = ConstructNode(context, scene, name);
    if (!node) {
        BASE_NS::string typeStr(type);
        CORE_LOG_E("Failed to create node (name: '%s', type: '%s')", name.c_str(), typeStr.c_str());
        return ImportResult{
            context.CreateDiagnostics("Failed to create node (name: '" + name + "', type: '" + typeStr + "')")};
    }
    ErrorHandler h(context);
    if (auto err = ImportNodeCommon(context, node); h.Handle(err)) {
        return ImportResult{err};
    }
    ImportResult result{interface_pointer_cast<META_NS::IObject>(node)};
    result.error = h;
    return result;
}

ImportResult AbstractImportNode::Import(ImportContext& context)
{
    auto scene = context.GetImportParameters().scene;
    if (!scene) {
        CORE_LOG_E("Invalid state: scene not set when constructing node");
        return ImportResult{context.CreateDiagnostics("Invalid state: scene not set when constructing node")};
    }
    BASE_NS::string type;
    if (auto err = context.GetRequiredString("type", type)) {
        return ImportResult{err};
    }
    // should we check the node types here?
    return context.ImportSubType(type, context.GetJsonValue());
}

SCENE_NS::INode::Ptr ImportNode::ConstructNode(
    ImportContext& context, const SCENE_NS::IScene::Ptr& scene, const BASE_NS::string& name)
{
    SCENE_NS::INode::Ptr node;
    if (context.GetImportParameters().object) {
        node = scene
                   ->CreateNode(interface_pointer_cast<SCENE_NS::INode>(context.GetImportParameters().object),
                       name,
                       SCENE_NS::ClassId::Node)
                   .GetResult();
    } else {
        // no parent? must be root node
        node = scene->GetRootNode().GetResult();
        if (auto named = interface_cast<META_NS::INamed>(node)) {
            named->Name()->SetValue(name);
        }
    }
    return node;
}

ImportResult ImportNode::Import(ImportContext& context)
{
    return ImportNodeBase::ImportNode(context, "node");
}

SCENE_IMP_END_NAMESPACE()
