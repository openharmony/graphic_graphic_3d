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

#ifndef META_BASE_INTERFACE_UTILS_H
#define META_BASE_INTERFACE_UTILS_H

#include <base/containers/vector.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/ids.h>
#include <meta/base/shared_ptr.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Checks if object implements any of the given interfaces.
 * @note The function returns false for a null object, regardless of uids.
 * @note If the object is valid but the uid list is empty, the function returns true.
 * @param object The object to check.
 * @param uids The uids to check against.
 * @return True if the object implements any of the uids.
 */
static inline bool ObjectImplementsAny(
    const BASE_NS::shared_ptr<const CORE_NS::IInterface>& object, const BASE_NS::array_view<const TypeId>& ids) noexcept
{
    if (object) {
        for (const auto& id : ids) {
            if (object->GetInterface(id.ToUid())) {
                return true;
            }
        }
        return ids.empty();
    }
    return false;
}

/**
 * @brief Checks if object implements all of the given interfaces.
 * @note The function returns false for a null object, regardless of uids.
 * @note If the object is valid but the uid list is empty, the function returns true.
 * @param object The object to check.
 * @param uids The uids to check against.
 * @return True if the object implements all of the uids.
 */
static inline bool ObjectImplementsAll(
    const BASE_NS::shared_ptr<const CORE_NS::IInterface>& object, const BASE_NS::array_view<const TypeId>& ids) noexcept
{
    if (object) {
        for (const auto& id : ids) {
            if (!object->GetInterface(id.ToUid())) {
                return false;
            }
        }
        return true;
    }
    return false;
}

/**
 * @brief Checks whether a given object implements one/all of a given set of interface UIDs.
 * @note The function returns false for a null object, regardless of uids.
 * @note If the object is valid but the uid list is empty, the function returns true.
 * @param object The object to check.
 * @param uids A list of Uids to check against.
 * @param strict If true, the object must implement all listed Uids. If false, it is enough to implement one.
 * @return True if object implements given requirements, false otherwise.
 */
static inline bool CheckInterfaces(const BASE_NS::shared_ptr<const CORE_NS::IInterface>& object,
    const BASE_NS::array_view<const TypeId>& ids, bool strict) noexcept
{
    return strict ? ObjectImplementsAll(object, ids) : ObjectImplementsAny(object, ids);
}

/**
 * @brief Checks if a list of uids matches a given set of requirements.
 * @param uids The uid list to check against.
 * @param reqs A uid list which must be found from <param>uids</param>.
 * @param strict If true, all of reqs must be found from uids. If false, one match is enough.
 * @return True if uids fulfills reqs, false otherwise.
 */
static bool CheckInterfaces(
    const BASE_NS::vector<BASE_NS::Uid>& uids, const BASE_NS::vector<BASE_NS::Uid>& reqs, bool strict) noexcept
{
    if (uids.empty() && !reqs.empty()) {
        return false;
    }
    if (reqs.empty()) {
        return true;
    }
    size_t matches = strict ? reqs.size() : 1;
    for (auto&& uid : uids) {
        for (auto&& req : reqs) {
            if (req == uid) {
                if (--matches == 0) {
                    return true;
                }
                break;
            }
        }
    }
    return false;
}

/**
 * @brief Returns a vector containing all of the source vector objects which implement To::UID casted
 *        to To::Ptr.
 * @note The result can contain less items than the input if all items of input do not implement To::UID.
 *       The result will never contain more items than input.
 * @param from The vector to convert from.
 */
template<class To, class From>
BASE_NS::vector<typename To::Ptr> PtrArrayCast(BASE_NS::vector<From>&& from)
{
    static_assert(IsKindOfPointer_v<From>, "Conversion source array must contain shared_ptrs.");
    static_assert(IsKindOfInterface_v<To>, "Conversion target type must be an IInterface.");
    using ToPtr = typename To::Ptr;
    if constexpr (BASE_NS::is_same_v<ToPtr, From>) {
        return BASE_NS::move(from);
    }
    BASE_NS::vector<ToPtr> converted;
    converted.reserve(from.size());
    for (auto&& obj : from) {
        if (auto t = interface_pointer_cast<To>(obj)) {
            converted.emplace_back(BASE_NS::move(t));
        }
    }
    return converted;
}

META_END_NAMESPACE()

#endif // META_BASE_INTERFACE_UTILS_H
