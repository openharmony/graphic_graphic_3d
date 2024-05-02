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
#ifndef META_INTERFACE_DETAIL_ANY_POINTER_COMPATIBILITY_H
#define META_INTERFACE_DETAIL_ANY_POINTER_COMPATIBILITY_H

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_traits.h>
#include <meta/base/shared_ptr.h>
#include <meta/interface/intf_any.h>

META_BEGIN_NAMESPACE()

// NOLINTBEGIN(readability-identifier-naming)
using SharedPtrIInterface = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using SharedPtrConstIInterface = BASE_NS::shared_ptr<const CORE_NS::IInterface>;
constexpr TypeId SharedPtrIInterfaceId = UidFromType<SharedPtrIInterface>();
constexpr TypeId SharedPtrConstIInterfaceId = UidFromType<SharedPtrConstIInterface>();
using WeakPtrIInterface = BASE_NS::weak_ptr<CORE_NS::IInterface>;
using WeakPtrConstIInterface = BASE_NS::weak_ptr<const CORE_NS::IInterface>;
constexpr TypeId WeakPtrIInterfaceId = UidFromType<WeakPtrIInterface>();
constexpr TypeId WeakPtrConstIInterfaceId = UidFromType<WeakPtrConstIInterface>();
// NOLINTEND(readability-identifier-naming)

template<bool IsConst, bool IsWeak>
struct AnyPointerCompatibility {
    using IIType = BASE_NS::conditional_t<IsConst, const CORE_NS::IInterface, CORE_NS::IInterface>;
    using IIPtrType = BASE_NS::conditional_t<IsConst, SharedPtrConstIInterface, SharedPtrIInterface>;

    template<typename Type>
    static BASE_NS::array_view<const TypeId> GetCompatibleTypes(CompatibilityDirection dir)
    {
        static constexpr TypeId typeId = UidFromType<Type>();
        if (dir == CompatibilityDirection::GET) {
            if constexpr (IsConst) {
                static TypeId uids[] = { typeId, SharedPtrConstIInterfaceId };
                return uids;
            }
            static TypeId uids[] = { typeId, SharedPtrIInterfaceId, SharedPtrConstIInterfaceId };
            return uids;
        }
        if (dir == CompatibilityDirection::SET) {
            if constexpr (IsConst) {
                static TypeId uids[] = { typeId, SharedPtrIInterfaceId, SharedPtrConstIInterfaceId };
                return uids;
            }
            static TypeId uids[] = { typeId, SharedPtrIInterfaceId };
            return uids;
        }
        if constexpr (IsConst) {
            static TypeId uids[] = { typeId, SharedPtrConstIInterfaceId };
            return uids;
        }

        static TypeId uids[] = { typeId, SharedPtrIInterfaceId };
        return uids;
    }

    static AnyReturnValue GetData(const TypeId& id, void* data, const IIPtrType& ptr)
    {
        if constexpr (!IsConst) {
            if (id == SharedPtrIInterfaceId) {
                *static_cast<SharedPtrIInterface*>(data) = ptr;
                return AnyReturn::SUCCESS;
            }
        }
        if (id == SharedPtrConstIInterfaceId) {
            *static_cast<SharedPtrConstIInterface*>(data) = ptr;
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }

    static bool SetData(const TypeId& id, const void* data, IIPtrType& out)
    {
        if constexpr (IsConst) {
            if (id == SharedPtrConstIInterfaceId) {
                out = *static_cast<const SharedPtrConstIInterface*>(data);
                return true;
            }
        }
        if (id == SharedPtrIInterfaceId) {
            out = *static_cast<const SharedPtrIInterface*>(data);
            return true;
        }
        return false;
    }
    static constexpr bool IsValidGetArgs(const TypeId& uid, const void* data, size_t size)
    {
        if constexpr (!IsConst) {
            if (data && uid == SharedPtrIInterfaceId && sizeof(SharedPtrIInterface) == size) {
                return true;
            }
        }
        if (data && uid == SharedPtrConstIInterfaceId && sizeof(SharedPtrConstIInterfaceId) == size) {
            return true;
        }
        return false;
    }
    static constexpr bool IsValidSetArgs(const TypeId& uid, const void* data, size_t size)
    {
        if constexpr (IsConst) {
            if (data && uid == SharedPtrConstIInterfaceId && sizeof(SharedPtrConstIInterface) == size) {
                return true;
            }
        }
        if (data && uid == SharedPtrIInterfaceId && sizeof(SharedPtrIInterfaceId) == size) {
            return true;
        }
        return false;
    }
};

template<bool IsConst>
struct AnyPointerCompatibility<IsConst, true> {
    using IIType = BASE_NS::conditional_t<IsConst, const CORE_NS::IInterface, CORE_NS::IInterface>;
    using IIPtrType = BASE_NS::conditional_t<IsConst, SharedPtrConstIInterface, SharedPtrIInterface>;

