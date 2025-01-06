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

#ifndef META_EXT_OBJECT_FWD_H
#define META_EXT_OBJECT_FWD_H

#include <base/util/uid_util.h>

#include <meta/ext/base_object_fwd.h>
#include <meta/ext/metadata_helpers.h>
#include <meta/interface/intf_attachment.h>
#include <meta/interface/intf_container_query.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/property/intf_property.h>
#include <meta/interface/static_object_metadata.h>

#include "object_factory.h"

META_BEGIN_NAMESPACE()

template<typename T>
inline T DefaultResult()
{
    return T {};
}
template<>
inline void DefaultResult<void>()
{}
#define META_IMPL_CALL_BASE(Interface, Function)                \
    [&] {                                                       \
        auto b = Super::template GetBaseAs<Interface>();        \
        if (b) {                                                \
            return b->Function;                                 \
        }                                                       \
        return META_NS::DefaultResult<decltype(b->Function)>(); \
    }()
/**
 * @brief A helper macro for classes deriving from BaseObjectFwd. Makes a forwarder for an interface
 *        property implemented by the super class.
 */
#define META_EXT_BASE_READONLY_PROPERTY(Interface, Type, Name) \
    META_FORWARD_READONLY_PROPERTY(Type, Name, META_IMPL_CALL_BASE(Interface, Name()))

/**
 * @brief A helper macro for classes deriving from BaseObjectFwd. Makes a forwarder for an interface
 *        property defined by the super class.
 */
#define META_EXT_BASE_PROPERTY(Interface, Type, Name) \
    META_FORWARD_PROPERTY(Type, Name, META_IMPL_CALL_BASE(Interface, Name()))

/**
 * @brief A helper macro for classes deriving from BaseObjectFwd. Calls a method through an interface defined
 *        by the super class.
 */
#define META_EXT_CALL_BASE(Interface, Function) META_IMPL_CALL_BASE(Interface, Function)

/**
 * @brief A helper class for implementing a class which implements the full set of object interfaces.
 */
class ObjectFwd : public IntroduceInterfaces<BaseObjectFwd, IMetadata, IOwner, IAttach> {
    META_OBJECT_NO_CLASSINFO(ObjectFwd, IntroduceInterfaces, ClassId::Object)

protected:
    bool AddProperty(const IProperty::Ptr& p) override
    {
        return META_EXT_CALL_BASE(IMetadata, AddProperty(p));
    }
    bool RemoveProperty(const IProperty::Ptr& p) override
    {
        return META_EXT_CALL_BASE(IMetadata, RemoveProperty(p));
    }
    bool AddFunction(const IFunction::Ptr& p) override
    {
        return META_EXT_CALL_BASE(IMetadata, AddFunction(p));
    }
    bool RemoveFunction(const IFunction::Ptr& p) override
    {
        return META_EXT_CALL_BASE(IMetadata, RemoveFunction(p));
    }
    bool AddEvent(const IEvent::Ptr& p) override
    {
        return META_EXT_CALL_BASE(IMetadata, AddEvent(p));
    }
    bool RemoveEvent(const IEvent::Ptr& p) override
    {
        return META_EXT_CALL_BASE(IMetadata, RemoveEvent(p));
    }
    BASE_NS::vector<META_NS::IProperty::Ptr> GetProperties() override
    {
        return META_EXT_CALL_BASE(IMetadata, GetProperties());
    }
    BASE_NS::vector<META_NS::IProperty::ConstPtr> GetProperties() const override
    {
        return META_EXT_CALL_BASE(IMetadata, GetProperties());
    }
    BASE_NS::vector<IFunction::Ptr> GetFunctions() override
    {
        return META_EXT_CALL_BASE(IMetadata, GetFunctions());
    }
    BASE_NS::vector<IFunction::ConstPtr> GetFunctions() const override
    {
        return META_EXT_CALL_BASE(IMetadata, GetFunctions());
    }
    BASE_NS::vector<IEvent::Ptr> GetEvents() override
    {
        return META_EXT_CALL_BASE(IMetadata, GetEvents());
    }
    BASE_NS::vector<IEvent::ConstPtr> GetEvents() const override
    {
        return META_EXT_CALL_BASE(IMetadata, GetEvents());
    }
    BASE_NS::vector<MetadataInfo> GetAllMetadatas(MetadataType types) const override
    {
        return META_EXT_CALL_BASE(IMetadata, GetAllMetadatas(types));
    }
    using IMetadata::GetProperty;
    IProperty::Ptr GetProperty(BASE_NS::string_view name, MetadataQuery q) override
    {
        return META_EXT_CALL_BASE(IMetadata, GetProperty(name, q));
    }
    IProperty::ConstPtr GetProperty(BASE_NS::string_view name, MetadataQuery q) const override
    {
        return META_EXT_CALL_BASE(IMetadata, GetProperty(name, q));
    }
    using IMetadata::GetFunction;
    IFunction::ConstPtr GetFunction(BASE_NS::string_view name, MetadataQuery q) const override
    {
        return META_EXT_CALL_BASE(IMetadata, GetFunction(name, q));
    }
    IFunction::Ptr GetFunction(BASE_NS::string_view name, MetadataQuery q) override
    {
        return META_EXT_CALL_BASE(IMetadata, GetFunction(name, q));
    }
    using IMetadata::GetEvent;
    IEvent::ConstPtr GetEvent(BASE_NS::string_view name, MetadataQuery q) const override
    {
        return META_EXT_CALL_BASE(IMetadata, GetEvent(name, q));
    }
    IEvent::Ptr GetEvent(BASE_NS::string_view name, MetadataQuery q) override
    {
        return META_EXT_CALL_BASE(IMetadata, GetEvent(name, q));
    }

protected: // IAttach
    bool Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext) override
    {
        return META_EXT_CALL_BASE(IAttach, Attach(attachment, dataContext));
    }

    bool Detach(const IObject::Ptr& attachment) override
    {
        return META_EXT_CALL_BASE(IAttach, Detach(attachment));
    }
    BASE_NS::vector<IObject::Ptr> GetAttachments(const BASE_NS::vector<TypeId>& uids, bool strict) const override
    {
        return META_EXT_CALL_BASE(IAttach, GetAttachments(uids, strict));
    }
    bool HasAttachments() const override
    {
        return META_EXT_CALL_BASE(IAttach, HasAttachments());
    }
    IContainer::Ptr GetAttachmentContainer(bool initializeAlways) const override
    {
        return META_EXT_CALL_BASE(IAttach, GetAttachmentContainer(initializeAlways));
    }

protected:
    ObjectFwd() = default;
    ~ObjectFwd() override = default;
    META_NO_COPY_MOVE(ObjectFwd)
};

META_END_NAMESPACE()

#endif
