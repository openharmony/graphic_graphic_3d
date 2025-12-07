/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SCENE_INTERFACE_RESOURCE_UTIL_H
#define SCENE_INTERFACE_RESOURCE_UTIL_H

#include <scene/interface/intf_node.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/serialization/intf_scene_exporter.h>

#include <meta/interface/builtin_objects.h>
#include <meta/interface/resource/intf_object_template.h>

SCENE_BEGIN_NAMESPACE()

inline META_NS::IObjectTemplate::Ptr CreateNodeTemplate(const CORE_NS::ResourceId& id, const INode::ConstPtr& node)
{
    auto templ = META_NS::GetObjectRegistry().Create<META_NS::IObjectTemplate>(META_NS::ClassId::ObjectTemplate);
    if (templ) {
        if (auto exporter = META_NS::GetObjectRegistry().Create<ISceneExporter>(ClassId::SceneExporter)) {
            if (auto obj = exporter->ExportNode(node)) {
                templ->Instantiator()->SetValue(ClassId::NodeInstantiator);
                templ->SetContent(obj);
                if (auto i = interface_cast<CORE_NS::ISetResourceId>(templ)) {
                    i->SetResourceId(id);
                }
            } else {
                templ.reset();
            }
        }
    }
    return templ;
}

SCENE_END_NAMESPACE()

#endif
