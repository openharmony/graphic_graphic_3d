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

#include <base/containers/string.h>
#include <base/math/matrix.h>
#define JSON2_IMPL
#include <core/json/json2.h>

// #include "loader/json_util.h"
#include "test_framework.h"
#include "util/frustum_util.h"

namespace {
template <typename T>
void ParseEmpty()
{
    {
        BASE_NS::string_view str;
        auto value = CORE_NS::json2::parse<T>(str.data());
        EXPECT_TRUE(!value);
    }
    {
        BASE_NS::string_view str{"invalid"};
        auto value = CORE_NS::json2::parse<T>(str.data());
        EXPECT_TRUE(!value);
    }
}

/**
 * @tc.name: parseEmpty
 * @tc.desc: Tests for Parse Empty. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRCJson2Test, parseEmpty, testing::ext::TestSize.Level1)
{
    ParseEmpty<CORE_NS::json2::readonly_tag>();
    ParseEmpty<CORE_NS::json2::writable_tag>();
}

template <typename Tag>
void Constructor()
{
    using Json = CORE_NS::json2::value_t<Tag>;
    {
        auto value = Json();
        EXPECT_FALSE(value);

        auto copyConstructed = Json(value);
        EXPECT_FALSE(copyConstructed);

        Json copyAssigned(Json::null());
        copyAssigned = value;
        EXPECT_FALSE(copyAssigned);

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_FALSE(moveConstructed);

        Json moveAssigned(Json::null());
        moveAssigned = BASE_NS::move(value);
        EXPECT_FALSE(moveAssigned);
    }
    {
        auto value = Json(Json::object());
        EXPECT_TRUE(value && value.is_object());

        auto copyConstructed = Json(value);
        EXPECT_TRUE(copyConstructed && copyConstructed.is_object());

        Json copyAssigned(Json::null());
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_object());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_object());

        Json moveAssigned(Json::null());
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_object());
    }
    {
        auto value = Json(Json::array());
        EXPECT_TRUE(value && value.is_array());

        auto copyConstructed = Json(value);
        EXPECT_TRUE(copyConstructed && copyConstructed.is_array());

        Json copyAssigned(Json::null());
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_array());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_array());

        Json moveAssigned(Json::null());
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_array());
    }
    {
        auto value = CORE_NS::json2::parse<Tag>("\"\"");
        EXPECT_TRUE(value && value.is_string());

        auto copyConstructed = Json(value);
        EXPECT_TRUE(copyConstructed && copyConstructed.is_string());

        Json copyAssigned(Json::null());
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_string());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_string());

        Json moveAssigned(Json::null());
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_string());
    }
    {
        auto value = Json::boolean(true);
        EXPECT_TRUE(value && value.is_boolean());

        auto copyConstructed = Json(value);
        EXPECT_TRUE(copyConstructed && copyConstructed.is_boolean());

        Json copyAssigned(Json::null());
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_boolean());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_boolean());

        Json moveAssigned(Json::null());
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_boolean());
    }
    {
        auto value = Json::number(1.f);
        EXPECT_TRUE(value && value.is_number() && value.is_floating_point());

        auto copyConstructed = Json(value);
        EXPECT_TRUE(copyConstructed && copyConstructed.is_number() && copyConstructed.is_floating_point());

        Json copyAssigned(Json::null());
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_number() && copyAssigned.is_floating_point());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_number() && moveConstructed.is_floating_point());

        Json moveAssigned(Json::null());
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_number() && moveAssigned.is_floating_point());
    }
    {
        auto value = Json::number(1.0);
        EXPECT_TRUE(value && value.is_number() && value.is_floating_point());

        auto copyConstructed = Json(value);
        EXPECT_TRUE(copyConstructed && copyConstructed.is_number() && copyConstructed.is_floating_point());

        Json copyAssigned(Json::null());
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_number() && copyAssigned.is_floating_point());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_number() && moveConstructed.is_floating_point());

        Json moveAssigned(Json::null());
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_number() && moveAssigned.is_floating_point());
    }
    {
        auto value = Json::number(1);
        EXPECT_TRUE(value && value.is_number() && value.is_signed_int());

        auto copyConstructed = Json(value);
        EXPECT_TRUE(copyConstructed && copyConstructed.is_number() && copyConstructed.is_signed_int());

        Json copyAssigned(Json::null());
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_number() && copyAssigned.is_signed_int());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_number() && moveConstructed.is_signed_int());

        Json moveAssigned(Json::null());
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_number() && moveAssigned.is_signed_int());
    }
    {
        auto value = Json::number(1U);
        EXPECT_TRUE(value && value.is_number() && value.is_unsigned_int());

        auto copyConstructed = Json(value);
        EXPECT_TRUE(copyConstructed && copyConstructed.is_number() && copyConstructed.is_unsigned_int());

        Json copyAssigned(Json::null());
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_number() && copyAssigned.is_unsigned_int());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_number() && moveConstructed.is_unsigned_int());

        Json moveAssigned(Json::null());
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_number() && moveAssigned.is_unsigned_int());
    }
}

/**
 * @tc.name: constructors
 * @tc.desc: Tests for Constructors. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRCJson2Test, constructors, testing::ext::TestSize.Level1)
{
    Constructor<CORE_NS::json2::readonly_tag>();
    Constructor<CORE_NS::json2::writable_tag>();
}

template <typename T>
void ParseTypes()
{
    // check basic json types
    {
        constexpr const char* strings[] = {
            "\"\"", "\"foo\"", "\"\\\"\"", "\"\\b\"", "\"\\f\"", "\"\\n\"", "\"\\r\"", "\"\\t\"", "\"\\u1234\""};
        for (const auto str : strings) {
            auto value = CORE_NS::json2::parse<T>(str);
            EXPECT_TRUE(value && value.is_string());
        }
        BASE_NS::string_view str{"\"hello\""};
        auto value = CORE_NS::json2::parse<T>(str.data());
        ASSERT_TRUE(value && value.is_string());
        EXPECT_EQ(value.as_string(), "hello");
    }
    {
        constexpr const char* numbers[] = {"0",
            "-0",
            "12",
            "-30",
            "0.0",
            "-0.0",
            "1.0",
            "-1.0",
            "0e0",
            "0E0",
            "0e+0",
            "0E+0",
            "0e-0",
            "0E-0",
            "-0e0",
            "-0E0",
            "-0e+0",
            "-0E+0",
            "-0e-0",
            "-0E-0",
            "-2e1",
            "2E1",
            "2e+1",
            "2E+1",
            "2e-10",
            "2E-100",
            "-2e10",
            "-2E10",
            "-2e+100",
            "-2E+10",
            "-2e-10",
            "-2E-10"};
        for (const auto str : numbers) {
            auto value = CORE_NS::json2::parse<T>(str);
            EXPECT_TRUE(value && value.is_number());
        }

        auto value = CORE_NS::json2::parse<T>("42");
        ASSERT_TRUE(value && value.is_number() && value.is_unsigned_int());
        EXPECT_EQ(value.template as_number<uint32_t>(), 42U);

        value = CORE_NS::json2::parse<T>("-42");
        ASSERT_TRUE(value && value.is_number() && value.is_signed_int());
        EXPECT_EQ(value.template as_number<int32_t>(), -42);

        value = CORE_NS::json2::parse<T>("42.25");
        ASSERT_TRUE(value && value.is_number() && value.is_floating_point());
        EXPECT_EQ(value.template as_number<float>(), 42.25f);

        value = CORE_NS::json2::parse<T>("-42.25");
        ASSERT_TRUE(value && value.is_number() && value.is_floating_point());
        EXPECT_EQ(value.template as_number<float>(), -42.25f);

        value = CORE_NS::json2::parse<T>("foo");
        ASSERT_FALSE(value);
        ASSERT_FALSE(value.is_number());
        ASSERT_FALSE(value.is_floating_point());
    }
    {
        BASE_NS::string_view str{"false"};
        auto value = CORE_NS::json2::parse<T>(str.data());
        ASSERT_TRUE(value && value.is_boolean());
        EXPECT_EQ(value.as_boolean(), false);
    }
    {
        BASE_NS::string_view str{"true"};
        const auto value = CORE_NS::json2::parse<T>(str.data());
        ASSERT_TRUE(value && value.is_boolean());
        EXPECT_EQ(value.as_boolean(), true);
    }
    {
        BASE_NS::string_view str{"null"};
        auto value = CORE_NS::json2::parse<T>(str.data());
        ASSERT_TRUE(value && value.is_null());
    }
    {
        BASE_NS::string_view str{"{}"};
        auto value = CORE_NS::json2::parse<T>(str.data());
        EXPECT_TRUE(value && value.is_object());
    }
    {
        BASE_NS::string_view str{"[]"};
        auto value = CORE_NS::json2::parse<T>(str.data());
        EXPECT_TRUE(value && value.is_array());
    }
}

/**
 * @tc.name: parseTypes
 * @tc.desc: Tests for Parse Types. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRCJson2Test, parseTypes, testing::ext::TestSize.Level1)
{
    ParseTypes<CORE_NS::json2::readonly_tag>();
    ParseTypes<CORE_NS::json2::writable_tag>();
}

template <typename T>
void ParseInvalid()
{
    {
        constexpr const char* objects[] = {"",
            "{",
            "{ ",
            "{foo",
            "{\"foo",
            "{\"foo\"",
            "{\"foo\"42",
            "{\"foo\":",
            "{\"foo\": ",
            "{\"foo\": }",
            "{\"foo\":42",
            "{\"foo\":42,",
            "{\"foo\":42,bar",
            "{\"foo\":42,\"bar",
            "{\"foo\":42,\"bar\"",
            "{\"foo\":42,\"bar\"",
            "{\"foo\":42,\"bar\":24",
            "{} {",
            "{} }",
            "{} [",
            "{} ]",
            "}"};

        for (const auto str : objects) {
            auto value = CORE_NS::json2::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
    {
        constexpr const char* arrays[] = {"[",
            "[foo]",
            "[\"foo\":]",
            "[\"foo]",
            "[1,]",
            "[\"foo\":42,nul,true]",
            "[] [",
            "[] ]",
            "[] {",
            "[] }",
            "1, 2"};
        for (const auto str : arrays) {
            auto value = CORE_NS::json2::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
    {
        constexpr const char* strings[] = {
            "\"f", "\"\\", "\"\\a\"", "\"\"\"", "\"\b\"", "\"\f\"", "\"\n\"", "\"\r\"", "\"\t\"", "\"\\u00\""};
        for (const auto str : strings) {
            auto value = CORE_NS::json2::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
    {
        constexpr const char* numbers[] = {"01",
            "0a",
            "0.0a",
            "0.0e",
            "0.0E-",
            "-a",
            "-01",
            "-0a",
            "-0.0a",
            "-0.0e",
            "-0.0E-",
            "1a",
            "1.0a",
            "1.0e",
            "1.0E-",
            "-1a",
            "-1.0a",
            "-1.0e",
            "-1.0E-",
            "0."};
        for (const auto str : numbers) {
            auto value = CORE_NS::json2::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
    {
        constexpr const char* nulls[] = {"a", "n", "nu", "nult", "nulll"};
        for (const auto str : nulls) {
            auto value = CORE_NS::json2::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
    {
        constexpr const char* booleans[] = {"t", "tr", "truu", "truee", "f", "fa", "falss", "falsee"};
        for (const auto str : booleans) {
            auto value = CORE_NS::json2::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
}

/**
 * @tc.name: invalid
 * @tc.desc: Tests for Invalid. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRCJson2Test, invalid, testing::ext::TestSize.Level1)
{
    ParseInvalid<CORE_NS::json2::readonly_tag>();
    ParseInvalid<CORE_NS::json2::writable_tag>();
}

UNIT_TEST(SRCJson2Test, escapedLiteral, testing::ext::TestSize.Level1)
{
    using namespace CORE_NS::json2::literals;

    auto s = "\\n"_escaped;
    static_assert(BASE_NS::is_same_v<decltype(s), CORE_NS::json2::escaped_string_view>);

    EXPECT_EQ("\\n", s.escaped);
    EXPECT_EQ("\n", s.value());
}

template <typename T>
void ToString()
{
    BASE_NS::string_view str{"[{\"object\":{}, \"array\":[], \"string\":\"value\", \"number\":42.25, \"boolean\":true, "
                             "\"null\":null}, [], \"hello\", 123.456, -123.456, -235, 564, false, true, null]"};
    const CORE_NS::json2::value_t<T> inValue = CORE_NS::json2::parse<T>(str.data());
    EXPECT_TRUE(inValue);
    BASE_NS::string out = CORE_NS::json2::to_string<T>(inValue);
    const CORE_NS::json2::value_t<T> outValue = CORE_NS::json2::parse<T>(out.data());
    ASSERT_TRUE(inValue.is_array());
    ASSERT_TRUE(outValue.is_array());
    ASSERT_EQ(inValue, outValue);
}
}  // namespace

/**
 * @tc.name: toString
 * @tc.desc: Tests for To String. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRCJson2Test, toString, testing::ext::TestSize.Level1)
{
    ToString<CORE_NS::json2::readonly_tag>();
    ToString<CORE_NS::json2::writable_tag>();

    {
        // parse a JSON containing Unicode characters, decode the escaped characters, and convert back to JSON. the end
        // result should be the same as the initial JSON.
        constexpr const BASE_NS::string_view in{R"({"\uD835\uDD77\uD835\uDD9A\uD835\uDD92\uD835\uDD8A":"\u2661"})"};
        auto inValue = CORE_NS::json2::parse<CORE_NS::json2::writable_tag>(in);
        ASSERT_TRUE(inValue.is_object());
        ASSERT_EQ(1U, inValue.as_object().size());
        auto& pair = inValue.as_object()[0];
        pair.escapedKey = pair.escapedKey;
        pair.value = CORE_NS::json2::value::string(pair.value.as_string());
        const BASE_NS::string out = CORE_NS::json2::to_string(inValue);
        EXPECT_EQ(in, out);
    }
}

UNIT_TEST(SRCJson2Test, assignStringViewToStandalone, testing::ext::TestSize.Level1)
{
    CORE_NS::json2::value value;
    value = CORE_NS::json2::value::string("hello\"world");

    ASSERT_TRUE(value.is_string());
    EXPECT_EQ("hello\\\"world", value.as_escaped_string().escaped);
    EXPECT_EQ("hello\"world", value.as_escaped_string().value());
    EXPECT_EQ("hello\"world", value.as_string());
}
UNIT_TEST(SRCJson2Test, assignEscapedStringViewToStandalone, testing::ext::TestSize.Level1)
{
    using namespace CORE_NS::json2::literals;

    CORE_NS::json2::value value;
    value = CORE_NS::json2::value::escaped_string("hello\\\"world"_escaped);

    ASSERT_TRUE(value.is_string());
    EXPECT_EQ("hello\\\"world", value.as_escaped_string().escaped);
    EXPECT_EQ("hello\"world", value.as_escaped_string().value());
    EXPECT_EQ("hello\"world", value.as_string());
}
UNIT_TEST(SRCJson2Test, assignEscapedStringViewToStandalone2, testing::ext::TestSize.Level1)
{
    CORE_NS::json2::value value;
    value = CORE_NS::json2::value::escaped_string("hello\\\"world");

    ASSERT_TRUE(value.is_string());
    EXPECT_EQ("hello\\\"world", value.as_escaped_string().escaped);
    EXPECT_EQ("hello\"world", value.as_escaped_string().value());
    EXPECT_EQ("hello\"world", value.as_string());
}

/**
 * @tc.name: standaloneJson
 * @tc.desc: Tests for Standalone Json. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRCJson2Test, standaloneJson, testing::ext::TestSize.Level1)
{
    BASE_NS::string_view str{
        "[{\"foo\":\"bar\"}, [{\"greeting\":\"Hello World\"}], \"hello\", 123.456, 42, -42, false, true, null]"};
    const CORE_NS::json2::view inValue = CORE_NS::json2::parse(str);
    ASSERT_TRUE(inValue.is_array());

    auto standalone = CORE_NS::json2::value(inValue);
    ASSERT_TRUE(standalone.is_array());
    auto readonlyIt = inValue.as_array().cbegin();
    for (const auto& value : standalone.as_array()) {
        ASSERT_EQ(value.type(), readonlyIt->type());
        switch (value.type()) {
            case CORE_NS::json2::type::uninitialized:
                ASSERT_FALSE(value);
                break;
            case CORE_NS::json2::type::object: {
                EXPECT_EQ(value.as_object().size(), readonlyIt->as_object().size());
                break;
            }
            case CORE_NS::json2::type::array:
                EXPECT_EQ(value.as_array().size(), readonlyIt->as_array().size());
                break;
            case CORE_NS::json2::type::floating_point:
                EXPECT_DOUBLE_EQ(value.as_strict_floating_point(), readonlyIt->as_strict_floating_point());
                break;
            case CORE_NS::json2::type::signed_int:
                EXPECT_EQ(value.as_strict_signed_int(), readonlyIt->as_strict_signed_int());
                break;
            case CORE_NS::json2::type::unsigned_int:
                EXPECT_EQ(value.as_strict_unsigned_int(), readonlyIt->as_strict_unsigned_int());
                break;
            case CORE_NS::json2::type::boolean:
                EXPECT_EQ(value.as_boolean(), readonlyIt->as_boolean());
                break;
            case CORE_NS::json2::type::null:
                break;
            default:
                break;
        }
        ++readonlyIt;
    }
}

/**
 * @tc.name: unescape
 * @tc.desc: Tests for Unescape. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRCJson2Test, unescape, testing::ext::TestSize.Level1)
{
    {
        constexpr BASE_NS::string_view escaped = "foo\\\"bar";
        constexpr BASE_NS::string_view expected = u8"foo\"bar";
        const auto unescaped = CORE_NS::json2::Detail::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }

    {
        constexpr BASE_NS::string_view escaped = "\\\\\\/\\b\\n\\t\\r\\f";
        constexpr BASE_NS::string_view expected = u8"\\/\b\n\t\r\f";
        const auto unescaped = CORE_NS::json2::Detail::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }

    {
        constexpr BASE_NS::string_view escaped = "\\u573a\\u666f";
        constexpr BASE_NS::string_view expected = u8"\u573a\u666f";
        const auto unescaped = CORE_NS::json2::Detail::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    // UTF-8 examples from Wikipedia (https://en.wikipedia.org/wiki/UTF-8)
    {
        constexpr BASE_NS::string_view escaped = "\\u0024";
        constexpr BASE_NS::string_view expected = u8"\u0024";
        const auto unescaped = CORE_NS::json2::Detail::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    {
        constexpr BASE_NS::string_view escaped = "\\u00A3";
        constexpr BASE_NS::string_view expected = u8"\u00A3";
        const auto unescaped = CORE_NS::json2::Detail::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    {
        constexpr BASE_NS::string_view escaped = "\\u0939";
        constexpr BASE_NS::string_view expected = u8"\u0939";
        const auto unescaped = CORE_NS::json2::Detail::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    {
        constexpr BASE_NS::string_view escaped = "\\u20AC";
        constexpr BASE_NS::string_view expected = u8"\u20AC";
        const auto unescaped = CORE_NS::json2::Detail::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    {
        constexpr BASE_NS::string_view escaped = "\\uD55C";
        constexpr BASE_NS::string_view expected = u8"\uD55C";
        const auto unescaped = CORE_NS::json2::Detail::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    {
        constexpr BASE_NS::string_view escaped = "\\uD800\\uDF48";
        constexpr BASE_NS::string_view expected = u8"\U00010348";
        const auto unescaped = CORE_NS::json2::Detail::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    // UTF-16 surrogate pair example from Wikipedia (https://en.wikipedia.org/wiki/JSON)
    {
        constexpr BASE_NS::string_view escaped = "\\uD83D\\uDE10";
        constexpr BASE_NS::string_view expected = u8"\U0001F610";
        const auto unescaped = CORE_NS::json2::Detail::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    // Examples from RFC 3629
    {
        constexpr BASE_NS::string_view escaped = "\\u0041\\u2262\\u0391\\u002E";
        constexpr BASE_NS::string_view expected = u8"\u0041\u2262\u0391\u002E";
        const auto unescaped = CORE_NS::json2::Detail::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }

    {
        constexpr BASE_NS::string_view escaped = "\\uD55C\\uAD6D\\uC5B4";
        constexpr BASE_NS::string_view expected = u8"\uD55C\uAD6D\uC5B4";
        const auto unescaped = CORE_NS::json2::Detail::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    {
        constexpr BASE_NS::string_view escaped = "\\u65E5\\u672C\\u8A9E";
        constexpr BASE_NS::string_view expected = u8"\u65E5\u672C\u8A9E";
        const auto unescaped = CORE_NS::json2::Detail::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    {
        constexpr BASE_NS::string_view escaped = "\\uD84C\\uDFB4";
        constexpr BASE_NS::string_view expected = u8"\U000233B4";
        const auto unescaped = CORE_NS::json2::Detail::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
}

/**
 * @tc.name: escape
 * @tc.desc: Tests for Escape. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRCJson2Test, escape, testing::ext::TestSize.Level1)
{
    {
        constexpr BASE_NS::string_view unescaped = u8"\"";
        constexpr BASE_NS::string_view expected = "\\\"";
        const auto escaped = CORE_NS::json2::Detail::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }

    {
        constexpr BASE_NS::string_view unescaped = u8"\\/\b\n\t\r\f";
        constexpr BASE_NS::string_view expected = "\\\\/\\b\\n\\t\\r\\f";
        const auto escaped = CORE_NS::json2::Detail::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }

    {
        constexpr BASE_NS::string_view unescaped = u8"\u573a\u666f";
        constexpr BASE_NS::string_view expected = "\\u573A\\u666F";
        const auto escaped = CORE_NS::json2::Detail::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }

    // UTF-8 examples from Wikipedia (https://en.wikipedia.org/wiki/UTF-8)
    {
        constexpr BASE_NS::string_view unescaped = u8"\u0024";
        constexpr BASE_NS::string_view expected = "$";
        const auto escaped = CORE_NS::json2::Detail::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\u00A3";
        constexpr BASE_NS::string_view expected = "\\u00A3";
        const auto escaped = CORE_NS::json2::Detail::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\u0939";
        constexpr BASE_NS::string_view expected = "\\u0939";
        const auto escaped = CORE_NS::json2::Detail::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\u20AC";
        constexpr BASE_NS::string_view expected = "\\u20AC";
        const auto escaped = CORE_NS::json2::Detail::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\uD55C";
        constexpr BASE_NS::string_view expected = "\\uD55C";
        const auto escaped = CORE_NS::json2::Detail::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\U00010348";
        constexpr BASE_NS::string_view expected = "\\uD800\\uDF48";
        const auto escaped = CORE_NS::json2::Detail::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }

    // UTF-16 surrogate pair example from Wikipedia (https://en.wikipedia.org/wiki/JSON)
    {
        constexpr BASE_NS::string_view unescaped = u8"\U0001F610";
        constexpr BASE_NS::string_view expected = "\\uD83D\\uDE10";
        const auto escaped = CORE_NS::json2::Detail::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }

    // Examples from RFC 3629
    {
        constexpr BASE_NS::string_view unescaped = u8"\u0041\u2262\u0391\u002E";
        constexpr BASE_NS::string_view expected = "A\\u2262\\u0391.";
        const auto escaped = CORE_NS::json2::Detail::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\uD55C\uAD6D\uC5B4";
        constexpr BASE_NS::string_view expected = "\\uD55C\\uAD6D\\uC5B4";
        const auto escaped = CORE_NS::json2::Detail::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\u65E5\u672C\u8A9E";
        constexpr BASE_NS::string_view expected = "\\u65E5\\u672C\\u8A9E";
        const auto escaped = CORE_NS::json2::Detail::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\U000233B4";
        constexpr BASE_NS::string_view expected = "\\uD84C\\uDFB4";
        const auto escaped = CORE_NS::json2::Detail::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
}
UNIT_TEST(SRCJson2Test, extraAfterString, testing::ext::TestSize.Level1)
{
    const auto* s = R"("\\"a")";
    auto res = CORE_NS::json2::parse(s);
    if (res) {
        EXPECT_STREQ("", res.as_string().c_str());
    }
    ASSERT_FALSE(res);
}
UNIT_TEST(SRCJson2Test, unfinishedArray, testing::ext::TestSize.Level1)
{
    const auto* s = "[";
    auto res = CORE_NS::json2::parse(s);
    ASSERT_FALSE(res);
}
UNIT_TEST(SRCJson2Test, unfinishedObject, testing::ext::TestSize.Level1)
{
    const auto* s = "{";
    auto res = CORE_NS::json2::parse(s);
    ASSERT_FALSE(res);
}
UNIT_TEST(SRCJson2Test, unfinishedString, testing::ext::TestSize.Level1)
{
    const auto* s = "\"";
    auto res = CORE_NS::json2::parse(s);
    ASSERT_EQ(res.type(), CORE_NS::json2::type::uninitialized);
    ASSERT_FALSE(res);
}
UNIT_TEST(SRCJson2Test, parseAndToString, testing::ext::TestSize.Level1)
{
    auto s = R"({"he\"llo\n":"wor\tld\t"})";
    auto parsedView = CORE_NS::json2::parse(s);

    ASSERT_TRUE(parsedView);
    ASSERT_TRUE(parsedView.is_object());
    ASSERT_EQ(1, parsedView.as_object().size());

    EXPECT_STREQ("he\"llo\n", parsedView.as_object()[0].key().c_str());
    EXPECT_STREQ(R"(he\"llo\n)", BASE_NS::string{parsedView.as_object()[0].escapedKey.escaped}.c_str());
    EXPECT_STREQ(s, CORE_NS::json2::to_string(parsedView).c_str());
}
UNIT_TEST(SRCJson2Test, convenienceObjectInitializer, testing::ext::TestSize.Level1)
{
    using namespace CORE_NS::json2::literals;
    using JSON = CORE_NS::json2::value;

    const CORE_NS::json2::escaped_string a = "asd"_escaped;
    EXPECT_EQ("asd", a.value());
    const JSON b = CORE_NS::json2::value::escaped_string("asd"_escaped);
    EXPECT_EQ("asd", b.as_string());
    const CORE_NS::json2::escaped_string c = "asd"_escaped_value;
    EXPECT_EQ("asd", c.value());
    const JSON d = JSON::escaped_string("asd"_escaped_value);
    EXPECT_EQ("asd", d.as_string());

    JSON::object_t o{
        {"k1", JSON::string("v1\t")},
        {"k2"_escaped, JSON::string("v2\t")},
        {"k3"_escaped, JSON::string("v3\t")},
        {"k4"_escaped, JSON::escaped_string("v4\\t"_escaped)},
        {"k5", JSON::escaped_string("v5\\t"_escaped)},
    };

    auto jsonValue = JSON::object(BASE_NS::move(o));
    EXPECT_STREQ("v1\t", jsonValue.find("k1")->as_string().c_str());
    EXPECT_STREQ("v2\t", jsonValue.find("k2")->as_string().c_str());
    EXPECT_STREQ("v3\t", jsonValue.find("k3")->as_string().c_str());
    EXPECT_STREQ("v4\t", jsonValue.find("k4")->as_string().c_str());
    EXPECT_STREQ("v5\t", jsonValue.find("k5")->as_string().c_str());
}

UNIT_TEST(SRCJson2Test, toStringAndParse, testing::ext::TestSize.Level1)
{
    using JSON = CORE_NS::json2::value;

    const JSON o = JSON::object({
        {"k\"1", JSON::string("1")},
        {"k\n2", JSON::escaped_string("\\\\r\\\\na")},
    });

    auto string = CORE_NS::json2::to_string(o);

    auto parsedValue = CORE_NS::json2::parse(string);
    ASSERT_TRUE(parsedValue);
    ASSERT_TRUE(parsedValue.is_object());
    ASSERT_EQ(2, parsedValue.as_object().size());

    ASSERT_EQ(o, parsedValue);
}
/**
 * @tc.name: boolean
 * @tc.desc: Tests for Boolean. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRCJson2Test, boolean, testing::ext::TestSize.Level1)
{
    using JSON = CORE_NS::json2::value;
    const char* str = "{\"checked\":false}";
    auto value = CORE_NS::json2::parse<CORE_NS::json2::writable_tag>(str);
    EXPECT_TRUE(value.is_object());
    auto& checked = value["checked"];
    ASSERT_TRUE(checked.is_boolean());
    EXPECT_FALSE(checked.as_boolean());
    // check that assigning a boolean results in a boolean.
    checked = JSON::boolean(true);

    EXPECT_TRUE(checked.is_boolean());
    EXPECT_TRUE(checked.as_boolean());
    checked = JSON::string("foo");
    EXPECT_TRUE(checked.is_string());

    checked = JSON::boolean(false);
    ASSERT_TRUE(checked.is_boolean());
    EXPECT_FALSE(checked.as_boolean());
}

/**
 * @tc.name: JsonUtil
 * @tc.desc: Tests for Json Util. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRCJson2Test, JsonUtil, testing::ext::TestSize.Level1)
{
    BASE_NS::string_view str{"{\"number\":1984, \"boolean\": true, \"charstr\": \"abc\"}"};
    const CORE_NS::json2::view jsonData = CORE_NS::json2::parse(str.data());
    float num = .0f;
    bool boolean = false;
    BASE_NS::basic_string<char> charstr;
    BASE_NS::string_view numEle{"number"};
    BASE_NS::string numError = "numError";
    BASE_NS::string_view boolEle{"boolean"};
    BASE_NS::string boolError = "boolError";
    BASE_NS::string_view strEle{"charstr"};
    BASE_NS::string strError = "strError";

    BASE_NS::string_view arrayStr{
        "[{\"foo\":\"bar\"}, [{\"greeting\":\"Hello World\"}], \"hello\", 123.456, false, true, null]"};
    const CORE_NS::json2::view arrayJson = CORE_NS::json2::parse(arrayStr.data());
    BASE_NS::string arrayCon[2] = {"ele1", "ele2"};

    /*BASE_NS::string_view floatStr{
        "1.0"
    };
    const CORE_NS::json2::value floatJson = CORE_NS::json2::parse(floatStr.data());
    double df = floatJson.as_number<double>();

    BASE_NS::string_view intStr{
        "1"
    };
    const CORE_NS::json2::value intJson = CORE_NS::json2::parse(intStr.data());
    double di = intJson.as_number<double>();

    BASE_NS::string_view signIntStr{
        "-1"
    };
    const CORE_NS::json2::value signIntJson = CORE_NS::json2::parse(signIntStr.data());
    double dsi = signIntJson.as_number<double>();*/
    constexpr const char* numbers[18] = {"01",
        "0a",
        "0.0a",
        "0.0e",
        "0.0E-",
        "-01",
        "-0a",
        "-0.0a",
        "-0.0e",
        "-0.0E-",
        "1a",
        "1.0a",
        "1.0e",
        "1.0E-",
        "-1a",
        "-1.0a",
        "-1.0e",
        "-1.0E-"};
    for (const auto s : numbers) {
        auto value = CORE_NS::json2::parse(s);
        EXPECT_EQ(sizeof(value.as_number<double>()), 8);
    }
}

