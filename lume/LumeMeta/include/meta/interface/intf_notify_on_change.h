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

#ifndef META_INTERFACE_INOTIFY_ON_CHANGE_H
#define META_INTERFACE_INOTIFY_ON_CHANGE_H

#include <meta/base/interface_macros.h>
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

META_REGISTER_INTERFACE(INotifyOnChangeCallback, "b3a17e52-6d94-4f81-a2c0-9e4b8f7d1c53")

class INotifyOnChangeDirect;

/**
 * @brief Lightweight change callback interface. Implementors receive direct virtual
 *        call notifications without the overhead of the full event system.
 */
class INotifyOnChangeCallback : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, INotifyOnChangeCallback)
public:
    /** @brief Called by INotifyOnChangeDirect implementation to notify that the value has changed. */
    virtual void NotifyChanged(INotifyOnChangeDirect& sender) = 0;
};

META_REGISTER_INTERFACE(INotifyOnChangeDirect, "f8d92b3c-4a1e-49d8-b7c6-2e0f5a831d9e")

/**
 * @brief Lightweight change notification interface using direct callbacks instead of events.
 *        More efficient than INotifyOnChange when the full event system is not needed.
 */
class INotifyOnChangeDirect : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, INotifyOnChangeDirect)
public:
    /** @brief Register a callback for change notifications.
     *  The callback is stored as a weak reference — it will not prevent the callback from being destroyed. */
    virtual void AddChangeCallback(const INotifyOnChangeCallback::Ptr& callback) = 0;
    /** @brief Remove a previously registered callback. */
    virtual void RemoveChangeCallback(const INotifyOnChangeCallback::Ptr& callback) = 0;
};

META_END_NAMESPACE()

#endif
