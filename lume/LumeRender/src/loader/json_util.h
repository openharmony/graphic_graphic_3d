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

#ifndef LOADER_JSON_UTIL_H
#define LOADER_JSON_UTIL_H

#include <cerrno>
#include <cstdlib>

#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/math/mathf.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <base/util/uid_util.h>
#include <core/json/json.h>
#include <core/namespace.h>

#include "util/log.h"
#include "util/string_util.h"

RENDER_BEGIN_NAMESPACE()
#define CORE_JSON_SERIALIZE_ENUM(ENUM_TYPE, ...)                                                                    \
    template<typename BasicJsonType>                                                                                \
    inline void to_json(BasicJsonType& j, const ENUM_TYPE& e)                                                       \
    {                                                                                                               \
        static_assert(std::is_enum<ENUM_TYPE>::value, #ENUM_TYPE " must be an enum!");                              \
        static constexpr std::pair<ENUM_TYPE, BASE_NS::string_view> m[] = __VA_ARGS__;                              \
        auto it = std::find_if(std::begin(m), std::end(m),                                                          \
            [e](const std::pair<ENUM_TYPE, BASE_NS::string_view>& ej_pair) -> bool { return ej_pair.first == e; }); \
        j = ((it != std::end(m)) ? it : std::begin(m))->second;                                                     \
    }                                                                                                               \
    template<typename BasicJsonType>                                                                                \
    inline bool FromJson(const BasicJsonType& j, ENUM_TYPE& e)                                                      \
    {                                                                                                               \
        static_assert(std::is_enum<ENUM_TYPE>::value, #ENUM_TYPE " must be an enum!");                              \
        static constexpr std::pair<ENUM_TYPE, BASE_NS::string_view> m[] = __VA_ARGS__;                              \
        if (j.is_string()) {                                                                                        \
            auto it = std::find_if(std::begin(m), std::end(m),                                                      \
                [name = j.string_](const std::pair<ENUM_TYPE, BASE_NS::string_view>& ej_pair) -> bool {             \
                    return ej_pair.second == name;                                                                  \
                });                                                                                                 \
            e = ((it != std::end(m)) ? it : std::begin(m))->first;                                                  \
            return true;                                                                                            \
        }                                                                                                           \
        return false;                                                                                               \
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

template<class T>
struct JsonContext {
    T data;
    BASE_NS::string error;
};

inline bool ParseHex(BASE_NS::string_view str, uint32_t& val)
{
    errno = 0;
    char* end;
    constexpr const int hexadecimalBase = 16;
    val = std::strtoul(str.data(), &end, hexadecimalBase);
    return !(end != (str.end().ptr()) || errno != 0);
}

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

template<class JsonType, typename T, typename = BASE_NS::enable_if_t<BASE_NS::is_arithmetic_v<T>>>
bool SafeGetJsonValue(const JsonType& jsonData, const BASE_NS::string_view element, BASE_NS::string& error, T& output)
{
    if (auto const pos = jsonData.find(element); pos) {
        if (!FromJson(*pos, output)) {
            error += element + ": expected number.\n";
            return false;
        }
    }
    return true;
}

template<class JsonType, class T, BASE_NS::enable_if_t<BASE_NS::is_convertible_v<T, BASE_NS::string_view>, bool> = true>
bool SafeGetJsonValue(const JsonType& jsonData, const BASE_NS::string_view element, BASE_NS::string& error, T& output)
{
    if (auto const pos = jsonData.find(element); pos) {
        if (!FromJson(*pos, output)) {
            error += element + ": expected string.\n";
            return false;
        }
    }
    return true;
}

template<class JsonType, class T>
bool SafeGetJsonEnum(const JsonType& jsonData, const BASE_NS::string_view element, BASE_NS::string& error, T& output)
{
    if (auto const pos = jsonData.find(element); pos) {
        if (!FromJson(*pos, output)) {
            error += Detail::INVALID_DATATYPE + element + " (" + CORE_NS::json::to_string(*pos) + ")\n";
            return false;
        }
    }
    return true;
}

template<class T, class JsonType>
bool SafeGetJsonBitfield(
    const JsonType& jData, const BASE_NS::string_view element, BASE_NS::string& error, uint32_t& output)
{
    if (auto const pos = jData.find(element); pos) {
        output = 0;

        if (pos->is_string()) {
            for (const auto& field : StringUtil::Split(pos->string_, BASE_NS::string_view("|"))) {
                if (const T value = Detail::Convert<T>(field); value != static_cast<T>(0x7FFFFFFF)) {
                    output |= value;
                } else {
                    PLUGIN_LOG_W("Unknown bit value in \'%s\' \'%s\'", element.data(), BASE_NS::string(field).data());
                }
            }
        } else {
            error += Detail::INVALID_DATATYPE + element + " (" + CORE_NS::json::to_string(*pos) + ")\n";
            return false;
        }
    }
    return true;
}

template<class JsonType>
bool SafeGetJsonUidValue(
    const JsonType& jsonData, const BASE_NS::string_view element, BASE_NS::string& error, BASE_NS::Uid& output)
{
    if (auto const pos = jsonData.find(element); pos) {
        output = {};
        if (pos->is_string()) {
            constexpr size_t uidLength { to_string(BASE_NS::Uid {}).size() }; // default uid length
            if (pos->string_.size() == uidLength) {
                output = StringToUid(pos->string_);
                return true;
            } else {
                error += element + ": size does not match uid.\n";
            }
        } else {
            error += element + ": expected string as uid.\n";
        }
    }
    return false;
}

template<class ArrayType, class JsonType, class ResultType>
void ParseArray(JsonType const& jData, char const* element, BASE_NS::vector<ArrayType>& out, ResultType& res)
{
    if (auto const array = jData.find(element); array && array->is_array()) {
        out.reserve(out.size() + array->array_.size());
        Detail::Transform(array->array_, std::back_inserter(out), [&res](const JsonType& value) {
            JsonContext<ArrayType> result;
            FromJson(value, result);
            if (!result.error.empty()) {
                res.error += result.error;
            }
            return result.data;
        });
    }
}

template<class JsonType>
void SafeGetJsonMask(
    const JsonType& jsonData, const BASE_NS::string_view element, BASE_NS::string& error, uint32_t& output)
{
    if (const auto mask = jsonData.find(element); mask) {
        if (mask->is_string() && ParseHex(mask->string_, output)) {
        } else if (mask->is_number()) {
            output = mask->template as_number<uint32_t>();
        } else {
            error += "Failed to read value: " + element + " (" + CORE_NS::json::to_string(*mask) + ")";
        }
    }
}

template<class JsonType, typename T,
    BASE_NS::enable_if_t<BASE_NS::is_array_v<decltype(T::data)> &&
                             BASE_NS::is_arithmetic_v<BASE_NS::remove_extent_t<decltype(T::data)>>,
        bool> = true>
inline void FromJson(const JsonType& jsonData, T& output)
{
    FromJson(jsonData, output.data);
}
RENDER_END_NAMESPACE()

#endif // LOADER_JSON_UTIL_H
