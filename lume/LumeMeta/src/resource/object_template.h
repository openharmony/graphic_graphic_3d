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

#ifndef META_SRC_RESOURCE_OBJECT_TEMPLATE_H
#define META_SRC_RESOURCE_OBJECT_TEMPLATE_H

#include <core/resources/intf_resource_manager.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/resource/resource.h>
#include <meta/interface/resource/intf_object_template.h>
#include <meta/interface/serialization/intf_export_context.h>
#include <meta/interface/serialization/intf_serializable.h>

META_BEGIN_NAMESPACE()

class ObjectTemplate : public IntroduceInterfaces<MetaObject, Resource, IObjectTemplate, IIterable> {
    META_OBJECT(ObjectTemplate, ClassId::ObjectTemplate, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IContent, IObject::Ptr, Content)
    META_STATIC_PROPERTY_DATA(IContent, bool, ContentSearchable)
    META_STATIC_PROPERTY_DATA(IObjectTemplate, ObjectId, Instantiator)
    META_END_STATIC_DATA()

    META_IMPLEMENT_READONLY_PROPERTY(IObject::Ptr, Content)
    META_IMPLEMENT_PROPERTY(bool, ContentSearchable)
    META_IMPLEMENT_PROPERTY(ObjectId, Instantiator)

    IObject::Ptr Instantiate(const SharedPtrIInterface& context) const override;
    META_NS::IObject::Ptr Update(IObject& inst, const SharedPtrIInterface& context) const override;

    bool SetContent(const IObject::Ptr& content) override;

    IterationResult Iterate(const IterationParameters& params) override;
    IterationResult Iterate(const IterationParameters& params) const override;

    CORE_NS::ResourceType GetResourceType() const override;

    ReturnError Export(IExportContext& c) const override
    {
        if (c.Context().GetUserContext() != GetSelf()) {
            return Resource::Export(c);
        }
        return Serializer(c) & AutoSerialize();
    }

private:
    template<typename Func>
    IterationResult IterateImpl(const Func& f) const;
};

class ObjectTemplateType : public IntroduceInterfaces<MetaObject, CORE_NS::IResourceType> {
    META_OBJECT(ObjectTemplateType, ClassId::ObjectTemplateType, IntroduceInterfaces)
public:
    BASE_NS::string GetResourceName() const override;
    BASE_NS::Uid GetResourceType() const override;
    CORE_NS::IResource::Ptr LoadResource(const StorageInfo&) const override;
    bool SaveResource(const CORE_NS::IResource::ConstPtr&, const StorageInfo&) const override;
    bool ReloadResource(const StorageInfo&, const CORE_NS::IResource::Ptr&) const override;
};

META_END_NAMESPACE()

#endif
