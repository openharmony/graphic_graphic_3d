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

#ifndef SCENE_API_EXTERNAL_NODE_H
#define SCENE_API_EXTERNAL_NODE_H

#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_scene.h>

#include <core/resources/intf_resource.h>

#include <meta/interface/intf_attach.h>

SCENE_BEGIN_NAMESPACE()

inline INode::Ptr AddExternalNode(INode& parent, const IScene::Ptr& external, BASE_NS::string_view nodeName)
{
    INode::Ptr node;
    if (auto i = interface_cast<INodeImport>(&parent)) {
        node = i->ImportChildScene(external, nodeName).GetResult();
        if (node) {
            if (auto ext = META_NS::GetObjectRegistry().Create<IExternalNode>(ClassId::ExternalNode)) {
                if (auto att = interface_cast<META_NS::IAttach>(node)) {
                    if (auto res = interface_cast<CORE_NS::IResource>(external)) {
                        ext->SetResourceId(res->GetResourceId());
                    }
                    att->Attach(ext);
                }
            }
        }
    }
    return node;
}

SCENE_END_NAMESPACE()

#endif // SCENE_API_EXTERNAL_NODE_H
