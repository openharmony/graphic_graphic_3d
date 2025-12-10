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
#include <scene/interface/serialization/intf_scene_importer.h>

#include "../serialization/scene_importer.h"
#include "../serialization/scene_ser.h"
#include "../serialization/util.h"
#include "../util.h"

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
    NodeTemplateUpdater(INode& node, const INodeTemplateInternal& templ)
        : SceneImportContext(node.GetScene(), GetResourceManager(node.GetScene())), node_(node), templ_(templ)
    {}

    void HandleReadOnlyProperty(const META_NS::IProperty::ConstPtr& p, const BASE_NS::string& head)
    {
        if (META_NS::ArrayPropertyLock arr { p }) {
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

    void AddObjectProperties(const META_NS::IObject& obj, const BASE_NS::string& head)
    {
        if (auto i = interface_cast<META_NS::IMetadata>(&obj)) {
            for (auto&& p : GetAllProperties(*i)) {
                if (i->GetMetadata(META_NS::MetadataType::PROPERTY, p->GetName()).readOnly) {
                    HandleReadOnlyProperty(p, head);
                } else {
                    if (META_NS::IsValueSet(*p)) {
                        props_.push_back({ head, p });
                    }
                }
            }
        }
        if (auto att = interface_cast<META_NS::IAttach>(&obj)) {
            for (auto&& v : att->GetAttachments()) {
                if (!interface_cast<META_NS::IProperty>(v) && !interface_cast<IComponent>(v)) {
                    if (auto vobj = interface_pointer_cast<META_NS::IObject>(v)) {
                        if (META_NS::IsFlagSet(vobj, IMPORTED_FROM_TEMPLATE_BIT)) {
                            if (SerialiseAttachment(v)) {
                                CollectUserChanges(*vobj, head + "!" + EscapeSerName(vobj->GetName()));
                            }
                        } else {
                            atts_.push_back({ head, vobj, obj.GetSelf() });
                        }
                    }
                }
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
            META_NS::IterateStrategy { META_NS::TraversalType::NO_HIERARCHY });
    }

    void ApplyUserChanges(INode& node)
    {
        if (auto obj = interface_cast<META_NS::IObject>(&node)) {
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
            for (auto&& v : atts_) {
                if (auto target = interface_pointer_cast<META_NS::IAttach>(FindObject(*obj, v.path))) {
                    if (auto oatt = interface_cast<META_NS::IAttach>(v.oldParent)) {
                        oatt->Detach(v.attachment);
                    }
                    target->Attach(v.attachment);
                }
            }
        }
    }

    META_NS::IObject::Ptr Update()
    {
        META_NS::IObject::Ptr res;
        if (auto obj = interface_cast<META_NS::IObject>(&node_)) {
            if (auto node = interface_pointer_cast<INode>(obj->GetSelf())) {
                CollectUserChanges(*obj, "");
                if (auto parent = node_.GetParent().GetResult()) {
                    parent->RemoveChild(node);
                    if (auto newNode = ImportNode(templ_, parent)) {
                        ApplyUserChanges(*newNode);
                        scene_->RemoveNode(BASE_NS::move(node)).Wait();
                        res = interface_pointer_cast<META_NS::IObject>(newNode);
                    } else {
                        CORE_LOG_E("Failed to import template");
                    }
                }
            }
        }
        return res;
    }

private:
    INode& node_;
    const INodeTemplateInternal& templ_;
    BASE_NS::vector<UserSetProperty> props_;
    BASE_NS::vector<UserSetAttachment> atts_;
};
} // namespace

META_NS::IObject::Ptr NodeInstantiator::Instantiate(
    const META_NS::IObjectTemplate& templ, const META_NS::SharedPtrIInterface& context)
{
    if (auto parent = interface_pointer_cast<INode>(context)) {
        auto obj = META_NS::GetValue(templ.Content());
        if (interface_cast<INodeTemplateInternal>(obj)) {
            if (auto importer = META_NS::GetObjectRegistry().Create<ISceneImporter>(ClassId::SceneImporter)) {
                return interface_pointer_cast<META_NS::IObject>(importer->ImportNode(*obj, parent));
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
        auto tobj = META_NS::GetValue(templ.Content());
        if (auto to = interface_cast<INodeTemplateInternal>(tobj)) {
            res = NodeTemplateUpdater(*instantiation, *to).Update();
        }
    }
    return res;
}

SCENE_END_NAMESPACE()