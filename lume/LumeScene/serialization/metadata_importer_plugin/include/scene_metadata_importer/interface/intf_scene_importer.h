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

#ifndef SCENE_MIMP_INTERFACE_ISCENE_IMPORTER_H
#define SCENE_MIMP_INTERFACE_ISCENE_IMPORTER_H

#include <scene/interface/intf_scene.h>
#include <scene/interface/resource/intf_exported_node.h>

#include <core/io/intf_file.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

struct NodeImportOptions {
    bool applyAsTemplate{true};
    BASE_NS::string group;
    /// Rename the top-most node being imported if not empty
    BASE_NS::string name;
};

class ISceneImporter : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISceneImporter, "600c1a5f-ea6c-4078-abd0-c7b908fcb3f9")
public:
    virtual IScene::Ptr ImportScene(
        CORE_NS::IFile&, const CORE_NS::ResourceId& requestedId, BASE_NS::string_view resourceIndex) = 0;
    IScene::Ptr ImportScene(CORE_NS::IFile& f, const CORE_NS::ResourceId& requestedId)
    {
        return ImportScene(f, requestedId, {});
    }

    virtual INodeImportResult::Ptr ImportNode(CORE_NS::IFile&, const INode::Ptr& parent, NodeImportOptions) = 0;
    virtual INodeImportResult::Ptr ImportNode(const META_NS::IObject&, const INode::Ptr& parent, NodeImportOptions) = 0;

    INodeImportResult::Ptr ImportNode(CORE_NS::IFile& f, const INode::Ptr& parent)
    {
        return ImportNode(f, parent, {});
    }
    INodeImportResult::Ptr ImportNode(const META_NS::IObject& obj, const INode::Ptr& parent)
    {
        return ImportNode(obj, parent, {});
    }
};

META_REGISTER_CLASS(SceneImporter, "8e8a519c-dacd-41a1-81ef-06b8a3b828de", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

#endif
