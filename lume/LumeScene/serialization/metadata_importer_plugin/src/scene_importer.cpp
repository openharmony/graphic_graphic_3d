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

#include <algorithm>
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
#include <scene/interface/resource/intf_render_resource_manager.h>

#include <meta/api/metadata_util.h>
#include <meta/base/memfile.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/serialization/intf_importer.h>
#include <meta/interface/serialization/intf_ser_input.h>
#include <meta/interface/serialization/intf_ser_output.h>

#include "node_template_rewrite.h"
#include "resource_util.h"
#include "serialization_util.h"

namespace {
static void CopyResourceInfos(const BASE_NS::array_view<const CORE_NS::ResourceInfo>& infos,
    CORE_NS::IResourceManager::Ptr& dest, const CORE_NS::ResourceContextPtr& context)
{
    if (auto destExt = interface_cast<META_NS::IResourceManagerExtension>(dest)) {
        for (auto&& i : infos) {
            META_NS::ResourceData d{i};
            if (auto destI = destExt->GetResources({i.id}, context); !destI.empty()) {
                d.object = destI.front().object;
            }
            destExt->AddResources({d}, context);
        }
    }
}
}  // namespace

SCENE_BEGIN_NAMESPACE()

static BASE_NS::string_view::size_type FindFirstSeparator(BASE_NS::string_view path)
{
    BASE_NS::string_view::size_type res{};

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
            if (res.ec != std::errc{} || res.ptr != element.data() + element.size() - 1) {
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
        if (META_NS::ArrayPropertyLock arr{p}) {
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

static META_NS::IObject::Ptr FindAttachmentChild(META_NS::IObject& obj, const BASE_NS::string& name)
{
    auto att = interface_cast<META_NS::IAttach>(&obj);
    if (!att) {
        return nullptr;
    }
    auto attcont = att->GetAttachmentContainer(true);
    auto child = interface_pointer_cast<META_NS::IObject>(
        attcont->FindAny({name, META_NS::TraversalType::NO_HIERARCHY, {}, false}));
    if (!child) {
        CORE_LOG_W("No such attachment for object [%s]", name.c_str());
    }
    return child;
}

static META_NS::IObject::Ptr FindPropertyChild(
    META_NS::IObject& obj, const BASE_NS::string& name, size_t index, bool dive)
{
    auto md = interface_cast<META_NS::IMetadata>(&obj);
    if (!md) {
        return nullptr;
    }
    auto p = md->GetProperty(name);
    if (!p) {
        CORE_LOG_W("No such property for object [%s]", name.c_str());
    }
    // if the path is not empty we try to take the object in the property to continue
    if (dive) {
        return GetIObjectFromProperty(p, index);
    }
    return interface_pointer_cast<META_NS::IObject>(p);
}

static META_NS::IObject::Ptr FindContainerChild(META_NS::IObject& obj, const BASE_NS::string& name)
{
    auto cont = interface_cast<META_NS::IContainer>(&obj);
    if (!cont) {
        return nullptr;
    }
    auto child = cont->FindByName(name);
    if (!child) {
        CORE_LOG_W("No such child for object [%s]", name.c_str());
    }
    return child;
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
        child = FindAttachmentChild(obj, name);
    } else if (pre == '.') {
        child = FindPropertyChild(obj, name, index, !path.empty());
    } else if (pre == '/') {
        child = FindContainerChild(obj, name);
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
            META_NS::CopyToDefault(p, *target);
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
        AddFlatProperty(p, node, templateContext);
    }
}

void SceneImportContext::AddFlatAttachments(
    const ISceneExternalNodeSer& in, const META_NS::IObject::Ptr& parent, bool templateContext)
{
    for (auto&& p : in.GetAttachments()) {
        attachments_.push_back(AttachInfo{interface_pointer_cast<META_NS::IObject>(p), parent, templateContext});
    }
}

void SceneImportContext::AddComponent(const IComponentSer::Ptr& c, const INode::Ptr& node, bool templateContext)
{
    if (auto component = scene_->CreateComponent(node, c->GetName()).GetResult()) {
        if (auto out = interface_cast<META_NS::IObject>(component)) {
            if (auto meta = interface_cast<META_NS::IMetadata>(c)) {
                AddFlatProperties(*meta, *out, templateContext);
            }
        }
    } else {
        CORE_LOG_W("Failed to add component: %s", c->GetName().c_str());
    }
}

void SceneImportContext::AddFlatComponents(
    const ISceneExternalNodeSer& in, META_NS::IObject& parent, bool templateContext)
{
    for (auto&& p : in.GetComponents()) {
        IComponentSer::Ptr comp;
        if (p) {
            comp = interface_pointer_cast<IComponentSer>(p->GetAttachment());
        }
        if (comp) {
            if (auto node = interface_pointer_cast<SCENE_NS::INode>(FindObject(parent, p->GetPath()))) {
                AddComponent(comp, node, templateContext);
            }
        }
    }
}

void SceneImportContext::SetAttachments(const ISceneNodeSer& in, const META_NS::IObject::Ptr& out)
{
    for (auto&& v : in.GetAttachments()) {
        attachments_.push_back(AttachInfo{v, out, isTemplateContext_});
    }
}

void SceneImportContext::SetComponents(const BASE_NS::vector<IComponentSer::Ptr>& comps, const INode::Ptr& node)
{
    for (auto&& c : comps) {
        if (c) {
            AddComponent(c, node);
        }
    }
}

void SceneImportContext::CopyObjectValues(const ISceneNodeSer& in, const META_NS::IObject::Ptr& out)
{
    if (auto meta = interface_cast<META_NS::IMetadata>(&in)) {
        AddFlatProperties(*meta, *out, isTemplateContext_);
    }
    SetAttachments(in, out);
}

META_NS::IObject::Ptr SceneImportContext::FindTemplateAttachment(
    META_NS::IAttach& parent, const META_NS::IObject::Ptr& old)
{
    BASE_NS::vector<META_NS::IObject::Ptr> res;
    for (auto&& att : parent.GetAttachments()) {
        if (META_NS::IsFlagSet(att, IMPORTED_FROM_TEMPLATE_BIT)) {
            if (att->GetClassId() == old->GetClassId()) {
                res.push_back(att);
            }
        }
    }
    if (res.size() > 1) {
        // which one is correct, try last effort with name
        for (auto&& v : res) {
            if (v->GetName() == old->GetName()) {
                return v;
            }
        }
    }
    return res.empty() ? nullptr : res[0];
}

static void SetCurrentAsDefaultAndCopyValue(
    const META_NS::IProperty::Ptr& source, const META_NS::IProperty::Ptr& target)
{
    if (META_NS::PropertyLock plock{source}) {
        if (META_NS::PropertyLock lock{target}) {
            lock->SetDefaultValueAny(target->GetValue());
            if (!target->SetValue(source->GetValue())) {
                CORE_LOG_W("Failed to set property value [%s]", target->GetName().c_str());
            }
        }
    }
}

void SceneImportContext::CopyAttachmentProperties(const META_NS::IObject::Ptr& from, const META_NS::IObject::Ptr& to)
{
    auto fromMd = interface_cast<META_NS::IMetadata>(from);
    auto toMd = interface_cast<META_NS::IMetadata>(to);
    if (!fromMd || !toMd) {
        return;
    }
    for (auto&& p : fromMd->GetProperties()) {
        if (META_NS::IsValueSet(*p)) {
            if (auto target = toMd->GetProperty(p->GetName())) {
                SetCurrentAsDefaultAndCopyValue(p, target);
            }
        }
    }
}

static void SetPropertiesToDefault(const META_NS::IObject::Ptr& obj)
{
    auto md = interface_cast<META_NS::IMetadata>(obj);
    if (!md) {
        return;
    }
    for (auto&& p : md->GetProperties()) {
        if (META_NS::PropertyLock lock{p}) {
            if (auto sc = interface_cast<META_NS::IStackProperty>(p)) {
                sc->SetDefaultValue(p->GetValue());
            }
            p->ResetValue();
        }
    }
}

INode::Ptr SceneImportContext::ConstructNode(const ISceneNodeSer& ser, INode& parent)
{
    INode::Ptr res;
    if (auto obj = interface_cast<META_NS::IObject>(&parent)) {
        res = scene_->CreateNode(interface_pointer_cast<INode>(obj->GetSelf()), ser.GetName(), ser.GetId()).GetResult();
        if (auto obj = interface_pointer_cast<META_NS::IObject>(res)) {
            SetComponents(ser.GetComponents(), res);
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
        CORE_NS::ResourceIdContext pres{in.GetResourceId(), scene_};
        auto resource = resources_->GetResource(pres);
        // we first try with context and if not found, try without
        if (!resource) {
            pres.context = nullptr;
            resource = resources_->GetResource(pres);
        }
        auto sgroups = in.GetSubresourceGroups();
        if (auto res = interface_pointer_cast<IScene>(resource)) {
            node = imp->ImportChildScene(res, in.GetName(), sgroups.empty() ? "" : sgroups.front()).GetResult();
        } else if (auto res = interface_pointer_cast<META_NS::IObjectTemplate>(resource)) {
            node = imp->ImportTemplate(res, sgroups.empty() ? "" : sgroups.front()).GetResult();
        } else {
            CORE_LOG_W("No resource/invalid resource: %s", in.GetResourceId().ToString().c_str());
        }
        if (auto n = interface_pointer_cast<META_NS::IObject>(node)) {
            if (auto m = interface_cast<META_NS::IMetadata>(&in)) {
                AddFlatComponents(in, *n, isTemplateContext_);
                AddFlatPropertiesForExternal(*m, *n, isTemplateContext_);
                AddFlatAttachments(in, n, isTemplateContext_);
            }
        }
        // if we spawned this resource, clean it up
        if (resource.use_count() == 2) {
            resources_->PurgeResource(pres);
        }
    } else {
        CORE_LOG_W("Failed to import scene resource to scene");
    }
    return node;
}

void SceneImportContext::BuildChildNodes(const META_NS::IObject& data, INode& node)
{
    META_NS::Internal::ConstIterate(
        interface_cast<META_NS::IIterable>(&data),
        [&](META_NS::IObject::Ptr obj) {
            if (obj) {
                BuildSceneNodeHierarchy(*obj, node);
            }
            return true;
        },
        META_NS::IterateStrategy{META_NS::TraversalType::NO_HIERARCHY});
}

INode::Ptr SceneImportContext::BuildSceneNodeHierarchy(const META_NS::IObject& data, INode& parent)
{
    INode::Ptr node;
    if (auto ext = interface_cast<ISceneExternalNodeSer>(&data)) {
        node = LoadExternalNode(*ext, parent);
    } else if (auto nodeser = interface_cast<ISceneNodeSer>(&data)) {
        node = ConstructNode(*nodeser, parent);
        if (node) {
            BuildChildNodes(data, *node);
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
            iScene->SetResourceGroups(StringToResourceGroupBundle(scene_, groups));
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
            // if root node is set to non-serialize, we just use the default constructed one.
            return scene_;
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
        auto obj = interface_pointer_cast<META_NS::IObject>(root);
        if (!obj) {
            return scene_;
        }
        SetComponents(rootdata->GetComponents(), root);
        CopyObjectValues(*rootdata, obj);
        BuildChildNodes(*nodes.front(), *root);
        return scene_;
    }
    return nullptr;
}

INode::Ptr SceneImportContext::BuildSceneHierarchy(INode& node, META_NS::IObject& data, const NodeImportOptions& opts)
{
    if (!opts.name.empty()) {
        if (auto ext = interface_cast<ISceneExternalNodeSer>(&data)) {
            ext->SetName(opts.name);
        } else if (auto np = interface_cast<ISceneNodeSer>(&data)) {
            np->SetName(opts.name);
        }
    }
    return BuildSceneNodeHierarchy(data, node);
}

static void DoAttach(const META_NS::IObject::Ptr& att, META_NS::IAttach& target, bool isInTemplate)
{
    if (isInTemplate) {
        SetPropertiesToDefault(att);
    }
    META_NS::SetObjectFlags(att, IMPORTED_FROM_TEMPLATE_BIT, isInTemplate);
    if (!target.Attach(att)) {
        CORE_LOG_W("Failed to attach");
    }
}

void SceneImportContext::DoExternalAttach(const IExternalAttachment& ext, const AttachInfo& v)
{
    if (auto target = interface_pointer_cast<META_NS::IAttach>(FindObject(*v.parent, ext.GetPath()))) {
        if (META_NS::IsFlagSet(ext.GetAttachment(), IMPORTED_FROM_TEMPLATE_BIT)) {
            if (auto existing = FindTemplateAttachment(*target, ext.GetAttachment())) {
                CopyAttachmentProperties(ext.GetAttachment(), existing);
            }
        } else {
            DoAttach(ext.GetAttachment(), *target, v.isInTemplate);
        }
    } else {
        CORE_LOG_W("Failed to find object to attach to [%s]", ext.GetPath().c_str());
    }
}

void SceneImportContext::DoDeferredAttach()
{
    for (auto&& v : attachments_) {
        if (auto ext = interface_cast<IExternalAttachment>(v.data)) {
            DoExternalAttach(*ext, v);
        } else if (auto target = interface_cast<META_NS::IAttach>(v.parent)) {
            DoAttach(v.data, *target, v.isInTemplate);
        }
    }
}

void SceneImportContext::DoPostLoad(const IScene::Ptr& scene)
{
    auto is = scene ? scene->GetInternalScene() : nullptr;
    if (is) {
        is->RunDirectlyOrInTask([is]() { is->PostLoad(); });
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

    // Cache an rman for the duration of the scene import so image resource
    // types dispatch loads to the shared worker pool; the scope is cleared
    // when this function returns.
    IRenderContext::Ptr renderCtx;
    if (auto is = scene_ ? scene_->GetInternalScene() : nullptr) {
        renderCtx = is->GetContext();
        if (renderCtx) {
            rman_ = META_NS::GetObjectRegistry().Create<IRenderResourceManager>(
                ClassId::RenderResourceManager, CreateRenderContextArg(renderCtx));
        }
    }
    IRenderContext::RenderContextScope importScope(
        renderCtx, IRenderContext::ScopeType::SceneLoad | IRenderContext::ScopeType::ImporterDeferred);

    importer->SetResourceManager(resources_);
    importer->SetUserContext(interface_pointer_cast<META_NS::IObject>(scene_));

    auto obj = importer->Import(in, META_NS::ImportOptions{true});
    if (!obj) {
        return nullptr;
    }
    auto node = interface_cast<ISceneNodeSer>(obj);
    if (!node) {
        return nullptr;
    }
    auto scene = BuildSceneHierarchy(*node);
    if (!scene) {
        return nullptr;
    }
    importer->ResolveDeferred();
    if (rman_) {
        rman_->WaitAllPendingLoads();
    }
    DoDeferredAttach();
    DoPostLoad(scene);
    return scene;
}
INodeImportResult::Ptr SceneImportContext::ImportNode(
    CORE_NS::IFile& in, const INode::Ptr& parent, NodeImportOptions opts)
{
    isTemplateContext_ = opts.applyAsTemplate;

    INode::Ptr res;
    if (parent) {
        auto importer = META_NS::GetObjectRegistry().Create<META_NS::IFileImporter>(META_NS::ClassId::JsonImporter);
        importer->SetResourceManager(resources_);
        importer->SetUserContext(interface_pointer_cast<META_NS::IObject>(scene_));
        if (auto obj = importer->Import(in, META_NS::ImportOptions{true})) {
            res = BuildSceneHierarchy(*parent, *obj, opts);
            if (res) {
                importer->ResolveDeferred();
                DoDeferredAttach();
                DoPostLoad(scene_);
            }
        }
    }
    return res ? INodeImportResult::Ptr(new NodeImportResult{res}) : nullptr;
}

static void RewriteGroupInOptionData(
    const BASE_NS::string& oldGroup, const BASE_NS::string& group, CORE_NS::ResourceInfo& info)
{
    if (oldGroup.empty() || group.empty()) {
        return;
    }

    auto importer = META_NS::GetObjectRegistry().Create<META_NS::ISerInput>(META_NS::ClassId::JsonInput);
    auto exporter = META_NS::GetObjectRegistry().Create<META_NS::ISerOutput>(META_NS::ClassId::JsonOutput);
    if (!importer || !exporter) {
        return;
    }
    auto opts = interface_cast<META_NS::IObjectResourceOptions>(info.options);
    if (!opts) {
        return;
    }

    auto optionData = opts->GetOptionData();
    if (optionData.empty()) {
        return;
    }
    BASE_NS::string in{reinterpret_cast<char*>(optionData.data()), optionData.size()};
    auto node = importer->Process(in);
    if (node) {
        // change the groups
        NodeTemplateGroupRewrite ntgw(oldGroup, group);
        node = ntgw.Process(node);
        if (ntgw.changed) {
            auto s = exporter->Process(node);
            optionData = BASE_NS::vector<uint8_t>(
                reinterpret_cast<uint8_t*>(s.data()), reinterpret_cast<uint8_t*>(s.data() + s.size()));
            opts->SetOptionData(optionData);
        }
    }
}

void SceneImportContext::MergeExistingResources(
    BASE_NS::vector<CORE_NS::ResourceInfo>& resources, BASE_NS::vector<CORE_NS::ResourceIdContext>& importedResources)
{
    auto existingResources = GetAlreadyExistingResources(resources, resources_, scene_);

    // need to instantiate the resources before overwriting the resource data so that we can merge the data
    BASE_NS::vector<CORE_NS::IResource::Ptr> resInstances;
    for (auto&& r : existingResources) {
        auto instance = resources_->GetResource({r, scene_});
        resInstances.push_back(instance);
        importedResources.erase(
            std::remove_if(importedResources.begin(), importedResources.end(), [&](auto l) { return l.id == r; }),
            importedResources.end());
    }

    CopyResourceInfos(resources, resources_, scene_);

    META_NS::IObject::Ptr context(this, [](auto) {});
    auto ext = interface_cast<META_NS::IResourceManagerExtension>(resources_);
    for (auto&& r : resInstances) {
        if (r && ext) {
            ext->ReapplyOptions(r, context);
            ext->UpdateOptionsData({r->GetResourceId()}, context);
        }
    }
}

META_NS::ISerNode::ConstPtr SceneImportContext::HandleTemplateResources(const IExportedNodeInternal& templ,
    const NodeImportOptions& opts, BASE_NS::vector<CORE_NS::ResourceIdContext>& importedResources,
    BASE_NS::vector<CORE_NS::ResourceIdContext>& associatedResources)
{
    META_NS::ISerNode::ConstPtr node = templ.GetNodeHierarchy();

    auto expNode = interface_cast<IExportedNode>(&templ);
    if (!expNode) {
        return node;
    }

    auto oldGroup = expNode->GetPrimaryGroup();
    if (!oldGroup.empty() && !opts.group.empty()) {
        NodeTemplateGroupRewrite ntgw(oldGroup, opts.group);
        node = ntgw.Process(node);
    }

    auto resources = expNode->GetResources();

    for (auto&& r : resources) {
        if (!opts.group.empty() && !oldGroup.empty() && r.id.group == oldGroup) {
            r.id.group = opts.group;
        }
        if (r.options) {
            RewriteGroupInOptionData(oldGroup, opts.group, r);
        }
        associatedResources.push_back({r.id, scene_});
        importedResources.push_back({r.id, scene_});
    }

    // remove animations, those will come in from embedded gltf and are not re-used (we don't want to end up with
    // duplicates)
    resources.erase(std::remove_if(resources.begin(),
                        resources.end(),
                        [](const auto& i) { return i.type == ClassId::AnimationResource.Id().ToUid(); }),
        resources.end());

    MergeExistingResources(resources, importedResources);

    return node;
}

INodeImportResult::Ptr SceneImportContext::ImportNode(
    const IExportedNodeInternal& templ, const INode::Ptr& parent, NodeImportOptions opts)
{
    isTemplateContext_ = opts.applyAsTemplate;

    INode::Ptr res;
    BASE_NS::vector<CORE_NS::ResourceIdContext> importedResources;
    BASE_NS::vector<CORE_NS::ResourceIdContext> associatedResources;
    if (parent) {
        auto node = HandleTemplateResources(templ, opts, importedResources, associatedResources);

        auto importer = META_NS::GetObjectRegistry().Create<META_NS::IFileImporter>(META_NS::ClassId::JsonImporter);
        importer->SetResourceManager(resources_);
        importer->SetUserContext(interface_pointer_cast<META_NS::IObject>(scene_));

        if (auto obj = importer->Import(node, META_NS::ImportOptions{true})) {
            res = BuildSceneHierarchy(*parent, *obj, opts);
            if (res) {
                importer->ResolveDeferred();
                DoDeferredAttach();
                DoPostLoad(scene_);
            }
        }
    }
    return res ? INodeImportResult::Ptr(
                     new NodeImportResult{res, BASE_NS::move(importedResources), BASE_NS::move(associatedResources)})
               : nullptr;
}
bool SceneImportContext::LoadResourceIndex(BASE_NS::string_view resourceIndex)
{
    CORE_LOG_D("Loading index file: %s", BASE_NS::string(resourceIndex).c_str());
    auto res = GetResourceManager(scene_);
    return res->Import(resourceIndex, scene_) == CORE_NS::IResourceManager::Result::OK;
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

IScene::Ptr SceneImporter::ImportScene(
    CORE_NS::IFile& in, const CORE_NS::ResourceId& requestedId, BASE_NS::string_view resourceIndex)
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
    if (!m) {
        return nullptr;
    }
    auto resources = context->GetResources();
    if (!resources) {
        return nullptr;
    }
    auto scene = m->CreateScene().GetResult();
    if (!scene) {
        return nullptr;
    }
    SceneImportContext ic(scene, resources, requestedId);
    if (!resourceIndex.empty() && !ic.LoadResourceIndex(resourceIndex)) {
        CORE_LOG_W("Failed to load resource index");
        return nullptr;
    }
    return ic.ImportScene(in);
}
INodeImportResult::Ptr SceneImporter::ImportNode(CORE_NS::IFile& in, const INode::Ptr& parent, NodeImportOptions opts)
{
    if (!parent) {
        return nullptr;
    }
    if (auto resources = GetResourceManager(parent->GetScene())) {
        return SceneImportContext(parent->GetScene(), resources).ImportNode(in, parent, opts);
    }
    return nullptr;
}

INodeImportResult::Ptr SceneImporter::ImportNode(
    const META_NS::IObject& obj, const INode::Ptr& parent, NodeImportOptions opts)
{
    if (!parent) {
        return nullptr;
    }
    if (auto resources = GetResourceManager(parent->GetScene())) {
        if (auto templ = interface_cast<IExportedNodeInternal>(&obj)) {
            return SceneImportContext(parent->GetScene(), resources).ImportNode(*templ, parent, opts);
        }
    }
    return nullptr;
}

bool CreateObjectResourceOptions(const CORE_NS::IResource::ConstPtr& p, const CORE_NS::ResourceManagerPtr& rm,
    const CORE_NS::ResourceContextPtr& context, CORE_NS::IResourceOptions& options)
{
    auto opts = interface_cast<META_NS::IObjectResourceOptions>(&options);
    if (!opts) {
        return false;
    }
    bool res = false;

    auto in = interface_cast<META_NS::IMetadata>(p);
    auto out = interface_cast<META_NS::IMetadata>(opts);
    if (auto i = interface_cast<META_NS::IDerivedFromTemplate>(p)) {
        auto resource = i->GetTemplateId();
        opts->SetBaseResource(resource);
        res = resource.IsValid();
    }
    if (in && out) {
        if (auto att = interface_cast<META_NS::IAttach>(out)) {
            att->GetAttachmentContainer(true)->RemoveAll();
        }
        res |= SerCloneAllToDefaultIfSet(*in, *out);
        if (res) {
            META_NS::MemFile file;
            opts->Save(file, rm, context);
            opts->SetOptionData(file.Data());
        }
    }
    return res;
}

void ApplyObjectResourceOptions(const CORE_NS::ResourceId& id, const CORE_NS::IResource::Ptr& res,
    CORE_NS::IResourceOptions& options, const CORE_NS::ResourceManagerPtr& rm,
    const CORE_NS::ResourceContextPtr& context)
{
    auto opts = interface_cast<META_NS::IObjectResourceOptions>(&options);
    if (!opts || opts->GetOptionData().empty()) {
        return;
    }
    META_NS::MemFile file(opts->GetOptionData());
    opts->Load(file, rm, context);
    if (auto i = interface_cast<META_NS::IDerivedFromTemplate>(res)) {
        auto base = opts->GetBaseResource();
        if (base.IsValid()) {
            // hack, first try without context and then with context
            auto r = rm->GetResource(CORE_NS::ResourceIdContext{base});
            if (!r) {
                r = rm->GetResource({base, context});
            }
            if (!r) {
                CORE_LOG_W("Could not load base resource for %s", id.ToString().c_str());
            }
            if (!i->SetTemplate(r)) {
                CORE_LOG_W("Failed to apply template for resource %s", id.ToString().c_str());
            }
        }
    }
    auto in = interface_cast<META_NS::IMetadata>(opts);
    auto out = interface_cast<META_NS::IMetadata>(res);
    if (in && out) {
        auto rcontext = interface_cast<IResourceContext>(context);
        SerCopy(*in, *out, rcontext && rcontext->IsNodeTemplateContext());
    }
}

BASE_NS::vector<CORE_NS::ResourceId> GetAlreadyExistingResources(
    const BASE_NS::array_view<const CORE_NS::ResourceInfo>& infos, CORE_NS::IResourceManager::Ptr& dest,
    const CORE_NS::ResourceContextPtr& context)
{
    BASE_NS::vector<CORE_NS::ResourceId> res;
    if (dest) {
        for (auto&& i : infos) {
            auto destI = dest->GetResourceInfo({i.id, context});
            if (destI.id == i.id && destI.path == i.path && destI.type == i.type) {
                res.push_back(i.id);
            }
        }
    }
    return res;
}

ResourceGroupBundle StringToResourceGroupBundle(
    const IScene::Ptr& scene, const BASE_NS::vector<BASE_NS::string>& groups)
{
    ResourceGroupBundle bundle;
    if (auto res = interface_pointer_cast<META_NS::IOwnedResourceGroups>(GetResourceManager(scene))) {
        for (auto&& g : groups) {
            bundle.PushGroupHandleToBack(res->GetGroupHandle(g, scene));
        }
    }
    return bundle;
}

SCENE_END_NAMESPACE()
