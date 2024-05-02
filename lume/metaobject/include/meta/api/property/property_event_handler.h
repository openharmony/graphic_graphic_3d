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
#ifndef META_API_PROPERTY_EVENT_HANDLER_H
#define META_API_PROPERTY_EVENT_HANDLER_H

#include <meta/api/event_handler.h>
#include <meta/base/interface_macros.h>
#include <meta/interface/property/property_events.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The PropertyEventHandler class is a helper class for adding event handlers to property events.
 */

template<typename EventType, BASE_NS::shared_ptr<IEvent> (INotifyOnChange::*EventFunc)() const>
class PropertyEventHandler {
public:
    using CallableTypePtr = typename EventType::InterfaceTypePtr;

    META_NO_COPY(PropertyEventHandler)
    META_DEFAULT_MOVE(PropertyEventHandler)
    PropertyEventHandler() noexcept = default;
    virtual ~PropertyEventHandler() = default;

    bool Subscribe(const IProperty::ConstPtr& property, const CallableTypePtr& onInvoked)
    {
        // This basic from is used with array properties as well, so use the typeless subscribe here
        return handler_.TypelessSubscribe((property.get()->*EventFunc)(), onInvoked);
    }

    bool Subscribe(const IProperty::ConstPtr& property, const CallableTypePtr& onInvoked, const ITaskQueue::Ptr& queue)
    {
        return handler_.Subscribe<EventType>((property.get()->*EventFunc)(), onInvoked, queue);
    }

    bool Subscribe(const IProperty::ConstPtr& property, const CallableTypePtr& onInvoked, const BASE_NS::Uid& queueId)
    {
        return handler_.Subscribe<EventType>((property.get()->*EventFunc)(), onInvoked, queueId);
    }

    template<typename Func, typename = EnableIfCanInvokeWithArguments<Func, typename EventType::FunctionType>>
    bool Subscribe(const IProperty::ConstPtr& property, Func func, const ITaskQueue::Ptr& queue = nullptr)
    {
        return handler_.Subscribe<EventType>((property.get()->*EventFunc)(), BASE_NS::move(func), queue);
    }

    template<typename Func, typename = EnableIfCanInvokeWithArguments<Func, typename EventType::FunctionType>>
    bool Subscribe(const IProperty::ConstPtr& property, Func func, const BASE_NS::Uid& queueId)
    {
        return handler_.Subscribe<EventType>((property.get()->*EventFunc)(), BASE_NS::move(func), queueId);
    }

    void Unsubscribe()
    {
        handler_.Unsubscribe();
    }

private:
    EventHandler handler_;
};

/**
 * @brief EventHandler for handling IProperty::OnChanged event.
 */
class PropertyChangedEventHandler final : public PropertyEventHandler<IOnChanged, &IProperty::EventOnChanged> {
public:
    PropertyChangedEventHandler() = default;
    PropertyChangedEventHandler(const IProperty::ConstPtr& property, const IOnChanged::InterfaceTypePtr& onInvoked)
    {
        Subscribe(property, onInvoked);
    }
};

META_END_NAMESPACE()

#endif
