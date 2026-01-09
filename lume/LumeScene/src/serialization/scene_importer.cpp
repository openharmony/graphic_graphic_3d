/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "scene_importer.h"

#include <charconv>
#include <scene/ext/intf_component.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/scene_utils.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_application_context.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_scene_manager.h>

#include <meta/api/metadata_util.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/serialization/intf_importer.h>

#include "../resource/util.h"
#include "../util.h"
#include "util.h"

SCENE_BEGIN_NAMESPACE()

static BASE_NS::string_view::size_type FindFirstSeparator(BASE_NS::string_view path)
{
    BASE_NS::string_view::size_type res {};

    while (res != BASE_NS::string_view::npos) {
        res = path.find_first_of(".!/", res);
        if (res != BASE_NS::string_view::npos) {
            if (res == 0 || path[res - 1] != ESCAPE_SER_CHAR) {
                return res;
            }
            ++res;
        }
    }
    return res;
}

BASE_NS::string GetNameAndSub(BASE_NS::string_view element, size_t& index)
{
    if (element.size() > 3 && element.back() == ']' && element[element.size() - 2] != ESCAPE_SER_CHAR) {
        auto pos = element.find_last_of('[');
        if (pos != BASE_NS::string_view::npos) {
            std::from_chars_result res =
                std::from_chars(element.data() + pos + 1, element.data() + element.size() - 1, index);
            if (res.ec != std::errc {} || res.ptr != element.data() + element.size() - 1) {
                CORE_LOG_W("Invalid index");
            }
            element = element.substr(0, pos);
        }
    }
    return UnescapeSerName(element);
}

static META_NS::IObject::Ptr GetIObjectFromProperty(const META_NS::IProperty::Ptr& p, size_t index)
{
    META_NS::IObject::Ptr res;
    if (index != -1) {
        if (META_NS::ArrayPropertyLock arr { p }) {
            if (index < arr->GetSize()) {
                res = interface_pointer_cast<META_NS::IObject>(META_NS::GetPointer(arr->GetAnyAt(index)));
            } else {
                CORE_LOG_W("Index out of bounds: %s [%zu]", p->GetName().c_str(), index);
            }
        } else {
            CORE_LOG_W("Trying to index non-array property: %s", p->GetName().c_str());
        }
    } else {
        if (auto obj = META_NS::GetPointer(p)) {
            res = interface_pointer_cast<META_NS::IObject>(obj);
        }
    }
    return res;
}

META_NS::IObject::Ptr FindObject(META_NS::IObject& obj, BASE_NS::string_view path)
{
    if (path.empty()) {
        return obj.GetSelf();
    }
    char pre = '/';
    if (path[0] == '!' || path[0] == '.' || path[0] == '/') {
        pre = path[0];
        path.remove_prefix(1);
    }
    auto pos = FindFirstSeparator(path);
    auto element = path.substr(0, pos);
    std::size_t index = -1;
    BASE_NS::string name = GetNameAndSub(element, index);
    path.remove_prefix(element.size());
    META_NS::IObject::Ptr child;
    if (pre == '!') {
        if (auto att = interface_cast<META_NS::IAttach>(&obj)) {
            auto attcont = att->GetAttachmentContainer(true);
            child = interface_pointer_cast<META_NS::IObject>(
                attcont->FindAny({ name, META_NS::TraversalType::NO_HIERARCHY, {}, false }));
            if (!child) {
                CORE_LOG_W("No such attachment for object [%s]", name.c_str());
            }
        }
    } else if (pre == '.') {
        if (auto prop = interface_cast<META_NS::IMetadata>(&obj)) {
            auto p = prop->GetProperty(name);
            child = interface_pointer_cast<META_NS::IObject>(p);
            if (!p) {
                CORE_LOG_W("No such property for object [%s]", name.c_str());
            }
            if (!path.empty()) {
                // if the path is not empty we try to take the object in the property to continue
                child = GetIObjectFromProperty(p, index);
            }
        }

    } else if (pre == '/') {
        if (auto cont = interface_cast<META_NS::IContainer>(&obj)) {
            child = cont->FindByName(name);
            if (!child) {
                CORE_LOG_W("No such child for object [%s]", name.c_str());
            }
        }
    }
    if (child && !path.empty()) {
        child = FindObject(*child, path);
    }
    return child;
}

