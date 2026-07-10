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

#include "node_template_updater.h"

#include <scene/ext/intf_component.h>
#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_scene.h>

#include <meta/api/metadata_util.h>
#include <meta/api/resource/resource_template_access.h>

#include "../import_context.h"
#include "../resolve_object.h"
#include "index.h"

SCENE_IMP_BEGIN_NAMESPACE()

namespace {

constexpr const BASE_NS::string_view ESCAPED_SER_CHARS = "./![]";
constexpr const char ESCAPE_SER_CHAR = '\\';

BASE_NS::string EscapeSerName(BASE_NS::string_view str)
{
    BASE_NS::string res{str};
    for (size_t i = 0; i != res.size(); ++i) {
        if (ESCAPED_SER_CHARS.find(res[i]) != BASE_NS::string_view::npos) {
            res.insert(i, &ESCAPE_SER_CHAR, 1);
            ++i;
        }
    }
    return res;
}

template<typename Obj>
bool IsNativeComponent(const Obj& obj)
{
    return interface_cast<SCENE_NS::IComponent>(obj) && META_NS::IsFlagSet(obj, META_NS::ObjectFlagBits::NATIVE);
}

template<typename Obj>
bool SerialiseAttachment(const Obj& obj)
{
    return META_NS::IsFlagSet(obj, META_NS::ObjectFlagBits::SERIALIZE) && !interface_cast<META_NS::IEvent>(obj) &&
           !interface_cast<META_NS::IFunction>(obj) && !interface_cast<META_NS::IProperty>(obj) &&
           !interface_cast<SCENE_NS::IComponent>(obj) && !interface_cast<SCENE_NS::IExternalNode>(obj);
}

META_NS::IObject::Ptr FindTemplateAttachment(META_NS::IAttach& parent, const META_NS::IObject::Ptr& old)
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
        for (auto&& v : res) {
            if (v->GetName() == old->GetName()) {
                return v;
            }
        }
    }
    return res.empty() ? nullptr : res[0];
}

void SetCurrentAsDefaultAndCopyValue(const META_NS::IProperty::Ptr& source, const META_NS::IProperty::Ptr& target)
{
    META_NS::PropertyLock plock{source};
    META_NS::PropertyLock lock{target};
    if (!plock || !lock) {
        return;
    }
    lock->SetDefaultValueAny(target->GetValue());
    if (!target->SetValue(source->GetValue())) {
        CORE_LOG_W("Failed to set property value [%s]", target->GetName().c_str());
    }
}

void CopyPropertyIfSet(const META_NS::IProperty::Ptr& p, META_NS::IMetadata& toMd)
{
    if (!META_NS::IsValueSet(*p)) {
        return;
    }
    if (auto target = toMd.GetProperty(p->GetName())) {
        SetCurrentAsDefaultAndCopyValue(p, target);
    }
}

void CopyAttachmentProperties(const META_NS::IObject::Ptr& from, const META_NS::IObject::Ptr& to)
{
    auto fromMd = interface_cast<META_NS::IMetadata>(from);
    auto toMd = interface_cast<META_NS::IMetadata>(to);
    if (!fromMd || !toMd) {
        return;
    }
    for (auto&& p : fromMd->GetProperties()) {
        CopyPropertyIfSet(p, *toMd);
    }
}

bool IsStructuralAttachment(const META_NS::IObject::Ptr& v)
{
    return !interface_cast<META_NS::IEvent>(v) && !interface_cast<META_NS::IFunction>(v) &&
           !interface_cast<META_NS::IProperty>(v) && !interface_cast<SCENE_NS::IComponent>(v);
}

void ApplyPropertyValue(ImportContext& context, META_NS::IObject& obj, const UserSetProperty& v)
{
    auto path = v.path + "." + EscapeSerName(v.property->GetName());
    if (auto res = ResolveObject(context, obj.GetSelf(), path)) {
        if (auto target = interface_pointer_cast<META_NS::IProperty>(res.object)) {
            META_NS::CopyValue(v.property, *target);
        }
        return;
    }
    if (META_NS::IsFlagSet(v.property, IMPORTED_FROM_TEMPLATE_BIT)) {
        return;
    }
    if (auto res2 = ResolveObject(context, obj.GetSelf(), v.path)) {
        if (auto target = interface_pointer_cast<META_NS::IMetadata>(res2.object)) {
            META_NS::Clone(v.property, *target);
        }
    }
}

}  // namespace

NodeTemplateUpdater::NodeTemplateUpdater(
    IImporterInternal& importer, const META_NS::IObjectTemplate& templ, SCENE_NS::INode& node)
    : importer_(importer), node_(node), templ_(templ)
{}

