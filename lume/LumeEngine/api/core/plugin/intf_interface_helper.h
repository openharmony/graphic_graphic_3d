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

#ifndef API_CORE_PLUGIN_IINTERFACE_HELPER_H
#define API_CORE_PLUGIN_IINTERFACE_HELPER_H

#include <atomic>

#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>

CORE_BEGIN_NAMESPACE()
namespace Internal {
template<typename Me, typename Interface, typename... Interfaces>
const CORE_NS::IInterface* GetInterface(const Me* me, const BASE_NS::Uid& uid)
{
    if (uid == Interface::UID) {
        return static_cast<const Interface*>(me);
    }
    if constexpr (sizeof...(Interfaces) > 0) {
        return GetInterface<Me, Interfaces...>(me, uid);
    } else {
        return nullptr;
    }
}
} // namespace Internal

template<typename... Interfaces>
class IInterfaceHelper : public Interfaces... {
public:
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override
    {
        if (uid == CORE_NS::IInterface::UID) {
            return static_cast<const CORE_NS::IInterface*>(static_cast<const void*>(this));
        }
        return Internal::GetInterface<IInterfaceHelper, Interfaces...>(this, uid);
    }
    IInterface* GetInterface(const BASE_NS::Uid& uid) override
    {
        if (uid == CORE_NS::IInterface::UID) {
            return static_cast<CORE_NS::IInterface*>(static_cast<void*>(this));
        }
        return const_cast<CORE_NS::IInterface*>(Internal::GetInterface<IInterfaceHelper, Interfaces...>(this, uid));
    }

    void Ref() override
    {
        refcnt_.fetch_add(1, std::memory_order_relaxed);
    }

    void Unref() override
    {
        if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete this;
        }
    }

protected:
    std::atomic<int32_t> refcnt_ { 0 };
};
CORE_END_NAMESPACE()

#endif // API_CORE_PLUGIN_IINTERFACE_HELPER_H