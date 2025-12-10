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

#include <test_framework.h>

#include <base/containers/unordered_map.h>
#include <core/io/intf_file_manager.h>

#include <meta/api/make_callback.h>
#include <meta/api/object.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/loaders/intf_file_content_loader.h>

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

META_BEGIN_NAMESPACE()
namespace UTest {

MATCHER_P(HasProperty, name, "")
{
    const META_NS::IObject::Ptr& object = arg;
    auto meta = interface_cast<META_NS::IMetadata>(object);
    bool found = false;

    if (!meta || !object) {
        *result_listener << "where object type is invalid";
        return false;
    }

    for (auto prop : meta->GetProperties()) {
        if (prop->GetName() == name) {
            found = true;
            break;
        }
    }

    if (!found) {
        *result_listener << "where object did not have property '" << name.data() << "'";
    }
    return found;
}

constexpr BASE_NS::string_view TRANSLATION_STRING_KEY = "strings";

class API_CsvLoaderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        loader_ = META_NS::GetObjectRegistry().Create<IFileContentLoader>(ClassId::CsvStringResourceLoader);
        ASSERT_NE(loader_, nullptr);
    }

    using CsvContentItemType = BASE_NS::pair<BASE_NS::string, BASE_NS::vector<BASE_NS::string>>;
    using CsvContentType = BASE_NS::vector<CsvContentItemType>;

    void CheckTranslations(
        const BASE_NS::string& path, const BASE_NS::vector<BASE_NS::string>& keys, const CsvContentType& content)
    {
        loader_->SetFile(GetTestEnv()->engine->GetFileManager().OpenFile(path));
        Object object(loader_->Create({}));
        ASSERT_TRUE(object);
        ASSERT_NE(object.GetPtr(), nullptr);

        // Object should have a "strings" property
        ASSERT_THAT(object, HasProperty(TRANSLATION_STRING_KEY));
        auto stringsProp = Metadata(object).GetProperty<IObject::Ptr>(TRANSLATION_STRING_KEY);
        ASSERT_TRUE(stringsProp);
        Object strings(stringsProp->GetValue());
        ASSERT_TRUE(strings);
        EXPECT_EQ(Metadata(strings).GetProperties().size(), content.size());

        // Go through our expected content and chech that the object has all of the languages and
        // corresponding strings
        for (const auto& item : content) {
            BASE_NS::string_view lang(item.first);
            ASSERT_THAT(strings, HasProperty(lang));

            auto translationsProp = Metadata(strings).GetProperty<IObject::Ptr>(lang);
            ASSERT_TRUE(translationsProp);
            Object translations(translationsProp->GetValue());
            ASSERT_TRUE(translations);

            // Must contain expected amount of translation strings
            const auto& expectedTranslations = item.second;
            ASSERT_EQ(Metadata(translations).GetProperties().size(), expectedTranslations.size());
            ASSERT_EQ(keys.size(), expectedTranslations.size());

            // Check that each translation found in the object matches our expected translation table
            for (size_t i = 0; i < expectedTranslations.size(); i++) {
                auto prop = Metadata(translations).GetProperty<BASE_NS::string>(keys[i]);
                ASSERT_TRUE(prop) << "lang: " << item.first.data() << ", index: " << i;
                ASSERT_EQ(prop->GetValue(), expectedTranslations[i])
                    << "lang: " << item.first.data() << ", index: " << i;
            }
        }
    }

    IFileContentLoader::Ptr loader_;
};

