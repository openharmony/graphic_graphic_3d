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
#include <memory>

#include "ohos/texture_layer.h"
#include "texture_info.h"
#include "3d_widget_adapter_log.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {

class TextureLayerUT : public ::testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: TextureLayerConstructor001
 * @tc.desc: test TextureLayer constructor
 * @tc.type: FUNC
 */
HWTEST_F(TextureLayerUT, TextureLayerConstructor001, TestSize.Level1)
{
    WIDGET_LOGD("TextureLayerConstructor001");
    int32_t key = 100;

    auto textureLayer = std::make_unique<TextureLayer>(key);

    // Verify texture layer is created
    EXPECT_NE(textureLayer, nullptr);
}

/**
 * @tc.name: GetTextureInfo001
 * @tc.desc: test GetTextureInfo returns default values
 * @tc.type: FUNC
 */
HWTEST_F(TextureLayerUT, GetTextureInfo001, TestSize.Level1)
{
    WIDGET_LOGD("GetTextureInfo001");
    int32_t key = 100;
    auto textureLayer = std::make_unique<TextureLayer>(key);

    TextureInfo textureInfo = textureLayer->GetTextureInfo();

    // Verify default values
    EXPECT_EQ(textureInfo.width_, 0U);
    EXPECT_EQ(textureInfo.height_, 0U);
    EXPECT_EQ(textureInfo.textureId_, 0U);
    EXPECT_EQ(textureInfo.nativeWindow_, nullptr);
    EXPECT_LT(std::fabs(textureInfo.widthScale_ - 1.0f), 0.0001f);
    EXPECT_LT(std::fabs(textureInfo.heightScale_ - 1.0f), 0.0001f);
    EXPECT_LT(std::fabs(textureInfo.customRatio_ - 0.1f), 0.0001f);
    EXPECT_EQ(textureInfo.recreateWindow_, true);
}

/**
 * @tc.name: DestroyRenderTarget001
 * @tc.desc: test DestroyRenderTarget without initialization
 * @tc.type: FUNC
 */
HWTEST_F(TextureLayerUT, DestroyRenderTarget001, TestSize.Level1)
{
    WIDGET_LOGD("DestroyRenderTarget001");
    int32_t key = 100;
    auto textureLayer = std::make_unique<TextureLayer>(key);

    // Should handle gracefully without initialization
    textureLayer->DestroyRenderTarget();

    // Verify texture info is reset
    TextureInfo textureInfo = textureLayer->GetTextureInfo();
    EXPECT_EQ(textureInfo.width_, 0U);
    EXPECT_EQ(textureInfo.height_, 0U);
}

/**
 * @tc.name: SetBackgroundColor001
 * @tc.desc: test SetBackgroundColor with valid color
 * @tc.type: FUNC
 */
HWTEST_F(TextureLayerUT, SetBackgroundColor001, TestSize.Level1)
{
    WIDGET_LOGD("SetBackgroundColor001");
    int32_t key = 100;
    auto textureLayer = std::make_unique<TextureLayer>(key);

    // Should handle gracefully without initialization
    textureLayer->SetBackgroundColor(0xFF0000FF);

    // Verify no crash
    EXPECT_NE(textureLayer, nullptr);
}

/**
 * @tc.name: SetBackgroundColor002
 * @tc.desc: test SetBackgroundColor with different colors
 * @tc.type: FUNC
 */
HWTEST_F(TextureLayerUT, SetBackgroundColor002, TestSize.Level1)
{
    WIDGET_LOGD("SetBackgroundColor002");
    int32_t key = 100;
    auto textureLayer = std::make_unique<TextureLayer>(key);

    // Test multiple color settings
    textureLayer->SetBackgroundColor(0x00000000); // transparent
    textureLayer->SetBackgroundColor(0xFFFFFFFF); // white
    textureLayer->SetBackgroundColor(0xFF000000); // black
    textureLayer->SetBackgroundColor(0x00FF00FF); // magenta

    // Verify no crash
    EXPECT_NE(textureLayer, nullptr);
}

/**
 * @tc.name: OnWindowChange001
 * @tc.desc: test OnWindowChange with basic parameters
 * @tc.type: FUNC
 */
HWTEST_F(TextureLayerUT, OnWindowChange001, TestSize.Level1)
{
    WIDGET_LOGD("OnWindowChange001");
    int32_t key = 100;
    auto textureLayer = std::make_unique<TextureLayer>(key);

    float offsetX = 10.0f;
    float offsetY = 20.0f;
    float width = 1080.0f;
    float height = 1920.0f;
    float scale = 1.0f;
    bool recreateWindow = false;
    SurfaceType surfaceType = SurfaceType::SURFACE_TEXTURE;

    TextureInfo textureInfo = textureLayer->OnWindowChange(
        offsetX, offsetY, width, height, scale, recreateWindow, surfaceType);

    // Verify texture info is updated
    EXPECT_EQ(textureInfo.width_, static_cast<uint32_t>(width));
    EXPECT_EQ(textureInfo.height_, static_cast<uint32_t>(height));
    EXPECT_EQ(textureInfo.recreateWindow_, recreateWindow);
}

/**
 * @tc.name: OnWindowChange002
 * @tc.desc: test OnWindowChange with recreateWindow flag
 * @tc.type: FUNC
 */
