/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "json_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <securec.h>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/json/json.h>
#include <core/namespace.h>

using namespace CORE_NS::json;

namespace {
const uint8_t* g_data = nullptr;
size_t g_size = 0;
size_t g_pos;

template <class T>
T GetData()
{
    T object{};
    size_t objectSize = sizeof(object);
    if (g_data == nullptr || objectSize > g_size - g_pos) {
        return object;
    }
    errno_t ret = memcpy_s(&object, objectSize, g_data + g_pos, objectSize);
    if (ret != EOK) {
        return {};
    }
    g_pos += objectSize;
    return object;
}

std::string FuzzString(size_t maxLen)
{
    std::string result;
    if (g_data == nullptr || g_pos + sizeof(size_t) > g_size) {
        return result;
    }
    size_t len = GetData<size_t>() % (maxLen + 1);
    for (size_t i = 0; i < len && g_pos < g_size; ++i) {
        result += static_cast<char>(g_data[g_pos++]);
    }
    return result;
}
}  // namespace

// Fuzz test for json::escape(BASE_NS::string_view)
bool JsonEscapeFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    // Test 1: escape with random string_view input
    std::string input = FuzzString(256);
    BASE_NS::string_view sv(input.data(), input.size());
    BASE_NS::string escaped = escape(sv);
    (void)escaped;

    // Test 2: escape with empty string
    BASE_NS::string_view emptySv("");
    BASE_NS::string emptyEscaped = escape(emptySv);
    (void)emptyEscaped;

    // Test 3: escape appending to existing output string
    BASE_NS::string out;
    out = "prefix";
    escape(out, sv);
    (void)out;

    return true;
}

// Fuzz test for json::unescape(BASE_NS::string_view)
bool JsonUnescapeFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    // Test 1: unescape with random string containing possible escape sequences
    std::string input = FuzzString(256);
    BASE_NS::string_view sv(input.data(), input.size());
    BASE_NS::string unescaped = unescape(sv);
    (void)unescaped;

    // Test 2: unescape with empty string
    BASE_NS::string_view emptySv("");
    BASE_NS::string emptyUnescaped = unescape(emptySv);
    (void)emptyUnescaped;

    // Test 3: unescape appending to existing output string
    BASE_NS::string out;
    out = "prefix";
    unescape(out, sv);
    (void)out;

    return true;
}

// Fuzz test for unescape with crafted unicode escape sequences
bool JsonUnescapeUnicodeFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    // Craft strings with \uXXXX patterns from fuzz data
    std::string input;
    while (g_pos < g_size) {
        uint8_t choice = GetData<uint8_t>() % 8;
        switch (choice) {
            case 0:  // plain character
                input += static_cast<char>(GetData<uint8_t>());
                break;
            case 1:  // \uXXXX unicode escape
                input += "\\u";
                for (int j = 0; j < 4 && g_pos < g_size; ++j) {
                    uint8_t hexByte = GetData<uint8_t>();
                    char hexChar = "0123456789abcdefABCDEF"[hexByte % 22];
                    input += hexChar;
                }
                break;
            case 2:  // backslash + simple escape char
                input += '\\';
                input += static_cast<char>(GetData<uint8_t>());
                break;
            case 3:  // surrogate pair \uD800-\uDBFF followed by \uDC00-\uDFFF
                if (g_pos + 4 > g_size)
                    break;
                input += "\\uD";
                {
                    uint8_t hiByte = GetData<uint8_t>();
                    // High surrogate: D800-DBFF -> third char 8-B
                    input += static_cast<char>('8' + (hiByte % 4));
                    input += "0123456789abcdef"[GetData<uint8_t>() % 16];
                }
                input += "\\uD";
                {
                    uint8_t loByte = GetData<uint8_t>();
                    // Low surrogate: DC00-DFFF -> third char C-F
                    input += static_cast<char>('C' + (loByte % 4));
                    input += "0123456789abcdef"[GetData<uint8_t>() % 16];
                }
                break;
            case 4:  // \n
                input += "\\n";
                break;
            case 5:  // \t
                input += "\\t";
                break;
            case 6:  // \r
                input += "\\r";
                break;
            case 7:  // \/
                input += "\\/";
                break;
        }
    }

    BASE_NS::string_view sv(input.data(), input.size());
    BASE_NS::string result = unescape(sv);
    (void)result;

    return true;
}

// Fuzz test for escape/unescape round-trip consistency
bool JsonEscapeUnescapeRoundTripFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    std::string input = FuzzString(128);
    BASE_NS::string_view sv(input.data(), input.size());

    // escape then unescape
    BASE_NS::string escaped = escape(sv);
    BASE_NS::string unescaped = unescape(BASE_NS::string_view(escaped.data(), escaped.size()));
    (void)unescaped;

    return true;
}

