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

#include "util/json_util.h"
#include "util/log.h"
#include "util/string_util.h"

RENDER_BEGIN_NAMESPACE()
#define RENDER_JSON_SERIALIZE_ENUM(ENUM_TYPE, ...)                                                 \
    template<typename BasicJsonType>                                                               \
    inline bool FromJson(const BasicJsonType& j, ENUM_TYPE& e)                                     \
    {                                                                                              \
        static_assert(std::is_enum<ENUM_TYPE>::value, #ENUM_TYPE " must be an enum!");             \
        using StorageType = std::underlying_type_t<ENUM_TYPE>;                                     \
        struct Pair {                                                                              \
            constexpr Pair(StorageType v, const char* c) : value(v), name(c)                       \
            {}                                                                                     \
            constexpr Pair(ENUM_TYPE v, const char* c) : value(StorageType(v)), name(c)            \
            {}                                                                                     \
            StorageType value;                                                                     \
            BASE_NS::string_view name;                                                             \
        };                                                                                         \
        static constexpr Pair m[] = __VA_ARGS__;                                                   \
        if (j.is_string()) {                                                                       \
            auto it = std::find_if(std::begin(m), std::end(m),                                     \
                [name = j.string_](const Pair& ej_pair) -> bool { return ej_pair.name == name; }); \
            e = static_cast<ENUM_TYPE>(((it != std::end(m)) ? it : std::begin(m))->value);         \
            return true;                                                                           \
        }                                                                                          \
        return false;                                                                              \
    }

template<class T>
struct JsonContext {
    T data;
    BASE_NS::string error;
};

inline bool ParseHex(BASE_NS::string_view str, uint32_t& val)
{
    if (!str.empty()) {
        errno = 0;
        char* end;
        constexpr const int hexadecimalBase = 16;
        const unsigned long result = std::strtoul(str.data(), &end, hexadecimalBase);
        if ((result <= UINT32_MAX) && (end == str.end().ptr()) && (errno == 0)) {
            val = result;
            return true;
        }
    }
    val = 0U;
    return false;
}

template<class JsonType, class T, BASE_NS::enable_if_t<BASE_NS::is_arithmetic_v<T>, bool> = true>
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
void ParseArray(JsonType const& jData, const char* element, BASE_NS::vector<ArrayType>& out, ResultType& res)
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
            error += "Failed to read value: " + element + " (" + CORE_NS::json::to_string(*mask) + ')';
        }
    }
}
RENDER_END_NAMESPACE()

#endif // LOADER_JSON_UTIL_H
