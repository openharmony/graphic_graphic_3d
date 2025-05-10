/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: State change notification interface
 * Author: Mikael Kilpeläinen
 * Create: 2023-05-15
 */

#ifndef META_INTERFACE_INOTIFY_ON_CHANGE_H
#define META_INTERFACE_INOTIFY_ON_CHANGE_H

#include <meta/base/types.h>
#include <meta/interface/event.h>
#include <meta/interface/metadata_query.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(INotifyOnChange, "32de3870-a07d-4998-b563-2d24a5328df3")

/**
 * @brief State change notification interface
 */
class INotifyOnChange : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, INotifyOnChange)
public:
    /**
     * @brief Event for notifying about state change.
     */
    // META_EVENT(IEvent, OnChanged)
    virtual BASE_NS::shared_ptr<IEvent> EventOnChanged(MetadataQuery) const = 0;
    Event<IEvent> OnChanged() const
    {
        return EventOnChanged(MetadataQuery::CONSTRUCT_ON_REQUEST);
    }
};

META_END_NAMESPACE()

#endif
