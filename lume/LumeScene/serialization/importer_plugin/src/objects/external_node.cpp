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

#include "external_node.h"

#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_application_context.h>
#include <scene/interface/intf_node_import.h>

#include <meta/interface/resource/intf_resource.h>

#include "../import_helpers.h"
#include "../resolve_object.h"
#include "attachment.h"
#include "builtin.h"
#include "node.h"
#include "property.h"

SCENE_IMP_BEGIN_NAMESPACE()

static ImportResult LoadGltf(ImportContext& context, BASE_NS::string_view uri)
{
    auto args = CreateRenderContextArg(context.GetRenderContext());
    if (args) {
        if (auto op = META_NS::ConstructProperty<SCENE_NS::SceneOptions>(
                "Options", context.GetConfig().staticConfig.options.scene)) {
            args->AddProperty(op);
        }
    }
    auto sman = META_NS::GetObjectRegistry().Create<SCENE_NS::ISceneManager>(SCENE_NS::ClassId::SceneManager, args);
    if (!sman) {
        CORE_LOG_E("No scene manager");
        return ImportResult{context.CreateDiagnostics("No scene manager")};
    }
    auto scene = sman->CreateScene(uri).GetResult();
    if (!scene) {
        CORE_LOG_E("Failed to create scene");
        return ImportResult{context.CreateDiagnostics("Failed to create scene")};
    }
    return ImportResult{interface_pointer_cast<META_NS::IObject>(scene)};
}

static IDiagnostics::Ptr ImportPropertiesWithPath(ImportContext& context, const META_NS::IObject::Ptr& base)
{
    ErrorHandler h(context);
    auto props = context.GetOptArray("overrides");
    for (auto&& p : props) {
        auto ncont = context.CreateContext(p);
        auto path = ncont.GetOptString("path");
        auto res = ResolveObject(context, base, path, true);
        if (res.error && h.Handle(res.error)) {
            return res.error;
        }
        auto prop = interface_cast<META_NS::IProperty>(res.object);
        if (!prop) {
            CORE_LOG_E("Override not pointing to property: %s", path.c_str());
            if (h.Handle(context.CreateDiagnostics("Override not pointing to property"))) {
                return h;
            }
        }
        if (prop) {
            if (auto err = ImportProperty(ncont, *prop); h.Handle(err)) {
                return err;
            }
        }
    }
    return h;
}

static IDiagnostics::Ptr ImportAttachmentsWithPath(ImportContext& context, const META_NS::IObject::Ptr& base)
{
    ErrorHandler h(context);
    auto arr = context.GetOptArray("attachments");
    for (auto&& v : arr) {
        auto ncont = context.CreateContext(v);
        auto path = ncont.GetOptString("path");
        auto res = ResolveObject(context, base, path, true);
        if (!res) {
            if (h.Handle(res.error)) {
                return res.error;
            }
        } else {
            if (auto err = ImportAttachment(ncont, res.object); h.Handle(err)) {
                return err;
            }
        }
    }
    return h;
}

static IDiagnostics::Ptr ImportComponentAtPath(
    ImportContext& context, ImportContext& compContext, const META_NS::IObject::Ptr& base)
{
    auto path = compContext.GetOptString("path");
    auto res = ResolveObject(context, base, path, true);
    if (!res) {
        return res.error;
    }
    auto node = interface_pointer_cast<SCENE_NS::INode>(res.object);
    if (!node) {
        CORE_LOG_E("Component path not pointing to node: %s", path.c_str());
        return context.CreateDiagnostics("Component path not pointing to node");
    }
    return ImportNodeBase::ImportComponent(compContext, node);
}

static IDiagnostics::Ptr ImportComponentsWithPath(ImportContext& context, const META_NS::IObject::Ptr& base)
{
    ErrorHandler h(context);
    auto arr = context.GetOptArray("components");
    for (auto&& v : arr) {
        auto ncont = context.CreateContext(v);
        if (auto err = ImportComponentAtPath(context, ncont, base); h.Handle(err)) {
            return err;
        }
    }
    return h;
}

