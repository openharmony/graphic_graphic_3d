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

#ifndef META_API_PROPERTY_ARRAY_PROPERTY_EVENT_HANDLER_H
#define META_API_PROPERTY_ARRAY_PROPERTY_EVENT_HANDLER_H

#include <meta/api/property/array_property_changes_recognizer.h>
#include <meta/api/property/property_event_handler.h>
#include <meta/interface/property/array_property.h>
#include <meta/interface/simple_event.h>

META_BEGIN_NAMESPACE()

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
