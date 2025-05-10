/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Helper for property event handler management.
 * Author: Lauri Jaaskela
 * Create: 2022-05-16
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
template<typename EventType, BASE_NS::shared_ptr<IEvent> (INotifyOnChange::*EventFunc)(MetadataQuery) const>
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
        return handler_.TypelessSubscribe((property.get()->*EventFunc)(MetadataQuery::CONSTRUCT_ON_REQUEST), onInvoked);
    }

    bool Subscribe(const IProperty::ConstPtr& property, const CallableTypePtr& onInvoked, const ITaskQueue::Ptr& queue)
    {
        return handler_.Subscribe<EventType>(
            (property.get()->*EventFunc)(MetadataQuery::CONSTRUCT_ON_REQUEST), onInvoked, queue);
    }

    bool Subscribe(const IProperty::ConstPtr& property, const CallableTypePtr& onInvoked, const BASE_NS::Uid& queueId)
    {
        return handler_.Subscribe<EventType>(
            (property.get()->*EventFunc)(MetadataQuery::CONSTRUCT_ON_REQUEST), onInvoked, queueId);
    }

    template<typename Func, typename = EnableIfCanInvokeWithArguments<Func, typename EventType::FunctionType>>
    bool Subscribe(const IProperty::ConstPtr& property, Func func, const ITaskQueue::Ptr& queue = nullptr)
    {
        return handler_.Subscribe<EventType>(
            (property.get()->*EventFunc)(MetadataQuery::CONSTRUCT_ON_REQUEST), BASE_NS::move(func), queue);
    }

    template<typename Func, typename = EnableIfCanInvokeWithArguments<Func, typename EventType::FunctionType>>
    bool Subscribe(const IProperty::ConstPtr& property, Func func, const BASE_NS::Uid& queueId)
    {
        return handler_.Subscribe<EventType>(
            (property.get()->*EventFunc)(MetadataQuery::CONSTRUCT_ON_REQUEST), BASE_NS::move(func), queueId);
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
