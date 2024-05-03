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

#ifndef CORE__LOADER__JSON_UTIL_H
#define CORE__LOADER__JSON_UTIL_H

#include <algorithm>
#include <cstdlib>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/namespace.h>
#include <core/json/json.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
template<typename T, BASE_NS::enable_if_t<BASE_NS::is_same_v<bool, T>, bool> = true>
inline void from_json(const json::value& jsonData, T& result)
{
    if (jsonData.is_boolean()) {
        result = static_cast<T>(jsonData.boolean_);
    }
}

template<typename T, BASE_NS::enable_if_t<!BASE_NS::is_same_v<bool, T> && BASE_NS::is_arithmetic_v<T>, bool> = true>
inline void from_json(const json::value& jsonData, T& result)
{
    if (jsonData.is_number()) {
        result = jsonData.as_number<T>();
    }
}

template<typename T, BASE_NS::enable_if_t<BASE_NS::is_convertible_v<T, BASE_NS::string_view>, bool> = true>
inline bool from_json(const CORE_NS::json::value& jsonData, T& result)
{
    if (jsonData.is_string()) {
        result = BASE_NS::string_view { jsonData.string_ };
        return true;
    }
    return false;
}

namespace Detail {
template<typename T>
inline T Convert(const json::value& value)
{
    T result;
    from_json(value, result);
    return result;
}

template<typename Container, typename OutIt, typename Fn>
inline OutIt Transform(Container&& container, OutIt dest, Fn func)
{
    return std::transform(container.begin(), container.end(), dest, func);
}
} // namespace Detail

template<typename T>
inline void from_json(const json::value& jsonData, BASE_NS::array_view<T> container)
{
    if (jsonData.is_array()) {
        const auto view =
            BASE_NS::array_view(jsonData.array_.data(), BASE_NS::Math::min(jsonData.array_.size(), container.size()));
        Detail::Transform(view, container.begin(), Detail::Convert<T>);
    }
}

template<typename T, size_t N>
inline void from_json(const json::value& jsonData, T (&container)[N])
{
    if (jsonData.is_array()) {
        const auto view = BASE_NS::array_view(jsonData.array_.data(), BASE_NS::Math::min(jsonData.array_.size(), N));
        Detail::Transform(view, std::begin(container), Detail::Convert<T>);
    }
}

template<typename T, BASE_NS::enable_if_t<BASE_NS::is_arithmetic_v<T>, bool> = true>
bool SafeGetJsonValue(
    const json::value& jsonData, const BASE_NS::string_view element, BASE_NS::string& error, T& output)
{
    if (auto const pos = jsonData.find(element); pos) {
        if constexpr (BASE_NS::is_same_v<bool, T>) {
            if (pos->is_boolean()) {
                output = pos->boolean_;
                return true;
            }
        } else {
            if (pos->is_number()) {
                output = pos->as_number<T>();
                return true;
            }
        }
        error += element + ": expected number.\n";
    }
    return false;
}

template<class T, BASE_NS::enable_if_t<BASE_NS::is_convertible_v<T, BASE_NS::string_view>, bool> = true>
bool SafeGetJsonValue(
    const json::value& jsonData, const BASE_NS::string_view element, BASE_NS::string& error, T& output)
{
    if (auto const pos = jsonData.find(element); pos) {
        if (pos->is_string()) {
            output = BASE_NS::string_view { pos->string_ };
            return true;
        } else {
            error += element + ": expected string.\n";
        }
    }
    return false;
}
CORE_END_NAMESPACE()

#endif