static void AddFlatProperty(const META_NS::IProperty::ConstPtr& p, META_NS::IObject& node, bool templateContext)
{
    if (auto target = interface_pointer_cast<META_NS::IProperty>(FindObject(node, p->GetName()))) {
        if (templateContext) {
            META_NS::CopyToDefaultAndReset(p, *target);
        } else {
            META_NS::CopyValue(p, *target);
        }
        META_NS::SetObjectFlags(target, IMPORTED_FROM_TEMPLATE_BIT, templateContext);
    } else {
        CORE_LOG_W("Failed to resolve property [%s]", p->GetName().c_str());
    }
}

void AddFlatProperties(const META_NS::IMetadata& in, META_NS::IObject& node, bool templateContext)
{
    for (auto&& p : in.GetProperties()) {
        AddFlatProperty(p, node, templateContext);
    }
}
void AddFlatProperties(const META_NS::IMetadata& in, META_NS::IObject& node)
{
    AddFlatProperties(in, node, false);
}

void SceneImportContext::AddFlatPropertiesForExternal(
    const META_NS::IMetadata& in, META_NS::IObject& node, bool templateContext)
{
    for (auto&& p : in.GetProperties()) {
        if (auto r = META_NS::GetPointer<CORE_NS::IResource>(p)) {
            auto rid = r->GetResourceId();
            if (rid.IsValid()) {
                if (auto rmr = resources_->GetResource(rid, scene_)) {
                    if (r != rmr) {
                        CORE_LOG_D("Resetting resource '%s'", rid.ToString().c_str());
                        auto np = META_NS::DuplicatePropertyType(META_NS::GetObjectRegistry(), p);
                        META_NS::SetPointer(np, rmr);
                        p = np;
                    }
                }
            }
        }
        AddFlatProperty(p, node, templateContext);
    }
}

void SceneImportContext::AddFlatAttachments(const ISceneExternalNodeSer& in, const META_NS::IObject::Ptr& parent)
{
    for (auto&& p : in.GetAttachments()) {
        META_NS::SetObjectFlags(p, IMPORTED_FROM_TEMPLATE_BIT, isTemplateContext_);
        attachments_.push_back(AttachInfo { interface_pointer_cast<META_NS::IObject>(p), parent });
    }
}

void SceneImportContext::SetAttachments(const ISceneNodeSer& in, const META_NS::IObject::Ptr& out)
{
    for (auto&& v : in.GetAttachments()) {
        META_NS::SetObjectFlags(v, IMPORTED_FROM_TEMPLATE_BIT, isTemplateContext_);
        attachments_.push_back(AttachInfo { v, out });
    }
}

void SceneImportContext::CopyObjectValues(const ISceneNodeSer& in, const META_NS::IObject::Ptr& out)
{
    if (auto meta = interface_cast<META_NS::IMetadata>(&in)) {
        AddFlatProperties(*meta, *out, isTemplateContext_);
    }
    SetAttachments(in, out);
}

INode::Ptr SceneImportContext::ConstructNode(const ISceneNodeSer& ser, INode& parent)
{
    INode::Ptr res;
    if (auto obj = interface_cast<META_NS::IObject>(&parent)) {
        res = scene_->CreateNode(interface_pointer_cast<INode>(obj->GetSelf()), ser.GetName(), ser.GetId()).GetResult();
        if (auto obj = interface_pointer_cast<META_NS::IObject>(res)) {
            CopyObjectValues(ser, obj);
        }
    }
    return res;
}

INode::Ptr SceneImportContext::LoadExternalNode(const ISceneExternalNodeSer& in, INode& parent)
{
    INode::Ptr node;
    // todo: unify handling of gltf and template external nodes
    if (auto imp = interface_cast<INodeImport>(&parent)) {
        auto resource = resources_->GetResource(in.GetResourceId());
        if (auto res = interface_pointer_cast<IScene>(resource)) {
            auto sgroups = in.GetSubresourceGroups();
            if (!sgroups.empty()) {
                CORE_LOG_D("purging resource group '%s'", sgroups.front().c_str());
                resources_->PurgeGroup(sgroups.front());
            }
            node = imp->ImportChildScene(res, in.GetName(), sgroups.empty() ? "" : sgroups.front()).GetResult();
        } else if (auto res = interface_pointer_cast<META_NS::IObjectTemplate>(resource)) {
            node = imp->ImportTemplate(res).GetResult();
        } else {
            CORE_LOG_W("No resource/invalid resource: %s", in.GetResourceId().ToString().c_str());
        }
        if (auto n = interface_pointer_cast<META_NS::IObject>(node)) {
            if (auto m = interface_cast<META_NS::IMetadata>(&in)) {
                AddFlatPropertiesForExternal(*m, *n, isTemplateContext_);
                AddFlatAttachments(in, n);
            }
        }
        // if we spawned this resource, clean it up
        if (resource.use_count() == 2) { // 2: count
            resources_->PurgeResource(in.GetResourceId());
        }
    } else {
        CORE_LOG_W("Failed to import scene resource to scene");
    }
    return node;
}