/**
 * @tc.name: frustumTest
 * @tc.desc: Tests for Frustum Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRCJson2Test, frustumTest, testing::ext::TestSize.Level1)
{
    BASE_NS::string a = "hello";
    BASE_NS::string_view k = a;

    CORE_NS::FrustumUtil f;
    const CORE_NS::FrustumUtil conF;
    const float matData[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    BASE_NS::Math::Mat4X4 testMat(matData);
    f.CreateFrustum(testMat);
    conF.CreateFrustum(testMat);

    CORE_NS::Frustum frus;
    EXPECT_TRUE(f.SphereFrustumCollision(frus, {1, 2, 3}, 1.0f));

    EXPECT_TRUE(f.GetInterface(CORE_NS::IFrustumUtil::UID));
    EXPECT_TRUE(f.GetInterface(CORE_NS::IInterface::UID));
    EXPECT_FALSE(f.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));
    EXPECT_TRUE(conF.GetInterface(CORE_NS::IFrustumUtil::UID));
    EXPECT_TRUE(conF.GetInterface(CORE_NS::IInterface::UID));
    EXPECT_FALSE(conF.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));

    f.Ref();
    f.Unref();
}

// Test edge cases for numeric parsing
UNIT_TEST(SRCJson2Test, numericEdgeCases, testing::ext::TestSize.Level1)
{
    // Test maximum/minimum values
    {
        auto value = CORE_NS::json2::parse("18446744073709551615");  // UINT64_MAX
        ASSERT_TRUE(value && value.is_unsigned_int());
        EXPECT_EQ(value.as_strict_unsigned_int(), UINT64_MAX);
    }

    {
        auto value = CORE_NS::json2::parse("-9223372036854775808");  // INT64_MIN
        ASSERT_TRUE(value && value.is_signed_int());
        EXPECT_EQ(value.as_strict_signed_int(), INT64_MIN);
    }

    {
        auto value = CORE_NS::json2::parse("2");  // Any positive number becomes unsigned
        ASSERT_EQ(CORE_NS::json2::type::unsigned_int, value.type());
        EXPECT_EQ(value.as_strict_unsigned_int(), 2);
    }
    {
        auto value = CORE_NS::json2::parse("9223372036854775807");  // INT64_MAX
        ASSERT_EQ(CORE_NS::json2::type::unsigned_int, value.type());
        EXPECT_EQ(value.as_strict_unsigned_int(), INT64_MAX);
    }

    // Test scientific notation edge cases
    {
        auto value = CORE_NS::json2::parse("1e308");
        ASSERT_TRUE(value && value.is_floating_point());
    }

    {
        auto value = CORE_NS::json2::parse("1e-308");
        ASSERT_TRUE(value && value.is_floating_point());
    }
}

UNIT_TEST(SRCJson2Test, conversionFromReadableToWritableUnintialized, testing::ext::TestSize.Level1)
{
    CORE_NS::json2::value writable;
    ASSERT_TRUE(writable.is_uninitialized());

    auto readable = CORE_NS::json2::view(writable);
    ASSERT_TRUE(readable.is_uninitialized());

    auto writable2 = CORE_NS::json2::value(readable);
    ASSERT_TRUE(writable2.is_uninitialized());
}

UNIT_TEST(SRCJson2Test, conversionFromReadableToWritableString, testing::ext::TestSize.Level1)
{
    auto writable = CORE_NS::json2::value::string("test");
    ASSERT_TRUE(writable.is_string());
    ASSERT_STREQ("test", writable.as_string().c_str());

    auto readable = CORE_NS::json2::view(writable);
    ASSERT_TRUE(readable.is_string());
    ASSERT_STREQ("test", readable.as_string().c_str());

    auto writable2 = CORE_NS::json2::value(readable);
    ASSERT_TRUE(writable2.is_string());
    ASSERT_STREQ("test", writable2.as_string().c_str());
}

// Test nested structures
UNIT_TEST(SRCJson2Test, deeplyNestedStructures, testing::ext::TestSize.Level1)
{
    // Deeply nested arrays
    {
        const char* str = "[[[[[[1]]]]]]";
        auto value = CORE_NS::json2::parse(str);
        ASSERT_TRUE(value && value.is_array());
        auto* current = &value;
        for (int i = 0; i < 5; ++i) {
            ASSERT_TRUE(current->is_array() && !current->as_array().empty());
            current = &current->as_array()[0];
        }
        ASSERT_TRUE(current->is_array() && current->as_array().size() == 1);
        EXPECT_TRUE(current->as_array()[0].is_unsigned_int());
    }

    // Deeply nested objects
    {
        const char* str = R"({"a":{"b":{"c":{"d":{"e":"value"}}}}})";
        auto value = CORE_NS::json2::parse(str);
        ASSERT_TRUE(value && value.is_object());

        auto* nested = value.find("a");
        ASSERT_NE(nested, nullptr);
        nested = nested->find("b");
        ASSERT_NE(nested, nullptr);
        nested = nested->find("c");
        ASSERT_NE(nested, nullptr);
        nested = nested->find("d");
        ASSERT_NE(nested, nullptr);
        nested = nested->find("e");
        ASSERT_NE(nested, nullptr);
        ASSERT_TRUE(nested->is_string());
        EXPECT_EQ(nested->as_string(), "value");
    }
}

// Test object find operations
UNIT_TEST(SRCJson2Test, objectFindOperations, testing::ext::TestSize.Level1)
{
    using namespace CORE_NS::json2::literals;

    const char* str = R"({"key1":"value1","key2":"value2","key3":null})";
    auto value = CORE_NS::json2::parse(str);

    // Test find_unescaped
    {
        auto* found = value.find("key1");
        ASSERT_NE(found, nullptr);
        EXPECT_TRUE(found->is_string());
        EXPECT_EQ(found->as_string(), "value1");
    }

    // Test finding non-existent key
    {
        auto* found = value.find("nonexistent");
        EXPECT_EQ(found, nullptr);
    }

    // Test find_escaped
    {
        auto* found = value.find("key2"_escaped);
        ASSERT_NE(found, nullptr);
        EXPECT_TRUE(found->is_string());
    }

    // Test finding null value
    {
        auto* found = value.find("key3");
        ASSERT_NE(found, nullptr);
        EXPECT_TRUE(found->is_null());
    }
}

// Test array operations
UNIT_TEST(SRCJson2Test, arrayOperations, testing::ext::TestSize.Level1)
{
    const char* str = R"([1, "string", true, null, {"nested": "object"}, [1, 2, 3]])";
    auto value = CORE_NS::json2::parse(str);

    ASSERT_TRUE(value && value.is_array());
    ASSERT_EQ(value.as_array().size(), 6u);

    EXPECT_TRUE(value.as_array()[0].is_unsigned_int());
    EXPECT_TRUE(value.as_array()[1].is_string());
    EXPECT_TRUE(value.as_array()[2].is_boolean());
    EXPECT_TRUE(value.as_array()[3].is_null());
    EXPECT_TRUE(value.as_array()[4].is_object());
    EXPECT_TRUE(value.as_array()[5].is_array());
}

// Test whitespace handling
UNIT_TEST(SRCJson2Test, whitespaceHandling, testing::ext::TestSize.Level1)
{
    const char* withWhitespace = R"(
        {
            "key1"  :  "value1"  ,
            "key2"  :  123  ,
            "array" :  [ 1 , 2 , 3 ]
        }
    )";

    auto value = CORE_NS::json2::parse(withWhitespace);
    ASSERT_TRUE(value && value.is_object());
    ASSERT_EQ(value.as_object().size(), 3u);
}

// Test special string characters
UNIT_TEST(SRCJson2Test, specialStringCharacters, testing::ext::TestSize.Level1)
{
    // Test control characters
    {
        const char* str = R"("\u0000\u0001\u001F")";
        auto value = CORE_NS::json2::parse(str);
        ASSERT_TRUE(value && value.is_string());
    }

    // Test mixed escapes
    {
        const char* str = R"("Line1\nLine2\tTabbed\rReturn\\Backslash\"Quote")";
        auto value = CORE_NS::json2::parse(str);
        ASSERT_TRUE(value && value.is_string());
        auto unescaped = value.as_string();
        EXPECT_NE(unescaped.find('\n'), BASE_NS::string::npos);
        EXPECT_NE(unescaped.find('\t'), BASE_NS::string::npos);
        EXPECT_NE(unescaped.find('\r'), BASE_NS::string::npos);
        EXPECT_NE(unescaped.find('\\'), BASE_NS::string::npos);
        EXPECT_NE(unescaped.find('"'), BASE_NS::string::npos);
    }
}

// Test operator[] for writable JSON
UNIT_TEST(SRCJson2Test, subscriptOperator, testing::ext::TestSize.Level1)
{
    CORE_NS::json2::value obj = CORE_NS::json2::value::object();

    // Access non-existent key creates it
    auto& value = obj["newKey"];
    EXPECT_FALSE(value);

    // Assign value
    value = CORE_NS::json2::value::number(42U);
    EXPECT_TRUE(value.is_unsigned_int());
    EXPECT_EQ(value.as_strict_unsigned_int(), 42U);

    // Access existing key
    auto& existing = obj["newKey"];
    EXPECT_TRUE(existing.is_unsigned_int());
    EXPECT_EQ(existing.as_strict_unsigned_int(), 42U);
}

// Test equality operators with mixed types
UNIT_TEST(SRCJson2Test, equalityWithMixedTags, testing::ext::TestSize.Level1)
{
    const char* str = R"({"key":"value","num":42})";

    auto readonly = CORE_NS::json2::parse<CORE_NS::json2::readonly_tag>(str);
    auto writable = CORE_NS::json2::parse<CORE_NS::json2::writable_tag>(str);

    EXPECT_TRUE(readonly == writable);
    EXPECT_FALSE(readonly != writable);

    // Modify writable
    CORE_NS::json2::value modified(writable);
    modified["key"] = CORE_NS::json2::value::string("different");

    EXPECT_FALSE(readonly == modified);
    EXPECT_TRUE(readonly != modified);
}

// Test object key ordering independence
UNIT_TEST(SRCJson2Test, objectKeyOrderingIndependence, testing::ext::TestSize.Level1)
{
    const char* str1 = R"({"a":1,"b":2,"c":3})";
    const char* str2 = R"({"c":3,"a":1,"b":2})";

    auto value1 = CORE_NS::json2::parse(str1);
    auto value2 = CORE_NS::json2::parse(str2);

    EXPECT_TRUE(value1 == value2);
}

// Test as_number conversions
UNIT_TEST(SRCJson2Test, asNumberConversions, testing::ext::TestSize.Level1)
{
    {
        auto value = CORE_NS::json2::parse("42.5");
        EXPECT_FLOAT_EQ(value.as_number<float>(), 42.5f);
        EXPECT_DOUBLE_EQ(value.as_number<double>(), 42.5);
        EXPECT_EQ(value.as_number<int>(), 42);
    }

    {
        auto value = CORE_NS::json2::parse("-100");
        EXPECT_EQ(value.as_number<int32_t>(), -100);
        EXPECT_EQ(value.as_number<int64_t>(), -100);
    }

    {
        auto value = CORE_NS::json2::parse("200");
        EXPECT_EQ(value.as_number<uint32_t>(), 200u);
        EXPECT_EQ(value.as_number<uint64_t>(), 200u);
    }

    // Note: non-number is illegal
}

// Test escaped string conversions
UNIT_TEST(SRCJson2Test, escapedStringConversions, testing::ext::TestSize.Level1)
{
    using namespace CORE_NS::json2::literals;

    // Test conversion between escaped string types
    const CORE_NS::json2::escaped_string writable = CORE_NS::json2::escaped_string::from_unescaped("test");
    const CORE_NS::json2::escaped_string_view readonly = writable;

    EXPECT_TRUE(CORE_NS::json2::escaped_string_view(writable) == readonly);
    EXPECT_FALSE(CORE_NS::json2::escaped_string_view(writable) != readonly);
}

// Test Unicode surrogate pair edge cases
UNIT_TEST(SRCJson2Test, unicodeSurrogatePairs, testing::ext::TestSize.Level1)
{
    // Invalid surrogate pairs (high without low)
    {
        const char* str = R"("\uD800")";
        auto value = CORE_NS::json2::parse(str);
        ASSERT_TRUE(value);
        ASSERT_TRUE(value.is_string());
    }

    // Multiple surrogate pairs
    {
        const char* str = R"("\uD83D\uDE00\uD83D\uDE01")";  // 😀😁
        auto value = CORE_NS::json2::parse(str);
        ASSERT_TRUE(value && value.is_string());
        auto unescaped = value.as_string();
        EXPECT_GT(unescaped.size(), 0u);
    }
}
UNIT_TEST(SRCJson2Test, noHexDigitFollowingBackslashU, testing::ext::TestSize.Level1)
{
    const char* str = R"("\uz00000")";
    auto value = CORE_NS::json2::parse(str);
    ASSERT_TRUE(value.is_uninitialized());
}
UNIT_TEST(SRCJson2Test, unicodeSequenceTooEarlyEnd, testing::ext::TestSize.Level1)
{
    const char* str = R"("\u123")";
    auto value = CORE_NS::json2::parse(str);
    ASSERT_TRUE(value.is_uninitialized());
}

UNIT_TEST(SRCJson2Test, tooLongUnsignedNumber, testing::ext::TestSize.Level1)
{
    const char* str = R"(123456789012345678901234567890123456789012345678901234567890)";
    auto value = CORE_NS::json2::parse(str);
    ASSERT_TRUE(value.is_uninitialized());
}
UNIT_TEST(SRCJson2Test, tooLongSignedNumber, testing::ext::TestSize.Level1)
{
    const char* str = R"(-123456789012345678901234567890123456789012345678901234567890)";
    auto value = CORE_NS::json2::parse(str);
    ASSERT_TRUE(value.is_uninitialized());
}
UNIT_TEST(SRCJson2Test, maxLengthFloatingPointNumber, testing::ext::TestSize.Level1)
{
    const char* str = R"(12345678901234567890123456789012345678901234567890123456789.1234)";
    auto value = CORE_NS::json2::parse(str);
    ASSERT_TRUE(value.is_floating_point());
}
UNIT_TEST(SRCJson2Test, tooLongFloatingPointNumber, testing::ext::TestSize.Level1)
{
    const char* str = R"(12345678901234567890123456789012345678901234567890123456789.12345)";
    auto value = CORE_NS::json2::parse(str);
    ASSERT_TRUE(value.is_uninitialized());
}
UNIT_TEST(SRCJson2Test, unexpectedTrue, testing::ext::TestSize.Level1)
{
    const char* str = R"(4 true)";
    auto value = CORE_NS::json2::parse(str);
    ASSERT_TRUE(value.is_uninitialized());
}
UNIT_TEST(SRCJson2Test, unexpectedFalse, testing::ext::TestSize.Level1)
{
    const char* str = R"(4 false)";
    auto value = CORE_NS::json2::parse(str);
    ASSERT_TRUE(value.is_uninitialized());
}
UNIT_TEST(SRCJson2Test, unexpectedNull, testing::ext::TestSize.Level1)
{
    const char* str = R"(0 null)";
    auto value = CORE_NS::json2::parse(str);
    ASSERT_TRUE(value.is_uninitialized());
}

// Test to_string formatting consistency
UNIT_TEST(SRCJson2Test, toStringFormatting, testing::ext::TestSize.Level1)
{
    using JSON = CORE_NS::json2::value;

    // Ensure to_string produces valid JSON that can be re-parsed
    const JSON original = JSON::object({
        {"string", JSON::escaped_string("value")},
        {"number", JSON::number(42U)},
        {"float", JSON::number(0.25)},
        {"bool", JSON::number(true)},
        {"null", JSON::null()},
    });

    auto stringified = CORE_NS::json2::to_string(original);
    auto reparsed = CORE_NS::json2::parse(stringified);

    auto restringified = CORE_NS::json2::to_string(reparsed);
    EXPECT_STREQ(stringified.c_str(), restringified.c_str());

    EXPECT_EQ(original, reparsed);
}

UNIT_TEST(SRCJson2Test, cleanupBehavior, testing::ext::TestSize.Level1)
{
    // Test that reassignment properly cleans up
    CORE_NS::json2::value value = CORE_NS::json2::value::object();
    EXPECT_TRUE(value.is_object());

    value = CORE_NS::json2::value::array();
    EXPECT_TRUE(value.is_array());

    value = CORE_NS::json2::value::number(42u);
    EXPECT_TRUE(value.is_unsigned_int());

    value = CORE_NS::json2::value::escaped_string("test");
    EXPECT_TRUE(value.is_string());
}
UNIT_TEST(SRCJson2Test, partRessignment, testing::ext::TestSize.Level1)
{
    // Test that reassignment properly cleans up
    CORE_NS::json2::value value = CORE_NS::json2::parse<CORE_NS::json2::writable_tag>(R"(
      { "string": "world",
        "bool": true,
        "signed": -2,
        "unsigned": 2,
        "float": 0.5,
        "array": [1,2,3],
        "object": {"key": "value"}
      }
    )");

    ASSERT_TRUE(value.is_object());
    using namespace CORE_NS::json2::literals;
    {
        value.find("string"_escaped)->as_escaped_string().escaped += " cup\\t";
        EXPECT_EQ("world cup\t", value["string"].as_string());
    }
    {
        value.find("bool"_escaped)->as_boolean() ^= true;
        EXPECT_FALSE(value["bool"].as_boolean());
        value.find("bool"_escaped)->as_boolean() ^= true;
        EXPECT_TRUE(value["bool"].as_boolean());
    }
    {
        value.find("signed"_escaped)->as_strict_signed_int() -= 40;
        EXPECT_EQ(-42, value["signed"].as_strict_signed_int());
    }
    {
        value.find("unsigned"_escaped)->as_strict_unsigned_int() += 40;
        EXPECT_EQ(42U, value["unsigned"].as_strict_unsigned_int());
    }
    {
        value.find("float"_escaped)->as_strict_floating_point() *= 84.0;
        EXPECT_DOUBLE_EQ(42.0, value["float"].as_strict_floating_point());
    }
    {
        auto& arr = value.find("array"_escaped)->as_array();
        arr.push_back(CORE_NS::json2::value::number(4U));
        ASSERT_EQ(4U, arr.size());
        EXPECT_EQ(4, value["array"].as_array()[3].as_strict_unsigned_int());
    }
    {
        auto& obj = value.find("object"_escaped)->as_object();
        obj.emplace_back("answer"_escaped, CORE_NS::json2::value::number(42U));
        ASSERT_EQ(2U, obj.size());
        EXPECT_EQ(42, value["object"]["answer"].as_strict_unsigned_int());
    }
}

UNIT_TEST(SRCJson2Test, readonlySubscript, testing::ext::TestSize.Level1)
{
    // Test that reassignment properly cleans up
    const auto value = CORE_NS::json2::parse<CORE_NS::json2::readonly_tag>(R"(
        {"string": "world"}
    )");

    ASSERT_TRUE(value.is_object());
    using namespace CORE_NS::json2::literals;

    ASSERT_STREQ("world", value.find("string")->as_string().c_str());
    ASSERT_STREQ("world", value.find("string"_escaped)->as_string().c_str());

    ASSERT_FALSE(value.find("nonexistent"));
    ASSERT_FALSE(value.find("nonexistent"_escaped));
}

UNIT_TEST(SRCJson2Test, standaloneValueSubscript, testing::ext::TestSize.Level1)
{
    // Test that reassignment properly cleans up
    auto value = CORE_NS::json2::parse<CORE_NS::json2::writable_tag>(R"(
        {"string": "world"}
    )");

    ASSERT_TRUE(value.is_object());
    using namespace CORE_NS::json2::literals;

    ASSERT_STREQ("world", value["string"].as_string().c_str());
    ASSERT_STREQ("world", value["string"_escaped].as_string().c_str());
    ASSERT_STREQ("world", value.find("string")->as_string().c_str());
    ASSERT_STREQ("world", value.find("string"_escaped)->as_string().c_str());

    ASSERT_FALSE(value.find("nonexistent"));
    ASSERT_FALSE(value.find("nonexistent"_escaped));

    {
        auto& nonexistent1 = value["nonexistent"];
        ASSERT_EQ(CORE_NS::json2::type::uninitialized, nonexistent1.type());
        nonexistent1 = CORE_NS::json2::value::number(5);

        ASSERT_EQ(CORE_NS::json2::type::signed_int, value["nonexistent"].type());
    }

    {
        value["nonexistent2"] = CORE_NS::json2::value::string("Hello world!\n");
        ASSERT_EQ(CORE_NS::json2::type::string, value["nonexistent2"].type());
    }
}

// Test equality for uninitialized values
UNIT_TEST(SRCJson2Test, equalityUninitialized, testing::ext::TestSize.Level1)
{
    auto v1 = CORE_NS::json2::value::uninitialized();
    auto v2 = CORE_NS::json2::value::uninitialized();
    auto v3 = CORE_NS::json2::value::null();

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 != v2);
    EXPECT_FALSE(v1 == v3);
    EXPECT_TRUE(v1 != v3);
}

// Test equality for null values
UNIT_TEST(SRCJson2Test, equalityNull, testing::ext::TestSize.Level1)
{
    auto v1 = CORE_NS::json2::value::null();
    auto v2 = CORE_NS::json2::value::null();
    auto v3 = CORE_NS::json2::value::boolean(false);

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 != v2);
    EXPECT_FALSE(v1 == v3);
    EXPECT_TRUE(v1 != v3);
}

// Test equality for boolean values
UNIT_TEST(SRCJson2Test, equalityBoolean, testing::ext::TestSize.Level1)
{
    auto v1 = CORE_NS::json2::value::boolean(true);
    auto v2 = CORE_NS::json2::value::boolean(true);
    auto v3 = CORE_NS::json2::value::boolean(false);
    auto v4 = CORE_NS::json2::value::number(1);

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 != v2);
    EXPECT_FALSE(v1 == v3);
    EXPECT_TRUE(v1 != v3);
    EXPECT_FALSE(v1 == v4);  // Different types
    EXPECT_TRUE(v1 != v4);
}

// Test equality for string values
UNIT_TEST(SRCJson2Test, equalityString, testing::ext::TestSize.Level1)
{
    auto v1 = CORE_NS::json2::value::string("hello");
    auto v2 = CORE_NS::json2::value::string("hello");
    auto v3 = CORE_NS::json2::value::string("world");
    auto v4 = CORE_NS::json2::value::number(42);

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 != v2);
    EXPECT_FALSE(v1 == v3);
    EXPECT_TRUE(v1 != v3);
    EXPECT_FALSE(v1 == v4);  // Different types
    EXPECT_TRUE(v1 != v4);
}

// Test equality for floating point values
UNIT_TEST(SRCJson2Test, equalityFloatingPoint, testing::ext::TestSize.Level1)
{
    auto v1 = CORE_NS::json2::value::number(3.14);
    auto v2 = CORE_NS::json2::value::number(3.14);
    auto v3 = CORE_NS::json2::value::number(2.71);
    auto v4 = CORE_NS::json2::value::number(3);  // Signed int

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 != v2);
    EXPECT_FALSE(v1 == v3);
    EXPECT_TRUE(v1 != v3);
    EXPECT_FALSE(v1 == v4);  // Different numeric types
    EXPECT_TRUE(v1 != v4);
}

// Test equality for signed integer values
UNIT_TEST(SRCJson2Test, equalitySignedInt, testing::ext::TestSize.Level1)
{
    auto v1 = CORE_NS::json2::value::number(-42);
    auto v2 = CORE_NS::json2::value::number(-42);
    auto v3 = CORE_NS::json2::value::number(-100);
    auto v4 = CORE_NS::json2::value::number(42U);  // Unsigned int

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 != v2);
    EXPECT_FALSE(v1 == v3);
    EXPECT_TRUE(v1 != v3);
    EXPECT_FALSE(v1 == v4);  // Different numeric types
    EXPECT_TRUE(v1 != v4);
}

// Test equality for unsigned integer values
UNIT_TEST(SRCJson2Test, equalityUnsignedInt, testing::ext::TestSize.Level1)
{
    auto v1 = CORE_NS::json2::value::number(42U);
    auto v2 = CORE_NS::json2::value::number(42U);
    auto v3 = CORE_NS::json2::value::number(100U);
    auto v4 = CORE_NS::json2::value::number(-42);  // Signed int

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 != v2);
    EXPECT_FALSE(v1 == v3);
    EXPECT_TRUE(v1 != v3);
    EXPECT_FALSE(v1 == v4);  // Different numeric types
    EXPECT_TRUE(v1 != v4);
}

// Test equality for empty arrays
UNIT_TEST(SRCJson2Test, equalityEmptyArrays, testing::ext::TestSize.Level1)
{
    auto v1 = CORE_NS::json2::value::array();
    auto v2 = CORE_NS::json2::value::array();
    auto v3 = CORE_NS::json2::value::object();

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 != v2);
    EXPECT_FALSE(v1 == v3);  // Different types
    EXPECT_TRUE(v1 != v3);
}

// Test equality for arrays with different sizes
UNIT_TEST(SRCJson2Test, equalityArraysDifferentSizes, testing::ext::TestSize.Level1)
{
    CORE_NS::json2::value::array_t arr1;
    arr1.push_back(CORE_NS::json2::value::number(1));
    arr1.push_back(CORE_NS::json2::value::number(2));

    CORE_NS::json2::value::array_t arr2;
    arr2.push_back(CORE_NS::json2::value::number(1));

    auto v1 = CORE_NS::json2::value::array(BASE_NS::move(arr1));
    auto v2 = CORE_NS::json2::value::array(BASE_NS::move(arr2));

    EXPECT_FALSE(v1 == v2);
    EXPECT_TRUE(v1 != v2);
}

// Test equality for arrays with same size but different elements
UNIT_TEST(SRCJson2Test, equalityArraysDifferentElements, testing::ext::TestSize.Level1)
{
    CORE_NS::json2::value::array_t arr1;
    arr1.push_back(CORE_NS::json2::value::number(1));
    arr1.push_back(CORE_NS::json2::value::number(2));

    CORE_NS::json2::value::array_t arr2;
    arr2.push_back(CORE_NS::json2::value::number(1));
    arr2.push_back(CORE_NS::json2::value::number(3));

    auto v1 = CORE_NS::json2::value::array(BASE_NS::move(arr1));
    auto v2 = CORE_NS::json2::value::array(BASE_NS::move(arr2));

    EXPECT_FALSE(v1 == v2);
    EXPECT_TRUE(v1 != v2);
}

// Test equality for arrays with identical elements
UNIT_TEST(SRCJson2Test, equalityArraysIdentical, testing::ext::TestSize.Level1)
{
    CORE_NS::json2::value::array_t arr1;
    arr1.push_back(CORE_NS::json2::value::number(1));
    arr1.push_back(CORE_NS::json2::value::string("test"));
    arr1.push_back(CORE_NS::json2::value::boolean(true));

    CORE_NS::json2::value::array_t arr2;
    arr2.push_back(CORE_NS::json2::value::number(1));
    arr2.push_back(CORE_NS::json2::value::string("test"));
    arr2.push_back(CORE_NS::json2::value::boolean(true));

    auto v1 = CORE_NS::json2::value::array(BASE_NS::move(arr1));
    auto v2 = CORE_NS::json2::value::array(BASE_NS::move(arr2));

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 != v2);
}

// Test equality for empty objects
UNIT_TEST(SRCJson2Test, equalityEmptyObjects, testing::ext::TestSize.Level1)
{
    auto v1 = CORE_NS::json2::value::object();
    auto v2 = CORE_NS::json2::value::object();
    auto v3 = CORE_NS::json2::value::array();

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 != v2);
    EXPECT_FALSE(v1 == v3);  // Different types
    EXPECT_TRUE(v1 != v3);
}

// Test equality for objects with different sizes
UNIT_TEST(SRCJson2Test, equalityObjectsDifferentSizes, testing::ext::TestSize.Level1)
{
    CORE_NS::json2::value::object_t obj1;
    obj1.emplace_back("key1", CORE_NS::json2::value::number(1));
    obj1.emplace_back("key2", CORE_NS::json2::value::number(2));

    CORE_NS::json2::value::object_t obj2;
    obj2.emplace_back("key1", CORE_NS::json2::value::number(1));

    auto v1 = CORE_NS::json2::value::object(BASE_NS::move(obj1));
    auto v2 = CORE_NS::json2::value::object(BASE_NS::move(obj2));

    EXPECT_FALSE(v1 == v2);
    EXPECT_TRUE(v1 != v2);
}

// Test equality for objects with same keys but different values
UNIT_TEST(SRCJson2Test, equalityObjectsDifferentValues, testing::ext::TestSize.Level1)
{
    CORE_NS::json2::value::object_t obj1;
    obj1.emplace_back("key1", CORE_NS::json2::value::number(1));
    obj1.emplace_back("key2", CORE_NS::json2::value::number(2));

    CORE_NS::json2::value::object_t obj2;
    obj2.emplace_back("key1", CORE_NS::json2::value::number(1));
    obj2.emplace_back("key2", CORE_NS::json2::value::number(3));

    auto v1 = CORE_NS::json2::value::object(BASE_NS::move(obj1));
    auto v2 = CORE_NS::json2::value::object(BASE_NS::move(obj2));

    EXPECT_FALSE(v1 == v2);
    EXPECT_TRUE(v1 != v2);
}

// Test equality for objects with key present in one but not the other
UNIT_TEST(SRCJson2Test, equalityObjectsMissingKey, testing::ext::TestSize.Level1)
{
    CORE_NS::json2::value::object_t obj1;
    obj1.emplace_back("key1", CORE_NS::json2::value::number(1));
    obj1.emplace_back("key2", CORE_NS::json2::value::number(2));

    CORE_NS::json2::value::object_t obj2;
    obj2.emplace_back("key1", CORE_NS::json2::value::number(1));
    obj2.emplace_back("key3", CORE_NS::json2::value::number(2));

    auto v1 = CORE_NS::json2::value::object(BASE_NS::move(obj1));
    auto v2 = CORE_NS::json2::value::object(BASE_NS::move(obj2));

    EXPECT_FALSE(v1 == v2);
    EXPECT_TRUE(v1 != v2);
}

// Test equality for objects with identical key-value pairs
UNIT_TEST(SRCJson2Test, equalityObjectsIdentical, testing::ext::TestSize.Level1)
{
    CORE_NS::json2::value::object_t obj1;
    obj1.emplace_back("key1", CORE_NS::json2::value::number(1));
    obj1.emplace_back("key2", CORE_NS::json2::value::string("test"));
    obj1.emplace_back("key3", CORE_NS::json2::value::boolean(true));

    CORE_NS::json2::value::object_t obj2;
    obj2.emplace_back("key1", CORE_NS::json2::value::number(1));
    obj2.emplace_back("key2", CORE_NS::json2::value::string("test"));
    obj2.emplace_back("key3", CORE_NS::json2::value::boolean(true));

    auto v1 = CORE_NS::json2::value::object(BASE_NS::move(obj1));
    auto v2 = CORE_NS::json2::value::object(BASE_NS::move(obj2));

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 != v2);
}

// Test equality for objects with different key ordering (should be equal)
UNIT_TEST(SRCJson2Test, equalityObjectsDifferentOrdering, testing::ext::TestSize.Level1)
{
    CORE_NS::json2::value::object_t obj1;
    obj1.emplace_back("alpha", CORE_NS::json2::value::number(1));
    obj1.emplace_back("beta", CORE_NS::json2::value::number(2));
    obj1.emplace_back("gamma", CORE_NS::json2::value::number(3));

    CORE_NS::json2::value::object_t obj2;
    obj2.emplace_back("gamma", CORE_NS::json2::value::number(3));
    obj2.emplace_back("alpha", CORE_NS::json2::value::number(1));
    obj2.emplace_back("beta", CORE_NS::json2::value::number(2));

    auto v1 = CORE_NS::json2::value::object(BASE_NS::move(obj1));
    auto v2 = CORE_NS::json2::value::object(BASE_NS::move(obj2));

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 != v2);
}

// Test equality between view and value (readonly vs writable)
UNIT_TEST(SRCJson2Test, equalityViewAndValueNull, testing::ext::TestSize.Level1)
{
    const char* json = "null";
    auto view = CORE_NS::json2::parse<CORE_NS::json2::readonly_tag>(json);
    auto value = CORE_NS::json2::parse<CORE_NS::json2::writable_tag>(json);

    EXPECT_TRUE(view == value);
    EXPECT_FALSE(view != value);
    EXPECT_TRUE(value == view);
    EXPECT_FALSE(value != view);
}

// Test equality between view and value for strings
UNIT_TEST(SRCJson2Test, equalityViewAndValueString, testing::ext::TestSize.Level1)
{
    const char* json = "\"hello world\"";
    auto view = CORE_NS::json2::parse<CORE_NS::json2::readonly_tag>(json);
    auto value = CORE_NS::json2::parse<CORE_NS::json2::writable_tag>(json);

    EXPECT_TRUE(view == value);
    EXPECT_FALSE(view != value);
    EXPECT_TRUE(value == view);
    EXPECT_FALSE(value != view);
}

// Test equality between view and value for numbers
UNIT_TEST(SRCJson2Test, equalityViewAndValueNumber, testing::ext::TestSize.Level1)
{
    const char* json = "42";
    auto view = CORE_NS::json2::parse<CORE_NS::json2::readonly_tag>(json);
    auto value = CORE_NS::json2::parse<CORE_NS::json2::writable_tag>(json);

    EXPECT_TRUE(view == value);
    EXPECT_FALSE(view != value);
    EXPECT_TRUE(value == view);
    EXPECT_FALSE(value != view);
}

// Test equality between view and value for arrays
UNIT_TEST(SRCJson2Test, equalityViewAndValueArray, testing::ext::TestSize.Level1)
{
    const char* json = "[1, 2, 3, \"test\", true]";
    auto view = CORE_NS::json2::parse<CORE_NS::json2::readonly_tag>(json);
    auto value = CORE_NS::json2::parse<CORE_NS::json2::writable_tag>(json);

    EXPECT_TRUE(view == value);
    EXPECT_FALSE(view != value);
    EXPECT_TRUE(value == view);
    EXPECT_FALSE(value != view);
}

// Test equality between view and value for objects
UNIT_TEST(SRCJson2Test, equalityViewAndValueObject, testing::ext::TestSize.Level1)
{
    const char* json = "{\"key1\": 1, \"key2\": \"value\", \"key3\": true}";
    auto view = CORE_NS::json2::parse<CORE_NS::json2::readonly_tag>(json);
    auto value = CORE_NS::json2::parse<CORE_NS::json2::writable_tag>(json);

    EXPECT_TRUE(view == value);
    EXPECT_FALSE(view != value);
    EXPECT_TRUE(value == view);
    EXPECT_FALSE(value != view);
}

// Test equality between view and value for complex nested structures
UNIT_TEST(SRCJson2Test, equalityViewAndValueComplex, testing::ext::TestSize.Level1)
{
    const char* json = R"({
        "array": [1, 2, {"nested": "value"}],
        "object": {"key": [true, false, null]},
        "number": 42.5,
        "string": "test"
    })";
    auto view = CORE_NS::json2::parse<CORE_NS::json2::readonly_tag>(json);
    auto value = CORE_NS::json2::parse<CORE_NS::json2::writable_tag>(json);

    EXPECT_TRUE(view == value);
    EXPECT_FALSE(view != value);
    EXPECT_TRUE(value == view);
    EXPECT_FALSE(value != view);
}

// Test inequality between view and value with modifications
UNIT_TEST(SRCJson2Test, inequalityViewAndValueModified, testing::ext::TestSize.Level1)
{
    const char* json = "{\"key\": 1}";
    auto view = CORE_NS::json2::parse<CORE_NS::json2::readonly_tag>(json);
    auto value = CORE_NS::json2::parse<CORE_NS::json2::writable_tag>(json);

    // Modify the writable value
    value["key"] = CORE_NS::json2::value::number(2);

    EXPECT_FALSE(view == value);
    EXPECT_TRUE(view != value);
    EXPECT_FALSE(value == view);
    EXPECT_TRUE(value != view);
}

// Test equality between two views
UNIT_TEST(SRCJson2Test, equalityTwoViews, testing::ext::TestSize.Level1)
{
    const char* json1 = "[1, 2, 3]";
    const char* json2 = "[1, 2, 3]";
    auto view1 = CORE_NS::json2::parse<CORE_NS::json2::readonly_tag>(json1);
    auto view2 = CORE_NS::json2::parse<CORE_NS::json2::readonly_tag>(json2);

    EXPECT_TRUE(view1 == view2);
    EXPECT_FALSE(view1 != view2);
}

// Test equality between two writable values
UNIT_TEST(SRCJson2Test, equalityTwoValues, testing::ext::TestSize.Level1)
{
    const char* json1 = "[1, 2, 3]";
    const char* json2 = "[1, 2, 3]";
    auto value1 = CORE_NS::json2::parse<CORE_NS::json2::writable_tag>(json1);
    auto value2 = CORE_NS::json2::parse<CORE_NS::json2::writable_tag>(json2);

    EXPECT_TRUE(value1 == value2);
    EXPECT_FALSE(value1 != value2);
}

// Test equality with nested arrays containing different elements
UNIT_TEST(SRCJson2Test, equalityNestedArraysDifferent, testing::ext::TestSize.Level1)
{
    const char* json1 = "[[1, 2], [3, 4]]";
    const char* json2 = "[[1, 2], [3, 5]]";
    auto v1 = CORE_NS::json2::parse(json1);
    auto v2 = CORE_NS::json2::parse(json2);

    EXPECT_FALSE(v1 == v2);
    EXPECT_TRUE(v1 != v2);
}

// Test equality with nested objects containing different values
UNIT_TEST(SRCJson2Test, equalityNestedObjectsDifferent, testing::ext::TestSize.Level1)
{
    const char* json1 = R"({"outer": {"inner": 1}})";
    const char* json2 = R"({"outer": {"inner": 2}})";
    auto v1 = CORE_NS::json2::parse(json1);
    auto v2 = CORE_NS::json2::parse(json2);

    EXPECT_FALSE(v1 == v2);
    EXPECT_TRUE(v1 != v2);
}
UNIT_TEST(SRCJson2Test, selfAssignment, testing::ext::TestSize.Level1)
{
    using JSON = CORE_NS::json2::value;

    // 1. Copy Self-Assignment with String
    {
        JSON val = JSON::string("test_string");
        ASSERT_TRUE(val.is_string());

#ifndef __OHOS__
        // Explicit self-assignment
        val = val;

        EXPECT_TRUE(val.is_string());
        EXPECT_EQ(val.as_string(), "test_string");
#endif
    }

    // 2. Move Self-Assignment with String
    {
        JSON val = JSON::string("test_string_move");
        ASSERT_TRUE(val.is_string());

        // Explicit move self-assignment
        val = BASE_NS::move(val);

        EXPECT_TRUE(val.is_string());
        EXPECT_EQ(val.as_string(), "test_string_move");
    }

    // 3. Complex Object Self-Assignment
    // (Ensures the protection works for union members requiring destructors like object_t)
    {
        JSON val = JSON::object();
        val["key"] = JSON::number(42);

#ifndef __OHOS__
        // Copy self-assign
        val = val;
        ASSERT_TRUE(val.is_object());
        EXPECT_EQ(val["key"].as_number<int>(), 42);

        // Move self-assign
        val = BASE_NS::move(val);
        ASSERT_TRUE(val.is_object());
        EXPECT_EQ(val["key"].as_number<int>(), 42);
#endif
    }
}

/**
 * @tc.name: assignNull
 * @tc.desc: Tests for assigning null to an object via copy and move to ensure cleanup.
 * @tc.type: FUNC
 */
