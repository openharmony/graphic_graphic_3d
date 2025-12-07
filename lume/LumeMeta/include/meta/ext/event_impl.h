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
#ifndef META_EXT_EVENT_IMPL_H
#define META_EXT_EVENT_IMPL_H

#include <base/containers/type_traits.h>
#include <base/containers/vector.h>
#include <core/log.h>

#include <meta/api/function.h>
#include <meta/api/threading/mutex.h>
#include <meta/base/interface_traits.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_cloneable.h>
#include <meta/interface/intf_event.h>
#include <meta/interface/object_macros.h>
#include <meta/interface/property/property_events.h>

META_BEGIN_NAMESPACE()

template<typename EventType>
struct EventImplTraits {
    static bool IsCompatibleInterface(const ICallable::Ptr& c)
    {
        if (auto f = interface_pointer_cast<IFunction>(c)) {
            return IsFunctionCompatible<typename EventType::FunctionType>(f);
        }
        return c->GetInterface(EventType::UID) != nullptr || c->GetInterface(IOnChanged::UID) != nullptr;
    }
    template<typename... Args>
    static void Call(const ICallable::Ptr& p, Args&... args)
    {
        if (const auto f = interface_pointer_cast<IFunction>(p)) {
            if constexpr ((true && ... && HasUid_v<PlainType_t<Args>>)) {
                CallMetaFunction<void>(f, BASE_NS::forward<Args>(args)...);
            } else {
                CORE_LOG_E("Invalid function signature for meta function call");
            }
        } else {
            if (const auto i = interface_cast<typename EventType::InterfaceType>(p)) {
                i->Invoke(args...);
            } else {
                // Allow to always call IOnChanged that takes no parameters, this allows to add handlers for unknown
                // event types
                if (auto ai = interface_cast<IOnChanged::InterfaceType>(p)) {
                    ai->Invoke();
                } else {
                    CORE_LOG_E("Invalid callable type for event callback");
                }
            }
        }
    }
};

template<typename BaseClass, typename signature = typename BaseClass::FunctionType>
class EventImpl;

class GenericEvent {
public:
    using Token = typename IEvent::Token;

    GenericEvent(BASE_NS::string name) : name_(BASE_NS::move(name)) {}
    GenericEvent(const GenericEvent& e) : handlers_(e.handlers_), name_(e.name_) {}
    virtual ~GenericEvent()
    {
        // check that the invocation doesn't destroy its event impl
        CORE_ASSERT_MSG(threadId_ == CORE_NS::ThreadId {}, "Event not allowed to destroy itself when invoked");
    }

protected:
    void ResetEvent()
    {
        CORE_NS::UniqueLock lock { mutex_ };
        handlers_.clear();
    }
    Token AddEventHandler(const ICallable::Ptr& p, Token userToken)
    {
        Token newToken = userToken ? userToken : (uintptr_t)p.get();
        CORE_NS::UniqueLock lock { mutex_ };
        for (auto it = handlers_.begin(); it != handlers_.end(); ++it) {
            if (newToken == it->token) {
                // Already connected.
                ++it->count;
                return newToken;
            }
        }
        handlers_.push_back(HandlerData { newToken, p });
        return newToken;
    }
    void EnableEventHandler(Token p, bool enabled)
    {
        if (p == 0) {
            return;
        }
        CORE_NS::UniqueLock lock { mutex_ };
        for (auto it = handlers_.begin(); it != handlers_.end(); ++it) {
            if (p == it->token) {
                if (!enabled) {
                    ++it->disabled;
                } else if (it->disabled > 0) {
                    --it->disabled;
                }
                return;
            }
        }
    }
    bool RemoveEventHandler(Token p, bool reenable)
    {
        if (p == 0) {
            return true;
        }
        CORE_NS::UniqueLock lock { mutex_ };
        for (auto it = handlers_.begin(); it != handlers_.end(); ++it) {
            if (p == it->token) {
                if (--it->count == 0) {
                    handlers_.erase(it);
                } else if (reenable && it->disabled > 0) {
                    --it->disabled;
                }
                return true;
            }
        }
        CORE_LOG_F("%s: Tried to remove a non-existent handler", name_.c_str());
        return false;
    }
    BASE_NS::vector<ICallable::ConstPtr> GetEventHandlers() const
    {
        CORE_NS::UniqueLock lock { mutex_ };
        BASE_NS::vector<ICallable::ConstPtr> handlers;
        handlers.reserve(handlers_.size());
        for (auto&& p : handlers_) {
            handlers.emplace_back(p.ptr);
        }
        return handlers;
    }
    bool HasEventHandlers() const
    {
        CORE_NS::UniqueLock lock { mutex_ };
        return !handlers_.empty();
    }
    BASE_NS::string GetName(const char* defaultName) const
    {
        CORE_NS::UniqueLock lock { mutex_ };
        return name_.empty() ? BASE_NS::string(defaultName) : name_;
    }
    struct InvokeData {
        size_t currentCallCount {};
        BASE_NS::vector<ICallable::WeakPtr> handlers;
        bool resetThreadId { false };
    };
    bool BeginInvoke(InvokeData& d)
    {
        if (threadId_ != CORE_NS::ThreadId {} && threadId_ != CORE_NS::CurrentThreadId()) {
            return false;
        }
        d.resetThreadId = threadId_ == CORE_NS::ThreadId {};
        if (d.resetThreadId) {
            threadId_ = CORE_NS::CurrentThreadId();
        }

        d.currentCallCount = ++callCount_;
        // we collect handlers to separate list of weak_ptr
        if (!handlers_.empty()) {
            // because callable can remove other handlers or callables
            auto& h = d.handlers;
            h.reserve(handlers_.size());
            for (auto&& p : handlers_) {
                if (p.disabled == 0) {
                    h.emplace_back(p.ptr);
                }
            }
        }
        return true;
    }
    void EndInvoke(const InvokeData& d)
    {
        if (d.resetThreadId) {
            threadId_ = {};
        }
    }

