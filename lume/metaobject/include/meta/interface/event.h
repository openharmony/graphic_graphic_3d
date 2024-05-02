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

#ifndef META_INTERFACE_EVENT_H
#define META_INTERFACE_EVENT_H

#include <meta/base/namespace.h>
#include <meta/base/shared_ptr.h>

META_BEGIN_NAMESPACE()
class IEvent;
template<typename EventT>
class Event;
template<>
class Event<IEvent> {
public:
    using EventType = IEvent;

    Event(const BASE_NS::shared_ptr<IEvent>& ev) : event_(ev) {}

    BASE_NS::shared_ptr<IEvent> operator->() const
    {
        return event_;
    }
    IEvent& operator*() const
    {
        return *event_;
    }
    operator BASE_NS::shared_ptr<IEvent>() const
    {
        return event_;
    }

private:
    BASE_NS::shared_ptr<IEvent> event_;
};
template<typename EventT>
class Event : public Event<IEvent> {
public:
    using Event<IEvent>::Event;
    Event(const BASE_NS::shared_ptr<EventT>& ev) : Event<IEvent>(interface_pointer_cast<IEvent>(ev)) {}
};

META_END_NAMESPACE()

#endif