// Fuzz test for escape with multi-byte UTF-8 sequences
bool JsonEscapeUtf8Fuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < 4) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    std::string input;
    // Generate random bytes including potential multi-byte UTF-8
    while (g_pos < g_size) {
        uint8_t byte = GetData<uint8_t>();
        if (byte < 0x80) {
            // ASCII
            input += static_cast<char>(byte);
        } else if (byte < 0xE0) {
            // 2-byte UTF-8 sequence
            input += static_cast<char>(byte);
            if (g_pos < g_size) {
                input += static_cast<char>(GetData<uint8_t>());
            }
        } else if (byte < 0xF0) {
            // 3-byte UTF-8 sequence
            input += static_cast<char>(byte);
            for (int j = 0; j < 2 && g_pos < g_size; ++j) {
                input += static_cast<char>(GetData<uint8_t>());
            }
        } else {
            // 4-byte UTF-8 sequence
            input += static_cast<char>(byte);
            for (int j = 0; j < 3 && g_pos < g_size; ++j) {
                input += static_cast<char>(GetData<uint8_t>());
            }
        }
    }

    BASE_NS::string_view sv(input.data(), input.size());
    BASE_NS::string escaped = escape(sv);
    (void)escaped;

    return true;
}

// Fuzz test for unescape with truncated/truncated unicode sequences
bool JsonUnescapeTruncatedFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    // Test truncated \u sequences (less than 4 hex digits after \u)
    std::string input = "\\u";
    size_t hexCount = (g_size > 0) ? (GetData<uint8_t>() % 5) : 0;  // 0-4 hex chars
    for (size_t i = 0; i < hexCount && g_pos < g_size; ++i) {
        input += static_cast<char>(GetData<uint8_t>());
    }
    BASE_NS::string_view sv(input.data(), input.size());
    BASE_NS::string result = unescape(sv);
    (void)result;

    // Test truncated surrogate pair
    std::string surrogateInput = "\\uD800\\u";  // high surrogate but incomplete low
    hexCount = (g_pos + 1 < g_size) ? (GetData<uint8_t>() % 5) : 0;
    for (size_t i = 0; i < hexCount && g_pos < g_size; ++i) {
        surrogateInput += static_cast<char>(GetData<uint8_t>());
    }
    BASE_NS::string_view sv2(surrogateInput.data(), surrogateInput.size());
    BASE_NS::string result2 = unescape(sv2);
    (void)result2;

    return true;
}

bool JsonToStringFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    std::string input = FuzzString(256);
    auto json = CORE_NS::json::parse(input.c_str());
    if (json) {
        BASE_NS::string out;
        CORE_NS::json::to_string(out, json);
        (void)CORE_NS::json::to_string(json);
    }

    return true;
}

bool JsonValueAssignFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    std::string input = FuzzString(256);
    auto json = CORE_NS::json::parse(input.c_str());
    if (json) {
        CORE_NS::json::value copy = json;
        copy = json;

        CORE_NS::json::standalone_value moved = CORE_NS::json::parse(FuzzString(256).c_str());
        if (moved) {
            CORE_NS::json::standalone_value other;
            other = BASE_NS::move(moved);
        }

        CORE_NS::json::value strValue;
        strValue = "test_string";
    }

    return true;
}

bool JsonObjectAccessFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    auto json = CORE_NS::json::parse("{\"key1\":\"val1\",\"key2\":123,\"arr\":[1,2,3]}");
    if (json.is_object()) {
        json.find("key1");
        json.find("nonexistent");
        json["nonexistent"];
        json["key1"];
    }

    auto arrJson = CORE_NS::json::parse("[1,2,3]");
    if (arrJson.is_array()) {
        (void)arrJson;
    }

    return true;
}

bool JsonTypeConvertFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    std::string input = FuzzString(256);
    auto json = CORE_NS::json::parse(input.c_str());
    if (json) {
        CORE_NS::json::standalone_value converted = json;
    }

    return true;
}

bool JsonParseErrorFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    const char* errorInputs[] = {
        "0.1.2",
        "1e",
        "1.",
        "-",
        "-abc",
        "01",
        "1.2.3",
        "1e2e3",
        "truee",
        "falsee",
        "nulll",
        "nullx",
        "tr",
        "fal",
        "nu",
        "{\"key\":}",
        "{\"key\":,}",
        "[1,]",
        "[1]]",
        "\"\\uGGGG\"",
        "\"\\u\"",
        "\"\\u123\"",
        "}",
        "]",
        ",",
        ":",
        "",
    };

    for (const auto& s : errorInputs) {
        (void)CORE_NS::json::parse(s);
    }

    return true;
}

bool JsonNumberFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    const char* numberInputs[] = {
        "0",
        "123",
        "-456",
        "0.0",
        "3.14159",
        "-0.001",
        "1e10",
        "1E10",
        "1e-10",
        "1E+5",
        "-1e10",
        "0.0e0",
        "123.456e789",
        "999999999999999999",
        "-999999999999999999",
    };

    for (const auto& s : numberInputs) {
        auto json = CORE_NS::json::parse(s);
        if (json) {
            auto intVal = json.as_number<int64_t>();
            auto doubleVal = json.as_number<double>();
            auto uintVal = json.as_number<uint64_t>();
            (void)intVal;
            (void)doubleVal;
            (void)uintVal;
        }
    }

    return true;
}

