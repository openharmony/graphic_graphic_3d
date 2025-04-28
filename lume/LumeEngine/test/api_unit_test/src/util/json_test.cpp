/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>

#include <base/containers/string.h>
#include <base/math/matrix.h>
#define JSON_IMPL
#include <core/json/json.h>
#include "TestRunner.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

using namespace testing::ext;
namespace {
struct TestContext {
    std::shared_ptr<ISceneInit> sceneInit_ = nullptr;
    CORE_NS::IEcs::Ptr ecs_;
};
static TestContext g_context;

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
static constexpr BASE_NS::Uid ENGINE_THREAD{"2070e705-d061-40e4-bfb7-90fad2c280af"};
static constexpr BASE_NS::Uid APP_THREAD{"b2e8cef3-453a-4651-b564-5190f8b5190d"};
static constexpr BASE_NS::Uid IO_QUEUE{"be88e9a0-9cd8-45ab-be48-937953dc258f"};
static constexpr BASE_NS::Uid JS_RELEASE_THREAD{"3784fa96-b25b-4e9c-bbf1-e897d36f73af"};

bool SceneDispose(TestContext &context)
{
    context.ecs_ = nullptr;
    context.sceneInit_ = nullptr;
    return true;
}

bool SceneCreate(TestContext &context)
{
    context.sceneInit_ = CreateTestScene();
    context.sceneInit_->LoadPluginsAndInit();
    if (!context.sceneInit_->GetEngineInstance().engine_) {
        WIDGET_LOGE("fail to get engine");
        return false;
    }
    context.ecs_ = context.sceneInit_->GetEngineInstance().engine_->CreateEcs();
    if (!context.ecs_) {
        WIDGET_LOGE("fail to get ecs");
        return false;
    }
    auto factory = GetInstance<Core::ISystemGraphLoaderFactory>(UID_SYSTEM_GRAPH_LOADER);
    auto systemGraphLoader = factory->Create(context.sceneInit_->GetEngineInstance().engine_->GetFileManager());
    systemGraphLoader->Load("rofs3D://systemGraph.json", *(context.ecs_));
    auto& ecs = *(context.ecs_);
    ecs.Initialize();

    using namespace SCENE_NS;
#if SCENE_META_TEST
    auto fun = [&context]() {
        auto &obr = META_NS::GetObjectRegistry();

        context.params_ = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        if (!context.params_) {
            CORE_LOG_E("default obj null");
        }
        context.scene_ =
            interface_pointer_cast<SCENE_NS::IScene>(obr.Create(SCENE_NS::ClassId::Scene, context.params_));
        
        auto onLoaded = META_NS::MakeCallback<META_NS::IOnChanged>([&context]() {
            bool complete = false;
            auto status = context.scene_->Status()->GetValue();
            if (status == SCENE_NS::IScene::SCENE_STATUS_READY) {
                // still in engine thread
                complete = true;
            } else if (status == SCENE_NS::IScene::SCENE_STATUS_LOADING_FAILED) {
                // make sure we don't have anything in result if error
                complete = true;
            }

            if (complete) {
                if (context.scene_) {
                    auto &obr = META_NS::GetObjectRegistry();
                    // make sure we have renderconfig
                    auto rc = context.scene_->RenderConfiguration()->GetValue();
                    if (!rc) {
                        // Create renderconfig
                        rc = obr.Create<SCENE_NS::IRenderConfiguration>(SCENE_NS::ClassId::RenderConfiguration);
                        context.scene_->RenderConfiguration()->SetValue(rc);
                    }

                    interface_cast<IEcsScene>(context.scene_)
                        ->RenderMode()
                        ->SetValue(IEcsScene::RenderMode::RENDER_ALWAYS);
                    auto duh = context.params_->GetArrayPropertyByName<IntfWeakPtr>("Scenes");
                    if (!duh) {
                        return ;
                    }
                    duh->AddValue(interface_pointer_cast<CORE_NS::IInterface>(context.scene_));
                }
            }
        });
        context.scene_->Asynchronous()->SetValue(false);
        context.scene_->Uri()->SetValue("scene://empty");
        return META_NS::IAny::Ptr{};
    };
    // Should it be possible to cancel? (ie. do we need to store the token for something ..)
    META_NS::GetTaskQueueRegistry()
        .GetTaskQueue(ENGINE_THREAD)
        ->AddWaitableTask(META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>(BASE_NS::move(fun)))
        ->Wait();
#endif
    return true;
}
} // namespace

