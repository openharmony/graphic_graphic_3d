/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Event wrapper
 * Author: Mikael Kilpeläinen
 * Create: 2023-11-13
 */

#ifndef META_INTERFACE_EVENT_H
#define META_INTERFACE_EVENT_H

#include <base/containers/shared_ptr.h>

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()
class IEvent;
class ICallable;
template<typename EventT>
class Event;
template<>
class Event<IEvent> {
public:
    using EventType = IEvent;
    using ValueType = BASE_NS::shared_ptr<IEvent>;

    Event(const ValueType& ev = nullptr) : event_(ev) {}

    ValueType operator->() const
    {
        return event_;
    }
    IEvent& operator*() const
    {
        return *event_;
    }
    operator ValueType() const
    {
        return event_;
    }
    explicit operator bool() const
    {
        return event_ != nullptr;
    }
    ValueType GetEvent() const
    {
        return event_;
    }

protected:
    BASE_NS::shared_ptr<IEvent> event_;
};
template<typename EventT>
class Event : public Event<IEvent> {
public:
    using Event<IEvent>::Event;
    using ValueType = BASE_NS::shared_ptr<EventT>;

    Event(const ValueType& ev = nullptr) : Event<IEvent>(interface_pointer_cast<IEvent>(ev)) {}
    BASE_NS::shared_ptr<ICallable> GetCallable() const
    {
        return interface_pointer_cast<ICallable>(event_);
    }
};

META_END_NAMESPACE()

#endif
