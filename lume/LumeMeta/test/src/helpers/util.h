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

#ifndef META_TEST_UTIL_HEADER
#define META_TEST_UTIL_HEADER

#include <base/containers/vector.h>
#include <core/log.h>

#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/construct_property.h>

META_BEGIN_NAMESPACE()

template<typename T>
typename META_NS::Property<T> CreateProperty(BASE_NS::string_view name = "", T value = {})
{
    return ConstructProperty<T>(name, value);
}

META_END_NAMESPACE()

BASE_BEGIN_NAMESPACE()

// our vector does not have op==
template<typename T>
bool operator==(const BASE_NS::vector<T>& l, const BASE_NS::vector<T>& r)
{
    if (l.size() != r.size()) {
        return false;
    }
    auto it1 = l.begin();
    auto it2 = r.begin();
    for (; it1 != l.end(); ++it1, ++it2) {
        if constexpr (META_NS::HasEqualOperator_v<T>) {
            if (!(*it1 == *it2)) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

template<typename T>
bool operator!=(const BASE_NS::vector<T>& l, const BASE_NS::vector<T>& r)
{
    return !(l == r);
}

BASE_END_NAMESPACE()

#endif // META_TEST_UTIL_HEADER
