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

#ifndef META_INTERFACE_IATTACHMENT_H
#define META_INTERFACE_IATTACHMENT_H

#include <base/containers/vector.h>

#include <meta/base/bit_field.h>
#include <meta/base/namespace.h>
#include <meta/base/shared_ptr.h>
#include <meta/base/types.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_object.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IAttachment, "1f661207-4478-4fbb-ac1b-8ee73365d2d8")
/**
 * @brief The IAttachment interface defines a common interface for classes which can be attached
 *        to objects that implement the IAttach interface.
 *
 *        meta/attachment.h defines a helper class for implementing an object which implements
 *        the IAttachment interface.
 */
class IAttachment : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IAttachment)
public:
    /**
     * @brief The current data context.
     */
    META_READONLY_PROPERTY(IObject::WeakPtr, DataContext);
    /**
     * @brief The object this attachment is attached to.
     */
    META_READONLY_PROPERTY(IAttach::WeakPtr, AttachedTo);
    /**
     * @brief Called by the framework when an the attachment is being attached to an IObject. If this
     *        function succeeds, the object is attached to the target.
     * @param object The IObject instance the attachment is attached to.
     * @param dataContext The data context for this attachment.
     * @note The data context can be the same object as the object being attached to, or
     *       something else. It is up to the attachment to decide how to handle them.
     * @return The implementation should return true if the attachment can be attached to target object.
     *         If the attachment cannot be added, the implementation should return false.
     */
    virtual bool Attaching(const IAttach::Ptr& target, const IObject::Ptr& dataContext) = 0;
    /**
     * @brief Detach the attachment from an object.
     * @param object The object to attach to.
     * @return If the attachment can be detached from the target, the implementation should return true.
     *         If detaching is not possible, the implementation should return false. In such a case the
     *         target may choose to not remove the attachment. During for example object destruction,
     *         the target will ignore the return value.
     */
    virtual bool Detaching(const IAttach::Ptr& target) = 0;
};

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IAttachment);

#endif
