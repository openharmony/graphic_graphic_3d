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

#ifndef CORE3D__LOADER__JSON_UTIL_H
#define CORE3D__LOADER__JSON_UTIL_H

#include <algorithm>
#include <cstdlib>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/math/mathf.h>
#include <core/json/json.h>
#include <core/log.h>
#include <core/namespace.h>

#include "util/string_util.h"

CORE3D_BEGIN_NAMESPACE()
template<class T, BASE_NS::enable_if_t<BASE_NS::is_arithmetic_v<T>, bool> = true>
inline bool SafeGetJsonValue(
    const CORE_NS::json::value& jsonData, const BASE_NS::string_view element, BASE_NS::string& error, T& output)
{
    if (auto const pos = jsonData.find(element); pos) {
        if (pos->is_number()) {
            output = pos->as_number<T>();
            return true;
        } else {
            error += element + ": expected number.\n";
        }
    }
    return false;
}

template<class T, BASE_NS::enable_if_t<BASE_NS::is_convertible_v<T, BASE_NS::string_view>, bool> = true>
inline bool SafeGetJsonValue(
    const CORE_NS::json::value& jsonData, const BASE_NS::string_view element, BASE_NS::string& error, T& output)
{
    if (auto const pos = jsonData.find(element); pos) {
        if (pos->is_string()) {
            output = T(pos->string_.data(), pos->string_.size());
            return true;
        } else {
            error += element + ": expected string.\n";
        }
    }
    return false;
}

template<typename T, BASE_NS::enable_if_t<BASE_NS::is_same_v<bool, T>, bool> = true>
inline bool FromJson(const CORE_NS::json::value& jsonData, T& result)
{
    if (jsonData.is_boolean()) {
        result = static_cast<T>(jsonData.boolean_);
        return true;
    }
    return false;
}

template<typename T, BASE_NS::enable_if_t<!BASE_NS::is_same_v<bool, T> && BASE_NS::is_arithmetic_v<T>, bool> = true>
inline bool FromJson(const CORE_NS::json::value& jsonData, T& result)
{
    if (jsonData.is_number()) {
        result = jsonData.as_number<T>();
        return true;
    }
    return false;
}

template<typename T, BASE_NS::enable_if_t<BASE_NS::is_convertible_v<T, BASE_NS::string_view>, bool> = true>
inline bool FromJson(const CORE_NS::json::value& jsonData, T& result)
{
    if (jsonData.is_string()) {
        result = BASE_NS::string_view { jsonData.string_ };
        return true;
    }
    return false;
}

namespace Detail {
constexpr const BASE_NS::string_view INVALID_DATATYPE = "Failed to read value, invalid datatype: ";
template<typename T>
inline T Convert(const CORE_NS::json::value& value)
{
    T result;
    FromJson(value, result);
    return result;
}

template<typename Container, typename OutIt, typename Fn>
inline OutIt Transform(Container&& container, OutIt dest, Fn func)
{
    return std::transform(container.begin(), container.end(), dest, func);
}
} // namespace Detail

template<class JsonType, typename T>
inline void FromJson(const JsonType& jsonData, BASE_NS::array_view<T> container)
{
    if (jsonData.is_array()) {
        const auto view =
            BASE_NS::array_view(jsonData.array_.data(), BASE_NS::Math::min(jsonData.array_.size(), container.size()));
        Detail::Transform(view, std::begin(container), [](const JsonType& value) { return Detail::Convert<T>(value); });
    }
}

template<class JsonType, typename T>
inline void FromJson(const JsonType& jsonData, BASE_NS::vector<T>& container)
{
    if (jsonData.is_array()) {
        Detail::Transform(jsonData.array_, std::back_inserter(container),
            [](const JsonType& value) { return Detail::Convert<T>(value); });
    }
}

template<class JsonType, typename T, size_t N>
inline void FromJson(const JsonType& jsonData, T (&container)[N])
{
    FromJson(jsonData, BASE_NS::array_view(container));
}

template<class JsonType, typename T,
    BASE_NS::enable_if_t<BASE_NS::is_array_v<decltype(T::data)> &&
                             BASE_NS::is_arithmetic_v<BASE_NS::remove_extent_t<decltype(T::data)>>,
        bool> = true>
inline void FromJson(const JsonType& jsonData, T& output)
{
    FromJson(jsonData, output.data);
}
CORE3D_END_NAMESPACE()

#endif