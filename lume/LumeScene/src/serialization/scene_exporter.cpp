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

#include <scene/ext/intf_component.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_mesh.h>

#include <meta/api/metadata_util.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/serialization/intf_exporter.h>

#include "../resource/node_instantiator.h"
#include "scene_ser.h"
#include "util.h"

SCENE_BEGIN_NAMESPACE()

static bool HandleReadOnlyProperty(
    const META_NS::IProperty::ConstPtr& p, const BASE_NS::string& head, META_NS::IMetadata& out)
{
    bool res = false;
    if (META_NS::ArrayPropertyLock arr { p }) {
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

static void AddObjectAttachments(const META_NS::IObject& node, ISceneNodeSer& out)
{
    if (auto i = interface_cast<META_NS::IAttach>(&node)) {
        BASE_NS::vector<META_NS::IObject::Ptr> vec;
        for (auto&& att : i->GetAttachments()) {
            if (SerialiseAttachment(att)) {
                vec.push_back(att);
            }
        }
        out.SetAttachments(BASE_NS::move(vec));
    }
}

static void AddExternalAttachments(
    const META_NS::IObject& node, const BASE_NS::string& path, ISceneExternalNodeSer& out)
{
    if (auto i = interface_cast<META_NS::IAttach>(&node)) {
        BASE_NS::vector<META_NS::IObject::Ptr> vec;
        for (auto&& att : i->GetAttachments()) {
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
        META_NS::IterateStrategy { META_NS::TraversalType::NO_HIERARCHY });
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

static META_NS::IObject::Ptr BuildExternalNode(const META_NS::IObject& node, const IExternalNode& ext)
{
    auto obj = META_NS::GetObjectRegistry().Create<ISceneExternalNodeSer>(ClassId::SceneExternalNodeSer);
    if (obj) {
        obj->SetName(node.GetName());
        obj->SetResourceId(ext.GetResourceId());
        obj->AddSubresourceGroup(ext.GetSubresourceGroup());
        if (auto mout = interface_cast<META_NS::IMetadata>(obj)) {
            AddObjectDataRecursive(node, *mout);
        }
    }
    return interface_pointer_cast<META_NS::IObject>(obj);
}

static META_NS::IObject::Ptr BuildObjectNode(const META_NS::IObject& node)
{
    META_NS::IObject::Ptr obj;
    if (auto att = interface_cast<META_NS::IAttach>(&node)) {
        auto ext = att->GetAttachments<IExternalNode>();
        if (!ext.empty()) {
            obj = BuildExternalNode(node, *ext.front());
        }
    }
    if (!obj) {
        obj = META_NS::GetObjectRegistry().Create(ClassId::SceneNodeSer);
        if (auto cont = interface_pointer_cast<META_NS::IContainer>(obj)) {
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
                META_NS::IterateStrategy { META_NS::TraversalType::NO_HIERARCHY });
        }
    }
    return obj;
}

static void SetResourceGroups(const IScene& scene, ISceneSer& out)
{
    if (auto iScene = scene.GetInternalScene()) {
        BASE_NS::vector<BASE_NS::string> groups;
        auto bundle = iScene->GetResourceGroups();
        for (auto&& g : bundle.GetAllHandles()) {
            groups.push_back(g->GetGroup());
        }
        out.SetResourceGroups(BASE_NS::move(groups));
    }
}

static META_NS::IObject::Ptr BuildObjectHierarchy(const IScene& scene)
{
    META_NS::IObject::Ptr res = META_NS::GetObjectRegistry().Create(ClassId::SceneSer);
    if (res) {
        if (auto obj = interface_cast<META_NS::IObject>(&scene)) {
            if (auto ser = interface_cast<ISceneNodeSer>(res)) {
                SetObjectData(*obj, *ser);
                if (auto ser = interface_cast<ISceneSer>(res)) {
                    SetResourceGroups(scene, *ser);
                }
                if (auto i = interface_cast<META_NS::IContainer>(res)) {
                    if (auto root = interface_pointer_cast<META_NS::IObject>(scene.GetRootNode().GetResult())) {
                        i->Add(BuildObjectNode(*root));
                    }
                }
            }
        }
    }
    return res;
}

static META_NS::IFileExporter::Ptr CreateExporter(const IScene::ConstPtr& scene)
{
    META_NS::IFileExporter::Ptr exporter;
    auto r = scene->GetInternalScene()->GetContext();
    if (r) {
        exporter = META_NS::GetObjectRegistry().Create<META_NS::IFileExporter>(META_NS::ClassId::JsonExporter);
        exporter->SetResourceManager(r->GetResources());
        exporter->SetMetadata(
            META_NS::SerMetadataValues().SetVersion(SCENE_EXPORTER_VERSION).SetType(SCENE_EXPORTER_TYPE));
        exporter->SetUserContext(
            interface_pointer_cast<META_NS::IObject>(IScene::Ptr(scene, const_cast<IScene*>(scene.get()))));
    }
    return exporter;
}

META_NS::ReturnError SceneExporter::ExportScene(CORE_NS::IFile& out, const IScene::ConstPtr& scene)
{
    META_NS::ReturnError res = META_NS::GenericError::FAIL;
    if (scene) {
        if (auto obj = BuildObjectHierarchy(*scene)) {
            auto exporter = CreateExporter(scene);
            res = exporter->Export(out, obj);
        }
    }
    return res;
}
META_NS::ReturnError SceneExporter::ExportNode(CORE_NS::IFile& out, const INode::ConstPtr& p)
{
    META_NS::ReturnError res = META_NS::GenericError::FAIL;
    if (auto n = interface_cast<META_NS::IObject>(p)) {
        if (auto obj = BuildObjectNode(*n)) {
            auto exporter = CreateExporter(p->GetScene());
            res = exporter->Export(out, obj);
        }
    }
    return res;
}

META_NS::IObject::Ptr SceneExporter::ExportNode(const INode::ConstPtr& p)
{
    META_NS::IObject::Ptr res;
    if (auto n = interface_cast<META_NS::IObject>(p)) {
        if (auto obj = BuildObjectNode(*n)) {
            auto exporter = CreateExporter(p->GetScene());
            if (auto ser = interface_pointer_cast<META_NS::IRootNode>(exporter->Export(obj))) {
                auto nt = META_NS::GetObjectRegistry().Create<INodeTemplateInternal>(ClassId::NodeTemplate);
                if (nt) {
                    nt->SetNodeHierarchy(ser->GetObject());
                    res = interface_pointer_cast<META_NS::IObject>(nt);
                }
            }
        }
    }
    return res;
}

SCENE_END_NAMESPACE()
