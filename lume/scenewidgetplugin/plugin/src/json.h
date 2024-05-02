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

#ifndef SCENE_PLUGIN_JSON_H
#define SCENE_PLUGIN_JSON_H

#include <core/json/json.h>

CORE_BEGIN_NAMESPACE()

constexpr int JSON_DEFAULT_INDENTATION = 2;

template<typename T>
BASE_NS::string to_formatted_string(const CORE_NS::json::value_t<T>& value,
    const int indentation = JSON_DEFAULT_INDENTATION, const int currentIndentation = 0);

#ifdef JSON_IMPL
using namespace CORE_NS::json;

template<typename T>
void append(BASE_NS::string& out, const typename value_t<T>::string& string)
{
    out += '"';
    out.append(escape(string));
    out += '"';
}

template<typename T>
void append(
    BASE_NS::string& out, const typename value_t<T>::object& object, const int indentation, int currentIndentation)
{
    if (object.empty()) {
        // Keep empty objects on one row.
        out.append("{}");
        return;
    }

    out += "{\n";
    currentIndentation += indentation;
    out.append(currentIndentation, ' ');

    int count = 0;
    for (const auto& v : object) {
        if (count++) {
            out += ",\n";
            out.append(currentIndentation, ' ');
        }
        CORE_NS::append<T>(out, v.key);
        out += ": ";
        out += to_formatted_string(v.value, indentation, currentIndentation);
    }
    currentIndentation -= indentation;
    out += '\n';
    out.append(currentIndentation, ' ');
    out += '}';
}

template<typename T>
void append(
    BASE_NS::string& out, const typename value_t<T>::array& array, const int indentation, int currentIndentation)
{
    if (array.empty()) {
        // Keep empty arrays on one row.
        out.append("[]");
        return;
    }

    out += "[\n";
    currentIndentation += indentation;
    out.append(currentIndentation, ' ');
    int count = 0;
    for (const auto& v : array) {
        if (count++) {
            out += ",\n";
            out.append(currentIndentation, ' ');
        }
        out += to_formatted_string(v, indentation, currentIndentation);
    }
    currentIndentation -= indentation;
    out += '\n';
    out.append(currentIndentation, ' ');
    out += ']';
}

template<typename T>
void append(BASE_NS::string& out, const double floatingPoint)
{
    const char* FLOATING_FORMAT_STR = "%.17g";
    const int size = snprintf(nullptr, 0, FLOATING_FORMAT_STR, floatingPoint);
    const size_t oldSize = out.size();
    out.resize(oldSize + size);
    const size_t newSize = out.size();
    // "At most bufsz - 1 characters are written." string has size() characters + 1 for null so use size() +
    // 1 as the total size. If resize() failed string size() hasn't changed, buffer will point to the null
    // character and bufsz will be 1 i.e. only the null character will be written.
    snprintf_s(out.data() + oldSize, newSize + 1 - oldSize, size, FLOATING_FORMAT_STR, floatingPoint);
}

template<typename T>
BASE_NS::string to_formatted_string(const value_t<T>& value, const int indentation, const int currentIndentation)
{
    BASE_NS::string out;
    switch (value.type) {
        case type::uninitialized:
            out += "{}";
            break;

        case type::object:
            append<T>(out, value.object_, indentation, currentIndentation);
            break;

        case type::array:
            append<T>(out, value.array_, indentation, currentIndentation);
            break;

        case type::string:
            CORE_NS::append<T>(out, value.string_);
            break;

        case type::floating_point:
            CORE_NS::append<T>(out, value.float_);
            break;

        case type::signed_int:
            out += BASE_NS::to_string(value.signed_);
            break;

        case type::unsigned_int:
            out += BASE_NS::to_string(value.unsigned_);
            break;

        case type::boolean:
            if (value.boolean_) {
                out += "true";
            } else {
                out += "false";
            }
            break;

        case type::null:
            out += "null";
            break;

        default:
            break;
    }
    return out;
}

// Explicit template instantiation for the needed types.
template BASE_NS::string to_formatted_string(const value& value, const int indentation, const int currentIndentation);
template BASE_NS::string to_formatted_string(
    const standalone_value& value, const int indentation, const int currentIndentation);

#endif // JSON_IMPL

CORE_END_NAMESPACE()

#endif // SCENE_PLUGIN_JSON_H
