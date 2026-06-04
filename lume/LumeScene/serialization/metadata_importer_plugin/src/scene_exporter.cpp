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
#include "scene_exporter.h"

#include <algorithm>
#include <scene/ext/intf_component.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_light_probe_group.h>
#include <scene/interface/intf_mesh.h>

#include <core/intf_engine.h>

#include <meta/api/metadata_util.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/resource/intf_resource_manager_extension.h>

#include "light_probe_group_ser.h"
#include "node_instantiator.h"
#include "resource_util.h"
#include "scene_ser.h"
#include "serialization_util.h"

SCENE_BEGIN_NAMESPACE()

static IComponentSer::Ptr CreateComponentSer(const IComponent::Ptr& comp)
{
    auto cs = META_NS::GetObjectRegistry().Create<IComponentSer>(ClassId::ComponentSer);
    if (cs) {
        cs->SetName(comp->GetName());
        if (auto gen = interface_cast<IGenericComponent>(comp)) {
            cs->SetId(gen->GetComponentId());
        }
        if (auto obj = interface_cast<META_NS::IObject>(comp)) {
            if (auto md = interface_cast<META_NS::IMetadata>(cs)) {
                AddObjectProperties(*obj, *md);
            }
        }
    }
    return cs;
}

static bool HandleReadOnlyProperty(
    const META_NS::IProperty::ConstPtr& p, const BASE_NS::string& head, META_NS::IMetadata& out)
{
    bool res = false;
    if (META_NS::ArrayPropertyLock arr{p}) {
        for (size_t i = 0; i != arr->GetSize(); ++i) {
            if (auto obj = interface_cast<META_NS::IObject>(META_NS::GetConstPointer(arr->GetAnyAt(i)))) {
                res |= AddObjectProperties(
                    *obj, head + "." + EscapeSerName(p->GetName()) + "[" + BASE_NS::to_string(i) + "]", out);
            }
        }
    } else {
        if (auto obj = META_NS::GetConstPointer<META_NS::IObject>(p)) {
            res |= AddObjectProperties(*obj, head + "." + EscapeSerName(p->GetName()), out);
        }
    }
    return res;
}

bool AddObjectProperties(const META_NS::IObject& obj, const BASE_NS::string& head, META_NS::IMetadata& out)
{
    bool res = false;
    if (auto i = interface_cast<META_NS::IMetadata>(&obj)) {
        for (auto&& p : GetAllProperties(*i)) {
            if (i->GetMetadata(META_NS::MetadataType::PROPERTY, p->GetName()).readOnly) {
                res |= HandleReadOnlyProperty(p, head, out);
            } else {
                res |= META_NS::CloneToDefaultIfSet(p, out, head + "." + EscapeSerName(p->GetName()));
            }
        }
    }
    return res;
}

static IEcsContext* GetEcsContext(const META_NS::IObject& node)
{
    IEcsContext* ecs = nullptr;
    if (auto n = interface_cast<INode>(&node)) {
        if (auto s = n->GetScene()) {
            if (auto is = s->GetInternalScene()) {
                ecs = &is->GetEcsContext();
            }
        }
    }
    return ecs;
}

static IComponentSer::Ptr CreateGenericComponentSer(IEcsContext* ecs, const META_NS::IObject::Ptr& att)
{
    if (ecs && interface_cast<IGenericComponent>(att)) {
        if (auto comp = interface_pointer_cast<IComponent>(att)) {
            if (!ecs->IsDefaultComponent(comp->GetName())) {
                return CreateComponentSer(comp);
            }
        }
    }
    return nullptr;
}

static void AddGenericComponent(
    IEcsContext* ecs, const META_NS::IObject::Ptr& att, BASE_NS::vector<IComponentSer::Ptr>& gComponents)
{
    if (auto s = CreateGenericComponentSer(ecs, att)) {
        gComponents.push_back(s);
    }
}