template<typename T>
void ParseEmpty()
{
    {
        BASE_NS::string_view str;
        auto value = CORE_NS::json::parse<T>(str.data());
        EXPECT_TRUE(!value);
    }
    {
        BASE_NS::string_view str { "invalid" };
        auto value = CORE_NS::json::parse<T>(str.data());
        EXPECT_TRUE(!value);
    }
}

class JsonTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SceneCreate(g_context);
    }
    static void TearDownTestSuite()
    {
        SceneDispose(g_context);
    }
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: ParseEmpty
 * @tc.desc: test ParseEmpty
 * @tc.type: FUNC
 */
HWTEST_F(JsonTest, ParseEmpty, TestSize.Level1)
{
    ParseEmpty<CORE_NS::json::readonly_tag>();
    ParseEmpty<CORE_NS::json::writable_tag>();
}

template<typename T>
void Constructor()
{
    using Json = CORE_NS::json::value_t<T>;
    {
        auto value = Json();
        EXPECT_FALSE(value);

        auto copyConstructed = value;
        EXPECT_FALSE(copyConstructed);

        Json copyAssigned(typename Json::null {});
        copyAssigned = value;
        EXPECT_FALSE(copyAssigned);

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_FALSE(moveConstructed);

        Json moveAssigned(typename Json::null {});
        moveAssigned = BASE_NS::move(value);
        EXPECT_FALSE(moveAssigned);
    }
    {
        auto value = Json(typename Json::object {});
        EXPECT_TRUE(value && value.is_object());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_object());

        Json copyAssigned(typename Json::null {});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_object());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_object());

        Json moveAssigned(typename Json::null {});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_object());
    }
    {
        auto value = Json(typename Json::array {});
        EXPECT_TRUE(value && value.is_array());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_array());

        Json copyAssigned(typename Json::null {});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_array());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_array());

        Json moveAssigned(typename Json::null {});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_array());
    }
    {
        typename Json::string str { "" };
        auto value = Json(str);
        EXPECT_TRUE(value && value.is_string());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_string());

        Json copyAssigned(typename Json::null {});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_string());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_string());

        Json moveAssigned(typename Json::null {});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_string());
    }
    {
        auto value = Json(true);
        EXPECT_TRUE(value && value.is_boolean());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_boolean());

        Json copyAssigned(typename Json::null {});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_boolean());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_boolean());

        Json moveAssigned(typename Json::null {});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_boolean());
    }
    {
        auto value = Json(1.f);
        EXPECT_TRUE(value && value.is_number() && value.is_floating_point());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_number() && copyConstructed.is_floating_point());

        Json copyAssigned(typename Json::null {});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_number() && copyAssigned.is_floating_point());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_number() && moveConstructed.is_floating_point());

        Json moveAssigned(typename Json::null {});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_number() && moveAssigned.is_floating_point());
    }
    {
        auto value = Json(1.0);
        EXPECT_TRUE(value && value.is_number() && value.is_floating_point());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_number() && copyConstructed.is_floating_point());

        Json copyAssigned(typename Json::null {});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_number() && copyAssigned.is_floating_point());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_number() && moveConstructed.is_floating_point());

        Json moveAssigned(typename Json::null {});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_number() && moveAssigned.is_floating_point());
    }
    {
        auto value = Json(1);
        EXPECT_TRUE(value && value.is_number() && value.is_signed_int());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_number() && copyConstructed.is_signed_int());

        Json copyAssigned(typename Json::null {});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_number() && copyAssigned.is_signed_int());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_number() && moveConstructed.is_signed_int());

        Json moveAssigned(typename Json::null {});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_number() && moveAssigned.is_signed_int());
    }
    {
        auto value = Json(1U);
        EXPECT_TRUE(value && value.is_number() && value.is_unsigned_int());

        auto copyConstructed = value;
        EXPECT_TRUE(copyConstructed && copyConstructed.is_number() && copyConstructed.is_unsigned_int());

        Json copyAssigned(typename Json::null {});
        copyAssigned = value;
        EXPECT_TRUE(copyAssigned && copyAssigned.is_number() && copyAssigned.is_unsigned_int());

        auto moveConstructed = BASE_NS::move(copyConstructed);
        EXPECT_TRUE(moveConstructed && moveConstructed.is_number() && moveConstructed.is_unsigned_int());

        Json moveAssigned(typename Json::null {});
        moveAssigned = BASE_NS::move(value);
        EXPECT_TRUE(moveAssigned && moveAssigned.is_number() && moveAssigned.is_unsigned_int());
    }
}