HWTEST_F(TextureLayerUT, OnWindowChange002, TestSize.Level1)
{
    WIDGET_LOGD("OnWindowChange002");
    int32_t key = 100;
    auto textureLayer = std::make_unique<TextureLayer>(key);

    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float width = 1920.0f;
    float height = 1080.0f;
    float scale = 2.0f;
    bool recreateWindow = true;
    SurfaceType surfaceType = SurfaceType::SURFACE_WINDOW;

    TextureInfo textureInfo = textureLayer->OnWindowChange(
        offsetX, offsetY, width, height, scale, recreateWindow, surfaceType);

    // Verify texture info is updated
    EXPECT_EQ(textureInfo.width_, static_cast<uint32_t>(width));
    EXPECT_EQ(textureInfo.height_, static_cast<uint32_t>(height));
    EXPECT_EQ(textureInfo.recreateWindow_, recreateWindow);
}

/**
 * @tc.name: OnWindowChange003
 * @tc.desc: test OnWindowChange with WindowChangeInfo
 * @tc.type: FUNC
 */
HWTEST_F(TextureLayerUT, OnWindowChange003, TestSize.Level1)
{
    WIDGET_LOGD("OnWindowChange003");
    int32_t key = 100;
    auto textureLayer = std::make_unique<TextureLayer>(key);

    WindowChangeInfo windowChangeInfo;
    windowChangeInfo.offsetX = 50.0f;
    windowChangeInfo.offsetY = 50.0f;
    windowChangeInfo.width = 800.0f;
    windowChangeInfo.height = 600.0f;
    windowChangeInfo.scale = 1.5f;
    windowChangeInfo.widthScale = 0.8f;
    windowChangeInfo.heightScale = 0.8f;
    windowChangeInfo.recreateWindow = false;
    windowChangeInfo.surfaceType = SurfaceType::SURFACE_TEXTURE;
    windowChangeInfo.transformType = 90;
    windowChangeInfo.backgroundColor = 0xFF0000FF;
    windowChangeInfo.producerSurfaceId = 0x0;

    TextureInfo textureInfo = textureLayer->OnWindowChange(windowChangeInfo);

    // Verify texture info is updated
    EXPECT_EQ(textureInfo.width_, static_cast<uint32_t>(windowChangeInfo.width));
    EXPECT_EQ(textureInfo.height_, static_cast<uint32_t>(windowChangeInfo.height));
    EXPECT_LT(std::fabs(textureInfo.widthScale_ - windowChangeInfo.widthScale), 0.0001f);
    EXPECT_LT(std::fabs(textureInfo.heightScale_ - windowChangeInfo.heightScale), 0.0001f);
    EXPECT_EQ(textureInfo.recreateWindow_, windowChangeInfo.recreateWindow);
}

/**
 * @tc.name: OnWindowChange004
 * @tc.desc: test OnWindowChange with different surface types
 * @tc.type: FUNC
 */
HWTEST_F(TextureLayerUT, OnWindowChange004, TestSize.Level1)
{
    WIDGET_LOGD("OnWindowChange004");
    int32_t key = 100;
    auto textureLayer = std::make_unique<TextureLayer>(key);

    // Test SURFACE_WINDOW
    WindowChangeInfo info1;
    info1.width = 1080.0f;
    info1.height = 1920.0f;
    info1.surfaceType = SurfaceType::SURFACE_WINDOW;
    TextureInfo textureInfo1 = textureLayer->OnWindowChange(info1);
    EXPECT_EQ(textureInfo1.width_, 1080U);

    // Test SURFACE_TEXTURE
    WindowChangeInfo info2;
    info2.width = 800.0f;
    info2.height = 600.0f;
    info2.surfaceType = SurfaceType::SURFACE_TEXTURE;
    TextureInfo textureInfo2 = textureLayer->OnWindowChange(info2);
    EXPECT_EQ(textureInfo2.width_, 800U);

    // Test SURFACE_BUFFER
    WindowChangeInfo info3;
    info3.width = 640.0f;
    info3.height = 480.0f;
    info3.surfaceType = SurfaceType::SURFACE_BUFFER;
    TextureInfo textureInfo3 = textureLayer->OnWindowChange(info3);
    EXPECT_EQ(textureInfo3.width_, 640U);
}

/**
 * @tc.name: OnWindowChange005
 * @tc.desc: test OnWindowChange with default surface type
 * @tc.type: FUNC
 */
HWTEST_F(TextureLayerUT, OnWindowChange005, TestSize.Level1)
{
    WIDGET_LOGD("OnWindowChange005");
    int32_t key = 100;
    auto textureLayer = std::make_unique<TextureLayer>(key);

    WindowChangeInfo windowChangeInfo;
    windowChangeInfo.width = 1024.0f;
    windowChangeInfo.height = 768.0f;
    windowChangeInfo.surfaceType = SurfaceType::UNDEFINE; // Should default to SURFACE_WINDOW

    TextureInfo textureInfo = textureLayer->OnWindowChange(windowChangeInfo);

    // Verify texture info is updated with default surface type
    EXPECT_EQ(textureInfo.width_, 1024U);
    EXPECT_EQ(textureInfo.height_, 768U);
}

/**
 * @tc.name: SetParent001
 * @tc.desc: test SetParent with null parent
 * @tc.type: FUNC
 */
HWTEST_F(TextureLayerUT, SetParent001, TestSize.Level1)
{
    WIDGET_LOGD("SetParent001");
    int32_t key = 100;
    auto textureLayer = std::make_unique<TextureLayer>(key);

    std::shared_ptr<Rosen::RSNode> parent = nullptr;

    // Should handle gracefully with null parent
    textureLayer->SetParent(parent);

    // Verify no crash
    EXPECT_NE(textureLayer, nullptr);
}

} // namespace OHOS::Render3D
