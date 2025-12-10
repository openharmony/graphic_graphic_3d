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

#ifndef SCENE_SRC_SERIALIZATION_SCENE_EXPORTER_H
#define SCENE_SRC_SERIALIZATION_SCENE_EXPORTER_H

#include <scene/interface/serialization/intf_scene_exporter.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

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

SCENE_END_NAMESPACE()

#endif