UNIT_TEST(SRCJson2Test, assignNull, testing::ext::TestSize.Level1)
{
    using JSON = CORE_NS::json2::value;

    // 1. Copy Assignment of Null
    {
        // Start with a non-null value (Allocated string)
        JSON val = JSON::string("should_become_null");
        ASSERT_TRUE(val.is_string());

        JSON nullVal = JSON::null();
        ASSERT_TRUE(nullVal.is_null());

        // Perform Copy Assignment
        val = nullVal;

        EXPECT_TRUE(val.is_null());
        EXPECT_FALSE(val.is_string());
        EXPECT_EQ(val.type(), CORE_NS::json2::type::null);
    }

    // 2. Move Assignment of Null
    {
        // Start with a non-null value (Object)
        JSON val = JSON::object();
        val["key"] = JSON::boolean(true);
        ASSERT_TRUE(val.is_object());

        // Perform Move Assignment (Factory creates temporary)
        val = JSON::null();

        EXPECT_TRUE(val.is_null());
        EXPECT_FALSE(val.is_object());
        EXPECT_EQ(val.type(), CORE_NS::json2::type::null);
    }

    // 3. Assign Null to Uninitialized
    {
        JSON val;  // Default constructor makes it uninitialized
        ASSERT_TRUE(val.is_uninitialized());

        val = JSON::null();

        EXPECT_TRUE(val.is_null());
        EXPECT_FALSE(val.is_uninitialized());
    }
}

UNIT_TEST(SRCJson2Test, convertArray, testing::ext::TestSize.Level1)
{
    using JSON = CORE_NS::json2::value;

    const auto value = JSON::array({JSON::number(1), JSON::array({JSON::boolean(true), JSON::boolean(false)})});

    const auto view = CORE_NS::json2::view(value);

    ASSERT_EQ(value, view);
    ASSERT_EQ(value.to_string(), view.to_string());

    const auto value2 = CORE_NS::json2::value(view);

    ASSERT_EQ(value, value2);
    ASSERT_EQ(value.to_string(), value2.to_string());
}