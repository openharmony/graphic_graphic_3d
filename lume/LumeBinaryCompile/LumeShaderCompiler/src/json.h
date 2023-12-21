/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef LUMEJSON_H
#define LUMEJSON_H

#if defined __has_include
#if __has_include(<charconv>)
#define HAS_CHARCONV 1
#include <charconv>
#endif
#endif

#ifndef JSON_STRING_TYPE
#include <string_view>
#endif // JSON_STRING_TYPE
#ifndef JSON_ARRAY_TYPE
#include <vector>
#endif // JSON_ARRAY_TYPE

#include <string>

namespace json {
#ifndef JSON_STRING_TYPE
using string_t = std::string_view;
#else
using string_t = JSON_STRING_TYPE;
#endif

#ifndef JSON_ARRAY_TYPE
template<typename T>
using array_t = std::vector<T>;
#else
template<typename T>
using array_t = JSON_ARRAY_TYPE<T>;
#endif

/// simple json parser parser
enum class type : char {
    uninitialized = 0,
    object,
    array,
    string,
    number,
    boolean,
    null,
};

struct value;

// parses 'data' and writes the corresponding JSON structure to 'value'
char* parse_value(char* data, value& value);

// parses 'data' and return JSON structure. the value::type will be 'uninitialized' if parsing failed.
value parse(char* data);

std::string to_string(const value& value);

struct pair;

struct null {};
using object = array_t<pair>;
using array = array_t<value>;
using string = string_t;

struct value {
    type type{ type::uninitialized };
    union {
        object object_;
        array array_;
        string string_;
        double number_;
        bool boolean_;
    };

    value() : type{ type::uninitialized } {}
    value(object&& value) : type{ type::object }, object_(std::move(value)) {}
    value(array&& value) : type{ type::array }, array_(std::move(value)) {}
    value(string value) : type{ type::string }, string_(value) {}
    value(double value) : type{ type::number }, number_(value) {}
    explicit value(bool value) : type{ type::boolean }, boolean_(value) {}
    value(null value) : type{ type::null } {}

    template<typename T, std::enable_if_t<!std::is_same_v<T, bool> && std::is_arithmetic_v<T>, bool> = true>
    value(T value) : type{ type::number }, number_(static_cast<double>(value))
    {}

    template<typename T>
    value(array_t<T> values) : type{ type::array }, array_(array{})
    {
        array_.reserve(values.size());
        for (const auto& value : values) {
            array_.push_back({ value });
        }
    }

    template<typename T, size_t N>
    value(T (&value)[N]) : type{ type::array }, array_(array{})
    {
        array_.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            array_.push_back({ value[i] });
        }
    }

    value(value&& rhs) : type{ std::exchange(rhs.type, type::uninitialized) }
    {
        switch (type) {
            case type::uninitialized:
                break;
            case type::object:
                new (&object_) object(std::move(rhs.object_));
                break;
            case type::array:
                new (&array_) array(std::move(rhs.array_));
                break;
            case type::string:
                new (&string_) string_t(std::move(rhs.string_));
                break;
            case type::number:
                number_ = rhs.number_;
                break;
            case type::boolean:
                boolean_ = rhs.boolean_;
                break;
            case type::null:
                break;
            default:
                break;
        }
    }

    value& operator=(value&& rhs)
    {
        if (this != &rhs) {
            cleanup();
            type = std::exchange(rhs.type, type::uninitialized);
            switch (type) {
                case type::uninitialized:
                    break;
                case type::object:
                    new (&object_) object(std::move(rhs.object_));
                    break;
                case type::array:
                    new (&array_) array(std::move(rhs.array_));
                    break;
                case type::string:
                    new (&string_) string_t(std::move(rhs.string_));
                    break;
                case type::number:
                    number_ = rhs.number_;
                    break;
                case type::boolean:
                    boolean_ = rhs.boolean_;
                    break;
                case type::null:
                    break;
                default:
                    break;
            }
        }
        return *this;
    }

    ~value()
    {
        cleanup();
    }

    void cleanup()
    {
        switch (type) {
            case type::uninitialized:
                break;
            case type::object:
                object_.~vector();
                break;
            case type::array:
                array_.~vector();
                break;
            case type::string:
                break;
            case type::number:
                break;
            case type::boolean:
                break;
            case type::null:
                break;
            default:
                break;
        }
    }

    inline bool is_object() const noexcept
    {
        return type == type::object;
    }
    inline bool is_array() const noexcept
    {
        return type == type::array;
    }
    inline bool is_string() const noexcept
    {
        return type == type::string;
    }
    inline bool is_number() const noexcept
    {
        return type == type::number;
    }
    inline bool is_boolean() const noexcept
    {
        return type == type::boolean;
    }
    inline bool is_null() const noexcept
    {
        return type == type::null;
    }