static void EnsureLightProbeGroupSerAttached(const META_NS::IObject& node)
{
    if (!interface_cast<ILightProbeGroup>(&node)) {
        return;
    }
    auto attach = interface_pointer_cast<META_NS::IAttach>(node.GetSelf());
    if (!attach) {
        return;
    }
    for (const auto& att : attach->GetAttachments()) {
        if (att->GetClassId() == SCENE_MIMP_NS::ClassId::LightProbeGroupSer) {
            // node already has a LightProbeGroupSer serializer
            return;
        }
    }
    auto ser = META_NS::GetObjectRegistry().Create(SCENE_MIMP_NS::ClassId::LightProbeGroupSer);
    if (ser) {
        attach->Attach(ser);
    } else {
        CORE_LOG_W("Failed to create LightProbeGroupSer serializer for LightProbeGroupNode export");
    }
}

static void AddObjectAttachments(const META_NS::IObject& node, ISceneNodeSer& out)
{
    auto ecsContext = GetEcsContext(node);

    if (auto i = interface_cast<META_NS::IAttach>(&node)) {
        // LightProbeGroupNode is a special case for serialization as the serializer needs to access the ECS to store
        // the data. If the node is an ILightProbeGroup, this makes sure that the serializer attachment is created and
        // added
        EnsureLightProbeGroupSerAttached(node);
        BASE_NS::vector<META_NS::IObject::Ptr> vec;
        BASE_NS::vector<IComponentSer::Ptr> gComponents;
        for (auto&& att : i->GetAttachments()) {
            // serialise generic components
            AddGenericComponent(ecsContext, att, gComponents);

            if (SerialiseAttachment(att)) {
                vec.push_back(att);
            }
        }
        out.SetAttachments(BASE_NS::move(vec));
        if (auto node = interface_cast<ISceneNodeSer>(&out)) {
            node->SetComponents(gComponents);
        }
    }
}

static void AddExternalAttachments(
    const META_NS::IObject& node, const BASE_NS::string& path, ISceneExternalNodeSer& out)
{
    auto ecsContext = GetEcsContext(node);

    if (auto i = interface_cast<META_NS::IAttach>(&node)) {
        BASE_NS::vector<META_NS::IObject::Ptr> vec;
        for (auto&& att : i->GetAttachments()) {
            // serialise generic components
            if (auto s = CreateGenericComponentSer(ecsContext, att)) {
                out.AddComponent(path, s);
            }
            if (SerialiseAttachment(att)) {
                out.AddAttachment(path, att);
            }
        }
    }
}

static void AddObjectData(const META_NS::IObject& node, const BASE_NS::string& head, META_NS::IMetadata& out)
{
    if (auto ext = interface_cast<ISceneExternalNodeSer>(&out)) {
        AddExternalAttachments(node, head, *ext);
    }
    AddObjectProperties(node, head, out);
}

static void AddObjectDataRecursive(const META_NS::IObject& obj, const BASE_NS::string& head, META_NS::IMetadata& out)
{
    AddObjectData(obj, head, out);

    META_NS::Internal::ConstIterate(
        interface_cast<META_NS::IIterable>(&obj),
        [&](META_NS::IObject::Ptr nodeObj) {
            AddObjectDataRecursive(*nodeObj, head + "/" + EscapeSerName(nodeObj->GetName()), out);
            return true;
        },
        META_NS::IterateStrategy{META_NS::TraversalType::NO_HIERARCHY});
}

static void AddObjectDataRecursive(const META_NS::IObject& obj, META_NS::IMetadata& out)
{
    AddObjectDataRecursive(obj, "", out);
}

static void SetObjectData(const META_NS::IObject& node, ISceneNodeSer& out)
{
    if (auto outMeta = interface_cast<META_NS::IMetadata>(&out)) {
        out.SetName(node.GetName());
        out.SetId(node.GetClassId());
        AddObjectAttachments(node, out);
        AddObjectData(node, "", *outMeta);
    }
}

