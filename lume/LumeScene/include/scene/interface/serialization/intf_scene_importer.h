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

#ifndef SCENE_INTERFACE_SERIALIZATION_ISCENE_IMPORTER_H
#define SCENE_INTERFACE_SERIALIZATION_ISCENE_IMPORTER_H

#include <scene/interface/intf_scene.h>

#include <core/io/intf_file.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

class ISceneImporter : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISceneImporter, "600c1a5f-ea6c-4078-abd0-c7b908fcb3f9")
public:
    virtual IScene::Ptr ImportScene(CORE_NS::IFile&, const CORE_NS::ResourceId& requestedId) = 0;
    virtual INode::Ptr ImportNode(CORE_NS::IFile&, const INode::Ptr& parent) = 0;
    virtual INode::Ptr ImportNode(const META_NS::IObject&, const INode::Ptr& parent) = 0;
};

META_REGISTER_CLASS(SceneImporter, "8e8a519c-dacd-41a1-81ef-06b8a3b828de", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

#endif