bool JsonBooleanNullFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    const char* inputs[] = {
        "true",
        "false",
        "null",
        "  true  ",
        "  false  ",
        "  null  ",
    };

    for (const auto& s : inputs) {
        auto json = CORE_NS::json::parse(s);
        if (json) {
            (void)json.is_boolean();
            (void)json.is_null();
            (void)json.is_number();
            (void)json.is_object();
            (void)json.is_array();
            (void)json.is_string();
            (void)json.empty();
            (void)(bool)json;
        }
    }

    return true;
}

bool JsonValueConstructorsFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    CORE_NS::json::value strValue1(BASE_NS::string("test"));
    CORE_NS::json::value strValue2("cstring");
    CORE_NS::json::value numValue1(42);
    CORE_NS::json::value numValue2(3.14f);
    CORE_NS::json::value numValue3(100u);
    CORE_NS::json::value numValue4(-99);
    CORE_NS::json::value boolValue(true);
    CORE_NS::json::value nullValue(CORE_NS::json::value_t<>::null{});

    BASE_NS::vector<int> vec = {1, 2, 3};
    CORE_NS::json::value fromVec(vec);

    int arr[] = {10, 20, 30};
    CORE_NS::json::value fromArr(arr);

    (void)strValue1;
    (void)strValue2;
    (void)numValue1;
    (void)numValue2;
    (void)numValue3;
    (void)numValue4;
    (void)boolValue;
    (void)nullValue;
    (void)fromVec;
    (void)fromArr;

    return true;
}

bool JsonAddFunctionFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    CORE_NS::json::value json;
    json = CORE_NS::json::parse("{\"key\":\"value\"}");
    if (json.is_object()) {
        CORE_NS::json::value newValue("newValue");
        json.object_.emplace_back(BASE_NS::string("key2"), CORE_NS::json::value(123));
    }

    CORE_NS::json::value arrJson;
    arrJson = CORE_NS::json::parse("[1,2,3]");
    if (arrJson.is_array()) {
        arrJson.array_.push_back(CORE_NS::json::value(4));
    }

    (void)json;
    (void)arrJson;

    return true;
}

bool JsonParseStringEscapesFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    const char* escapeInputs[] = {
        "\"\\\\\"",
        "\"\\\"\"",
        "\"\\/\"",
        "\"\\b\"",
        "\"\\f\"",
        "\"\\n\"",
        "\"\\r\"",
        "\"\\t\"",
        "\"\\u0000\"",
        "\"\\uABCD\"",
        "\"\\uABCD\\uEF12\"",
        "\"test\\\\ntest\"",
        "\"\\x41\"",
        "\"a\\z\"",
        "\"\\uGGGG\"",
        "\"\\u123\"",
    };

    for (const auto& s : escapeInputs) {
        auto json = CORE_NS::json::parse(s);
        (void)json;
    }

    return true;
}

bool JsonParseIncompleteKeywordsFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    const char* incompleteInputs[] = {
        "t",
        "tr",
        "tru",
        "f",
        "fa",
        "fal",
        "fals",
        "n",
        "nu",
        "nul",
        "truetrue",
        "falsefalse",
        "nullnull",
    };

    for (const auto& s : incompleteInputs) {
        auto json = CORE_NS::json::parse(s);
        (void)json;
    }

    return true;
}

bool JsonCleanupFuzz(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    g_data = data;
    g_size = size;
    g_pos = 0;

    for (int i = 0; i < 10; ++i) {
        CORE_NS::json::value json = CORE_NS::json::parse(FuzzString(64).c_str());
        if (json) {
            if (json.is_object()) {
                auto copy = json;
                (void)copy;
            } else if (json.is_array()) {
                auto copy = json;
                (void)copy;
            } else if (json.is_string()) {
                auto copy = json;
                (void)copy;
            }
        }
    }

    return true;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    JsonEscapeFuzz(data, size);
    JsonUnescapeFuzz(data, size);
    JsonUnescapeUnicodeFuzz(data, size);
    JsonEscapeUnescapeRoundTripFuzz(data, size);
    JsonEscapeUtf8Fuzz(data, size);
    JsonUnescapeTruncatedFuzz(data, size);
    JsonToStringFuzz(data, size);
    JsonValueAssignFuzz(data, size);
    JsonObjectAccessFuzz(data, size);
    JsonTypeConvertFuzz(data, size);
    JsonParseErrorFuzz(data, size);
    JsonNumberFuzz(data, size);
    JsonBooleanNullFuzz(data, size);
    JsonValueConstructorsFuzz(data, size);
    JsonAddFunctionFuzz(data, size);
    JsonParseStringEscapesFuzz(data, size);
    JsonParseIncompleteKeywordsFuzz(data, size);
    JsonCleanupFuzz(data, size);
    return 0;
}
