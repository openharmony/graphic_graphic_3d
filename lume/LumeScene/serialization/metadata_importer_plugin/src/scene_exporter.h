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

#ifndef SCENE_MIMP_SRC_SCENE_EXPORTER_H
#define SCENE_MIMP_SRC_SCENE_EXPORTER_H

#include <optional>
#include <scene_metadata_importer/interface/intf_scene_exporter.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/minimal_object.h>
#include <meta/ext/object.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_refuri_builder.h>

SCENE_BEGIN_NAMESPACE()

constexpr META_NS::Version SCENE_EXPORTER_VERSION(0, 1);
constexpr BASE_NS::string_view SCENE_EXPORTER_TYPE("Scene");

class SceneExporter : public META_NS::IntroduceInterfaces<META_NS::MetaObject, ISceneExporter> {
    META_OBJECT(SceneExporter, SCENE_NS::ClassId::SceneExporter, IntroduceInterfaces)
public:
    META_NS::ReturnError ExportScene(CORE_NS::IFile&, const IScene::ConstPtr&) override;
    META_NS::ReturnError ExportNode(CORE_NS::IFile&, const INode::ConstPtr&) override;
    META_NS::IObject::Ptr ExportNode(const INode::ConstPtr&) override;
};

class IExternalNode;
class ISceneSer;
class IExportedNode;

META_REGISTER_CLASS(
    SceneExporterContext, "54cdec71-87cb-4c7f-9b27-f15859dbea3d", META_NS::ObjectCategoryBits::NO_CATEGORY)

class SceneExporterContext : public META_NS::IntroduceInterfaces<META_NS::MinimalObject, META_NS::ICollectResources,
                                 META_NS::IGetAnchorObject, META_NS::IResourceContext> {
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(ClassId::SceneExporterContext)
public:
    SceneExporterContext(const IScene::Ptr& scene) : scene_(scene)
    {}

    META_NS::IObject::Ptr BuildObjectNode(const META_NS::IObject& node);
    META_NS::IObject::Ptr BuildObjectHierarchy();

    META_NS::IFileExporter::Ptr CreateExporter();

    void CollectResources()
    {
        exportedResources_.emplace();
    }
    void SetExportedResources(IExportedNode&);

    void AddResource(CORE_NS::ResourceIdContext mid) override
    {
        if (exportedResources_) {
            exportedResources_->push_back(BASE_NS::move(mid));
        }
    }

private:
    IObject::ConstPtr GetAnchorObject() const override
    {
        return interface_pointer_cast<META_NS::IObject>(scene_);
    }
    CORE_NS::ResourceContextPtr GetContext() const override
    {
        return scene_;
    }

    void SetResourceGroups(ISceneSer& out);
    META_NS::IObject::Ptr BuildExternalNode(const META_NS::IObject& node, const IExternalNode& ext);

private:
    IScene::Ptr scene_;
    std::optional<BASE_NS::vector<CORE_NS::ResourceIdContext>> exportedResources_;
};

SCENE_END_NAMESPACE()

#endif