/**
 * @tc.name: Constructors
 * @tc.desc: test Constructors
 * @tc.type: FUNC
 */
HWTEST_F(JsonTest, Constructors, TestSize.Level1)
{
    Constructor<CORE_NS::json::readonly_tag>();
    Constructor<CORE_NS::json::writable_tag>();
}

template<typename T>
void ParseTypes()
{
    using Json = CORE_NS::json::value_t<T>;

    // check basic json types
    {
        constexpr const char* strings[] = { "\"\"", "\"foo\"", "\"\\\"\"", "\"\\b\"", "\"\\f\"", "\"\\n\"", "\"\\r\"",
            "\"\\t\"", "\"\\u1234\"" };
        for (const auto str : strings) {
            auto value = CORE_NS::json::parse<T>(str);
            EXPECT_TRUE(value && value.is_string());
        }
        BASE_NS::string_view str { "\"hello\"" };
        auto value = CORE_NS::json::parse<T>(str.data());
        ASSERT_TRUE(value && value.is_string());
        EXPECT_EQ(value.string_, "hello");
    }
    {
        constexpr const char* numbers[] = { "0", "-0", "12", "-30", "0.0", "-0.0", "1.0", "-1.0", "0e0", "0E0", "0e+0",
            "0E+0", "0e-0", "0E-0", "-0e0", "-0E0", "-0e+0", "-0E+0", "-0e-0", "-0E-0", "-2e1", "2E1", "2e+1", "2E+1",
            "2e-10", "2E-100", "-2e10", "-2E10", "-2e+100", "-2E+10", "-2e-10", "-2E-10" };
        for (const auto str : numbers) {
            auto value = CORE_NS::json::parse<T>(str);
            EXPECT_TRUE(value && value.is_number());
        }

        auto value = CORE_NS::json::parse<T>("42");
        ASSERT_TRUE(value && value.is_number() && value.is_unsigned_int());
        EXPECT_EQ(value.template as_number<uint32_t>(), 42U); // 42U: test equal input

        value = CORE_NS::json::parse<T>("-42");
        ASSERT_TRUE(value && value.is_number() && value.is_signed_int());
        EXPECT_EQ(value.template as_number<int32_t>(), -42); // -42: test equal input

        value = CORE_NS::json::parse<T>("42.25");
        ASSERT_TRUE(value && value.is_number() && value.is_floating_point());
        EXPECT_EQ(value.template as_number<float>(), 42.25f); // 42.25f: test equal input

        value = CORE_NS::json::parse<T>("-42.25");
        ASSERT_TRUE(value && value.is_number() && value.is_floating_point());
        EXPECT_EQ(value.template as_number<float>(), -42.25f); // -42.25f: test equal input

        value = CORE_NS::json::parse<T>("foo");
        ASSERT_FALSE(value && value.is_number() && value.is_floating_point());
        EXPECT_NE(value.template as_number<float>(), -42.25f); // -42.25f: test not equal input
    }
    {
        BASE_NS::string_view str { "false" };
        auto value = CORE_NS::json::parse<T>(str.data());
        ASSERT_TRUE(value && value.is_boolean());
        EXPECT_EQ(value.boolean_, false);
    }
    {
        BASE_NS::string_view str { "true" };
        auto value = CORE_NS::json::parse<T>(str.data());
        ASSERT_TRUE(value && value.is_boolean());
        EXPECT_EQ(value.boolean_, true);
    }
    {
        BASE_NS::string_view str { "null" };
        auto value = CORE_NS::json::parse<T>(str.data());
        ASSERT_TRUE(value && value.is_null());
    }
    {
        BASE_NS::string_view str { "{}" };
        auto value = CORE_NS::json::parse<T>(str.data());
        EXPECT_TRUE(value && value.is_object());
    }
    {
        BASE_NS::string_view str { "[]" };
        auto value = CORE_NS::json::parse<T>(str.data());
        EXPECT_TRUE(value && value.is_array());
    }
}

/**
 * @tc.name: ParseTypes
 * @tc.desc: test ParseTypes
 * @tc.type: FUNC
 */
HWTEST_F(JsonTest, ParseTypes, TestSize.Level1)
{
    ParseTypes<CORE_NS::json::readonly_tag>();
    ParseTypes<CORE_NS::json::writable_tag>();
}

