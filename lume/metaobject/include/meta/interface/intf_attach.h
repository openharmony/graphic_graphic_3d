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

#ifndef META_INTERFACE_IATTACH_H
#define META_INTERFACE_IATTACH_H

#include <base/containers/vector.h>

#include <meta/base/interface_utils.h>
#include <meta/base/namespace.h>
#include <meta/base/shared_ptr.h>
#include <meta/base/types.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_object.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IAttach, "2cc0d7b9-ccec-474e-acdd-d06fb5aeb714")

class IAttachment;

/**
 * @brief The IAttach interface defines a common interface for classes to which attachments can be attached.
 */
class IAttach : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IAttach)
public:
    /**
     * @brief Attach an attachment to this object. The object being attached to will be used as the data context.
     * @param attachment The attachment to add
     * @return True if the operation succeeded. False otherwise.
     */
    bool Attach(const IObject::Ptr& attachment)
    {
        return Attach(attachment, {});
    }
    /** Type helper for Attach */
    template<class T>
    bool Attach(const T& object)
    {
        return Attach(interface_pointer_cast<IObject>(object), {});
    }
    /**
     * @brief Attach an attachment to this object.
     * @param attachment The attachment to add
     * @param dataContext The object to use as the data context for the attachment. If dataContext is null,
     *                    the object being attached to will be used as the data context.
     * @return True if the operation succeeded. False otherwise.
     */
    virtual bool Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext) = 0;
    /** Type helper for Attach */
    template<class T, class U>
    bool Attach(const T& object, const U& dataContext)
    {
        return Attach(interface_pointer_cast<IObject>(object), interface_pointer_cast<IObject>(dataContext));
    }
    /**
     * @brief Detach an attachment from this object.
     * @param attachment The attachment to detach.
     * @return True if the opeation succeeded, false otherwise.
     */
    virtual bool Detach(const IObject::Ptr& attachment) = 0;
    /** Type helper for Detach */
    template<class T>
    bool Detach(const T& object)
    {
        return Detach(interface_pointer_cast<IObject>(object));
    }
    /**
     * @brief Returns true if any attachments have been attached to the object.
     */
    virtual bool HasAttachments() const = 0;
    /**
     * @brief Returns the attachment container or nullptr if no attachments have been added.
     */
    IContainer::Ptr GetAttachmentContainer() const
    {
        return GetAttachmentContainer(false);
    }
    /**
     * @brief Returns the container which hosts all attachments attached to the object.
     * @param initializeAlways If true, always return a valid IContainer::Ptr. If false,
     *        the function may return nullptr in case no attachments have been added to
     *        the object.
     * @return An IContainer instance which holds all attachments that have been added
     *         to the container.
     */
    virtual IContainer::Ptr GetAttachmentContainer(bool initializeAlways) const = 0;
    /**
     * @brief Returns a list of all attachments attached to this object.
     * @return A list of attachments which are attached to this object.
     */
    BASE_NS::vector<IObject::Ptr> GetAttachments() const
    {
        return GetAttachments({}, false);
    }
    /**
     * @brief Returns a list of all attachments attached to this object.
     * @param uids A list of interface Uids the returned attachments should implement. If the list is empty, return
     *             all attachments.
     * @param strict If true, the attachments added to the list must implement all of the given interfaces.
     *               If false, it is enough for the attachment to implement one (or more) of the given uids to be
     *               added to the list.
     * @return A list of attachments which are attached to this object and fullfil the requirements.
     */
    virtual BASE_NS::vector<IObject::Ptr> GetAttachments(const BASE_NS::vector<TypeId>& uids, bool strict) const = 0;
    /**
     * @brief A helper template which can be used to retrieve a list of attachments which implement a given interface.
     *        The returned list contains the found attachments pre-casted to T::Ptr.
     */
    template<class T>
    BASE_NS::vector<typename T::Ptr> GetAttachments() const
    {
        return PtrArrayCast<T>(GetAttachments({ T::UID }, false));
    }
};

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IAttach);

// Include intf_attachment.h to ensure that we get full implementation of our forward declarations
#include <meta/interface/intf_attachment.h>

#endif