    inline bool empty() const noexcept
    {
        if (is_object()) {
            return object_.empty();
        } else if (is_array()) {
            return array_.empty();
        }
        return true;
    }

    const value* find(string_t key) const noexcept;

    value& operator[](const string_t& key);
};

struct pair {
    string_t key;
    value value;
};

inline const value* value::find(string_t key) const noexcept
{
    if (type == type::object) {
        for (auto& t : object_) {
            if (t.key == key) {
                return &t.value;
            }
        }
    }
    return nullptr;
}

inline value& value::operator[](const string_t& key)
{
    if (type == type::object) {
        for (auto& t : object_) {
            if (t.key == key) {
                return t.value;
            }
        }
        object_.push_back({ key, value{} });
        return object_.back().value;
    }
    return *this;
}

inline bool HasKey(const value& object, const string_t key)
{
    if (object.type == type::object) {
        for (auto& t : object.object_) {
            if (t.key == key) {
                return true;
            }
        }
    }
    return false;
}

inline const value& GetKey(const value& object, const string_t key)
{
    static value dummy;
    if (object.type == type::object) {
        for (auto& t : object.object_) {
            if (t.key == key) {
                return t.value;
            }
        }
    }
    return dummy;
}

#ifdef JSON_IMPL
char* parse_value(char* data, value& value);
char* parse_number(char* data, value&);
char* parse_string(char* data, value&);

inline bool isWhite(char data)
{
    return ((data == ' ') || (data == '\n') || (data == '\r') || (data == '\t'));
}

inline bool isSign(char data)
{
    return ((data == '+') || (data == '-'));
}

inline bool isDigit(char data)
{
    return ((data >= '0') && (data <= '9'));
}

inline bool isHex(char data)
{
    return ((data >= '0') && (data <= '9')) || ((data >= 'a') && (data <= 'f')) || ((data >= 'A') && (data <= 'F'));
}

inline char* trim(char* data)
{
    while (*data && isWhite(*data)) {
        data++;
    }
    return data;
}

char* parse_object(char* data, value& res)
{
    data = trim(data);
    if (*data == '}') {
        // handle empty object.
        res = value(object{});
        return data + 1;
    }
    object values;
    for (; *data != 0;) {
        if (*data != '"') {
            // invalid json
            *data = 0;
            return data;
        }

        pair& pair = values.emplace_back();
        value key;
        data = trim(parse_string(data + 1, key));
        pair.key = key.string_;
        if (*data == ':') {
            data++;
        } else {
            // invalid json
            *data = 0;
            return data;
        }
        data = trim(data);
        data = trim(parse_value(data, pair.value));
        if (*data == '}') {
            // handle end
            res = value(std::move(values));
            return data + 1;
        } else if (*data == ',') {
            data = trim(data + 1);
        } else {
            // invalid json
            *data = 0;
            return data;
        }
    }
    return data;
}

char* parse_array(char* data, value& res)
{
    data = trim(data);
    if (*data == ']') {
        // empty array.
        res = value(array{});
        return data + 1;
    }
    array values;
    for (; *data != 0;) {
        value tmp;
        data = trim(parse_value(data, tmp));
        values.push_back(std::move(tmp));
        if (*data == ',') {
            data = trim(data + 1);
        } else if (*data == ']') {
            res = value(std::move(values));
            return data + 1;
        } else {
            // invalid json
            *data = 0;
            return data;
        }
    }
    return data;
}

// values
char* parse_string(char* data, value& res)
{
    char* start = data;
    for (; *data != 0; data++) {
        if (*data == '\\' && data[1]) {
            // escape.. (parse just enough to not stop too early)
            if (data[1] == '\\' || data[1] == '"' || data[1] == '/' || data[1] == 'b' || data[1] == 'f' ||
                data[1] == 'n' || data[1] == 'r' || data[1] == 't') {
                ++data;
                continue;
            } else if (data[1] == 'u') {
                data += 2;
                for (char* end = data + 4; data != end; ++data) {
                    if (*data == 0 || !isHex(*data)) {
                        *data = 0;
                        return data;
                    }
                }
                --data;
            } else {
                // invalid json
                *data = 0;
                return data;
            }
        } else if (*data == '"') {
            res = value(string_t{ start, (size_t)(data - start) });
            return data + 1;
        } else if (static_cast<unsigned char>(*data) < 0x20) {
            // invalid json
            *data = 0;
            return data;
        }
    }
    return data;
}

