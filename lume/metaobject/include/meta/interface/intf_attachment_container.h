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

#ifndef META_INTERFACE_IATTACHMENT_CONTAINER_H
#define META_INTERFACE_IATTACHMENT_CONTAINER_H

#include <meta/base/namespace.h>
#include <meta/interface/intf_attach.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IAttachmentContainer, "c7ca2933-25b5-425a-ab26-a08fee2bf233")

/**
 * @brief The IAttachmentContainer interface defines an interface which can be instantiated
 *        by implementing classes that want to implement the IAttach interface.
 *
 *        The IAttachmentContainer defines an interface which is compatible with IAttach interface,
 *        so that a class implementing IAttach can mostly forward the IAttach interface calls
 *        to an IAttachmentContainer implementation.
 *
 *        The MetaObject library contains a built-in thread-safe implementation of IAttachmentContainer,
 *        which can be instantiated by calling
 *        CORE_NS::CreateInstance<META_NS::IAttachmentContainer>(META_NS::IAttachmentContainer::UID);
 */
class IAttachmentContainer : public CORE_NS::IInterface {
    META_INTERFACE2(CORE_NS::IInterface, IAttachmentContainer)
public:
    /**
     * @brief Initialize the container.
     * @param owner The owner object which wants to implement IAttach. The container does not
     *              take ownership of the owner.
     */
    virtual bool Initialize(const META_NS::IAttach::Ptr& owner) = 0;
    /**
     * @brief See IAttach::Attach()
     */
    bool Attach(const IObject::Ptr& attachment)
    {
        return Attach(attachment, {});
    }
    template<class T>
    bool Attach(const T& object)
    {
        return Attach(interface_pointer_cast<IObject>(object), {});
    }
    /**
     * @brief See IAttach::Attach()
     */
    virtual bool Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext) = 0;
    template<class T, class U>
    bool Attach(const T& object, const U& dataContext)
    {
        return Attach(interface_pointer_cast<IObject>(object), interface_pointer_cast<IObject>(dataContext));
    }
    /**
     * @brief Attach an attachment to this object at given position.
     * @param pos The index where the attachment should be added. If pos >= container size, the attachment
     *            is added at the end of the container.
     * @param attachment The attachment to add
     * @param dataContext The object to use as the data context for the attachment. If dataContext is null,
     *                    the object being attached to will be used as the data context.
     * @return True if the operation succeeded. False otherwise.
     */
    virtual bool Attach(IContainer::SizeType pos, const IObject::Ptr& attachment, const IObject::Ptr& dataContext) = 0;
    template<class T, class U>
    bool Attach(IContainer::SizeType pos, const T& object, const U& dataContext)
    {
        return Attach(pos, interface_pointer_cast<IObject>(object), interface_pointer_cast<IObject>(dataContext));
    }
    /**
     * @brief See IAttach::Detach()
     */
    virtual bool Detach(const IObject::Ptr& attachment) = 0;
    template<class T>
    bool Detach(const T& object)
    {
        return Detach(interface_pointer_cast<IObject>(object));
    }
    /**
     * @brief See IAttach::GetAttachments()
     */
    BASE_NS::vector<IObject::Ptr> GetAttachments()
    {
        return GetAttachments({}, false);
    }
    template<class T>
    BASE_NS::vector<typename T::Ptr> GetAttachments()
    {
        return PtrArrayCast<T>(GetAttachments({}, false));
    }
    /**
     * @brief See IAttach::GetAttachments()
     */
    virtual BASE_NS::vector<IObject::Ptr> GetAttachments(const BASE_NS::vector<TypeId>& uids, bool strict) = 0;
    /**
     * @brief Remove all attachments from the container.
     * @note  All of the items are detached, i.e. IAttachment::Detaching is called on all the currently
     *        attached attachments.
     */
    virtual void RemoveAllAttachments() = 0;

    /**
     * @brief Find attachment by name
     */
    virtual IObject::Ptr FindByName(const BASE_NS::string& name) const = 0;
};

META_END_NAMESPACE()

#endif
