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

#ifndef SCENE_IMP_SRC_OBJECTS_NODE_TEMPLATE_UPDATER_H
#define SCENE_IMP_SRC_OBJECTS_NODE_TEMPLATE_UPDATER_H

#include <scene/ext/intf_component.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_scene.h>

#include <meta/interface/intf_attach.h>
#include <meta/interface/resource/intf_object_template.h>

#include "../import_context.h"

SCENE_IMP_BEGIN_NAMESPACE()

struct UserSetProperty {
    BASE_NS::string path;
    META_NS::IProperty::ConstPtr property;
};

struct UserSetAttachment {
    BASE_NS::string path;
    META_NS::IObject::Ptr attachment;
    META_NS::IObject::Ptr oldParent;
};

struct UserAddedComponent {
    BASE_NS::string path;
    BASE_NS::string name;
    META_NS::IObject::Ptr oldComponent;
};

class NodeTemplateUpdater {
public:
    NodeTemplateUpdater(IImporterInternal& importer, const META_NS::IObjectTemplate& templ, SCENE_NS::INode& node);
    META_NS::IObject::Ptr Update();

private:
    META_NS::IObject::Ptr Update(SCENE_NS::INode::Ptr node, const SCENE_NS::IExternalNode::Ptr& ext);
    META_NS::IObject::Ptr ImportAndApply(
        SCENE_NS::INodeImport& imp, const SCENE_NS::IExternalNode::Ptr& ext, const SCENE_NS::IScene::Ptr& scene);
    void HandleReadOnlyProperty(const META_NS::IProperty::ConstPtr& p, const BASE_NS::string& head);
    void HandleArrayProperty(
        META_NS::ArrayPropertyLock<const META_NS::IProperty>& arr, const BASE_NS::string& basePath);
    void AddObjectProperties(const META_NS::IObject& obj, const BASE_NS::string& head);
    void ProcessSerializableProperty(
        const META_NS::IMetadata& meta, const META_NS::IProperty::ConstPtr& p, const BASE_NS::string& head);
    void ProcessTemplateAttachment(
        const META_NS::IObject& parent, const META_NS::IObject::Ptr& vobj, const BASE_NS::string& head);
    void ProcessComponent(const META_NS::IObject& parent, const META_NS::IObject::Ptr& vobj, SCENE_NS::IComponent& comp,
        const BASE_NS::string& head);
    void CollectUserChanges(const META_NS::IObject& obj, const BASE_NS::string& head);
    void ApplyUserChanges(ImportContext& context, META_NS::IObject& obj);
    void ApplyAttachment(META_NS::IAttach& target, const META_NS::IObject::Ptr& att);
    void ApplyAttachments(ImportContext& context, META_NS::IObject& obj);
    void ApplyComponents(ImportContext& context, META_NS::IObject& obj);
    void DetachAttachments();

    IImporterInternal& importer_;
    SCENE_NS::INode& node_;
    const META_NS::IObjectTemplate& templ_;
    BASE_NS::vector<UserSetProperty> props_;
    BASE_NS::vector<UserSetAttachment> atts_;
    BASE_NS::vector<UserAddedComponent> comps_;
};

SCENE_IMP_END_NAMESPACE()

#endif