char* parse_number(char* data, value& res)
{
    char* beg = data;
    if (*data == '-') {
        // is neg..
        data++;
        if (!isDigit(*data)) {
            *data = 0;
            return data;
        }
    }
    bool fraction = false;
    bool exponent = false;

    if (*data == '0') {
        ++data;
        // after leading zero only '.', 'e' and 'E' allowed
        if (*data == '.') {
            ++data;
            fraction = true;
        } else if (*data == 'e' || *data == 'E') {
            ++data;
            exponent = true;
        }
    } else {
        for (; *data != 0; data++) {
            if (isDigit(*data))
                continue;
            if (*data == '.') {
                ++data;
                fraction = true;
            } else if (*data == 'e' || *data == 'E') {
                ++data;
                exponent = true;
            }
            break;
        }
    }

    if (fraction) {
        // exponent must start with a digit
        if (isDigit(*data)) {
            ++data;
        } else {
            // invalid json
            *data = 0;
            return data;
        }
        // fraction may contain digits up to begining of exponent ('e' or 'E')
        for (; *data != 0; data++) {
            if (isDigit(*data))
                continue;
            if (*data == 'e' || *data == 'E') {
                ++data;
                exponent = true;
            }
            break;
        }
    }
    if (exponent) {
        // exponent must start with '-' or '+' followed by a digit, or digit
        if (*data == '-' || *data == '+') {
            ++data;
        }
        if (isDigit(*data)) {
            ++data;
        } else {
            // invalid json
            *data = 0;
            return data;
        }
        for (; *data != 0; data++) {
            if (isDigit(*data))
                continue;
            break;
        }
    }
    if (data != beg) {
        char* end = data - 1;
        res = value(strtod(beg, &end));
        return data;
    }
    *data = 0;
    return data;
}

char* parse_boolean(char* data, value& res)
{
    if (*data == 't') {
        ++data;
        const char rue[] = { 'r', 'u', 'e' };
        for (unsigned i = 0u; i < sizeof(rue); ++i) {
            if (data[i] == 0 || data[i] != rue[i]) {
                *data = 0;
                return data;
            }
        }

        res = value(true);
        data += sizeof(rue);
    } else if (*data == 'f') {
        ++data;
        const char alse[] = { 'a', 'l', 's', 'e' };
        for (unsigned i = 0u; i < sizeof(alse); ++i) {
            if (data[i] == 0 || data[i] != alse[i]) {
                *data = 0;
                return data;
            }
        }
        res = value(false);
        data += sizeof(alse);
    } else {
        // invalid json
        *data = 0;
        return data;
    }
    return data;
}

char* parse_null(char* data, value& res)
{
    if (*data == 'n') {
        ++data;
        const char ull[] = { 'u', 'l', 'l' };
        for (unsigned i = 0u; i < sizeof(ull); ++i) {
            if (data[i] == 0 || data[i] != ull[i]) {
                *data = 0;
                return data;
            }
        }
        res = value(null{});
        data += sizeof(ull);
    } else {
        // invalid json
        *data = 0;
        return data;
    }
    return data;
}

char* parse_value(char* data, value& value)
{
    data = trim(data);
    if (*data == '{') {
        data = trim(parse_object(data + 1, value));
    } else if (*data == '[') {
        data = trim(parse_array(data + 1, value));
    } else if (*data == '"') {
        data = trim(parse_string(data + 1, value));
    } else if (isSign(*data) || isDigit(*data)) {
        data = trim(parse_number(data, value));
    } else if ((*data == 't') || (*data == 'f')) {
        data = trim(parse_boolean(data, value));
    } else if (*data == 'n') {
        data = trim(parse_null(data, value));
    } else {
        // invalid json
        *data = 0;
        return data;
    }
    return data;
}

void add(value& v, value&& value)
{
    switch (v.type) {
        case type::uninitialized:
            v = std::move(value);
            break;
        case type::object:
            v.object_.back().value = std::move(value);
            break;
        case type::array:
            v.array_.push_back(std::move(value));
            break;
        case type::string:
        case type::number:
        case type::boolean:
        case type::null:
        default:
            break;
    }
}

