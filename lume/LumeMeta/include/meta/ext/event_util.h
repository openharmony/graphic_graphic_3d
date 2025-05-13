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
#ifndef META_EXT_EVENT_UTIL_H
#define META_EXT_EVENT_UTIL_H

#include <meta/interface/intf_metadata.h>

META_BEGIN_NAMESPACE()

template<typename MyEvent, typename MyObj, typename... Args>
inline auto InvokeByName(const MyObj& obj, BASE_NS::string_view name, Args&&... args)
{
    if (auto i = interface_cast<IMetadata>(obj)) {
        if (auto ev = i->GetEvent(name, MetadataQuery::EXISTING)) {
            if (auto event = interface_cast<typename MyEvent::InterfaceType>(ev)) {
                return event->Invoke(BASE_NS::forward<Args>(args)...);
            }
            CORE_LOG_W("Trying to Invoke wrong type of event");
        }
    } else {
        CORE_LOG_W("Trying to Invoke event by name with type that doesn't support IMetadata");
    }
    if constexpr (!BASE_NS::is_same_v<typename MyEvent::ReturnType, void>) {
        return typename MyEvent::ReturnType {};
    }
}

META_END_NAMESPACE()
#endif