static ImportResult ImportExternalNodeLoad(ImportContext& context, const CORE_NS::IResource::Ptr& resource,
    BASE_NS::string_view name, BASE_NS::string_view group)
{
    auto imp = interface_cast<SCENE_NS::INodeImport>(context.GetImportParameters().object);
    if (!imp) {
        BASE_NS::string nameStr(name);
        CORE_LOG_E("Parent node does not implement INodeImport (external node: '%s')", nameStr.c_str());
        return ImportResult{
            context.CreateDiagnostics("Parent node does not implement INodeImport (external node: '" + nameStr + "')")};
    }
    SCENE_NS::INode::Ptr result;
    if (auto res = interface_pointer_cast<SCENE_NS::IScene>(resource)) {
        result = imp->ImportChildScene(res, name, group).GetResult();
    } else if (auto res = interface_pointer_cast<META_NS::IObjectTemplate>(resource)) {
        result = imp->ImportTemplate(res, group).GetResult();
    } else {
        auto resid = resource->GetResourceId().ToString();
        BASE_NS::string nameStr(name);
        CORE_LOG_E("Resource is neither a scene nor a template: '%s' (%s)", nameStr.c_str(), resid.c_str());
        return ImportResult{context.CreateDiagnostics(
            "Resource is neither a scene nor a template: '" + resid + "' (external node: '" + nameStr + "')")};
    }
    if (!result) {
        auto resid = resource->GetResourceId().ToString();
        BASE_NS::string nameStr(name);
        CORE_LOG_E("Failed to load external node: '%s' (%s)", nameStr.c_str(), resid.c_str());
        return ImportResult{context.CreateDiagnostics(
            "Failed to load external node: '" + resid + "' (external node: '" + nameStr + "')")};
    }
    return ImportResult{interface_pointer_cast<META_NS::IObject>(result)};
}

static ImportResult ImportExternalNode(
    ImportContext& context, const CORE_NS::ResourceId& rid, BASE_NS::string_view name, BASE_NS::string_view group)
{
    auto node = interface_cast<SCENE_NS::INode>(context.GetImportParameters().object);
    if (!node) {
        BASE_NS::string nameStr(name);
        CORE_LOG_E("Parent object is not a node (external node: '%s', resource: '%s')",
            nameStr.c_str(),
            rid.ToString().c_str());
        return ImportResult{context.CreateDiagnostics(
            "Parent object is not a node (external node: '" + nameStr + "', resource: '" + rid.ToString() + "')")};
    }
    auto imp = interface_cast<SCENE_NS::INodeImport>(node);
    auto resources = SCENE_NS::GetResourceManager(node->GetScene());
    if (!imp || !resources) {
        BASE_NS::string nameStr(name);
        CORE_LOG_E("Parent node does not support external-node import (external node: '%s', resource: '%s')",
            nameStr.c_str(),
            rid.ToString().c_str());
        return ImportResult{
            context.CreateDiagnostics("Parent node does not support external-node import (external node: '" + nameStr +
                                      "', resource: '" + rid.ToString() + "')")};
    }
    CORE_NS::ResourceIdContext pres{rid, node->GetScene()};
    auto resource = resources->GetResource(pres);
    // we first try with context and if not found, try without
    if (!resource) {
        pres.context = nullptr;
        resource = resources->GetResource(pres);
    }
    if (!resource) {
        CORE_LOG_E("No such resource: %s (external node: '%s')", rid.ToString().c_str(), BASE_NS::string(name).c_str());
        return ImportResult{context.CreateDiagnostics(
            "No such resource: '" + rid.ToString() + "' (external node: '" + BASE_NS::string(name) + "')")};
    }

    auto res = ImportExternalNodeLoad(context, resource, name, group);
    // if we spawned this resource, clean it up
    if (resource.use_count() == 2) {
        resources->PurgeResource(pres);
    }
    return res;
}

