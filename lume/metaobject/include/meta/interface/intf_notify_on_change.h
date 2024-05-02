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

#include <meta/base/types.h>
#include <meta/interface/event.h>

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
    virtual BASE_NS::shared_ptr<IEvent> EventOnChanged() const = 0;
    Event<IEvent> OnChanged() const
    {
        return EventOnChanged();
    }
};

META_END_NAMESPACE()

#endif
