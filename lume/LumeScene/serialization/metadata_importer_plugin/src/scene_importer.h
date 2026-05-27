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

#ifndef SCENE_MIMP_SRC_SCENE_IMPORTER_H
#define SCENE_MIMP_SRC_SCENE_IMPORTER_H

#include <scene/interface/intf_render_context.h>
#include <scene/interface/resource/intf_render_resource_manager.h>
#include <scene/interface/resource/intf_resource_context.h>
#include <scene/interface/scene_options.h>
#include <scene_metadata_importer/interface/intf_scene_importer.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/minimal_object.h>
#include <meta/ext/object.h>
#include <meta/interface/resource/intf_resource.h>

#include "node_instantiator.h"
#include "scene_ser.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(NodeImportResult, "2a2763e5-0546-4672-b9a6-2ae3ef2d65f0", META_NS::ObjectCategoryBits::NO_CATEGORY)

class NodeImportResult : public META_NS::IntroduceInterfaces<META_NS::MinimalObject, INodeImportResult> {
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(ClassId::NodeImportResult)
public:
    explicit NodeImportResult(INode::Ptr n, BASE_NS::vector<CORE_NS::ResourceIdContext> res = {},
        BASE_NS::vector<CORE_NS::ResourceIdContext> ass = {})
        : node_(n), res_(BASE_NS::move(res)), ass_(BASE_NS::move(ass))
    {}

    INode::Ptr GetNode() const override
    {
        return node_;
    }
    BASE_NS::vector<CORE_NS::ResourceIdContext> GetNewResourcesImported() const override
    {
        return res_;
    }
    BASE_NS::vector<CORE_NS::ResourceIdContext> GetAssociatedResources() const override
    {
        return ass_;
    }

private:
    INode::Ptr node_;
    BASE_NS::vector<CORE_NS::ResourceIdContext> res_;
    BASE_NS::vector<CORE_NS::ResourceIdContext> ass_;
};

class SceneImporter : public META_NS::IntroduceInterfaces<META_NS::MetaObject, ISceneImporter> {
    META_OBJECT(SceneImporter, SCENE_NS::ClassId::SceneImporter, IntroduceInterfaces)
public:
    bool Build(const META_NS::IMetadata::Ptr& d) override;

    IScene::Ptr ImportScene(CORE_NS::IFile&, const CORE_NS::ResourceId&, BASE_NS::string_view resourceIndex) override;
    INodeImportResult::Ptr ImportNode(CORE_NS::IFile&, const INode::Ptr& parent, NodeImportOptions) override;
    INodeImportResult::Ptr ImportNode(const META_NS::IObject&, const INode::Ptr& parent, NodeImportOptions) override;

private:
    IRenderContext::WeakPtr context_;
    SceneOptions opts_;
};

META_REGISTER_CLASS(
    SceneImportContext, "3bd2f46e-b90c-4e65-be6d-1baa1c362d66", META_NS::ObjectCategoryBits::NO_CATEGORY)

class SceneImportContext
    : public META_NS::IntroduceInterfaces<META_NS::MinimalObject, IResourceContext, META_NS::IResourceContext> {
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(ClassId::SceneImportContext)
public:
    SceneImportContext(IScene::Ptr scene, CORE_NS::IResourceManager::Ptr resources, CORE_NS::ResourceId rid = {});
    virtual ~SceneImportContext() = default;

    IScene::Ptr ImportScene(CORE_NS::IFile&);
    INodeImportResult::Ptr ImportNode(CORE_NS::IFile&, const INode::Ptr& parent, NodeImportOptions);
    INodeImportResult::Ptr ImportNode(const IExportedNodeInternal&, const INode::Ptr& parent, NodeImportOptions);

    bool LoadResourceIndex(BASE_NS::string_view resourceIndex);

    META_NS::IObject::Ptr FindTemplateAttachment(META_NS::IAttach& parent, const META_NS::IObject::Ptr& old);
    void CopyAttachmentProperties(const META_NS::IObject::Ptr& from, const META_NS::IObject::Ptr& to);

private:
    void AddComponent(const IComponentSer::Ptr& c, const INode::Ptr& node, bool templateContext = false);
    void AddFlatComponents(const ISceneExternalNodeSer& in, META_NS::IObject& parent, bool templateContext);
    void AddFlatPropertiesForExternal(const META_NS::IMetadata& in, META_NS::IObject& node, bool templateContext);
    void SetComponents(const BASE_NS::vector<IComponentSer::Ptr>& comps, const INode::Ptr& node);
    INode::Ptr ConstructNode(const ISceneNodeSer& ser, INode& parent);
    INode::Ptr LoadExternalNode(const ISceneExternalNodeSer& in, INode& parent);
    void BuildChildNodes(const META_NS::IObject& data, INode& node);
    INode::Ptr BuildSceneNodeHierarchy(const META_NS::IObject& data, INode& node);
    IScene::Ptr BuildSceneHierarchy(const ISceneNodeSer& data);
    virtual INode::Ptr BuildSceneHierarchy(INode& node, META_NS::IObject& data, const NodeImportOptions& opts);
    void AddFlatAttachments(const ISceneExternalNodeSer& in, const META_NS::IObject::Ptr& parent, bool templateContext);
    void SetAttachments(const ISceneNodeSer& in, const META_NS::IObject::Ptr& out);
    void CopyObjectValues(const ISceneNodeSer& in, const META_NS::IObject::Ptr& out);
    void SetResourceGroups(BASE_NS::vector<BASE_NS::string> groups);
    META_NS::ISerNode::ConstPtr HandleTemplateResources(const IExportedNodeInternal&, const NodeImportOptions&,
        BASE_NS::vector<CORE_NS::ResourceIdContext>&, BASE_NS::vector<CORE_NS::ResourceIdContext>&);
    void MergeExistingResources(BASE_NS::vector<CORE_NS::ResourceInfo>&, BASE_NS::vector<CORE_NS::ResourceIdContext>&);

    struct AttachInfo {
        META_NS::IObject::Ptr data;
        META_NS::IObject::Ptr parent;
        bool isInTemplate{};
    };

    void DoDeferredAttach();
    void DoExternalAttach(const IExternalAttachment& ext, const AttachInfo& v);
    void DoPostLoad(const IScene::Ptr& scene);

    IScene::Ptr GetScene() const override
    {
        return scene_;
    }
    bool IsNodeTemplateContext() const override
    {
        return isTemplateContext_;
    }
    CORE_NS::ResourceContextPtr GetContext() const override
    {
        return scene_;
    }

protected:
    IScene::Ptr scene_;
    CORE_NS::IResourceManager::Ptr resources_;
    CORE_NS::ResourceId requestedId_;
    IRenderResourceManager::Ptr rman_;

    BASE_NS::vector<AttachInfo> attachments_;
    bool isTemplateContext_{};
};

META_NS::IObject::Ptr FindObject(META_NS::IObject& obj, BASE_NS::string_view path);

SCENE_END_NAMESPACE()

#endif
