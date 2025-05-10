/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: Array Property Change Event Handler
 * Author: Slawomir Grabowski
 * Create: 2024-02-23
 */

#ifndef META_API_PROPERTY_ARRAY_PROPERTY_EVENT_HANDLER_H
#define META_API_PROPERTY_ARRAY_PROPERTY_EVENT_HANDLER_H

#include <meta/api/property/array_property_changes_recognizer.h>
#include <meta/api/property/property_event_handler.h>
#include <meta/interface/property/array_property.h>
#include <meta/interface/simple_event.h>

META_BEGIN_NAMESPACE()

/// Helper to have events with array property element changes
template<typename ValueType>
class ArrayPropertyChangedEventHandler {
public:
    ArrayPropertyChangedEventHandler() = default;

    using Property = ArrayProperty<ValueType>;
    using ArrayChange = ArrayChanges<ValueType>;
    using FunctionType = void(const ArrayChange&);

    template<typename T>
    bool Subscribe(const Property& property, T* instance, void (T::*callback)(const ArrayChange&))
    {
        changesRecognizer_.SetValue(property);
        return handler_.Subscribe(
            property, [=]() { (instance->*callback)(changesRecognizer_.OnArrayPropertyChanged(property)); });
    }

    bool Subscribe(const Property& property, void (*callback)(const ArrayChange&))
    {
        changesRecognizer_.SetValue(property);
        return handler_.Subscribe(property, [=]() { callback(changesRecognizer_.OnArrayPropertyChanged(property)); });
    }

    template<typename Func, typename = EnableIfCanInvokeWithArguments<Func, FunctionType>>
    bool Subscribe(const Property& property, Func func, const ITaskQueue::Ptr& queue = nullptr)
    {
        changesRecognizer_.SetValue(property);
        return handler_.Subscribe(
            property,
            [this, property, f = BASE_NS::move(func)]() { f(changesRecognizer_.OnArrayPropertyChanged(property)); },
            queue);
    }

    template<typename Func, typename = EnableIfCanInvokeWithArguments<Func, FunctionType>>
    bool Subscribe(const Property& property, Func func, const BASE_NS::Uid& queueId)
    {
        changesRecognizer_.SetValue(property);
        return handler_.Subscribe(
            property,
            [this, property, f = BASE_NS::move(func)]() { f(changesRecognizer_.OnArrayPropertyChanged(property)); },
            queueId);
    }

    void Unsubscribe()
    {
        handler_.Unsubscribe();
        changesRecognizer_.Clear();
    }

private:
    ArrayPropertyChangesRecognizer<ValueType> changesRecognizer_;
    META_NS::PropertyChangedEventHandler handler_;
};

META_END_NAMESPACE()

#endif
