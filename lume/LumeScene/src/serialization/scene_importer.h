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

#ifndef SCENE_SRC_SERIALIZATION_SCENE_IMPORTER_H
#define SCENE_SRC_SERIALIZATION_SCENE_IMPORTER_H

#include <scene/interface/intf_render_context.h>
#include <scene/interface/scene_options.h>
#include <scene/interface/serialization/intf_scene_importer.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

#include "../resource/node_instantiator.h"
#include "scene_ser.h"

SCENE_BEGIN_NAMESPACE()

class SceneImporter : public META_NS::IntroduceInterfaces<META_NS::MetaObject, ISceneImporter> {
    META_OBJECT(SceneImporter, SCENE_NS::ClassId::SceneImporter, IntroduceInterfaces)
public:
    bool Build(const META_NS::IMetadata::Ptr& d) override;

    IScene::Ptr ImportScene(CORE_NS::IFile&, const CORE_NS::ResourceId&) override;
    INode::Ptr ImportNode(CORE_NS::IFile&, const INode::Ptr& parent) override;
    INode::Ptr ImportNode(const META_NS::IObject&, const INode::Ptr& parent) override;

private:
    IRenderContext::WeakPtr context_;
    SceneOptions opts_;
};

class SceneImportContext {
public:
    SceneImportContext(IScene::Ptr scene, CORE_NS::IResourceManager::Ptr resources, CORE_NS::ResourceId rid = {});
    virtual ~SceneImportContext() = default;

    IScene::Ptr ImportScene(CORE_NS::IFile&);
    INode::Ptr ImportNode(CORE_NS::IFile&, const INode::Ptr& parent);
    INode::Ptr ImportNode(const INodeTemplateInternal&, const INode::Ptr& parent);

private:
    void AddFlatPropertiesForExternal(const META_NS::IMetadata& in, META_NS::IObject& node, bool templateContext);
    INode::Ptr ConstructNode(const ISceneNodeSer& ser, INode& parent);
    INode::Ptr LoadExternalNode(const ISceneExternalNodeSer& in, INode& parent);
    INode::Ptr BuildSceneNodeHierarchy(const META_NS::IObject& data, INode& node);
    IScene::Ptr BuildSceneHierarchy(const ISceneNodeSer& data);
    virtual INode::Ptr BuildSceneHierarchy(INode& node, const META_NS::IObject& data);
    void AddFlatAttachments(const ISceneExternalNodeSer& in, const META_NS::IObject::Ptr& parent);
    void SetAttachments(const ISceneNodeSer& in, const META_NS::IObject::Ptr& out);
    void CopyObjectValues(const ISceneNodeSer& in, const META_NS::IObject::Ptr& out);
    void SetResourceGroups(BASE_NS::vector<BASE_NS::string> groups);

    void DoDeferredAttach();

protected:
    IScene::Ptr scene_;
    CORE_NS::IResourceManager::Ptr resources_;
    CORE_NS::ResourceId requestedId_;

    struct AttachInfo {
        META_NS::IObject::Ptr data;
        META_NS::IObject::Ptr parent;
    };
    BASE_NS::vector<AttachInfo> attachments_;
    bool isTemplateContext_ {};
};

META_NS::IObject::Ptr FindObject(META_NS::IObject& obj, BASE_NS::string_view path);

SCENE_END_NAMESPACE()

#endif