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

SCENE_BEGIN_NAMESPACE()

class SceneImporter : public META_NS::IntroduceInterfaces<META_NS::MetaObject, ISceneImporter> {
    META_OBJECT(SceneImporter, SCENE_NS::ClassId::SceneImporter, IntroduceInterfaces)
public:
    bool Build(const META_NS::IMetadata::Ptr& d) override;
    IScene::Ptr ImportScene(CORE_NS::IFile&) override;
    INode::Ptr ImportNode(CORE_NS::IFile&, const INode::Ptr& parent) override;

private:
    IRenderContext::WeakPtr context_;
    SceneOptions opts_;
};

SCENE_END_NAMESPACE()

#endif