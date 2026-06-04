/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_INTERFACE_RESOURCE_IEXPORTED_NODE_H
#define SCENE_INTERFACE_RESOURCE_IEXPORTED_NODE_H

#include <scene/interface/intf_node.h>

#include <base/util/color.h>
#include <core/resources/intf_resource_manager.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

class IExportedNode : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IExportedNode, "6ad8dfcf-3318-45b3-b304-0178b6dd9d95")
public:
    virtual BASE_NS::vector<CORE_NS::ResourceInfo> GetResources() const = 0;
    virtual void SetResources(BASE_NS::vector<CORE_NS::ResourceInfo>) = 0;
    virtual BASE_NS::string GetPrimaryGroup() const = 0;
    virtual void SetPrimaryGroup(BASE_NS::string_view group) = 0;
};

class INodeTemplateContext : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, INodeTemplateContext, "b4574b2b-6f21-4c6e-9d68-4eca795dc53d")
public:
    virtual INode::Ptr GetTargetNode() const = 0;
    virtual BASE_NS::string GetGroup() const = 0;
    virtual bool ApplyAsTemplate() const = 0;

    virtual void SetTargetNode(INode::Ptr node) = 0;
    virtual void SetGroup(BASE_NS::string group) = 0;
    virtual void SetApplyAsTemplate(bool) = 0;
};

class INodeImportResult : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, INodeImportResult, "c8508636-9810-40e2-be00-e9c42ca61251")
public:
    virtual INode::Ptr GetNode() const = 0;
    virtual BASE_NS::vector<CORE_NS::ResourceIdContext> GetNewResourcesImported() const = 0;
    virtual BASE_NS::vector<CORE_NS::ResourceIdContext> GetAssociatedResources() const = 0;
};

SCENE_END_NAMESPACE()

#endif
