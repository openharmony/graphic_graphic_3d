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
#include <scene/api/external_node.h>
#include <scene/ext/intf_component.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_scene_manager.h>

#include <meta/api/metadata_util.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/serialization/intf_importer.h>

#include "scene_ser.h"
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

static META_NS::IObject::Ptr FindObject(META_NS::IObject& obj, BASE_NS::string_view path)
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
                attcont->FindAny<IComponent>(name, META_NS::TraversalType::NO_HIERARCHY));
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

static void AddFlatProperty(const META_NS::IProperty::ConstPtr& p, META_NS::IObject& parent)
{
    if (auto target = interface_pointer_cast<META_NS::IProperty>(FindObject(parent, p->GetName()))) {
        META_NS::CopyValue(p, *target);
    } else {
        CORE_LOG_W("Failed to resolve property [%s]", p->GetName().c_str());
    }
}

void AddFlatProperties(const META_NS::IMetadata& in, META_NS::IObject& parent)
{
    for (auto&& p : in.GetProperties()) {
        AddFlatProperty(p, parent);
    }
}

static void AddFlatAttachments(const ISceneExternalNodeSer& in, META_NS::IObject& parent)
{
    for (auto&& p : in.GetAttachments()) {
        if (auto target = interface_pointer_cast<META_NS::IAttach>(FindObject(parent, p->GetPath()))) {
            if (!target->Attach(p->GetAttachment())) {
                CORE_LOG_W("Failed to attach");
            }
        } else {
            CORE_LOG_W("Failed to find object to attach to [%s]", p->GetPath().c_str());
        }
    }
}

static void SetAttachments(const ISceneNodeSer& in, META_NS::IAttach& out)
{
    for (auto&& v : in.GetAttachments()) {
        out.Attach(v);
    }
}

static void CopyObjectValues(const ISceneNodeSer& in, META_NS::IObject& out)
{
    if (auto meta = interface_cast<META_NS::IMetadata>(&in)) {
        AddFlatProperties(*meta, out);
    }
    if (auto att = interface_cast<META_NS::IAttach>(&out)) {
        SetAttachments(in, *att);
    }
}

static INode::Ptr ConstructNode(const ISceneNodeSer& ser, INode& parent)
{
    auto& scene = *parent.GetScene();
    INode::Ptr res = scene.CreateNode(parent.GetPath().GetResult() + "/" + ser.GetName(), ser.GetId()).GetResult();
    if (auto obj = interface_cast<META_NS::IObject>(res)) {
        CopyObjectValues(ser, *obj);
    }
    return res;
}

static void LoadExternalNode(const ISceneExternalNodeSer& in, INode& parent)
{
    if (auto context = parent.GetScene()->GetInternalScene()->GetContext()) {
        if (!context->GetResources()) {
            CORE_LOG_W("Failed to import scene resource to scene, no resource manager set");
            return;
        }
        if (auto res = interface_pointer_cast<IScene>(context->GetResources()->GetResource(in.GetResourceId()))) {
            if (auto node = interface_pointer_cast<META_NS::IObject>(AddExternalNode(parent, res, in.GetName()))) {
                if (auto m = interface_cast<META_NS::IMetadata>(&in)) {
                    AddFlatProperties(*m, *node);
                    AddFlatAttachments(in, *node);
                }
            } else {
                CORE_LOG_W("Failed to import scene resource to scene");
            }
        } else {
            CORE_LOG_W("No resource/invalid resource: %s", in.GetResourceId().ToString().c_str());
        }
    }
}

static void BuildSceneNodeHierarchy(const ISceneNodeSer& data, INode& node)
{
    META_NS::Internal::ConstIterate(
        interface_cast<META_NS::IIterable>(&data),
        [&](META_NS::IObject::Ptr obj) {
            if (auto ext = interface_cast<ISceneExternalNodeSer>(obj)) {
                LoadExternalNode(*ext, node);
            } else if (auto ser = interface_cast<ISceneNodeSer>(obj)) {
                if (auto n = ConstructNode(*ser, node)) {
                    BuildSceneNodeHierarchy(*ser, *n);
                }
            }
            return true;
        },
        META_NS::IterateStrategy { META_NS::TraversalType::NO_HIERARCHY });
}

static IScene::Ptr BuildSceneHierarchy(const IScene::Ptr& scene, const ISceneNodeSer& data)
{
    if (auto obj = interface_cast<META_NS::IObject>(scene)) {
        CopyObjectValues(data, *obj);
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
        auto root = scene->GetRootNode().GetResult();
        if (auto obj = interface_cast<META_NS::IObject>(root)) {
            CopyObjectValues(*rootdata, *obj);
            BuildSceneNodeHierarchy(*rootdata, *root);
        }
        return scene;
    }
    return nullptr;
}

static INode::Ptr BuildSceneHierarchy(const INode::Ptr& node, const ISceneNodeSer& data)
{
    auto n = ConstructNode(data, *node);
    if (n) {
        BuildSceneNodeHierarchy(data, *n);
    }
    return n;
}

bool SceneImporter::Build(const META_NS::IMetadata::Ptr& d)
{
    bool res = Super::Build(d);
    if (res) {
        auto context = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
        if (!context) {
            CORE_LOG_E("Invalid arguments to construct SceneImporter");
            return false;
        }
        opts_ = GetBuildArg<SceneOptions>(d, "Options");
        context_ = context;
    }
    return res;
}

IScene::Ptr SceneImporter::ImportScene(CORE_NS::IFile& in)
{
    IScene::Ptr res;
    auto importer = META_NS::GetObjectRegistry().Create<META_NS::IFileImporter>(META_NS::ClassId::JsonImporter);
    auto context = context_.lock();
    if (context) {
        importer->SetResourceManager(context->GetResources());
    }
    auto md = CreateRenderContextArg(context);
    if (md) {
        md->AddProperty(META_NS::ConstructProperty<SceneOptions>("Options", opts_));
    }
    if (auto m = META_NS::GetObjectRegistry().Create<SCENE_NS::ISceneManager>(SCENE_NS::ClassId::SceneManager, md)) {
        if (auto scene = m->CreateScene().GetResult()) {
            importer->SetUserContext(interface_pointer_cast<META_NS::IObject>(scene));
            if (auto obj = importer->Import(in)) {
                if (auto i = interface_cast<ISceneNodeSer>(obj)) {
                    res = BuildSceneHierarchy(scene, *i);
                }
            }
        }
    }
    return res;
}
INode::Ptr SceneImporter::ImportNode(CORE_NS::IFile& in, const INode::Ptr& parent)
{
    INode::Ptr res;
    auto importer = META_NS::GetObjectRegistry().Create<META_NS::IFileImporter>(META_NS::ClassId::JsonImporter);
    auto context = context_.lock();
    if (context) {
        importer->SetResourceManager(context->GetResources());
    }
    importer->SetUserContext(interface_pointer_cast<META_NS::IObject>(parent->GetScene()));
    if (auto obj = importer->Import(in)) {
        if (auto i = interface_cast<ISceneNodeSer>(obj)) {
            res = BuildSceneHierarchy(parent, *i);
        }
    }
    return res;
}

SCENE_END_NAMESPACE()