value parse(char* data)
{
    array stack;
    // push an uninitialized value which will get the final value during parsing
    stack.push_back(value{});

    bool acceptValue = true;
    while (*data) {
        data = trim(data);
        if (*data == '{') {
            // start of an object
            if (!acceptValue) {
                return {};
            }
            data = trim(data + 1);
            if (*data == '}') {
                data = trim(data + 1);
                // handle empty object.
                add(stack.back(), value(object{}));
                acceptValue = false;
            } else if (*data == '"') {
                // try to read the key
                value key;
                data = trim(parse_string(data + 1, key));

                if (*data != ':') {
                    // invalid json
                    return {};
                }
                data = trim(data + 1);
                // push the object with key and missing value on the stack and hope to find a value next
                stack.push_back(value(object{}));
                stack.back().object_.push_back(pair{ key.string_, value{} });
                acceptValue = true;
            } else {
                // invalid json
                return {};
            }
        } else if (*data == '}') {
            // end of an object
            if (stack.back().type != type::object) {
                // invalid json
                return {};
            }
            // check are we missing a value ('{"":}', '{"":"",}' )
            if (acceptValue) {
                return {};
            }
            data = trim(data + 1);
            // move this object to the next in the stack
            auto value = std::move(stack.back());
            stack.pop_back();
            if (stack.empty()) {
                // invalid json
                return {};
            }
            add(stack.back(), std::move(value));
            acceptValue = false;
        } else if (*data == '[') {
            // start of an array
            if (!acceptValue) {
                return {};
            }
            data = trim(data + 1);
            if (*data == ']') {
                data = trim(data + 1);
                // handle empty array.
                add(stack.back(), value(array{}));
                acceptValue = false;
            } else {
                // push the empty array on the stack and hope to find values
                stack.push_back(value(array{}));
                acceptValue = true;
            }
        } else if (*data == ']') {
            // end of an array
            if (stack.back().type != type::array) {
                // invalid json
                return {};
            }
            // check are we missing a value ('[1,]' '[1]]')
            if (acceptValue) {
                // invalid json
                return {};
            }
            data = trim(data + 1);

            auto value = std::move(stack.back());
            stack.pop_back();
            if (stack.empty()) {
                // invalid json
                return {};
            }
            add(stack.back(), std::move(value));
            acceptValue = false;
        } else if (*data == ',') {
            // comma is allowed when the previous value was complete and we have an incomplete object or array on the
            // stack.
            if (!acceptValue && stack.back().type == type::object) {
                data = trim(data + 1);
                if (*data != '"') {
                    // invalid json
                    return {};
                }
                // try to read the key
                value key;
                data = trim(parse_string(data + 1, key));

                if (*data != ':') {
                    // invalid json
                    return {};
                }
                data = trim(data + 1);
                stack.back().object_.push_back(pair{ key.string_, value{} });
                acceptValue = true;
            } else if (!acceptValue && stack.back().type == type::array) {
                data = trim(data + 1);
                acceptValue = true;
            } else {
                // invalid json
                return {};
            }
        } else if (*data == '"') {
            value value;
            data = trim(parse_string(data + 1, value));
            if (acceptValue && value.type == type::string) {
                add(stack.back(), std::move(value));
                acceptValue = false;
            } else {
                // invalid json
                return {};
            }
        } else if (isSign(*data) || isDigit(*data)) {
            value value;
            data = trim(parse_number(data, value));
            if (acceptValue && value.type == type::number) {
                add(stack.back(), std::move(value));
                acceptValue = false;
            } else {
                // invalid json
                return {};
            }
        } else if ((*data == 't') || (*data == 'f')) {
            value value;
            data = trim(parse_boolean(data, value));
            if (acceptValue && value.type == type::boolean) {
                add(stack.back(), std::move(value));
                acceptValue = false;
            } else {
                // invalid json
                return {};
            }
        } else if (*data == 'n') {
            value value;
            data = trim(parse_null(data, value));
            if (acceptValue && value.type == type::null) {
                add(stack.back(), std::move(value));
                acceptValue = false;
            } else {
                // invalid json
                return {};
            }
        } else {
            // invalid json
            return {};
        }
    }
    // check if we are missing a value ('{"":' '[')
    if (acceptValue) {
        return {};
    }

    auto value = std::move(stack.front());
    return value;
}

// end of parser

std::string to_string(const value& value)
{
    std::string out;
    switch (value.type) {
        case type::uninitialized:
            break;

        case type::object: {
            out += '{';
            int count = 0;
            for (const auto& v : value.object_) {
                if (count++) {
                    out += ',';
                }
                out += '"';
                out.append(v.key.data(), v.key.size());
                out += '"';
                out += ':';
                out += to_string(v.value);
            }
            out += '}';
            break;
        }

        case type::array: {
            out += '[';
            int count = 0;
            for (const auto& v : value.array_) {
                if (count++) {
                    out += ',';
                }
                out += to_string(v);
            }
            out += ']';
            break;
        }

        case type::string:
            out += '"';
            out.append(value.string_.data(), value.string_.size());
            out += '"';
            break;

        case type::number:
            if (value.number_ == static_cast<int64_t>(value.number_)) {
                out += std::to_string(static_cast<int64_t>(value.number_));
            } else {
#if defined(HAS_CHARCONV) && !defined(__linux__) && !defined(__APPLE__)
                char asStr[64]{};
                if (auto result = std::to_chars(std::begin(asStr), std::end(asStr), value.number_);
                    result.ec == std::errc()) {
                    out.append(asStr, static_cast<size_t>(result.ptr - asStr));
                }
#else
                out += std::to_string(value.number_).data();
#endif
            }
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
#endif // JSON_IMPL
} // namespace json
#endif // !LUMEJSON_H