/**
 * @tc.name: Basic
 * @tc.desc: Tests for Basic. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_CsvLoaderTest, Basic, testing::ext::TestSize.Level1)
{
    constexpr auto path = "test://csv/basic.csv";
    static BASE_NS::vector<BASE_NS::string> keys = { "First string", "Second string", "Click me" };
    static CsvContentType content = { { "fi", { "Eka merkkijono", "Toka merkkijono", "Klikkaa mua" } },
        { "en", { "First string", "Second string", "Click me" } },
        { "zh", { "第一个字符串", "第二个字符串", "点我" } } };

    CheckTranslations(path, keys, content);
}

/**
 * @tc.name: Quotes
 * @tc.desc: Tests for Quotes. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_CsvLoaderTest, Quotes, testing::ext::TestSize.Level1)
{
    constexpr auto path = "test://csv/quotes.csv";
    // The CSV file contains quoted strings with commas, double quotes and also leading and
    // trailing whitespace between opening/closing quote and the delimiter. Those should be
    // handled correctly.
    static BASE_NS::vector<BASE_NS::string> keys = { "Click,me", "Click,\"12" };
    static CsvContentType content = { { "fi", { "Klikkaa,mua", "Klikkaa,\"12" } },
        { "en", { "Click,me", "Click,\"12" } }, { "zh", { "点,我", "点,12" } } };

    CheckTranslations(path, keys, content);
}

/**
 * @tc.name: Multiline
 * @tc.desc: Tests for Multiline. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_CsvLoaderTest, Multiline, testing::ext::TestSize.Level1)
{
    constexpr auto path = "test://csv/multiline.csv";
    // These strings/keys should match the values in above csv file
    static BASE_NS::vector<BASE_NS::string> keys = { "First string", "Click me" };
    static CsvContentType content = { { "fi", { "Eka \nmerkkijono", "Klikkaa mua" } },
        { "en", { "First \nstring", "Click me" } }, { "zh", { "第一\n个字符串", "点我" } } };

    CheckTranslations(path, keys, content);
}

/**
 * @tc.name: Complex
 * @tc.desc: Tests for Complex. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_CsvLoaderTest, Complex, testing::ext::TestSize.Level1)
{
    constexpr auto path = "test://csv/complex.csv";
    // These strings/keys should match the values in above csv file
    static BASE_NS::vector<BASE_NS::string> keys = { "1991", "1992", "1993", "1994" };
    static CsvContentType content = { { "make", { "Ford", "Chevy", "Chevy", "Jeep" } },
        { "model",
            { "E350", "Venture \"Extended Edition\"", "Venture \"Extended Edition, Very Large\"", "Grand Cherokee" } },
        { "description", { "ac, abs, moon", "", "", "MUST SELL!\nair, moon roof, loaded" } },
        { "price", { "3000.00", "4900.00", "5000.00", "4799.00" } } };

    CheckTranslations(path, keys, content);
}

/**
 * @tc.name: CsvInvalid
 * @tc.desc: Tests for Csv Invalid. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_CsvLoaderTest, CsvInvalid, testing::ext::TestSize.Level1)
{
    constexpr auto path1 = "test://csv/invalid_content.csv";
    constexpr auto path2 = "test://csv/invalid_structure.csv";
    constexpr auto path3 = "test://csv/invalid_header.csv";
    constexpr auto path4 = "test://csv/invalid_empty.csv";
    constexpr auto path5 = "test://csv/invalid_file_doesnt_exist.csv";
    int changed = 0;

    loader_->ContentChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([&changed] { changed++; }));

    loader_->SetFile(GetTestEnv()->engine->GetFileManager().OpenFile(path1));
    EXPECT_EQ(loader_->Create({}), nullptr);
    loader_->SetFile(GetTestEnv()->engine->GetFileManager().OpenFile(path2));
    EXPECT_EQ(loader_->Create({}), nullptr);
    loader_->SetFile(GetTestEnv()->engine->GetFileManager().OpenFile(path3));
    EXPECT_EQ(loader_->Create({}), nullptr);
    loader_->SetFile(GetTestEnv()->engine->GetFileManager().OpenFile(path4));
    EXPECT_EQ(loader_->Create({}), nullptr);
    loader_->SetFile(GetTestEnv()->engine->GetFileManager().OpenFile(path5));
    EXPECT_EQ(loader_->Create({}), nullptr);

    EXPECT_EQ(changed, 5);
}

} // namespace UTest
META_END_NAMESPACE()
