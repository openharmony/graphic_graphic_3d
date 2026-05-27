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

#ifndef SCENE_SRC_RESOURCE_NODE_TEMPLATE_H
#define SCENE_SRC_RESOURCE_NODE_TEMPLATE_H

#include <scene/interface/resource/intf_exported_node.h>
#include <scene/interface/resource/types.h>

#include <base/containers/unordered_map.h>
#include <core/resources/intf_resource.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/object_fwd.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/resource/intf_object_template.h>
#include <meta/interface/serialization/intf_ser_transformation.h>
#include <meta/interface/serialization/intf_serializable.h>

SCENE_BEGIN_NAMESPACE()

class NodeTemplate
    : public META_NS::IntroduceInterfaces<META_NS::ObjectFwd, META_NS::INamed, CORE_NS::IResource,
          META_NS::ISerializable, CORE_NS::ISetResourceId, META_NS::IObjectTemplate, META_NS::IIterable> {
    META_OBJECT(NodeTemplate, ClassId::NodeTemplate, IntroduceInterfaces, META_NS::ClassId::ObjectTemplate)
public:
    META_EXT_BASE_PROPERTY(INamed, BASE_NS::string, Name)
    META_EXT_BASE_READONLY_PROPERTY(IContent, IObject::Ptr, Content)
    META_EXT_BASE_PROPERTY(IContent, bool, ContentSearchable)
    META_EXT_BASE_PROPERTY(IObjectTemplate, META_NS::ObjectId, Instantiator)

    IObject::Ptr Instantiate(const META_NS::SharedPtrIInterface& context) const override
    {
        return META_EXT_CALL_BASE(IObjectTemplate, Instantiate(context));
    }

    META_NS::IObject::Ptr Update(IObject& inst, const META_NS::SharedPtrIInterface& context) const override
    {
        return META_EXT_CALL_BASE(IObjectTemplate, Update(inst, context));
    }

    bool SetContent(const IObject::Ptr& content) override
    {
        return META_EXT_CALL_BASE(IContent, SetContent(content));
    }

    META_NS::IterationResult Iterate(const META_NS::IterationParameters& params) override
    {
        return META_EXT_CALL_BASE(IIterable, Iterate(params));
    }

    META_NS::IterationResult Iterate(const META_NS::IterationParameters& params) const override
    {
        return META_EXT_CALL_BASE(IIterable, Iterate(params));
    }

    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::NodeTemplateResource.Id().ToUid();
    }
    CORE_NS::ResourceId GetResourceId() const override
    {
        return META_EXT_CALL_BASE(IResource, GetResourceId());
    }
    CORE_NS::ResourceContextWeakPtr GetContext() const override
    {
        return META_EXT_CALL_BASE(IResource, GetContext());
    }

    META_NS::ReturnError Export(META_NS::IExportContext& c) const override
    {
        return META_EXT_CALL_BASE(ISerializable, Export(c));
    }

    META_NS::ReturnError Import(META_NS::IImportContext& c) override
    {
        return META_EXT_CALL_BASE(ISerializable, Import(c));
    }

    void SetResourceId(CORE_NS::ResourceIdContext id) override
    {
        return META_EXT_CALL_BASE(ISetResourceId, SetResourceId(id));
    }
};

class NodeTemplateContext : public META_NS::IntroduceInterfaces<META_NS::MetaObject, INodeTemplateContext> {
    META_OBJECT(NodeTemplateContext, ClassId::NodeTemplateContext, IntroduceInterfaces)
public:
    INode::Ptr GetTargetNode() const override
    {
        return node_;
    }
    BASE_NS::string GetGroup() const override
    {
        return group_;
    }
    virtual bool ApplyAsTemplate() const override
    {
        return applyAsTemplate_;
    }

    void SetTargetNode(INode::Ptr n) override
    {
        node_ = BASE_NS::move(n);
    }
    void SetGroup(BASE_NS::string group) override
    {
        group_ = BASE_NS::move(group);
    }
    void SetApplyAsTemplate(bool v) override
    {
        applyAsTemplate_ = v;
    }

private:
    INode::Ptr node_;
    BASE_NS::string group_;
    bool applyAsTemplate_{true};
};

SCENE_END_NAMESPACE()

#endif