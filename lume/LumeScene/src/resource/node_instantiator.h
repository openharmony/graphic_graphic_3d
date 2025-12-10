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

#ifndef SCENE_SRC_RESOURCE_NODE_INTANTIATOR_H
#define SCENE_SRC_RESOURCE_NODE_INTANTIATOR_H

#include <scene/interface/resource/types.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/resource/intf_object_template.h>
#include <meta/interface/serialization/intf_serializable.h>

SCENE_BEGIN_NAMESPACE()
struct SerNodeHierarchy {
    META_NS::ISerNode::ConstPtr nodes;
};
SCENE_END_NAMESPACE()
META_TYPE(SCENE_NS::SerNodeHierarchy)
SCENE_BEGIN_NAMESPACE()

class INodeTemplateInternal : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, INodeTemplateInternal, "d4c180cc-d85a-473f-9ca8-d557a5324086")
public:
    virtual META_NS::ISerNode::ConstPtr GetNodeHierarchy() const = 0;
    virtual void SetNodeHierarchy(META_NS::ISerNode::ConstPtr) = 0;
};

META_REGISTER_CLASS(NodeTemplate, "86202cb7-e1b8-4c5b-b2f3-0ad22f4b9677", META_NS::ObjectCategoryBits::NO_CATEGORY)

class NodeTemplate
    : public META_NS::IntroduceInterfaces<META_NS::MetaObject, META_NS::ISerializable, INodeTemplateInternal> {
    META_OBJECT(NodeTemplate, ClassId::NodeTemplate, IntroduceInterfaces)
public:
    META_NS::ReturnError Export(META_NS::IExportContext& c) const override
    {
        return META_NS::Serializer(c) & META_NS::AutoSerialize() & META_NS::NamedValue("nodes", sNodes_);
    }
    META_NS::ReturnError Import(META_NS::IImportContext& c) override
    {
        return META_NS::Serializer(c) & META_NS::AutoSerialize() & META_NS::NamedValue("nodes", sNodes_);
    }
    META_NS::ISerNode::ConstPtr GetNodeHierarchy() const override
    {
        return sNodes_.nodes;
    }
    void SetNodeHierarchy(META_NS::ISerNode::ConstPtr p) override
    {
        sNodes_.nodes = BASE_NS::move(p);
    }

private:
    SerNodeHierarchy sNodes_;
};

class NodeInstantiator : public META_NS::IntroduceInterfaces<META_NS::MetaObject, META_NS::IObjectInstantiator> {
    META_OBJECT(NodeInstantiator, ClassId::NodeInstantiator, IntroduceInterfaces)
public:
    IObject::Ptr Instantiate(
        const META_NS::IObjectTemplate& templ, const META_NS::SharedPtrIInterface& context) override;
    META_NS::IObject::Ptr Update(META_NS::IObject& inst, const META_NS::IObjectTemplate& templ,
        const META_NS::SharedPtrIInterface& context) override;
};

SCENE_END_NAMESPACE()

#endif