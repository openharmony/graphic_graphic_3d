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

#ifndef META_EXT_OBJECT_H
#define META_EXT_OBJECT_H


#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/ext/base_object.h>
#include <meta/ext/metadata_helpers.h>
#include <meta/ext/object_factory.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_attachment.h>
#include <meta/interface/intf_attachment_container.h>
#include <meta/interface/intf_container_query.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/intf_property.h>
#include <meta/interface/static_object_metadata.h>


META_BEGIN_NAMESPACE()

/**
 * @brief A helper class for implementing a class which implements the full set of object interfaces.
 */
class MetaObject : public IntroduceInterfaces<BaseObject, IMetadata, IOwner, IAttach> {
    using Super = IntroduceInterfaces<BaseObject, IMetadata, IOwner, IAttach>;

public:
    bool Build(const IMetadata::Ptr& d) override
    {
        bool res = Super::Build(d);
        if (res) {
            data_ = GetObjectRegistry().ConstructObjectDataContainer();
            auto attc = interface_cast<IAttachmentContainer>(data_);
            res = attc && attc->Initialize(GetSelf<IAttach>());
        }
        return res;
    }

    bool AddProperty(const IProperty::Ptr& p) override
    {
        return data_->AddProperty(p);
    }
    bool RemoveProperty(const IProperty::Ptr& p) override
    {
        return data_->RemoveProperty(p);
    }
    bool AddFunction(const IFunction::Ptr& p) override
    {
        return data_->AddFunction(p);
    }
    bool RemoveFunction(const IFunction::Ptr& p) override
    {
        return data_->RemoveFunction(p);
    }
    bool AddEvent(const IEvent::Ptr& p) override
    {
        return data_->AddEvent(p);
    }
    bool RemoveEvent(const IEvent::Ptr& p) override
    {
        return data_->RemoveEvent(p);
    }
    BASE_NS::vector<META_NS::IProperty::Ptr> GetProperties() override
    {
        return data_->GetProperties();
    }
    BASE_NS::vector<META_NS::IProperty::ConstPtr> GetProperties() const override
    {
        return const_cast<const IMetadata&>(*data_).GetProperties();
    }
    BASE_NS::vector<IFunction::Ptr> GetFunctions() override
    {
        return data_->GetFunctions();
    }
    BASE_NS::vector<IFunction::ConstPtr> GetFunctions() const override
    {
        return const_cast<const IMetadata&>(*data_).GetFunctions();
    }
    BASE_NS::vector<IEvent::Ptr> GetEvents() override
    {
        return data_->GetEvents();
    }
    BASE_NS::vector<IEvent::ConstPtr> GetEvents() const override
    {
        return const_cast<const IMetadata&>(*data_).GetEvents();
    }
    BASE_NS::vector<MetadataInfo> GetAllMetadatas(MetadataType types) const override
    {
        return const_cast<const IMetadata&>(*data_).GetAllMetadatas(types);
    }
    using IMetadata::GetProperty;
    IProperty::Ptr GetProperty(BASE_NS::string_view name, MetadataQuery q) override
    {
        return data_->GetProperty(name, q);
    }
    IProperty::ConstPtr GetProperty(BASE_NS::string_view name, MetadataQuery q) const override
    {
        return const_cast<const IMetadata&>(*data_).GetProperty(name, q);
    }
    using IMetadata::GetFunction;
    IFunction::ConstPtr GetFunction(BASE_NS::string_view name, MetadataQuery q) const override
    {
        return const_cast<const IMetadata&>(*data_).GetFunction(name, q);
    }
    IFunction::Ptr GetFunction(BASE_NS::string_view name, MetadataQuery q) override
    {
        return data_->GetFunction(name, q);
    }
    using IMetadata::GetEvent;
    IEvent::ConstPtr GetEvent(BASE_NS::string_view name, MetadataQuery q) const override
    {
        return const_cast<const IMetadata&>(*data_).GetEvent(name, q);
    }
    IEvent::Ptr GetEvent(BASE_NS::string_view name, MetadataQuery q) override
    {
        return data_->GetEvent(name, q);
    }

protected: // IAttach
    bool Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext) override
    {
        return interface_cast<IAttachmentContainer>(data_)->Attach(attachment, dataContext);
    }

    bool Detach(const IObject::Ptr& attachment) override
    {
        return interface_cast<IAttachmentContainer>(data_)->Detach(attachment);
    }
    BASE_NS::vector<IObject::Ptr> GetAttachments(const BASE_NS::vector<TypeId>& uids, bool strict) const override
    {
        return interface_cast<IAttachmentContainer>(data_)->GetAttachments(uids, strict);
    }
    bool HasAttachments() const override
    {
        return interface_cast<IContainer>(data_)->GetSize() != 0;
    }
    IContainer::Ptr GetAttachmentContainer(bool initializeAlways) const override
    {
        return interface_pointer_cast<IContainer>(data_);
    }

protected:
    MetaObject() = default;
    ~MetaObject() override = default;
    META_NO_COPY_MOVE(MetaObject)

private:
    mutable IMetadata::Ptr data_;
};

META_END_NAMESPACE()

#endif
