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

#ifndef META_INTERFACE_IEVENT_H
#define META_INTERFACE_IEVENT_H

#include <base/containers/array_view.h>
#include <base/containers/string_view.h>
#include <base/util/uid.h>
#include <core/log.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/interface/intf_callable.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IEvent, "8b3c8322-54d6-4d1c-943c-207190576a4e")

class IEvent : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEvent)

public:
    /** handler identifier, at least big enough to keep an object pointer. */
    using Token = uint64_t;

    /** Returns the interface id that the callables must implement. */
    virtual BASE_NS::Uid GetCallableUid() const = 0;
    /** Returns the name of this event. Same as GetEventTypeName if not explicitly set. */
    virtual BASE_NS::string GetName() const = 0;
    /** Returns the event type name. */
    virtual BASE_NS::string GetEventTypeName() const = 0;

    /**
     * @brief Add handler which is invoked when event is triggered
     * @param userToken Optional token that is used to identify the handler.
     *                  Notice that this should be unique or it can collide with other handler tokens.
     * @return Default constructed Token if "invalid bindable", token already in use or other error.
     */
    virtual Token AddHandler(const ICallable::Ptr&, Token userToken) = 0;
    Token AddHandler(const ICallable::Ptr& p)
    {
        return AddHandler(p, {});
    }

    /**
     * @brief Remove handler with given token (Token returned by AddHandler previously).
     *        if handler added multiple times, see if re-enable it (so you don't have to call EnableHandler separately)
     */
    virtual bool RemoveHandler(Token, bool reenable) = 0;
    virtual bool RemoveHandler(Token t)
    {
        return RemoveHandler(t, false);
    }

    /**
     * @brief Enabled/Disable handler without removing it
     */
    virtual void EnableHandler(Token, bool enabled) = 0;

    /**
     * @brief Remove all handlers
     */
    virtual void Reset() = 0;

    /**
     * @brief Check if callable can be added as handler.
     */
    virtual bool IsCompatibleWith(const ICallable::Ptr&) const = 0;

    /**
     * @brief Get list of current handlers.
     */
    virtual BASE_NS::vector<ICallable::ConstPtr> GetHandlers() const = 0;

    /**
     * @brief Returns true if the event has any handlers, false otherwise.
     */
    virtual bool HasHandlers() const = 0;
};

template<typename EventType, typename Ret, typename... Args>
class IEventCallable : public ICallable {
    META_INTERFACE(ICallable, IEventCallable, EventType::UID)
public:
    using ReturnType = Ret;
    using InterfaceType = IEventCallable<EventType, Ret, Args...>;
    using InterfaceTypePtr = typename InterfaceType::Ptr;
    using FunctionType = Ret(Args...);
    virtual Ret Invoke(Args...) = 0;
};

template<typename MyEvent, typename... Args>
inline auto Invoke(const IEvent::Ptr& event, Args&&... args)
{
    // interpret null as the event not being constructed as not needed
    if (event) {
        if (auto i = interface_cast<typename MyEvent::InterfaceType>(event)) {
            return i->Invoke(BASE_NS::forward<Args>(args)...);
        }
        CORE_LOG_W("Trying to Invoke wrong type of event");
    }
    if constexpr (!BASE_NS::is_same_v<typename MyEvent::ReturnType, void>) {
        return typename MyEvent::ReturnType {};
    }
}

class ScopedDisableEventHandler {
    META_NO_COPY_MOVE(ScopedDisableEventHandler)
public:
    ScopedDisableEventHandler(BASE_NS::shared_ptr<IEvent> event, IEvent::Token token)
        : event_(BASE_NS::move(event)), token_(token)
    {
        if (event_ && token_) {
            event_->EnableHandler(token_, false);
        }
    }
    ~ScopedDisableEventHandler()
    {
        if (event_ && token_) {
            event_->EnableHandler(token_, true);
        }
    }

private:
    BASE_NS::shared_ptr<IEvent> event_ {};
    IEvent::Token token_ {};
};

META_END_NAMESPACE()

#endif
