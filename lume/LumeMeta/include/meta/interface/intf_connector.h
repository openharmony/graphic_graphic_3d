/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: Interface for connector objects
 * Author: Mikael Kilpeläinen
 * Create: 2023-02-24
 */

#ifndef META_INTERFACE_CONNECTOR_H
#define META_INTERFACE_CONNECTOR_H

#include <core/plugin/intf_interface.h>

#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_call_context.h>
#include <meta/interface/intf_callable.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Interface for objects that do event <-> meta function connections.
 *        Such objects are usually attachments and so the actual connection is done when attaching to the destination
 * object.
 */
class IConnector : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IConnector, "a6c9f964-da3e-45e1-8301-ca8c4ff1c628")
public:
    /**
     * @brief Connect event to meta function.
     * @param source Object that contains the event.
     * @param event Name of the event.
     * @param func Name of the function, the attached object is used as destination object.
     * @return True on success, otherwise false.
     * Note: This doesn't do the actual connection, it is done when the attachement is added to an object.
     */
    virtual bool Connect(const IObject::Ptr& source, const BASE_NS::string& event, const BASE_NS::string& func) = 0;

    /**
     * @brief Get the source object or null if not set (or alive).
     */
    virtual IObject::Ptr GetSource() const = 0;
    /**
     * @brief Get the event name.
     */
    virtual BASE_NS::string GetEventName() const = 0;
    /**
     * @brief Get the function name.
     */
    virtual BASE_NS::string GetFunctionName() const = 0;
};

META_END_NAMESPACE()

#endif
