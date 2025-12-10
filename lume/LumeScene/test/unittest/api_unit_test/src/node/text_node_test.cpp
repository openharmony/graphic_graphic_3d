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

#include <scene/interface/intf_node.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/intf_text.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginTextNodeTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        ScenePluginTest::SetUp();
        auto scene = CreateEmptyScene();
        auto node = scene->CreateNode("//test", ClassId::TextNode).GetResult();
        ASSERT_TRUE(node);
        node->Position()->SetValue({ 0, 0, 0 });
        text_ = interface_pointer_cast<IText>(node);
        ASSERT_TRUE(text_);
        UpdateScene();
    }

    void TearDown() override
    {
        text_.reset();
    }

protected:
    IText::Ptr text_;
};

/**
 * @tc.name: Text
 * @tc.desc: Tests for Text. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginTextNodeTest, Text, testing::ext::TestSize.Level1)
{
    text_->Text()->SetValue("Hello");
    text_->FontFamily()->SetValue("Segoe UI");
    text_->FontStyle()->SetValue("Regular");
    text_->FontSize()->SetValue(10.0f);
    text_->Font3DThickness()->SetValue(10.0f);
    text_->FontMethod()->SetValue(FontMethod::RASTER);
    text_->TextColor()->SetValue({ 1.0f, 0.0f, 1.0f, 0.0f });

    UpdateScene();

    EXPECT_EQ(text_->Text()->GetValue(), "Hello");
    EXPECT_EQ(text_->FontFamily()->GetValue(), "Segoe UI");
    EXPECT_EQ(text_->FontStyle()->GetValue(), "Regular");
    EXPECT_EQ(text_->FontSize()->GetValue(), 10.0f);
    EXPECT_EQ(text_->Font3DThickness()->GetValue(), 10.0f);
    EXPECT_EQ(text_->FontMethod()->GetValue(), FontMethod::RASTER);
    EXPECT_EQ(text_->TextColor()->GetValue(), BASE_NS::Color(1.0f, 0.0f, 1.0f, 0.0f));

    text_->Text()->SetValue("Other");
    UpdateScene();

    EXPECT_EQ(text_->TextColor()->GetValue(), BASE_NS::Color(1.0f, 0.0f, 1.0f, 0.0f));
}

} // namespace UTest

SCENE_END_NAMESPACE()