    template<typename Type>
    static BASE_NS::array_view<const TypeId> GetCompatibleTypes(CompatibilityDirection dir)
    {
        static constexpr TypeId typeId = UidFromType<Type>();
        if (dir == CompatibilityDirection::GET) {
            if constexpr (IsConst) {
                static TypeId uids[] = { typeId, SharedPtrConstIInterfaceId, WeakPtrConstIInterfaceId };
                return uids;
            }
            static TypeId uids[] = { typeId, SharedPtrIInterfaceId, SharedPtrConstIInterfaceId, WeakPtrIInterfaceId,
                WeakPtrConstIInterfaceId };
            return uids;
        }
        if (dir == CompatibilityDirection::SET) {
            if constexpr (IsConst) {
                static TypeId uids[] = { typeId, SharedPtrIInterfaceId, SharedPtrConstIInterfaceId, WeakPtrIInterfaceId,
                    WeakPtrConstIInterfaceId };
                return uids;
            }
            static TypeId uids[] = { typeId, SharedPtrIInterfaceId, WeakPtrIInterfaceId };
            return uids;
        }
        if constexpr (IsConst) {
            static TypeId uids[] = { typeId, SharedPtrConstIInterfaceId, WeakPtrConstIInterfaceId };
            return uids;
        }

        static TypeId uids[] = { typeId, SharedPtrIInterfaceId, WeakPtrIInterfaceId };
        return uids;
    }

    static AnyReturnValue GetData(const TypeId& id, void* data, const IIPtrType& ptr)
    {
        if (AnyPointerCompatibility<IsConst, false>::GetData(id, data, ptr)) {
            return AnyReturn::SUCCESS;
        }
        if constexpr (!IsConst) {
            if (id == WeakPtrIInterfaceId) {
                *static_cast<WeakPtrIInterface*>(data) = ptr;
                return AnyReturn::SUCCESS;
            }
        }
        if (id == WeakPtrConstIInterfaceId) {
            *static_cast<WeakPtrConstIInterface*>(data) = ptr;
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }

    static bool SetData(const TypeId& id, const void* data, IIPtrType& out)
    {
        if (AnyPointerCompatibility<IsConst, false>::SetData(id, data, out)) {
            return true;
        }
        if constexpr (IsConst) {
            if (id == WeakPtrConstIInterfaceId) {
                out = static_cast<const WeakPtrConstIInterface*>(data)->lock();
                return true;
            }
        }
        if (id == WeakPtrIInterfaceId) {
            out = static_cast<const WeakPtrIInterface*>(data)->lock();
            return true;
        }
        return false;
    }
    static constexpr bool IsValidGetArgs(const TypeId& uid, const void* data, size_t size)
    {
        if (AnyPointerCompatibility<IsConst, false>::IsValidGetArgs(uid, data, size)) {
            return true;
        }
        if constexpr (!IsConst) {
            if (data && uid == WeakPtrIInterfaceId && sizeof(WeakPtrIInterface) == size) {
                return true;
            }
        }
        if (data && uid == WeakPtrConstIInterfaceId && sizeof(WeakPtrConstIInterface) == size) {
            return true;
        }
        return false;
    }
    static constexpr bool IsValidSetArgs(const TypeId& uid, const void* data, size_t size)
    {
        if (AnyPointerCompatibility<IsConst, false>::IsValidSetArgs(uid, data, size)) {
            return true;
        }
        if constexpr (IsConst) {
            if (data && uid == WeakPtrConstIInterfaceId && sizeof(WeakPtrConstIInterface) == size) {
                return true;
            }
        }
        if (data && uid == WeakPtrIInterfaceId && sizeof(WeakPtrIInterface) == size) {
            return true;
        }
        return false;
    }
};

template<typename Type>
using AnyPC = AnyPointerCompatibility<IsConstPtr_v<Type>, IsWeakPtr_v<Type>>;

META_END_NAMESPACE()

#endif