META_NS::IObject::Ptr SceneExporterContext::BuildExternalNode(const META_NS::IObject& node, const IExternalNode& ext)
{
    if (exportedResources_) {
        exportedResources_->push_back(ext.GetResourceId());
    }

    auto obj = META_NS::GetObjectRegistry().Create<ISceneExternalNodeSer>(ClassId::SceneExternalNodeSer);
    if (obj) {
        obj->SetName(node.GetName());
        obj->SetResourceId(ext.GetResourceId().id);
        auto handle = ext.GetSubresourceGroup();
        obj->AddSubresourceGroup(handle ? handle->GetGroup() : "");
        if (auto mout = interface_cast<META_NS::IMetadata>(obj)) {
            AddObjectDataRecursive(node, *mout);
        }
    }
    return interface_pointer_cast<META_NS::IObject>(obj);
}

META_NS::IObject::Ptr SceneExporterContext::BuildObjectNode(const META_NS::IObject& node)
{
    if (!META_NS::IsFlagSet(&node, META_NS::ObjectFlagBits::SERIALIZE)) {
        return nullptr;
    }
    META_NS::IObject::Ptr obj;
    if (auto att = interface_cast<META_NS::IAttach>(&node)) {
        auto ext = att->GetAttachments<IExternalNode>();
        if (!ext.empty()) {
            obj = BuildExternalNode(node, *ext.front());
        }
    }
    if (!obj) {
        obj = META_NS::GetObjectRegistry().Create(ClassId::SceneNodeSer);
        auto cont = interface_pointer_cast<META_NS::IContainer>(obj);
        if (!cont) {
            return obj;
        }
        if (auto i = interface_cast<ISceneNodeSer>(cont)) {
            SetObjectData(node, *i);
        }
        META_NS::Internal::ConstIterate(
            interface_cast<META_NS::IIterable>(&node),
            [&](META_NS::IObject::Ptr nodeObj) {
                if (auto obj = BuildObjectNode(*nodeObj)) {
                    cont->Add(obj);
                }
                return true;
            },
            META_NS::IterateStrategy{META_NS::TraversalType::NO_HIERARCHY});
    }
    return obj;
}

void SceneExporterContext::SetResourceGroups(ISceneSer& out)
{
    if (auto iScene = scene_->GetInternalScene()) {
        BASE_NS::vector<BASE_NS::string> groups;
        auto bundle = iScene->GetResourceGroups();
        for (auto&& g : bundle.GetAllHandles()) {
            groups.push_back(g->GetGroup());
        }
        out.SetResourceGroups(BASE_NS::move(groups));
    }
}

META_NS::IObject::Ptr SceneExporterContext::BuildObjectHierarchy()
{
    META_NS::IObject::Ptr res = META_NS::GetObjectRegistry().Create(ClassId::SceneSer);
    if (!res) {
        return res;
    }
    auto obj = interface_cast<META_NS::IObject>(scene_);
    if (!obj) {
        return res;
    }
    auto ser = interface_cast<ISceneNodeSer>(res);
    if (!ser) {
        return res;
    }
    SetObjectData(*obj, *ser);
    if (auto ser = interface_cast<ISceneSer>(res)) {
        SetResourceGroups(*ser);
    }
    if (auto i = interface_cast<META_NS::IContainer>(res)) {
        if (auto root = interface_pointer_cast<META_NS::IObject>(scene_->GetRootNode().GetResult())) {
            if (auto nodeObj = BuildObjectNode(*root)) {
                i->Add(nodeObj);
            }
        }
    }
    return res;
}