void NodeTemplateUpdater::HandleArrayProperty(
    META_NS::ArrayPropertyLock<const META_NS::IProperty>& arr, const BASE_NS::string& basePath)
{
    for (size_t i = 0; i != arr->GetSize(); ++i) {
        if (auto obj = interface_cast<META_NS::IObject>(META_NS::GetConstPointer(arr->GetAnyAt(i)))) {
            AddObjectProperties(*obj, basePath + "[" + BASE_NS::to_string(i) + "]");
        }
    }
}

void NodeTemplateUpdater::HandleReadOnlyProperty(const META_NS::IProperty::ConstPtr& p, const BASE_NS::string& head)
{
    if (META_NS::ArrayPropertyLock arr{p}) {
        HandleArrayProperty(arr, head + "." + EscapeSerName(p->GetName()));
    } else if (auto obj = META_NS::GetConstPointer<META_NS::IObject>(p)) {
        AddObjectProperties(*obj, head + "." + EscapeSerName(p->GetName()));
    }
}

void NodeTemplateUpdater::ProcessSerializableProperty(
    const META_NS::IMetadata& meta, const META_NS::IProperty::ConstPtr& p, const BASE_NS::string& head)
{
    if (meta.GetMetadata(META_NS::MetadataType::PROPERTY, p->GetName()).readOnly) {
        HandleReadOnlyProperty(p, head);
    } else if (META_NS::IsValueSet(*p)) {
        props_.push_back({head, p});
    }
}

void NodeTemplateUpdater::ProcessTemplateAttachment(
    const META_NS::IObject& parent, const META_NS::IObject::Ptr& vobj, const BASE_NS::string& head)
{
    if (META_NS::IsFlagSet(vobj, IMPORTED_FROM_TEMPLATE_BIT) && SerialiseAttachment(vobj)) {
        CollectUserChanges(*vobj, head + "!" + EscapeSerName(vobj->GetName()));
    } else if (!META_NS::IsFlagSet(vobj, IMPORTED_FROM_TEMPLATE_BIT)) {
        atts_.push_back({head, vobj, parent.GetSelf()});
    }
}

void NodeTemplateUpdater::ProcessComponent(const META_NS::IObject& parent, const META_NS::IObject::Ptr& vobj,
    SCENE_NS::IComponent& comp, const BASE_NS::string& head)
{
    if (IsNativeComponent(vobj)) {
        // Native component: only collect user-modified properties
        AddObjectProperties(*vobj, head + "!" + EscapeSerName(vobj->GetName()));
    } else if (META_NS::IsFlagSet(vobj, IMPORTED_FROM_TEMPLATE_BIT)) {
        // Template-imported component: collect user changes for property reapplication
        CollectUserChanges(*vobj, head + "!" + EscapeSerName(vobj->GetName()));
    } else {
        // User-added component: store for recreation on new node
        comps_.push_back({head, BASE_NS::string(comp.GetName()), vobj});
    }
}

void NodeTemplateUpdater::AddObjectProperties(const META_NS::IObject& obj, const BASE_NS::string& head)
{
    if (auto i = interface_cast<META_NS::IMetadata>(&obj)) {
        for (auto&& p : META_NS::GetAllMetaProperties(*i)) {
            if (META_NS::IsFlagSet(p, META_NS::ObjectFlagBits::SERIALIZE)) {
                ProcessSerializableProperty(*i, p, head);
            }
        }
    }
    if (auto att = interface_cast<META_NS::IAttach>(&obj)) {
        for (auto&& v : att->GetAttachments()) {
            if (IsStructuralAttachment(v)) {
                ProcessTemplateAttachment(obj, v, head);
            } else if (auto comp = interface_cast<SCENE_NS::IComponent>(v)) {
                ProcessComponent(obj, v, *comp, head);
            }
        }
    }
}

void NodeTemplateUpdater::CollectUserChanges(const META_NS::IObject& obj, const BASE_NS::string& head)
{
    AddObjectProperties(obj, head);

    META_NS::Internal::ConstIterate(
        interface_cast<META_NS::IIterable>(&obj),
        [this, &head](META_NS::IObject::Ptr nodeObj) {
            CollectUserChanges(*nodeObj, head + "/" + EscapeSerName(nodeObj->GetName()));
            return true;
        },
        META_NS::IterateStrategy{META_NS::TraversalType::NO_HIERARCHY});
}

void NodeTemplateUpdater::ApplyUserChanges(ImportContext& context, META_NS::IObject& obj)
{
    for (auto&& v : props_) {
        ApplyPropertyValue(context, obj, v);
    }
    ApplyAttachments(context, obj);
    ApplyComponents(context, obj);
}

