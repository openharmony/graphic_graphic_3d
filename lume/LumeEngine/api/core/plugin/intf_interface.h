/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef API_CORE_PLUGIN_IINTERFACE_H
#define API_CORE_PLUGIN_IINTERFACE_H

#include <base/containers/refcnt_ptr.h>
#include <base/containers/shared_ptr.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
/** Base type for named interfaces provided by plugin.
 */
class IInterface {
public:
    static constexpr BASE_NS::Uid UID { "00000000-0000-0000-0000-000000000000" };

    using Ptr = BASE_NS::refcnt_ptr<IInterface>;

    /** Access to provided sub-interfaces
     */
    virtual const IInterface* GetInterface(const BASE_NS::Uid& uid) const = 0;
    virtual IInterface* GetInterface(const BASE_NS::Uid& uid) = 0;

    template<typename InterfaceType>
    const InterfaceType* GetInterface() const
    {
        return static_cast<const InterfaceType*>(GetInterface(InterfaceType::UID));
    }

    template<typename InterfaceType>
    InterfaceType* GetInterface()
    {
        return static_cast<InterfaceType*>(GetInterface(InterfaceType::UID));
    }

    /** Take a new reference of the object.
     */
    virtual void Ref() = 0;

    /* Releases one reference of the object.
     * No methods of the class shall be called after unref.
     * The object could be destroyed, if last reference
     */
    virtual void Unref() = 0;

protected:
    IInterface() = default;
    virtual ~IInterface() = default;
};
CORE_END_NAMESPACE()

// NOLINTBEGIN(readability-identifier-naming) to keep std like syntax
template<class U, class T>
BASE_NS::shared_ptr<U> interface_pointer_cast(const BASE_NS::shared_ptr<T>& ptr)
{
    static_assert(BASE_NS::is_base_of_v<CORE_NS::IInterface, T>, "T is not an IInterface");
    static_assert(BASE_NS::is_base_of_v<CORE_NS::IInterface, U>, "U is not an IInterface");
    if (ptr) {
        if constexpr (BASE_NS::is_same_v<U, T>) {
            // same type.
            return ptr;
        } else {
            return BASE_NS::shared_ptr<U>(ptr, static_cast<U*>(static_cast<void*>(ptr->GetInterface(U::UID))));
        }
    }
    return {};
}

template<class U, class T>
BASE_NS::shared_ptr<const U> interface_pointer_cast(const BASE_NS::shared_ptr<const T>& ptr)
{
    static_assert(BASE_NS::is_base_of_v<CORE_NS::IInterface, T>, "T is not an IInterface");
    static_assert(BASE_NS::is_base_of_v<CORE_NS::IInterface, U>, "U is not an IInterface");
    if (ptr) {
        if constexpr (BASE_NS::is_same_v<U, T>) {
            // same type.
            return ptr;
        } else {
            return BASE_NS::shared_ptr<const U>(
                ptr, static_cast<const U*>(static_cast<const void*>(ptr->GetInterface(U::UID))));
        }
    }
    return {};
}

template<class U, class T>
BASE_NS::shared_ptr<U> interface_pointer_cast(const BASE_NS::weak_ptr<T>& weak)
{
    return interface_pointer_cast<U>(weak.lock());
}

template<class U, class T>
BASE_NS::shared_ptr<const U> interface_pointer_cast(const BASE_NS::weak_ptr<const T>& weak)
{
    return interface_pointer_cast<const U>(weak.lock());
}

template<class U, class T>
U* interface_cast(const BASE_NS::shared_ptr<T>& ptr)
{
    static_assert(BASE_NS::is_base_of_v<CORE_NS::IInterface, T>, "T is not an IInterface");
    static_assert(BASE_NS::is_base_of_v<CORE_NS::IInterface, U>, "U is not an IInterface");
    if (ptr) {
        if constexpr (BASE_NS::is_same_v<U, T>) {
            // same type.
            return ptr.get();
        } else {
            return static_cast<U*>(static_cast<void*>(ptr->GetInterface(U::UID)));
        }
    }
    return {};
}

template<class U, class T>
const U* interface_cast(const BASE_NS::shared_ptr<const T>& ptr)
{
    static_assert(BASE_NS::is_base_of_v<CORE_NS::IInterface, T>, "T is not an IInterface");
    static_assert(BASE_NS::is_base_of_v<CORE_NS::IInterface, U>, "U is not an IInterface");
    if (ptr) {
        if constexpr (BASE_NS::is_same_v<U, T>) {
            // same type.
            return ptr.get();
        } else {
            return static_cast<const U*>(static_cast<const void*>(ptr->GetInterface(U::UID)));
        }
    }
    return {};
}

template<class U>
U* interface_cast(CORE_NS::IInterface* ptr)
{
    static_assert(BASE_NS::is_base_of_v<CORE_NS::IInterface, U>, "U is not an IInterface");
    if (ptr) {
        return static_cast<U*>(static_cast<void*>(ptr->GetInterface(U::UID)));
    }
    return {};
}

template<class U>
const U* interface_cast(const CORE_NS::IInterface* ptr)
{
    static_assert(BASE_NS::is_base_of_v<CORE_NS::IInterface, U>, "U is not an IInterface");
    if (ptr) {
        return static_cast<const U*>(static_cast<const void*>(ptr->GetInterface(U::UID)));
    }
    return {};
}
// NOLINTEND(readability-identifier-naming) to keep std like syntax
#endif // API_CORE_PLUGIN_IINTERFACE_H
