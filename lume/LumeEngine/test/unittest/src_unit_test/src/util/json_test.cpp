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
#include <core/json/json.h>

#include "loader/json_util.h"
#include "test_framework.h"
#include "util/frustum_util.h"

namespace {
template <typename T>
void ParseEmpty()
{
    {
        BASE_NS::string_view str;
        auto value = CORE_NS::json::parse<T>(str.data());
        EXPECT_TRUE(!value);
    }
    {
        BASE_NS::string_view str{"invalid"};
        auto value = CORE_NS::json::parse<T>(str.data());
        EXPECT_TRUE(!value);
    }
}

/**
 * @tc.name: parseEmpty
 * @tc.desc: Tests for Parse Empty. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilJsonTest, parseEmpty, testing::ext::TestSize.Level1)
{
    ParseEmpty<CORE_NS::json::readonly_tag>();
    ParseEmpty<CORE_NS::json::writable_tag>();
}

template <typename T>
void Constructor()
{
    using Json = CORE_NS::json::value_t<T>;
    {
        auto value = Json();
        EXPECT_FALSE(value);

        auto copyConstructed = value;
        EXPECT_FALSE(copyConstructed);

        Json copyAssigned(typename Json::null{});
        copyAssigned = value;
        EXPECT_FALSE(copyAssigned);

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_FALSE(moveConstructed);

        Json moveAssigned(typename Json::null{});
        moveAssigned = BASE_NS::move(value);
        EXPECT_FALSE(moveAssigned);
    }
    {
        auto value = Json(typename Json::object{});
        EXPECT_TRUE(value && value.is_object());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_object());

        Json copyAssigned(typename Json::null{});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_object());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_object());

        Json moveAssigned(typename Json::null{});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_object());
    }
    {
        auto value = Json(typename Json::array{});
        EXPECT_TRUE(value && value.is_array());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_array());

        Json copyAssigned(typename Json::null{});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_array());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_array());

        Json moveAssigned(typename Json::null{});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_array());
    }
    {
        typename Json::string str{""};
        auto value = Json(str);
        EXPECT_TRUE(value && value.is_string());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_string());

        Json copyAssigned(typename Json::null{});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_string());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_string());

        Json moveAssigned(typename Json::null{});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_string());
    }
    {
        auto value = Json(true);
        EXPECT_TRUE(value && value.is_boolean());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_boolean());

        Json copyAssigned(typename Json::null{});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_boolean());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_boolean());

        Json moveAssigned(typename Json::null{});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_boolean());
    }
    {
        auto value = Json(1.f);
        EXPECT_TRUE(value && value.is_number() && value.is_floating_point());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_number() && copyConstructed.is_floating_point());

        Json copyAssigned(typename Json::null{});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_number() && copyAssigned.is_floating_point());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_number() && moveConstructed.is_floating_point());

        Json moveAssigned(typename Json::null{});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_number() && moveAssigned.is_floating_point());
    }
    {
        auto value = Json(1.0);
        EXPECT_TRUE(value && value.is_number() && value.is_floating_point());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_number() && copyConstructed.is_floating_point());

        Json copyAssigned(typename Json::null{});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_number() && copyAssigned.is_floating_point());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_number() && moveConstructed.is_floating_point());

        Json moveAssigned(typename Json::null{});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_number() && moveAssigned.is_floating_point());
    }
    {
        auto value = Json(1);
        EXPECT_TRUE(value && value.is_number() && value.is_signed_int());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_number() && copyConstructed.is_signed_int());

        Json copyAssigned(typename Json::null{});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_number() && copyAssigned.is_signed_int());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_number() && moveConstructed.is_signed_int());

        Json moveAssigned(typename Json::null{});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_number() && moveAssigned.is_signed_int());
    }
    {
        auto value = Json(1U);
        EXPECT_TRUE(value && value.is_number() && value.is_unsigned_int());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_number() && copyConstructed.is_unsigned_int());

        Json copyAssigned(typename Json::null{});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_number() && copyAssigned.is_unsigned_int());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_number() && moveConstructed.is_unsigned_int());

        Json moveAssigned(typename Json::null{});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_number() && moveAssigned.is_unsigned_int());
    }
}

/**
 * @tc.name: constructors
 * @tc.desc: Tests for Constructors. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilJsonTest, constructors, testing::ext::TestSize.Level1)
{
    Constructor<CORE_NS::json::readonly_tag>();
    Constructor<CORE_NS::json::writable_tag>();
}

template <typename T>
void ParseTypes()
{
    // check basic json types
    {
        constexpr const char* strings[] = {
            "\"\"", "\"foo\"", "\"\\\"\"", "\"\\b\"", "\"\\f\"", "\"\\n\"", "\"\\r\"", "\"\\t\"", "\"\\u1234\""};
        for (const auto str : strings) {
            auto value = CORE_NS::json::parse<T>(str);
            EXPECT_TRUE(value && value.is_string());
        }
        BASE_NS::string_view str{"\"hello\""};
        auto value = CORE_NS::json::parse<T>(str.data());
        ASSERT_TRUE(value && value.is_string());
        EXPECT_EQ(value.string_, "hello");
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
            auto value = CORE_NS::json::parse<T>(str);
            EXPECT_TRUE(value && value.is_number());
        }

        auto value = CORE_NS::json::parse<T>("42");
        ASSERT_TRUE(value && value.is_number() && value.is_unsigned_int());
        EXPECT_EQ(value.template as_number<uint32_t>(), 42U);

        value = CORE_NS::json::parse<T>("-42");
        ASSERT_TRUE(value && value.is_number() && value.is_signed_int());
        EXPECT_EQ(value.template as_number<int32_t>(), -42);  // -42: param

        value = CORE_NS::json::parse<T>("42.25");
        ASSERT_TRUE(value && value.is_number() && value.is_floating_point());
        EXPECT_EQ(value.template as_number<float>(), 42.25f);

        value = CORE_NS::json::parse<T>("-42.25");
        ASSERT_TRUE(value && value.is_number() && value.is_floating_point());
        EXPECT_EQ(value.template as_number<float>(), -42.25f);

        value = CORE_NS::json::parse<T>("foo");
        ASSERT_FALSE(value);
        ASSERT_FALSE(value.is_floating_point());
        ASSERT_FALSE(value.is_number());
        EXPECT_NE(value.template as_number<float>(), -42.25f);
    }
    {
        BASE_NS::string_view str{"false"};
        auto value = CORE_NS::json::parse<T>(str.data());
        ASSERT_TRUE(value && value.is_boolean());
        EXPECT_EQ(value.boolean_, false);
    }
    {
        BASE_NS::string_view str{"true"};
        auto value = CORE_NS::json::parse<T>(str.data());
        ASSERT_TRUE(value && value.is_boolean());
        EXPECT_EQ(value.boolean_, true);
    }
    {
        BASE_NS::string_view str{"null"};
        auto value = CORE_NS::json::parse<T>(str.data());
        ASSERT_TRUE(value && value.is_null());
    }
    {
        BASE_NS::string_view str{"{}"};
        auto value = CORE_NS::json::parse<T>(str.data());
        EXPECT_TRUE(value && value.is_object());
    }
    {
        BASE_NS::string_view str{"[]"};
        auto value = CORE_NS::json::parse<T>(str.data());
        EXPECT_TRUE(value && value.is_array());
    }
}

/**
 * @tc.name: parseTypes
 * @tc.desc: Tests for Parse Types. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilJsonTest, parseTypes, testing::ext::TestSize.Level1)
{
    ParseTypes<CORE_NS::json::readonly_tag>();
    ParseTypes<CORE_NS::json::writable_tag>();
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
            auto value = CORE_NS::json::parse<T>(str);
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
            auto value = CORE_NS::json::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
    {
        constexpr const char* strings[] = {
            "\"f", "\"\\", "\"\\a\"", "\"\"\"", "\"\b\"", "\"\f\"", "\"\n\"", "\"\r\"", "\"\t\"", "\"\\u00\""};
        for (const auto str : strings) {
            auto value = CORE_NS::json::parse<T>(str);
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
            auto value = CORE_NS::json::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
    {
        constexpr const char* nulls[] = {"a", "n", "nu", "nult", "nulll"};
        for (const auto str : nulls) {
            auto value = CORE_NS::json::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
    {
        constexpr const char* booleans[] = {"t", "tr", "truu", "truee", "f", "fa", "falss", "falsee"};
        for (const auto str : booleans) {
            auto value = CORE_NS::json::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
}

/**
 * @tc.name: invalid
 * @tc.desc: Tests for Invalid. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilJsonTest, invalid, testing::ext::TestSize.Level1)
{
    ParseInvalid<CORE_NS::json::readonly_tag>();
    ParseInvalid<CORE_NS::json::writable_tag>();
}

template <typename T>
void ToString()
{
    {
        BASE_NS::string_view str{
            "[{\"object\":{}, \"array\":[], \"string\":\"value\", \"number\":42.25, \"boolean\":true, "
            "\"null\":null}, [], \"hello\", 123.456, -123.456, -235, 564, false, true, null]"};
        const CORE_NS::json::value_t<T> inValue = CORE_NS::json::parse<T>(str.data());
        EXPECT_TRUE(inValue);
        BASE_NS::string out = CORE_NS::json::to_string<T>(inValue);
        const CORE_NS::json::value_t<T> outValue = CORE_NS::json::parse<T>(out.data());
        ASSERT_TRUE(inValue.is_array() && outValue.is_array());
        ASSERT_TRUE(inValue.array_.size() == outValue.array_.size());
        auto inIt = inValue.array_.begin();
        auto inEnd = inValue.array_.end();
        auto outIt = outValue.array_.begin();
        while (inIt != inEnd) {
            ASSERT_EQ(inIt->type, outIt->type);
            switch (inIt->type) {
                case CORE_NS::json::type::uninitialized:
                    ASSERT_FALSE(*inIt);
                    break;
                case CORE_NS::json::type::object:
                    EXPECT_EQ(inIt->object_.size(), outIt->object_.size());
                    break;
                case CORE_NS::json::type::array:
                    EXPECT_EQ(inIt->array_.size(), outIt->array_.size());
                    break;
                case CORE_NS::json::type::floating_point:
                    EXPECT_DOUBLE_EQ(inIt->float_, outIt->float_);
                    break;
                case CORE_NS::json::type::signed_int:
                    EXPECT_EQ(inIt->signed_, outIt->signed_);
                    break;
                case CORE_NS::json::type::unsigned_int:
                    EXPECT_EQ(inIt->unsigned_, outIt->unsigned_);
                    break;
                case CORE_NS::json::type::boolean:
                    EXPECT_EQ(inIt->boolean_, outIt->boolean_);
                    break;
                case CORE_NS::json::type::null:
                    break;
                default:
                    break;
            }
            ++inIt;
            ++outIt;
        }
    }
}
}  // namespace

/**
 * @tc.name: toString
 * @tc.desc: Tests for To String. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilJsonTest, toString, testing::ext::TestSize.Level1)
{
    ToString<CORE_NS::json::readonly_tag>();
    ToString<CORE_NS::json::writable_tag>();

    {
        // parse a JSON containing Unicode characters, decode the escaped characters, and convert back to JSON. the end
        // result should be the same as the initial JSON.
        constexpr const BASE_NS::string_view in{
            "{\"\\uD835\\uDD77\\uD835\\uDD9A\\uD835\\uDD92\\uD835\\uDD8A\":\"\\u2661\"}"};
        auto inValue = CORE_NS::json::parse<CORE_NS::json::writable_tag>(in.data());
        ASSERT_TRUE(inValue.is_object());
        ASSERT_EQ(1U, inValue.object_.size());
        auto& keyValue = inValue.object_[0];
        keyValue.key = CORE_NS::json::unescape(keyValue.key);
        keyValue.value = CORE_NS::json::unescape(keyValue.value.string_);
        const BASE_NS::string out = CORE_NS::json::to_string(inValue);
        EXPECT_EQ(in, out);
    }
}

/**
 * @tc.name: standaloneJson
 * @tc.desc: Tests for Standalone Json. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilJsonTest, standaloneJson, testing::ext::TestSize.Level1)
{
    BASE_NS::string_view str{
        "[{\"foo\":\"bar\"}, [{\"greeting\":\"Hello World\"}], \"hello\", 123.456, 42, -42, false, true, null]"};
    const CORE_NS::json::value inValue = CORE_NS::json::parse(str.data());
    ASSERT_TRUE(inValue.is_array());

    CORE_NS::json::standalone_value standalone(inValue);
    ASSERT_TRUE(standalone.is_array());
    auto readonlyIt = inValue.array_.cbegin();
    for (const auto& value : standalone.array_) {
        ASSERT_EQ(value.type, readonlyIt->type);
        switch (value.type) {
            case CORE_NS::json::type::uninitialized:
                ASSERT_FALSE(value);
                break;
            case CORE_NS::json::type::object: {
                EXPECT_EQ(value.object_.size(), readonlyIt->object_.size());
                break;
            }
            case CORE_NS::json::type::array:
                EXPECT_EQ(value.array_.size(), readonlyIt->array_.size());
                break;
            case CORE_NS::json::type::floating_point:
                EXPECT_DOUBLE_EQ(value.float_, readonlyIt->float_);
                break;
            case CORE_NS::json::type::signed_int:
                EXPECT_EQ(value.signed_, readonlyIt->signed_);
                break;
            case CORE_NS::json::type::unsigned_int:
                EXPECT_EQ(value.unsigned_, readonlyIt->unsigned_);
                break;
            case CORE_NS::json::type::boolean:
                EXPECT_EQ(value.boolean_, readonlyIt->boolean_);
                break;
            case CORE_NS::json::type::null:
                break;
            default:
                break;
        }
        ++readonlyIt;
    }
}

/**
 * @tc.name: crossTagConversion
 * @tc.desc: Verify the cross-tag converting constructor deep-copies strings (so the result owns
 *          its data and outlives the source buffer), recurses into nested objects/arrays,
 *          preserves keys and primitive leaf values, and works in both directions.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilJsonTest, crossTagConversion, testing::ext::TestSize.Level1)
{
    constexpr BASE_NS::string_view sourceText{
        R"({"name":"node","children":[{"id":1,"tag":"leaf"},{"id":2,"tag":"branch"}],"flag":true,"ratio":1.5})"};

    // Step 1: parse into a readonly value backed by an external buffer, then promote it into a
    // standalone copy that owns its strings. After this point the source buffer is destroyed,
    // proving that 'standalone' no longer depends on it.
    CORE_NS::json::standalone_value standalone;
    {
        BASE_NS::string buffer{sourceText};
        const CORE_NS::json::value parsed = CORE_NS::json::parse(buffer.data());
        ASSERT_TRUE(parsed.is_object());
        standalone = CORE_NS::json::standalone_value(parsed);
    }  // 'buffer' is freed here; 'standalone' must still be valid.

    // Object keys and string values must match the source exactly (key copy is the part most
    // likely to silently regress if the converter only copied 'value' members).
    ASSERT_TRUE(standalone.is_object());
    ASSERT_EQ(standalone.object_.size(), 4U);
    EXPECT_EQ(standalone.object_[0].key, "name");
    ASSERT_TRUE(standalone.object_[0].value.is_string());
    EXPECT_EQ(standalone.object_[0].value.string_, "node");
    EXPECT_EQ(standalone.object_[2].key, "flag");
    ASSERT_TRUE(standalone.object_[2].value.is_boolean());
    EXPECT_TRUE(standalone.object_[2].value.boolean_);
    EXPECT_EQ(standalone.object_[3].key, "ratio");
    ASSERT_TRUE(standalone.object_[3].value.is_floating_point());
    EXPECT_DOUBLE_EQ(standalone.object_[3].value.float_, 1.5);

    // Recursion: nested array of objects must be deep-copied with their own owned strings.
    ASSERT_EQ(standalone.object_[1].key, "children");
    ASSERT_TRUE(standalone.object_[1].value.is_array());
    const auto& children = standalone.object_[1].value.array_;
    ASSERT_EQ(children.size(), 2U);
    ASSERT_TRUE(children[0].is_object());
    ASSERT_EQ(children[0].object_.size(), 2U);
    EXPECT_EQ(children[0].object_[0].key, "id");
    EXPECT_EQ(children[0].object_[0].value.unsigned_, 1U);
    EXPECT_EQ(children[0].object_[1].key, "tag");
    EXPECT_EQ(children[0].object_[1].value.string_, "leaf");
    EXPECT_EQ(children[1].object_[1].value.string_, "branch");

    // Round-trip the standalone form back to a readonly view (writable -> readonly direction).
    // The view borrows from 'standalone' and must therefore see the same content.
    const CORE_NS::json::value roundTripView = CORE_NS::json::value(standalone);
    ASSERT_TRUE(roundTripView.is_object());
    ASSERT_EQ(roundTripView.object_.size(), 4U);
    EXPECT_EQ(roundTripView.object_[0].key, "name");
    EXPECT_EQ(roundTripView.object_[0].value.string_, "node");

    // Re-serializing the standalone copy must yield the same JSON as serializing the readonly
    // view: both directions of the converter produce equivalent trees.
    EXPECT_EQ(CORE_NS::json::to_string(standalone), CORE_NS::json::to_string(roundTripView));
}

/**
 * @tc.name: unescape
 * @tc.desc: Tests for Unescape. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilJsonTest, unescape, testing::ext::TestSize.Level1)
{
    {
        constexpr BASE_NS::string_view escaped = "foo\\\"bar";
        constexpr BASE_NS::string_view expected = u8"foo\"bar";
        const auto unescaped = CORE_NS::json::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }

    {
        constexpr BASE_NS::string_view escaped = "\\\\\\/\\b\\n\\t\\r\\f";
        constexpr BASE_NS::string_view expected = u8"\\/\b\n\t\r\f";
        const auto unescaped = CORE_NS::json::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }

    {
        constexpr BASE_NS::string_view escaped = "\\u573a\\u666f";
        constexpr BASE_NS::string_view expected = u8"\u573a\u666f";
        const auto unescaped = CORE_NS::json::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    // UTF-8 examples from Wikipedia (https://en.wikipedia.org/wiki/UTF-8)
    {
        constexpr BASE_NS::string_view escaped = "\\u0024";
        constexpr BASE_NS::string_view expected = u8"\u0024";
        const auto unescaped = CORE_NS::json::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    {
        constexpr BASE_NS::string_view escaped = "\\u00A3";
        constexpr BASE_NS::string_view expected = u8"\u00A3";
        const auto unescaped = CORE_NS::json::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    {
        constexpr BASE_NS::string_view escaped = "\\u0939";
        constexpr BASE_NS::string_view expected = u8"\u0939";
        const auto unescaped = CORE_NS::json::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    {
        constexpr BASE_NS::string_view escaped = "\\u20AC";
        constexpr BASE_NS::string_view expected = u8"\u20AC";
        const auto unescaped = CORE_NS::json::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    {
        constexpr BASE_NS::string_view escaped = "\\uD55C";
        constexpr BASE_NS::string_view expected = u8"\uD55C";
        const auto unescaped = CORE_NS::json::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    {
        constexpr BASE_NS::string_view escaped = "\\uD800\\uDF48";
        constexpr BASE_NS::string_view expected = u8"\U00010348";
        const auto unescaped = CORE_NS::json::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    // UTF-16 surrogate pair example from Wikipedia (https://en.wikipedia.org/wiki/JSON)
    {
        constexpr BASE_NS::string_view escaped = "\\uD83D\\uDE10";
        constexpr BASE_NS::string_view expected = u8"\U0001F610";
        const auto unescaped = CORE_NS::json::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    // Examples from RFC 3629
    {
        constexpr BASE_NS::string_view escaped = "\\u0041\\u2262\\u0391\\u002E";
        constexpr BASE_NS::string_view expected = u8"\u0041\u2262\u0391\u002E";
        const auto unescaped = CORE_NS::json::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }

    {
        constexpr BASE_NS::string_view escaped = "\\uD55C\\uAD6D\\uC5B4";
        constexpr BASE_NS::string_view expected = u8"\uD55C\uAD6D\uC5B4";
        const auto unescaped = CORE_NS::json::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    {
        constexpr BASE_NS::string_view escaped = "\\u65E5\\u672C\\u8A9E";
        constexpr BASE_NS::string_view expected = u8"\u65E5\u672C\u8A9E";
        const auto unescaped = CORE_NS::json::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
    {
        constexpr BASE_NS::string_view escaped = "\\uD84C\\uDFB4";
        constexpr BASE_NS::string_view expected = u8"\U000233B4";
        const auto unescaped = CORE_NS::json::unescape(escaped);
        EXPECT_EQ(unescaped, expected);
    }
}

/**
 * @tc.name: escape
 * @tc.desc: Tests for Escape. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilJsonTest, escape, testing::ext::TestSize.Level1)
{
    {
        constexpr BASE_NS::string_view unescaped = u8"\"";
        constexpr BASE_NS::string_view expected = "\\\"";
        const auto escaped = CORE_NS::json::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }

    {
        constexpr BASE_NS::string_view unescaped = u8"\\/\b\n\t\r\f";
        constexpr BASE_NS::string_view expected = "\\\\/\\b\\n\\t\\r\\f";
        const auto escaped = CORE_NS::json::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }

    {
        constexpr BASE_NS::string_view unescaped = u8"\u573a\u666f";
        constexpr BASE_NS::string_view expected = "\\u573A\\u666F";
        const auto escaped = CORE_NS::json::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }

    // UTF-8 examples from Wikipedia (https://en.wikipedia.org/wiki/UTF-8)
    {
        constexpr BASE_NS::string_view unescaped = u8"\u0024";
        constexpr BASE_NS::string_view expected = "$";
        const auto escaped = CORE_NS::json::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\u00A3";
        constexpr BASE_NS::string_view expected = "\\u00A3";
        const auto escaped = CORE_NS::json::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\u0939";
        constexpr BASE_NS::string_view expected = "\\u0939";
        const auto escaped = CORE_NS::json::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\u20AC";
        constexpr BASE_NS::string_view expected = "\\u20AC";
        const auto escaped = CORE_NS::json::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\uD55C";
        constexpr BASE_NS::string_view expected = "\\uD55C";
        const auto escaped = CORE_NS::json::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\U00010348";
        constexpr BASE_NS::string_view expected = "\\uD800\\uDF48";
        const auto escaped = CORE_NS::json::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }

    // UTF-16 surrogate pair example from Wikipedia (https://en.wikipedia.org/wiki/JSON)
    {
        constexpr BASE_NS::string_view unescaped = u8"\U0001F610";
        constexpr BASE_NS::string_view expected = "\\uD83D\\uDE10";
        const auto escaped = CORE_NS::json::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }

    // Examples from RFC 3629
    {
        constexpr BASE_NS::string_view unescaped = u8"\u0041\u2262\u0391\u002E";
        constexpr BASE_NS::string_view expected = "A\\u2262\\u0391.";
        const auto escaped = CORE_NS::json::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\uD55C\uAD6D\uC5B4";
        constexpr BASE_NS::string_view expected = "\\uD55C\\uAD6D\\uC5B4";
        const auto escaped = CORE_NS::json::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\u65E5\u672C\u8A9E";
        constexpr BASE_NS::string_view expected = "\\u65E5\\u672C\\u8A9E";
        const auto escaped = CORE_NS::json::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
    {
        constexpr BASE_NS::string_view unescaped = u8"\U000233B4";
        constexpr BASE_NS::string_view expected = "\\uD84C\\uDFB4";
        const auto escaped = CORE_NS::json::escape(unescaped);
        EXPECT_EQ(escaped, expected);
    }
}

/**
 * @tc.name: boolean
 * @tc.desc: Tests for Boolean. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilJsonTest, boolean, testing::ext::TestSize.Level1)
{
    const char* str = "{\"checked\":false}";
    auto value = CORE_NS::json::parse<CORE_NS::json::writable_tag>(str);
    EXPECT_TRUE(value.is_object());
    auto& checked = value["checked"];
    EXPECT_TRUE(checked.is_boolean());
    EXPECT_FALSE(checked.boolean_);
    // check that assigning a boolean results in a boolean.
    checked = true;
    EXPECT_TRUE(checked.is_boolean());
    EXPECT_TRUE(checked.boolean_);
    checked = "foo";
    EXPECT_TRUE(checked.is_string());
    checked = false;
    EXPECT_TRUE(checked.is_boolean());
    EXPECT_FALSE(checked.boolean_);
}

/**
 * @tc.name: JsonUtil
 * @tc.desc: Tests for Json Util. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilJsonTest, JsonUtil, testing::ext::TestSize.Level1)
{
    BASE_NS::string_view str{"{\"number\":1984, \"boolean\": true, \"charstr\": \"abc\"}"};
    const CORE_NS::json::value jsonData = CORE_NS::json::parse(str.data());
    float num = .0f;
    bool boolean = false;
    BASE_NS::basic_string<char> charstr;
    BASE_NS::string_view numEle{"number"};
    BASE_NS::string numError = "numError";
    BASE_NS::string_view boolEle{"boolean"};
    BASE_NS::string boolError = "boolError";
    BASE_NS::string_view strEle{"charstr"};
    BASE_NS::string strError = "strError";
    EXPECT_TRUE(CORE_NS::SafeGetJsonValue(jsonData, numEle, numError, num));
    EXPECT_TRUE(CORE_NS::SafeGetJsonValue(jsonData, boolEle, boolError, boolean));
    EXPECT_TRUE(CORE_NS::SafeGetJsonValue(jsonData, strEle, strError, charstr));

    BASE_NS::string_view arrayStr{
        "[{\"foo\":\"bar\"}, [{\"greeting\":\"Hello World\"}], \"hello\", 123.456, false, true, null]"};
    const CORE_NS::json::value arrayJson = CORE_NS::json::parse(arrayStr.data());
    BASE_NS::string arrayCon[2] = {"ele1", "ele2"};
    CORE_NS::from_json(arrayJson, arrayCon);

    /*BASE_NS::string_view floatStr{
        "1.0"
    };
    const CORE_NS::json::value floatJson = CORE_NS::json::parse(floatStr.data());
    double df = floatJson.as_number<double>();

    BASE_NS::string_view intStr{
        "1"
    };
    const CORE_NS::json::value intJson = CORE_NS::json::parse(intStr.data());
    double di = intJson.as_number<double>();

    BASE_NS::string_view signIntStr{
        "-1"
    };
    const CORE_NS::json::value signIntJson = CORE_NS::json::parse(signIntStr.data());
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
    for (const auto str : numbers) {
        auto value = CORE_NS::json::parse(str);
        EXPECT_EQ(sizeof(value.as_number<double>()), 8);
    }
}

/**
 * @tc.name: frustumTest
 * @tc.desc: Tests for Frustum Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilJsonTest, frustumTest, testing::ext::TestSize.Level1)
{
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
