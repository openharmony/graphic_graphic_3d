/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "gtest/gtest.h"

#include "texture_layer.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {
class TextureLayerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void TextureLayerTest::SetUpTestCase() {}
void TextureLayerTest::TearDownTestCase() {}
void TextureLayerTest::SetUp() {}
void TextureLayerTest::TearDown() {}

/**
 * @tc.name: CreateTextureImage001
 * @tc.desc: create default texture image
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(TextureLayerTest, CreateTextureImage001, TestSize.Level1)
{
    TextureImage image = TextureImage();
}

/**
 * @tc.name: CreateTextureImage002
 * @tc.desc: create texture image with texture info
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(TextureLayerTest, CreateTextureImage002, TestSize.Level1)
{
    TextureInfo textureInfo;
    TextureImage image(textureInfo);
}

/**
 * @tc.name: CreateTextureLayer001
 * @tc.desc: create texture image with texture info
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(TextureLayerTest, CreateTextureLayer001, TestSize.Level1)
{
    std::shared_ptr<TextureLayer> layer = std::make_shared<TextureLayer>();
    EXPECT_TRUE(nullptr != layer);
}

/**
 * @tc.name: GetBounds001
 * @tc.desc: get texture layer bounding-box
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(TextureLayerTest, GetBounds001, TestSize.Level1)
{
    std::shared_ptr<TextureLayer> layer = std::make_shared<TextureLayer>();
    ASSERT_TRUE(nullptr != layer);
    layer->onGetBounds();
}

/**
 * @tc.name: OnDraw001
 * @tc.desc: draw texture layer
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(TextureLayerTest, OnDraw001, TestSize.Level1)
{
    std::shared_ptr<TextureLayer> layer = std::make_shared<TextureLayer>();
    ASSERT_TRUE(nullptr != layer);
    SkCanvas canvas;
    layer->onDraw(&canvas);
}

/**
 * @tc.name: UpdateTexImageAsync001
 * @tc.desc: update texture image asyncly
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(TextureLayerTest, UpdateTexImageAsync001, TestSize.Level1)
{
    std::shared_ptr<TextureLayer> layer = std::make_shared<TextureLayer>();
    ASSERT_TRUE(nullptr != layer);
    std::shared_future<void> ftr;
    TextureInfo textureInfo;
    layer->UpdateAsyncTexImage(ftr, textureInfo, 0, 0);
}

/**
 * @tc.name: UpdateTexImageSync001
 * @tc.desc: update texture image syncly
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(TextureLayerTest, UpdateTexImageSync001, TestSize.Level1)
{
    std::shared_ptr<TextureLayer> layer = std::make_shared<TextureLayer>();
    ASSERT_TRUE(nullptr != layer);
    std::shared_future<void> ftr = layer->UpdateSyncTexImage([]() {});
    EXPECT_TRUE(ftr.valid());
}
}