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

#include "node_instantiator.h"

#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_node_import.h>
#include <scene_metadata_importer/interface/intf_scene_importer.h>

#include "resource_util.h"
#include "scene_importer.h"
#include "scene_ser.h"
#include "serialization_util.h"

SCENE_BEGIN_NAMESPACE()
namespace {
struct UserSetProperty {
    BASE_NS::string path;
    META_NS::IProperty::ConstPtr property;
};
struct UserSetAttachment {
    BASE_NS::string path;
    META_NS::IObject::Ptr attachment;
    META_NS::IObject::Ptr oldParent;
};

struct NodeTemplateUpdater : SceneImportContext {
    NodeTemplateUpdater(const META_NS::IObjectTemplate& templ, INode& node)
        : SceneImportContext(node.GetScene(), GetResourceManager(node.GetScene())), node_(node), templ_(templ)
    {}

    void HandleReadOnlyProperty(const META_NS::IProperty::ConstPtr& p, const BASE_NS::string& head)
    {
        if (META_NS::ArrayPropertyLock arr{p}) {
            for (size_t i = 0; i != arr->GetSize(); ++i) {
                if (auto obj = interface_cast<META_NS::IObject>(META_NS::GetConstPointer(arr->GetAnyAt(i)))) {
                    AddObjectProperties(
                        *obj, head + "." + EscapeSerName(p->GetName()) + "[" + BASE_NS::to_string(i) + "]");
                }
            }
        } else {
            if (auto obj = META_NS::GetConstPointer<META_NS::IObject>(p)) {
                AddObjectProperties(*obj, head + "." + EscapeSerName(p->GetName()));
            }
        }
    }

    void HandleSerializableProperty(
        const META_NS::IMetadata& meta, const META_NS::IProperty::ConstPtr& p, const BASE_NS::string& head)
    {
        if (meta.GetMetadata(META_NS::MetadataType::PROPERTY, p->GetName()).readOnly) {
            HandleReadOnlyProperty(p, head);
        } else if (META_NS::IsValueSet(*p)) {
            props_.push_back({head, p});
        }
    }

    void HandleAttachment(
        const META_NS::SharedPtrIInterface& v, const META_NS::IObject& obj, const BASE_NS::string& head)
    {
        if (interface_cast<META_NS::IEvent>(v) || interface_cast<META_NS::IFunction>(v) ||
            interface_cast<META_NS::IProperty>(v) || interface_cast<IComponent>(v)) {
            return;
        }
        // it is not quite right to remove the external node attachment, but for now we do it so resources
        // are not lost on update
        auto vobj = interface_pointer_cast<META_NS::IObject>(v);
        if (!vobj) {
            return;
        }
        if (META_NS::IsFlagSet(vobj, IMPORTED_FROM_TEMPLATE_BIT)) {
            if (SerialiseAttachment(v)) {
                CollectUserChanges(*vobj, head + "!" + EscapeSerName(vobj->GetName()));
            }
        } else {
            atts_.push_back({head, vobj, obj.GetSelf()});
        }
    }

    void AddObjectProperties(const META_NS::IObject& obj, const BASE_NS::string& head)
    {
        if (auto i = interface_cast<META_NS::IMetadata>(&obj)) {
            for (auto&& p : GetAllProperties(*i)) {
                if (IsFlagSet(p, META_NS::ObjectFlagBits::SERIALIZE)) {
                    HandleSerializableProperty(*i, p, head);
                }
            }
        }
        if (auto att = interface_cast<META_NS::IAttach>(&obj)) {
            for (auto&& v : att->GetAttachments()) {
                HandleAttachment(v, obj, head);
            }
        }
    }

    void CollectUserChanges(const META_NS::IObject& obj, const BASE_NS::string& head)
    {
        AddObjectProperties(obj, head);

        META_NS::Internal::ConstIterate(
            interface_cast<META_NS::IIterable>(&obj),
            [&](META_NS::IObject::Ptr nodeObj) {
                CollectUserChanges(*nodeObj, head + "/" + EscapeSerName(nodeObj->GetName()));
                return true;
            },
            META_NS::IterateStrategy{META_NS::TraversalType::NO_HIERARCHY});
    }

    void ApplyUserChanges(INode& node)
    {
        auto obj = interface_cast<META_NS::IObject>(&node);
        if (!obj) {
            return;
        }
        for (auto&& v : props_) {
            if (auto target = interface_pointer_cast<META_NS::IProperty>(
                    FindObject(*obj, v.path + "." + EscapeSerName(v.property->GetName())))) {
                META_NS::CopyValue(v.property, *target);
            } else if (!META_NS::IsFlagSet(v.property, IMPORTED_FROM_TEMPLATE_BIT)) {
                if (auto target = interface_pointer_cast<META_NS::IMetadata>(FindObject(*obj, v.path))) {
                    META_NS::Clone(v.property, *target);
                }
            }
        }
        ApplyAttachments(*obj);
    }