static ImportResult ImportExternalNode(ImportContext& context, const BASE_NS::string& url, BASE_NS::string_view type,
    BASE_NS::string_view name, BASE_NS::string_view group)
{
    auto imp = interface_cast<SCENE_NS::INodeImport>(context.GetImportParameters().object);
    if (!imp) {
        BASE_NS::string nameStr(name);
        CORE_LOG_E("Parent node does not implement INodeImport (external node: '%s', url: '%s')",
            nameStr.c_str(),
            url.c_str());
        return ImportResult{context.CreateDiagnostics(
            "Parent node does not implement INodeImport (external node: '" + nameStr + "', url: '" + url + "')")};
    }
    ImportResult res;
    if (type == "gltf") {
        res = ImportResult{LoadGltf(context, url)};
    } else if (type == "nodeTemplate") {
        res = LoadFile(context, url, "nodeTemplate");
    } else {
        BASE_NS::string typeStr(type);
        CORE_LOG_E("Invalid external type: '%s' (url: '%s')", typeStr.c_str(), url.c_str());
        return ImportResult{context.CreateDiagnostics("Invalid external type: '" + typeStr + "' (url: '" + url + "')")};
    }
    if (!res) {
        return res;
    }
    return ImportExternalNodeLoad(context, interface_pointer_cast<CORE_NS::IResource>(res.object), name, group);
}

static ImportResult ImportExternal(ImportContext& context, BASE_NS::string_view name)
{
    BASE_NS::string rgroup;
    if (auto err = context.GetRequiredString("resourceGroup", rgroup)) {
        return ImportResult{err};
    }
    auto sourceObj = context.GetOptObject("source");
    if (sourceObj.empty()) {
        CORE_LOG_E("Missing source");
        return ImportResult{context.CreateDiagnostics("Missing source")};
    }
    auto sourceContext = context.CreateContext(BASE_NS::move(sourceObj));
    auto rid = GetOptResourceId(sourceContext, "resourceId");
    if (rid.error) {
        return ImportResult{rid.error};
    }
    ImportResult res;
    if (rid.value) {
        auto trace = context.Trace("Importing external node with id '" + rid.value->ToString() + "'");
        res = ImportExternalNode(context, *rid.value, name, rgroup);
    } else {
        auto url = sourceContext.GetOptString("url");
        auto type = sourceContext.GetOptString("type");
        auto trace = context.Trace("Importing external node with url '" + url + "'");
        res = ImportExternalNode(context, url, type, name, rgroup);
    }
    if (res) {
        if (auto err = ImportPropertiesWithPath(context, res.object)) {
            return ImportResult{err};
        }
        if (auto err = ImportAttachmentsWithPath(context, res.object)) {
            return ImportResult{err};
        }
        if (auto err = ImportComponentsWithPath(context, res.object)) {
            return ImportResult{err};
        }
    }
    return res;
}

SCENE_NS::INode::Ptr ImportExternalNode::ConstructNode(
    ImportContext& context, const SCENE_NS::IScene::Ptr& scene, const BASE_NS::string& name)
{
    return nullptr;
}

ImportResult ImportExternalNode::Import(ImportContext& context)
{
    auto scene = context.GetImportParameters().scene;
    if (!scene) {
        CORE_LOG_E("Invalid state: scene not set when constructing external node");
        return ImportResult{context.CreateDiagnostics("Invalid state: scene not set when constructing external node")};
    }
    if (auto err = context.RequireString("type", "external")) {
        return ImportResult{err};
    }
    auto name = context.GetOptString("name");
    auto trace = context.Trace("Importing node '" + name + "' with type 'external'");
    auto res = ImportExternal(context, name);
    if (res) {
        ErrorHandler h(context);
        if (auto err = ImportTransform(context, interface_pointer_cast<SCENE_NS::INode>(res.object)); h.Handle(err)) {
            return ImportResult{err};
        }
        if (!res.error) {
            res.error = h;
        }
    }
    return res;
}

SCENE_IMP_END_NAMESPACE()