INode::Ptr SceneImportContext::BuildSceneNodeHierarchy(const META_NS::IObject& data, INode& parent)
{
    INode::Ptr node;
    if (auto ext = interface_cast<ISceneExternalNodeSer>(&data)) {
        node = LoadExternalNode(*ext, parent);
    } else if (auto nodeser = interface_cast<ISceneNodeSer>(&data)) {
        node = ConstructNode(*nodeser, parent);
        if (node) {
            META_NS::Internal::ConstIterate(
                interface_cast<META_NS::IIterable>(&data),
                [&](META_NS::IObject::Ptr obj) {
                    if (obj) {
                        BuildSceneNodeHierarchy(*obj, *node);
                    }
                    return true;
                },
                META_NS::IterateStrategy { META_NS::TraversalType::NO_HIERARCHY });
        }
    }
    return node;
}

void SceneImportContext::SetResourceGroups(BASE_NS::vector<BASE_NS::string> groups)
{
    if (auto iScene = scene_->GetInternalScene()) {
        if (requestedId_.IsValid()) {
            auto gname = requestedId_.ToString();
            auto it = groups.begin();
            for (; it != groups.end() && *it != gname; ++it) {
            }
            if (it == groups.end()) {
                groups.push_back(gname);
            }
        }
        if (!groups.empty()) {
            iScene->SetResourceGroups(StringToResourceGroupBundle(iScene->GetContext(), groups));
        }
    }
}

IScene::Ptr SceneImportContext::BuildSceneHierarchy(const ISceneNodeSer& data)
{
    if (auto obj = interface_pointer_cast<META_NS::IObject>(scene_)) {
        CopyObjectValues(data, obj);
    }
    if (auto sceneser = interface_cast<ISceneSer>(&data)) {
        SetResourceGroups(sceneser->GetResourceGroups());
    } else if (requestedId_.IsValid()) {
        SetResourceGroups({});
    }
    if (auto i = interface_cast<META_NS::IContainer>(&data)) {
        auto nodes = i->GetAll();
        if (nodes.empty()) {
            // should have root node at least
            return nullptr;
        }
        if (nodes.size() != 1) {
            CORE_LOG_W("multiple root nodes?!");
        }
        auto rootdata = interface_cast<ISceneNodeSer>(nodes.front());
        if (!rootdata) {
            CORE_LOG_W("invalid root node");
            return nullptr;
        }
        auto root = scene_->GetRootNode().GetResult();
        if (auto obj = interface_pointer_cast<META_NS::IObject>(root)) {
            CopyObjectValues(*rootdata, obj);
            META_NS::Internal::ConstIterate(
                interface_cast<META_NS::IIterable>(rootdata),
                [&](META_NS::IObject::Ptr obj) {
                    if (obj) {
                        BuildSceneNodeHierarchy(*obj, *root);
                    }
                    return true;
                },
                META_NS::IterateStrategy { META_NS::TraversalType::NO_HIERARCHY });
        }
        return scene_;
    }
    return nullptr;
}

INode::Ptr SceneImportContext::BuildSceneHierarchy(INode& node, const META_NS::IObject& data)
{
    return BuildSceneNodeHierarchy(data, node);
}

void SceneImportContext::DoDeferredAttach()
{
    for (auto&& v : attachments_) {
        if (auto ext = interface_cast<IExternalAttachment>(v.data)) {
            if (auto target = interface_pointer_cast<META_NS::IAttach>(FindObject(*v.parent, ext->GetPath()))) {
                if (!target->Attach(ext->GetAttachment())) {
                    CORE_LOG_W("Failed to attach");
                }
            } else {
                CORE_LOG_W("Failed to find object to attach to [%s]", ext->GetPath().c_str());
            }
        } else if (auto target = interface_cast<META_NS::IAttach>(v.parent)) {
            if (!target->Attach(v.data)) {
                CORE_LOG_W("Failed to attach");
            }
        }
    }
}