    void ApplyAttachment(META_NS::IAttach& target, const META_NS::IObject::Ptr& attachment)
    {
        if (META_NS::IsFlagSet(attachment, IMPORTED_FROM_TEMPLATE_BIT)) {
            if (auto existing = FindTemplateAttachment(target, attachment)) {
                CopyAttachmentProperties(attachment, existing);
            }
        } else {
            // user attachment, just re-attach
            target.Attach(attachment);
        }
    }

    void ApplyAttachments(META_NS::IObject& obj)
    {
        for (auto&& v : atts_) {
            if (!interface_cast<IExternalNode>(v.attachment)) {
                if (auto target = interface_pointer_cast<META_NS::IAttach>(FindObject(obj, v.path))) {
                    ApplyAttachment(*target, v.attachment);
                }
            }
        }
    }

    void DetachAttachments()
    {
        for (auto&& v : atts_) {
            if (auto oatt = interface_cast<META_NS::IAttach>(v.oldParent)) {
                oatt->Detach(v.attachment);
            }
        }
    }

    META_NS::IObject::Ptr Update(INode::Ptr node, const IExternalNode::Ptr& ext)
    {
        auto obj = interface_cast<META_NS::IObject>(node);
        if (!obj) {
            return nullptr;
        }
        META_NS::IObject::Ptr res;
        CollectUserChanges(*obj, "");
        if (auto parent = node->GetParent().GetResult()) {
            DetachAttachments();
            // this is needed to let editor know the node is gone
            parent->RemoveChild(node).Wait();
            scene_->RemoveNode(BASE_NS::move(node)).Wait();
            if (auto imp = interface_cast<INodeImport>(parent)) {
                auto handle = ext->GetSubresourceGroup();
                if (auto newNode = imp->ImportTemplate(META_NS::GetSelf<META_NS::IObjectTemplate>(&templ_),
                                          handle ? handle->GetGroup() : "")
                                       .GetResult()) {
                    ApplyUserChanges(*newNode);
                    res = interface_pointer_cast<META_NS::IObject>(newNode);
                } else {
                    CORE_LOG_E("Failed to import template");
                }
            }
        }
        return res;
    }

    META_NS::IObject::Ptr Update()
    {
        META_NS::IObject::Ptr res;
        if (auto obj = interface_cast<META_NS::IObject>(&node_)) {
            if (auto node = interface_pointer_cast<INode>(obj->GetSelf())) {
                if (auto ext = GetExternalNodeAttachment(node)) {
                    res = Update(BASE_NS::move(node), ext);
                } else {
                    CORE_LOG_W("Called template node update for non-external node");
                }
            }
        }
        return res;
    }

private:
    INode& node_;
    const META_NS::IObjectTemplate& templ_;
    BASE_NS::vector<UserSetProperty> props_;
    BASE_NS::vector<UserSetAttachment> atts_;
};
}  // namespace

META_NS::IObject::Ptr NodeInstantiator::Instantiate(
    const META_NS::IObjectTemplate& templ, const META_NS::SharedPtrIInterface& context)
{
    if (auto ntcontext = interface_cast<INodeTemplateContext>(context)) {
        auto obj = META_NS::GetValue(templ.Content());
        if (interface_cast<IExportedNodeInternal>(obj)) {
            if (auto importer = META_NS::GetObjectRegistry().Create<ISceneImporter>(ClassId::SceneImporter)) {
                return interface_pointer_cast<META_NS::IObject>(importer->ImportNode(*obj,
                    ntcontext->GetTargetNode(),
                    NodeImportOptions{ntcontext->ApplyAsTemplate(), ntcontext->GetGroup()}));
            }
        } else {
            CORE_LOG_E("Trying to instantiate invalid object template");
        }
    } else {
        CORE_LOG_E("Trying to instantiate object template with invalid context");
    }
    return nullptr;
}

META_NS::IObject::Ptr NodeInstantiator::Update(
    META_NS::IObject& inst, const META_NS::IObjectTemplate& templ, const META_NS::SharedPtrIInterface& context)
{
    META_NS::IObject::Ptr res;
    if (auto instantiation = interface_cast<INode>(&inst)) {
        res = NodeTemplateUpdater(templ, *instantiation).Update();
    }
    return res;
}

SCENE_END_NAMESPACE()