    struct HandlerData {
        Token token;
        ICallable::Ptr ptr;
        std::uint16_t count { 1 };
        std::uint16_t disabled {};
    };
    BASE_NS::vector<HandlerData> handlers_;
    size_t callCount_ {};
    BASE_NS::string name_;
    mutable CORE_NS::Mutex mutex_;
    mutable CORE_NS::ThreadId threadId_;
};

template<typename BaseClass, typename R, typename... ARG>
class EventImpl<BaseClass, R(ARG...)> final : public IntroduceInterfaces<BaseClass, IObject, IEvent, ICloneable>,
                                              public GenericEvent {
    static_assert(BASE_NS::is_void_v<R>, "EventHandler callable must return void");

    using Super = IntroduceInterfaces<BaseClass, IObject, IEvent, ICloneable>;
    using Token = GenericEvent::Token;
    using Traits = EventImplTraits<BaseClass>;

    BASE_NS::shared_ptr<CORE_NS::IInterface> GetClone() const override
    {
        BASE_NS::shared_ptr<EventImpl> p(new EventImpl(*this));
        return interface_pointer_cast<CORE_NS::IInterface>(p);
    }

    EventImpl(const EventImpl& e) : GenericEvent(e) {}

public:
    explicit EventImpl(BASE_NS::string name = {}) : GenericEvent(BASE_NS::move(name)) {}
    ~EventImpl() override
    {
        Reset();
    }

    EventImpl& operator=(const EventImpl&) = delete;
    META_NO_MOVE(EventImpl)

    void Reset() override
    {
        GenericEvent::ResetEvent();
    }

    bool IsCompatibleWith(const ICallable::Ptr& p) const override
    {
        return Traits::IsCompatibleInterface(p);
    }

    using IEvent::AddHandler;
    Token AddHandler(const ICallable::Ptr& p, Token userToken) override
    {
        if (p && Traits::IsCompatibleInterface(p)) {
            return GenericEvent::AddEventHandler(p, userToken);
        }
        CORE_LOG_F("%s: Tried to add a non-matching handler", GenericEvent::name_.c_str());
        return 0;
    }

    void EnableHandler(Token p, bool enabled) override
    {
        GenericEvent::EnableEventHandler(p, enabled);
    }

    bool RemoveHandler(Token p, bool reenable) override
    {
        return GenericEvent::RemoveEventHandler(p, reenable);
    }

    BASE_NS::vector<ICallable::ConstPtr> GetHandlers() const override
    {
        return GenericEvent::GetEventHandlers();
    }

    bool HasHandlers() const override
    {
        return GenericEvent::HasEventHandlers();
    }

    BASE_NS::Uid GetCallableUid() const override
    {
        return BaseClass::UID;
    }

    BASE_NS::string GetName() const override
    {
        return GenericEvent::GetName(BaseClass::NAME);
    }
    ObjectId GetClassId() const override
    {
        return {};
    }
    BASE_NS::string_view GetClassName() const override
    {
        return "Event";
    }
    BASE_NS::vector<BASE_NS::Uid> GetInterfaces() const override
    {
        return Super::GetInterfacesVector();
    }

    BASE_NS::string GetEventTypeName() const override
    {
        return BaseClass::NAME;
    }

    void Invoke(ARG... args) override
    {
        // Make sure event stays alive while we are invoking (a handler may lead to the event's refcount decreasing,
        // this makes sure that it stays >0 until the end of the function)
        auto self = BASE_NS::refcnt_ptr<EventImpl>(this);
        // Take lock after refcnt_ptr to make sure that if we get destroyed at the end, the lock is released first
        CORE_NS::UniqueLock lock { GenericEvent::mutex_ };
        GenericEvent::InvokeData invoke;
        if (GenericEvent::BeginInvoke(invoke)) {
            auto& handlers = invoke.handlers;
            for (auto it = handlers.begin();
                 it != handlers.end() && invoke.currentCallCount == GenericEvent::callCount_; ++it) {
                if (auto callable = it->lock()) {
                    lock.Unlock();
                    Traits::Call(callable, args...);
                    lock.Lock();
                }
            }
            GenericEvent::EndInvoke(invoke);
        }
    }
};
META_END_NAMESPACE()
#endif
