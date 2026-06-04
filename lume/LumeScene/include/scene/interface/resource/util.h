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
#include <scene/interface/resource/intf_exported_node.h>
#include <scene/interface/resource/types.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/api/util.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/resource/intf_object_template.h>

SCENE_BEGIN_NAMESPACE()

inline INode::Ptr ApplyAsNonTemplate(const META_NS::IObjectTemplate::ConstPtr& templ, const INode::Ptr& node)
{
    if (templ) {
        auto ntcontext = META_NS::GetObjectRegistry().Create<INodeTemplateContext>(ClassId::NodeTemplateContext);
        if (!ntcontext) {
            CORE_LOG_W("Invalid state");
            return INode::Ptr{};
        }
        ntcontext->SetTargetNode(node);
        ntcontext->SetApplyAsTemplate(false);

        auto nodeRes = interface_pointer_cast<INodeImportResult>(templ->Instantiate(ntcontext));
        return nodeRes ? nodeRes->GetNode() : nullptr;
    }
    return {};
}

inline META_NS::IObjectTemplate::Ptr LoadNodeTemplate(CORE_NS::IResourceManager& manager, BASE_NS::string_view path)
{
    auto res = manager.GetResource(CORE_NS::ResourceIdContext{BASE_NS::string(path)});
    if (!res) {
        if (manager.AddResource(
                CORE_NS::ResourceIdContext{BASE_NS::string(path)}, ClassId::NodeTemplateResource.Id().ToUid(), path)) {
            res = manager.GetResource(CORE_NS::ResourceIdContext{BASE_NS::string(path)});
        }
    }
    return interface_pointer_cast<META_NS::IObjectTemplate>(res);
}

SCENE_END_NAMESPACE()

#endif
