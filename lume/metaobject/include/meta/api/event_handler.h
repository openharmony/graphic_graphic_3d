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
#ifndef META_API_EVENT_HANDLER_H
#define META_API_EVENT_HANDLER_H

#include <meta/api/deferred_callback.h>
#include <meta/api/make_callback.h>
#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The EventHandler class is a helper class for adding event handlers to events.
 *
 *        EventHandler takes care of removing the event handler automatically
 *        when the target event changes or when the EventHandler object is destroyed.
 *
 *        Calling Subscribe when already subscribed will remove the old subscription.
 */
class EventHandler {
public:
    META_NO_COPY(EventHandler)
    EventHandler() noexcept = default;
    virtual ~EventHandler()
    {
        Unsubscribe();
    }
    EventHandler(EventHandler&& other) noexcept
        : event_ { BASE_NS::move(other.event_) }, token_ { BASE_NS::exchange(other.token_, {}) }
    {}
    EventHandler& operator=(EventHandler&& other) noexcept
    {
        if (&other != this) {
            Unsubscribe();
            event_ = BASE_NS::move(other.event_);
            token_ = BASE_NS::exchange(other.token_, {});
        }
        return *this;
    }

    template<typename EventType>
    EventHandler(const Event<EventType>& event, const typename EventType::InterfaceTypePtr& func,
        const ITaskQueue::Ptr& queue = nullptr)
    {
        Subscribe(event, func, queue);
    }

    template<typename EventType>
    EventHandler(
        const Event<EventType>& event, const typename EventType::InterfaceTypePtr& func, const BASE_NS::Uid& queueId)
    {
        Subscribe(event, func, queueId);
    }

    EventHandler(const BASE_NS::shared_ptr<IEvent>& event, const ICallable::Ptr& func)
    {
        TypelessSubscribe(event, func);
    }

    template<typename EventType, typename Func,
        typename = EnableIfCanInvokeWithArguments<Func, typename EventType::FunctionType>>
    EventHandler(const Event<EventType>& event, Func func, const ITaskQueue::Ptr& queue = nullptr)
    {
        Subscribe(event, BASE_NS::move(func), queue);
    }

    template<typename EventType, typename Func,
        typename = EnableIfCanInvokeWithArguments<Func, typename EventType::FunctionType>>
    EventHandler(
        const Event<EventType>& event, const typename EventType::InterfaceTypePtr& func, const BASE_NS::Uid& queueId)
    {
        Subscribe(event, BASE_NS::move(func), queueId);
    }

    /**
     * @brief Check if this handler has subscribed to an event
     */
    bool IsValid() const
    {
        return !event_.expired();
    }

    /**
     * @brief Subscribe to an event.
     * @param event Event to subscribe to.
     * @param func Callable to invoke when event is triggered.
     * @return True if the successfully subscribed.
     */
    bool TypelessSubscribe(const IEvent::Ptr& event, const ICallable::Ptr& func)
    {
        if (!event) {
            CORE_LOG_E("Cannot subscribe to null event");
            return false;
        }
        Unsubscribe();
        token_ = event->AddHandler(func);
        if (token_) {
            event_ = event;
        }
        return token_;
    }

    /**
     * @brief Subscribe to an event.
     * @param event Event to subscribe to.
     * @param func Callable to invoke when event is triggered.
     * @oaram queue Optional queue where 'func' is invoked, if null, invoked directly.
     * @return True if the successfully subscribed.
     */
    template<typename EventType>
    bool Subscribe(const Event<EventType>& event, const typename EventType::InterfaceTypePtr& func,
        const ITaskQueue::Ptr& queue = nullptr)
    {
        return queue ? TypelessSubscribe(event,
                           MakeDeferred<typename EventType::InterfaceType>(
                               [func](auto&&... args) { func->Invoke(BASE_NS::forward<decltype(args)>(args)...); },
                               queue))
                     : TypelessSubscribe(event, func);
    }

    /**
     * @brief Subscribe to an event.
     * @param event Event to subscribe to.
     * @param func Callable to invoke when event is triggered.
     * @param queueId Queue id where 'func' is invoked. Fails if no such queue exists.
     * @return True if the successfully subscribed.
     */
    template<typename EventType>
    bool Subscribe(
        const Event<EventType>& event, const typename EventType::InterfaceTypePtr& func, const BASE_NS::Uid& queueId)
    {
        auto queue = GetTaskQueueRegistry().GetTaskQueue(queueId);
        if (!queue) {
            CORE_LOG_E("Cannot initialize event handler for queue %s: Queue not registered.",
                BASE_NS::to_string(queueId).c_str());
        }
        return queue && Subscribe(event, func, queue);
    }

    /**
     * @brief Subscribe to an event. See above Subscribe function for parameters
     */
    template<typename EventType, typename Func,
        typename = EnableIfCanInvokeWithArguments<Func, typename EventType::FunctionType>>
    bool Subscribe(const Event<EventType>& event, Func func, const ITaskQueue::Ptr& queue = nullptr)
    {
        return Subscribe(event, MakeCallback<EventType>(BASE_NS::move(func)), queue);
    }

    /**
     * @brief Subscribe to an event. See above Subscribe function for parameters
     */
    template<typename EventType, typename Func,
        typename = EnableIfCanInvokeWithArguments<Func, typename EventType::FunctionType>>
    bool Subscribe(const Event<EventType>& event, Func func, const BASE_NS::Uid& queueId)
    {
        return Subscribe(event, MakeCallback<EventType>(BASE_NS::move(func)), queueId);
    }

    /**
     * @brief Subscribe to an event. See above Subscribe function for parameters
     */
    template<typename EventType, typename Func,
        typename = EnableIfCanInvokeWithArguments<Func, typename EventType::FunctionType>>
    bool Subscribe(const BASE_NS::shared_ptr<IEvent>& event, Func func, const ITaskQueue::Ptr& queue = nullptr)
    {
        auto ev = interface_pointer_cast<EventType>(event);
        return ev && Subscribe<EventType>(event, MakeCallback<EventType>(BASE_NS::move(func)), queue);
    }

    /**
     * @brief Subscribe to an event. See above Subscribe function for parameters
     */
    template<typename EventType, typename Func,
        typename = EnableIfCanInvokeWithArguments<Func, typename EventType::FunctionType>>
    bool Subscribe(const BASE_NS::shared_ptr<IEvent>& event, Func func, const BASE_NS::Uid& queueId)
    {
        auto ev = interface_pointer_cast<EventType>(event);
        return ev && Subscribe<EventType>(event, MakeCallback<EventType>(BASE_NS::move(func)), queueId);
    }

    /**
     * @brief Unsubscribe from the event.
     */
    void Unsubscribe()
    {
        if (auto e = event_.lock()) {
            e->RemoveHandler(token_);
            event_ = nullptr;
            token_ = {};
        }
    }

protected:
    IEvent::WeakPtr event_;
    IEvent::Token token_ {};
};

META_END_NAMESPACE()

#endif