void SceneExporterContext::SetExportedResources(IExportedNode& n)
{
    auto r = scene_->GetInternalScene()->GetContext();
    if (!r || !exportedResources_) {
        return;
    }
    auto resources = r->GetResources();
    auto ext = interface_cast<META_NS::IResourceManagerExtension>(resources);
    if (!ext) {
        return;
    }

    BASE_NS::vector<CORE_NS::ResourceIdContext> deps;
    BASE_NS::vector<CORE_NS::ResourceIdContext> d = *exportedResources_;

    std::size_t count = 0;
    do {
        d = ext->UpdateOptionsData(d);
        deps.insert(deps.end(), d.begin(), d.end());
    } while (!d.empty() && ++count < 6);

    BASE_NS::vector<CORE_NS::ResourceInfo> infos;
    for (auto&& v : *exportedResources_) {
        auto info = resources->GetResourceInfo(v);
        if (info.id.IsValid()) {
            infos.push_back(info);
        } else {
            CORE_LOG_W("Failed to find resource info: %s", v.id.ToString().c_str());
        }
    }
    for (auto&& v : deps) {
        auto info = resources->GetResourceInfo(v);
        if (info.id.IsValid()) {
            infos.push_back(info);
        } else {
            CORE_LOG_W("Failed to find resource info: %s", v.id.ToString().c_str());
        }
    }
    std::sort(infos.begin(), infos.end(), [](const auto& l, const auto& r) {
        return l.id.group < r.id.group || (l.id.group == r.id.group && l.id.name < r.id.name);
    });
    infos.erase(std::unique(infos.begin(), infos.end(), [](const auto& l, const auto& r) { return l.id == r.id; }),
        infos.end());
    n.SetResources(BASE_NS::move(infos));
    n.SetPrimaryGroup(scene_->GetResourceGroups().PrimaryGroup());
}

META_NS::IFileExporter::Ptr SceneExporterContext::CreateExporter()
{
    META_NS::IFileExporter::Ptr exporter;
    auto r = scene_->GetInternalScene()->GetContext();
    if (r) {
        exporter = META_NS::GetObjectRegistry().Create<META_NS::IFileExporter>(META_NS::ClassId::JsonExporter);
        exporter->SetResourceManager(r->GetResources());
        exporter->SetMetadata(
            META_NS::SerMetadataValues().SetVersion(SCENE_EXPORTER_VERSION).SetType(SCENE_EXPORTER_TYPE));
        exporter->SetUserContext(META_NS::IObject::Ptr(this, [](auto) {}));
    }
    return exporter;
}

META_NS::ReturnError SceneExporter::ExportScene(CORE_NS::IFile& out, const IScene::ConstPtr& scene)
{
    META_NS::ReturnError res = META_NS::GenericError::FAIL;
    if (scene) {
        SceneExporterContext context(scene->GetInternalScene()->GetScene());
        if (auto obj = context.BuildObjectHierarchy()) {
            auto exporter = context.CreateExporter();
            res = exporter->Export(out, obj);
        }
    }
    return res;
}
META_NS::ReturnError SceneExporter::ExportNode(CORE_NS::IFile& out, const INode::ConstPtr& p)
{
    META_NS::ReturnError res = META_NS::GenericError::FAIL;
    if (auto n = interface_cast<META_NS::IObject>(p)) {
        if (auto scene = p->GetScene()) {
            SceneExporterContext context(scene);
            if (auto obj = context.BuildObjectNode(*n)) {
                auto exporter = context.CreateExporter();
                res = exporter->Export(out, obj);
            }
        }
    }
    return res;
}

META_NS::IObject::Ptr SceneExporter::ExportNode(const INode::ConstPtr& p)
{
    auto n = interface_cast<META_NS::IObject>(p);
    auto scene = p->GetScene();
    if (!n || !scene) {
        return nullptr;
    }

    SceneExporterContext context(scene);
    context.CollectResources();

    if (auto rm = GetResourceManager(scene)) {
        for (auto&& r : rm->GetResources(scene)) {
            context.AddResource(CORE_NS::ResourceIdContext{r->GetResourceId(), r->GetContext()});
        }
    }

    auto obj = context.BuildObjectNode(*n);
    if (!obj) {
        return nullptr;
    }
    auto exporter = context.CreateExporter();
    auto ser = interface_pointer_cast<META_NS::IRootNode>(exporter->Export(obj));
    if (!ser) {
        return nullptr;
    }
    auto nt = META_NS::GetObjectRegistry().Create<IExportedNodeInternal>(ClassId::ExportedNode);
    if (!nt) {
        return nullptr;
    }
    nt->SetNodeHierarchy(ser->GetObject());
    if (auto i = interface_cast<IExportedNode>(nt)) {
        context.SetExportedResources(*i);
    }
    return interface_pointer_cast<META_NS::IObject>(nt);
}

SCENE_END_NAMESPACE()