SceneImportContext::SceneImportContext(
    IScene::Ptr scene, CORE_NS::IResourceManager::Ptr resources, CORE_NS::ResourceId rid)
    : scene_(BASE_NS::move(scene)), resources_(BASE_NS::move(resources)), requestedId_(BASE_NS::move(rid))
{}

IScene::Ptr SceneImportContext::ImportScene(CORE_NS::IFile& in)
{
    auto importer = META_NS::GetObjectRegistry().Create<META_NS::IFileImporter>(META_NS::ClassId::JsonImporter);
    if (!importer) {
        return nullptr;
    }
    importer->SetResourceManager(resources_);
    importer->SetUserContext(interface_pointer_cast<META_NS::IObject>(scene_));
    if (auto obj = importer->Import(in, META_NS::ImportOptions { true })) {
        if (auto i = interface_cast<ISceneNodeSer>(obj)) {
            if (auto scene = BuildSceneHierarchy(*i)) {
                importer->ResolveDeferred();
                DoDeferredAttach();
                return scene;
            }
        }
    }
    return nullptr;
}
INode::Ptr SceneImportContext::ImportNode(CORE_NS::IFile& in, const INode::Ptr& parent)
{
    INode::Ptr res;
    if (parent) {
        auto importer = META_NS::GetObjectRegistry().Create<META_NS::IFileImporter>(META_NS::ClassId::JsonImporter);
        importer->SetResourceManager(resources_);
        importer->SetUserContext(interface_pointer_cast<META_NS::IObject>(scene_));
        if (auto obj = importer->Import(in, META_NS::ImportOptions { true })) {
            res = BuildSceneHierarchy(*parent, *obj);
            if (res) {
                importer->ResolveDeferred();
                DoDeferredAttach();
            }
        }
    }
    return res;
}
INode::Ptr SceneImportContext::ImportNode(const INodeTemplateInternal& templ, const INode::Ptr& parent)
{
    isTemplateContext_ = true;

    INode::Ptr res;
    if (parent) {
        auto importer = META_NS::GetObjectRegistry().Create<META_NS::IFileImporter>(META_NS::ClassId::JsonImporter);
        importer->SetResourceManager(resources_);
        importer->SetUserContext(interface_pointer_cast<META_NS::IObject>(scene_));
        if (auto obj = importer->Import(templ.GetNodeHierarchy(), META_NS::ImportOptions { true })) {
            res = BuildSceneHierarchy(*parent, *obj);
            if (res) {
                importer->ResolveDeferred();
                DoDeferredAttach();
            }
        }
    }
    return res;
}

bool SceneImporter::Build(const META_NS::IMetadata::Ptr& d)
{
    bool res = Super::Build(d);
    if (res) {
        context_ = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
        opts_ = GetBuildArg<SceneOptions>(d, "Options");
    }
    return res;
}

IScene::Ptr SceneImporter::ImportScene(CORE_NS::IFile& in, const CORE_NS::ResourceId& requestedId)
{
    auto context = context_.lock();
    if (!context) {
        CORE_LOG_W("No context set for scene importer");
        return nullptr;
    }
    auto md = CreateRenderContextArg(context);
    if (md) {
        md->AddProperty(META_NS::ConstructProperty<SceneOptions>("Options", opts_));
    }
    auto m = GetSceneManager(context);
    if (!m) {
        // Could not get scene manager from context, create a new one
        m = META_NS::GetObjectRegistry().Create<SCENE_NS::ISceneManager>(SCENE_NS::ClassId::SceneManager, md);
    }
    if (m) {
        if (auto resources = context->GetResources()) {
            if (auto scene = m->CreateScene().GetResult()) {
                return SceneImportContext(scene, resources, requestedId).ImportScene(in);
            }
        }
    }
    return nullptr;
}
INode::Ptr SceneImporter::ImportNode(CORE_NS::IFile& in, const INode::Ptr& parent)
{
    if (!parent) {
        return nullptr;
    }
    if (auto resources = GetResourceManager(parent->GetScene())) {
        return SceneImportContext(parent->GetScene(), resources).ImportNode(in, parent);
    }
    return nullptr;
}

INode::Ptr SceneImporter::ImportNode(const META_NS::IObject& obj, const INode::Ptr& parent)
{
    if (!parent) {
        return nullptr;
    }
    if (auto resources = GetResourceManager(parent->GetScene())) {
        if (auto templ = interface_cast<INodeTemplateInternal>(&obj)) {
            return SceneImportContext(parent->GetScene(), resources).ImportNode(*templ, parent);
        }
    }
    return nullptr;
}

SCENE_END_NAMESPACE()
