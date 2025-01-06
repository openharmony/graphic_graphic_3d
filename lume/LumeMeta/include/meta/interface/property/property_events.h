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

#ifndef META_INTERFACE_PROPERTY_PROPERTY_EVENTS_H
#define META_INTERFACE_PROPERTY_PROPERTY_EVENTS_H

#include <meta/interface/simple_event.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The IOnTypedChanged interface defines a user-bindable interface for reacting to property value changes with
 * parameter.
 */
struct IOnTypedChangedInfo {
    constexpr static BASE_NS::Uid UID { "4c6fc26c-1d8a-4319-b95f-e2219560c1c5" };
    constexpr static char const* NAME { "OnChanged" };
};
using IOnChanged = SimpleEvent<IOnTypedChangedInfo, void()>;

META_END_NAMESPACE()

#endif