template<typename T>
void ParseInvalid()
{
    using Json = CORE_NS::json::value_t<T>;
    {
        constexpr const char* objects[] = { "", "{", "{ ", "{foo", "{\"foo", "{\"foo\"", "{\"foo\"42",
            "{\"foo\":", "{\"foo\": ", "{\"foo\": }", "{\"foo\":42", "{\"foo\":42,", "{\"foo\":42,bar",
            "{\"foo\":42,\"bar", "{\"foo\":42,\"bar\"", "{\"foo\":42,\"bar\"", "{\"foo\":42,\"bar\":24", "{} {", "{} }",
            "{} [", "{} ]", "}" };

        for (const auto str : objects) {
            auto value = CORE_NS::json::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
    {
        constexpr const char* arrays[] = { "[", "[foo]", "[\"foo\":]", "[\"foo]", "[1,]", "[\"foo\":42,nul,true]",
            "[] [", "[] ]", "[] {", "[] }", "1, 2" };
        for (const auto str : arrays) {
            auto value = CORE_NS::json::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
    {
        constexpr const char* strings[] = { "\"f", "\"\\", "\"\\a\"", "\"\"\"", "\"\b\"", "\"\f\"", "\"\n\"", "\"\r\"",
            "\"\t\"", "\"\\u00\"" };
        for (const auto str : strings) {
            auto value = CORE_NS::json::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
    {
        constexpr const char* numbers[] = { "01", "0a", "0.0a", "0.0e", "0.0E-", "-a", "-01", "-0a", "-0.0a", "-0.0e",
            "-0.0E-", "1a", "1.0a", "1.0e", "1.0E-", "-1a", "-1.0a", "-1.0e", "-1.0E-", "0." };
        for (const auto str : numbers) {
            auto value = CORE_NS::json::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
    {
        constexpr const char* nulls[] = { "a", "n", "nu", "nult", "nulll" };
        for (const auto str : nulls) {
            auto value = CORE_NS::json::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
    {
        constexpr const char* booleans[] = { "t", "tr", "truu", "truee", "f", "fa", "falss", "falsee" };
        for (const auto str : booleans) {
            auto value = CORE_NS::json::parse<T>(str);
            EXPECT_TRUE(!value);
        }
    }
}

/**
 * @tc.name: Invalid
 * @tc.desc: test Invalid
 * @tc.type: FUNC
 */
HWTEST_F(JsonTest, Invalid, TestSize.Level1)
{
    ParseInvalid<CORE_NS::json::readonly_tag>();
    ParseInvalid<CORE_NS::json::writable_tag>();
}

template<typename T>
void ToString()
{
    using Json = CORE_NS::json::value_t<T>;
    {
        BASE_NS::string_view str {
            "[{\"object\":{}, \"array\":[], \"string\":\"value\", \"number\":42.25, \"boolean\":true, "
            "\"null\":null}, [], \"hello\", 123.456, -123.456, -235, 564, false, true, null]"
        };
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

/**
 * @tc.name: ToString
 * @tc.desc: test ToString
 * @tc.type: FUNC
 */
HWTEST_F(JsonTest, ToString, TestSize.Level1)
{
    ToString<CORE_NS::json::readonly_tag>();
    ToString<CORE_NS::json::writable_tag>();

    {
        // parse a JSON containing Unicode characters, decode the escaped characters, and convert back to JSON. the end
        // result should be the same as the initial JSON.
        constexpr const BASE_NS::string_view in {
            "{\"\\uD835\\uDD77\\uD835\\uDD9A\\uD835\\uDD92\\uD835\\uDD8A\":\"\\u2661\"}"
        };
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
 * @tc.name: StandaloneJson
 * @tc.desc: test StandaloneJson
 * @tc.type: FUNC
 */
HWTEST_F(JsonTest, StandaloneJson, TestSize.Level1)
{
    BASE_NS::string_view str {
        "[{\"foo\":\"bar\"}, [{\"greeting\":\"Hello World\"}], \"hello\", 123.456, 42, -42, false, true, null]"
    };
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
 * @tc.name: Unescape
 * @tc.desc: test Unescape
 * @tc.type: FUNC
 */
HWTEST_F(JsonTest, Unescape, TestSize.Level1)
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
 * @tc.name: Escape
 * @tc.desc: test Escape
 * @tc.type: FUNC
 */
HWTEST_F(JsonTest, Escape, TestSize.Level1)
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