void NodeTemplateUpdater::ApplyAttachment(META_NS::IAttach& target, const META_NS::IObject::Ptr& attachment)
{
    if (META_NS::IsFlagSet(attachment, IMPORTED_FROM_TEMPLATE_BIT)) {
        if (auto existing = FindTemplateAttachment(target, attachment)) {
            CopyAttachmentProperties(attachment, existing);
        }
    } else {
        target.Attach(attachment);
    }
}

void NodeTemplateUpdater::ApplyAttachments(ImportContext& context, META_NS::IObject& obj)
{
    for (auto&& v : atts_) {
        if (!interface_cast<SCENE_NS::IExternalNode>(v.attachment)) {
            auto res = ResolveObject(context, obj.GetSelf(), v.path);
            if (auto target = interface_pointer_cast<META_NS::IAttach>(res.object)) {
                ApplyAttachment(*target, v.attachment);
            }
        }
    }
}

void NodeTemplateUpdater::ApplyComponents(ImportContext& context, META_NS::IObject& obj)
{
    auto scene = context.GetImportParameters().scene;
    if (!scene) {
        return;
    }
    for (auto&& v : comps_) {
        auto res = ResolveObject(context, obj.GetSelf(), v.path);
        auto node = interface_pointer_cast<SCENE_NS::INode>(res.object);
        if (!node) {
            return;
        }
        auto comp = scene->CreateComponent(node, v.name).GetResult();
        if (comp) {
            CopyAttachmentProperties(v.oldComponent, interface_pointer_cast<META_NS::IObject>(comp));
        }
    }
}

void NodeTemplateUpdater::DetachAttachments()
{
    for (auto&& v : atts_) {
        if (auto oatt = interface_cast<META_NS::IAttach>(v.oldParent)) {
            oatt->Detach(v.attachment);
        }
    }
}

META_NS::IObject::Ptr NodeTemplateUpdater::ImportAndApply(
    SCENE_NS::INodeImport& imp, const SCENE_NS::IExternalNode::Ptr& ext, const SCENE_NS::IScene::Ptr& scene)
{
    auto handle = ext->GetSubresourceGroup();
    auto newNode =
        imp.ImportTemplate(META_NS::GetSelf<META_NS::IObjectTemplate>(&templ_), handle ? handle->GetGroup() : "")
            .GetResult();
    if (!newNode) {
        CORE_LOG_E("Failed to import template");
        return nullptr;
    }
    auto newNodeObj = interface_pointer_cast<META_NS::IObject>(newNode);
    if (!newNodeObj) {
        CORE_LOG_E("Imported template node does not implement IObject");
        return nullptr;
    }
    ImporterConfig config = importer_.GetConfig();
    ImportContextParameters params;
    params.scene = scene;
    params.object = newNodeObj;
    // User-override paths apply against the just-instantiated subtree, so
    // `/` should anchor at that subtree's root.
    params.importRoot = params.object;
    ImportContext context(config, params, CORE_NS::json::value{});
    ApplyUserChanges(context, *newNodeObj);
    return newNodeObj;
}

META_NS::IObject::Ptr NodeTemplateUpdater::Update(SCENE_NS::INode::Ptr node, const SCENE_NS::IExternalNode::Ptr& ext)
{
    auto obj = interface_cast<META_NS::IObject>(node);
    if (!obj) {
        return nullptr;
    }
    CollectUserChanges(*obj, ".");
    auto parent = node->GetParent().GetResult();
    if (!parent) {
        return nullptr;
    }
    DetachAttachments();
    parent->RemoveChild(node).Wait();
    auto scene = node->GetScene();
    scene->RemoveNode(BASE_NS::move(node)).Wait();
    auto imp = interface_cast<SCENE_NS::INodeImport>(parent);
    if (!imp) {
        return nullptr;
    }
    return ImportAndApply(*imp, ext, scene);
}

META_NS::IObject::Ptr NodeTemplateUpdater::Update()
{
    auto obj = interface_cast<META_NS::IObject>(&node_);
    if (!obj) {
        return nullptr;
    }
    auto node = interface_pointer_cast<SCENE_NS::INode>(obj->GetSelf());
    if (!node) {
        return nullptr;
    }
    auto ext = SCENE_NS::GetExternalNodeAttachment(node);
    if (!ext) {
        CORE_LOG_W("Called template node update for non-external node");
        return nullptr;
    }
    return Update(BASE_NS::move(node), ext);
}

SCENE_IMP_END_NAMESPACE()
