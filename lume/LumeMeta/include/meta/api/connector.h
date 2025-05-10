/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Connector helpers
 * Author: Mikael Kilpeläinen
 * Create: 2023-04-24
 */
#ifndef META_API_CONNECTOR_H
#define META_API_CONNECTOR_H

#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_attachment.h>
#include <meta/interface/intf_connector.h>
#include <meta/interface/intf_object_registry.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Connect source object event to destination object meta function.
 * @param source Object that contains the specified event.
 * @param event Event name.
 * @param dest Destination object which implements the specified meta function, must implement IAttach interface.
 * @func Function name.
 * @return True on success, otherwise false.
 */
inline bool Connect(
    const IObject::Ptr& source, const BASE_NS::string& event, const IObject::Ptr& dest, const BASE_NS::string& func)
{
    auto attach = interface_cast<IAttach>(dest);
    if (!attach) {
        CORE_LOG_E("Object does not implement IAttach");
        return false;
    }
    auto connector = META_NS::GetObjectRegistry().Create<IConnector>(META_NS::ClassId::Connector);
    if (!connector) {
        CORE_LOG_E("Failed to create connector object");
        return false;
    }
    return connector->Connect(source, event, func) && attach->Attach(interface_pointer_cast<IAttachment>(connector));
}

/**
 * @brief Disconnect connection created with above Connect function.
 * @param source Object that contains the specified event.
 * @param event Event name.
 * @param dest Destination object which implements the specified meta function, must implement IAttach interface.
 * @func Function name.
 * @return True if connector object was detached successfully, otherwise false.
 */
inline bool Disconnect(
    const IObject::Ptr& source, const BASE_NS::string& event, const IObject::Ptr& dest, const BASE_NS::string& func)
{
    auto attach = interface_cast<IAttach>(dest);
    if (!attach) {
        CORE_LOG_E("Object does not implement IAttach");
        return false;
    }
    auto cont = attach->GetAttachmentContainer();
    if (cont) {
        auto list = cont->GetAll<IConnector>();
        for (auto&& v : list) {
            if (v->GetSource() == source && v->GetEventName() == event && v->GetFunctionName() == func) {
                return attach->Detach(interface_pointer_cast<IAttachment>(v));
            }
        }
    }
    return false;
}

META_END_NAMESPACE()

#endif
