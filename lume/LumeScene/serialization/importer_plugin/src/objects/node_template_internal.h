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

#ifndef SCENE_IMP_SRC_OBJECTS_NODE_TEMPLATE_INTERNAL_H
#define SCENE_IMP_SRC_OBJECTS_NODE_TEMPLATE_INTERNAL_H

#include <scene/interface/resource/intf_exported_node.h>
#include <scene/interface/resource/types.h>

#include <meta/ext/object.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/resource/intf_object_template.h>

#include "../import_context.h"

SCENE_IMP_BEGIN_NAMESPACE()

class INodeTemplatePayloadInternal : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, INodeTemplatePayloadInternal, "d4c180cc-d85a-473f-9ca8-d557a5324086")
public:
    virtual CORE_NS::json::value GetJson() const = 0;
    virtual void SetJson(CORE_NS::json::standalone_value json) = 0;
    virtual BASE_NS::string GetSourceFilename() const = 0;
    virtual void SetSourceFilename(BASE_NS::string_view filename) = 0;
    virtual size_t GetFileDepth() const = 0;
    virtual void SetFileDepth(size_t depth) = 0;
    virtual IImporterInternal::Ptr GetImporter() const = 0;
    virtual void SetImporter(IImporterInternal::WeakPtr) = 0;
};

META_REGISTER_CLASS(
    NodeTemplatePayload, "b313637b-d801-46d7-a945-5ef3149db269", META_NS::ObjectCategoryBits::NO_CATEGORY)

class NodeTemplatePayload
    : public META_NS::IntroduceInterfaces<META_NS::MetaObject, SCENE_NS::IExportedNode, INodeTemplatePayloadInternal> {
    META_OBJECT(NodeTemplatePayload, ClassId::NodeTemplatePayload, IntroduceInterfaces)
public:
    BASE_NS::vector<CORE_NS::ResourceInfo> GetResources() const override
    {
        return resources_;
    }
    void SetResources(BASE_NS::vector<CORE_NS::ResourceInfo> list) override
    {
        resources_ = BASE_NS::move(list);
    }
    BASE_NS::string GetPrimaryGroup() const override
    {
        return primaryGroup_;
    }
    void SetPrimaryGroup(BASE_NS::string_view group) override
    {
        primaryGroup_ = group;
    }
    CORE_NS::json::value GetJson() const override
    {
        return json_;
    }
    void SetJson(CORE_NS::json::standalone_value json) override
    {
        json_ = BASE_NS::move(json);
    }
    BASE_NS::string GetSourceFilename() const override
    {
        return sourceFilename_;
    }
    void SetSourceFilename(BASE_NS::string_view filename) override
    {
        sourceFilename_ = filename;
    }
    size_t GetFileDepth() const override
    {
        return fileDepth_;
    }
    void SetFileDepth(size_t depth) override
    {
        fileDepth_ = depth;
    }
    IImporterInternal::Ptr GetImporter() const override
    {
        return importer_.lock();
    }
    void SetImporter(IImporterInternal::WeakPtr i) override
    {
        importer_ = BASE_NS::move(i);
    }

private:
    CORE_NS::json::standalone_value json_;
    BASE_NS::string primaryGroup_;
    BASE_NS::string sourceFilename_;
    size_t fileDepth_{};
    BASE_NS::vector<CORE_NS::ResourceInfo> resources_;
    IImporterInternal::WeakPtr importer_;
};

META_REGISTER_CLASS(
    NodeTemplateInstantiator, "1844a5a6-53f1-48d7-a888-c0dcc8974e99", META_NS::ObjectCategoryBits::NO_CATEGORY)

class NodeTemplateInstantiator
    : public META_NS::IntroduceInterfaces<META_NS::MetaObject, META_NS::IObjectInstantiator> {
    META_OBJECT(NodeTemplateInstantiator, ClassId::NodeTemplateInstantiator, IntroduceInterfaces)
public:
    IObject::Ptr Instantiate(
        const META_NS::IObjectTemplate& templ, const META_NS::SharedPtrIInterface& context) override;

    META_NS::IObject::Ptr Update(META_NS::IObject& inst, const META_NS::IObjectTemplate& templ,
        const META_NS::SharedPtrIInterface& context) override;
};

SCENE_IMP_END_NAMESPACE()

#endif