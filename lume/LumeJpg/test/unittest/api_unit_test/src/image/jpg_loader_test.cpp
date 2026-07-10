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

#include <algorithm>
#include <cstdint>
#include <jpg/implementation_uids.h>
#include <vector>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <core/image/intf_image_container.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/plugin/intf_plugin_register.h>

#include "test_framework.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
using namespace BASE_NS;
using namespace CORE_NS;

namespace {

bool EnsureJpgPlugin()
{
    static bool loaded = []() {
        constexpr Uid jpg[]{JPGPlugin::UID_JPG_PLUGIN};
        return GetPluginRegister().LoadPlugins(jpg);
    }();
    return loaded;
}

struct TestImageData {
    uint32_t elementIndex;
    void* data;
};

struct TestImage {
    string imageUri;

    uint32_t width;
    uint32_t height;
    uint32_t depth;

    uint32_t blockPixelWidth;
    uint32_t blockPixelHeight;
    uint32_t blockPixelDepth;
    uint32_t bitsPerBlock;

    uint32_t mipCount;
    uint32_t layerCount;
    uint32_t imageFlags;

    Format format;
    IImageContainer::ImageType imageType;
    IImageContainer::ImageViewType imageViewType;

    uint32_t loaderFlags;

    std::vector<TestImageData> pixelDatas;
};

// NOTE: gtest asserts can only be called in a void function.
void CheckImage(CORE_NS::IImageLoaderManager* imageManager, const string_view aTest, const TestImage& aTestImage,
    IImageContainer::Ptr& aImageOut)
{
    string messageStr =
        "Test: '" +
        aTest;  //+"' image: '" + std::string_view(aTestImage.imageUri.data(), aTestImage.imageUri.size()) + "'";
    auto message = std::string_view(messageStr.data(), messageStr.size());

    auto& fileManager = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(fileManager != nullptr) << message;

    // Check that the file exists first.
    {
        auto file = fileManager->OpenFile(aTestImage.imageUri);
        ASSERT_TRUE(file != nullptr) << "Could not open file: " << message;
    }

    auto result = imageManager->LoadImage(aTestImage.imageUri.c_str(), aTestImage.loaderFlags);
    ASSERT_TRUE(result.success) << message << " error: " << result.error;
    ASSERT_NE(result.image, nullptr) << message;

    auto imageData = result.image->GetData();
    ASSERT_TRUE(imageData.size() != 0) << message;

    // Check that the loaded ImageDesc data matched with the expected.
    const IImageContainer::ImageDesc& imageDesc = result.image->GetImageDesc();

    const TestImage& expected = aTestImage;

    ASSERT_EQ(imageDesc.imageFlags, expected.imageFlags) << message;
    ASSERT_EQ(imageDesc.blockPixelWidth, expected.blockPixelWidth) << message;
    ASSERT_EQ(imageDesc.blockPixelHeight, expected.blockPixelHeight) << message;
    ASSERT_EQ(imageDesc.blockPixelDepth, expected.blockPixelDepth) << message;
    ASSERT_EQ(imageDesc.bitsPerBlock, expected.bitsPerBlock) << message;

    ASSERT_EQ(imageDesc.width, expected.width) << message;
    ASSERT_EQ(imageDesc.height, expected.height) << message;
    ASSERT_EQ(imageDesc.depth, expected.depth) << message;

    ASSERT_EQ(imageDesc.mipCount, expected.mipCount) << message;
    ASSERT_EQ(imageDesc.layerCount, expected.layerCount) << message;

    ASSERT_EQ(imageDesc.format, expected.format) << message;
    ASSERT_EQ(imageDesc.imageType, expected.imageType) << message;
    ASSERT_EQ(imageDesc.imageViewType, expected.imageViewType) << message;

    // Check image element data.
    auto imageBuffers = result.image->GetBufferImageCopies();
    const bool cubeMap = (imageDesc.imageFlags & IImageContainer::ImageFlags::FLAGS_CUBEMAP_BIT) != 0;

    const uint32_t bytesPerBlock = imageDesc.bitsPerBlock / 8;

    ASSERT_EQ(imageBuffers.size(), imageDesc.mipCount) << message;

    for (auto& imageSubelement : imageBuffers) {
        ASSERT_LT(imageSubelement.bufferOffset, imageData.size()) << message;

        // Ktx requires alignment to 4 bytes.
        ASSERT_EQ(imageSubelement.bufferOffset % 4, 0)
            << " Offset=" << imageSubelement.bufferOffset << " BytesPerBlock=" << bytesPerBlock << " " << message;

        // Vulkan requires that: "If the calling command's VkImage parameter's
        // format is not a depth/stencil format or a multi-planar format, then
        // bufferOffset must be a multiple of the format's texel block size."

        // Also take into account that the alignment to 4 bytes.
        const uint32_t alignment = ((bytesPerBlock - 1) / 4 + 1) * 4;

        ASSERT_EQ(imageSubelement.bufferOffset % alignment, 0)
            << " Offset=" << imageSubelement.bufferOffset << " BytesPerBlock=" << bytesPerBlock << " " << message;

        // For uncompressed formats check that the total buffer size matches with the format information.
        if ((imageDesc.imageFlags & IImageContainer::ImageFlags::FLAGS_COMPRESSED_BIT) == 0) {
            // NOTE: This is only correct for uncompressed formats.
            const uint32_t bytesPerPixel = bytesPerBlock / imageDesc.blockPixelWidth;

            const size_t subelementDataLength = bytesPerPixel * imageSubelement.bufferRowLength *
                                                imageSubelement.bufferImageHeight * imageSubelement.depth;
            ASSERT_LE(imageSubelement.bufferOffset + subelementDataLength, imageData.size()) << message;
        }
    }

    // Only checking the first element size for now.
    uint32_t bufferIndex = 0;
    ASSERT_EQ(imageBuffers[bufferIndex].layerCount, expected.layerCount) << message;
    ASSERT_EQ(imageBuffers[bufferIndex].width, expected.width) << message;
    ASSERT_EQ(imageBuffers[bufferIndex].height, expected.height) << message;
    ASSERT_EQ(imageBuffers[bufferIndex].depth, expected.depth) << message;

    aImageOut = std::move(result.image);
}

void CheckImageData(const char* aTest, const IImageContainer& aImage, uint32_t aElementIndex, void* pixelData)
{
    const IImageContainer::ImageDesc& imageDesc = aImage.GetImageDesc();
    const auto imageBuffers = aImage.GetBufferImageCopies();
    const uint8_t* imageData = &aImage.GetData()[0];
    ASSERT_TRUE(imageData != nullptr);

    // NOTE: this test only works for noncompressed formats (not accounting for block pixel size).
    ASSERT_TRUE((imageDesc.imageFlags & IImageContainer::ImageFlags::FLAGS_COMPRESSED_BIT) == 0);

    const unsigned int bytesPerBlock = imageDesc.bitsPerBlock / 8;

    const uint8_t* expected = (uint8_t*)pixelData;
    const uint8_t* p = &imageData[0] + imageBuffers[aElementIndex].bufferOffset;

    for (size_t y = 0; y < imageBuffers[aElementIndex].height; y++) {
        const uint8_t* row = p + imageBuffers[aElementIndex].bufferRowLength * bytesPerBlock * y;

        for (size_t x = 0; x < imageBuffers[aElementIndex].width; x++) {
            for (size_t byte = 0; byte < bytesPerBlock; byte++) {
                uint8_t byteValue = *row++;
                uint8_t expectedValue = *expected++;
                ASSERT_EQ(byteValue, expectedValue) << " x=" << x << " y=" << y << " byte=" << byte;
            }
        }
    }
}

constexpr IImageContainer::ImageType TYPE_2D = IImageContainer::ImageType::TYPE_2D;
constexpr IImageContainer::ImageViewType VTYPE_2D = IImageContainer::ImageViewType::VIEW_TYPE_2D;
constexpr uint32_t lrgb = IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
}  // namespace

class API_JpgLoaderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(EnsureJpgPlugin());
        m_imageLoaderManager = &UTest::GetTestEnv()->engine->GetImageLoaderManager();
        ASSERT_NE(m_imageLoaderManager, nullptr);
    }

    IImageLoaderManager* m_imageLoaderManager{nullptr};
};

/**
 * @tc.name: JpgPluginLoaded
 * @tc.desc: Verify that the JPG plugin loads successfully and registers expected MIME types.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_JpgLoaderTest, JpgPluginLoaded, testing::ext::TestSize.Level1)
{
    EXPECT_TRUE(EnsureJpgPlugin());

    auto& loaderManager = *m_imageLoaderManager;
    const auto types = loaderManager.GetSupportedTypes();

    bool foundJpg = false;
    bool foundJpeg = false;
    for (const auto& type : types) {
        if (type.mimeType == string_view("image/jpeg")) {
            if (type.fileExtension == string_view("jpg")) {
                foundJpg = true;
            }
            if (type.fileExtension == string_view("jpeg")) {
                foundJpeg = true;
            }
        }
    }
    EXPECT_TRUE(foundJpg) << "Expected 'jpg' extension not found in supported types";
    EXPECT_TRUE(foundJpeg) << "Expected 'jpeg' extension not found in supported types";
}

/**
 * @tc.name: LoadImages
 * @tc.desc: Load a few different images.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_JpgLoaderTest, LoadImages, testing::ext::TestSize.Level1)
{
    const TestImage testImages[] = {
        {"test://image/canine_284x323.jpg",
            284,
            323,
            1,
            1,
            1,
            1,
            32,
            1,
            1,
            0,
            Format::BASE_FORMAT_R8G8B8A8_UNORM,
            TYPE_2D,
            VTYPE_2D,
            lrgb},
        {"test://image/canine_512x512.jpg",
            512,
            512,
            1,
            1,
            1,
            1,
            32,
            1,
            1,
            0,
            Format::BASE_FORMAT_R8G8B8A8_UNORM,
            TYPE_2D,
            VTYPE_2D,
            lrgb},
    };

    for (const auto& testInput : testImages) {
        IImageContainer::Ptr image;
        ASSERT_NO_FATAL_FAILURE(CheckImage(m_imageLoaderManager, "loadImages", testInput, image))
            << testInput.imageUri.data();
        ASSERT_TRUE(image != nullptr) << testInput.imageUri.data();

        for (auto& pixelData : testInput.pixelDatas) {
            ASSERT_NO_FATAL_FAILURE(CheckImageData("loadImages", *image, pixelData.elementIndex, pixelData.data))
                << testInput.imageUri.data();
        }
    }
}

/**
 * @tc.name: LoadFromFile
 * @tc.desc: Load a JPEG via IFile handle and verify dimensions.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_JpgLoaderTest, LoadFromFile, testing::ext::TestSize.Level1)
{
    auto& fileManager = UTest::GetTestEnv()->engine->GetFileManager();
    auto file = fileManager.OpenFile("test://image/canine_284x323.jpg");
    ASSERT_NE(file, nullptr) << "Could not open test://image/canine_284x323.jpg";

    constexpr uint32_t loadFlags = IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
    auto result = m_imageLoaderManager->LoadImage(*file, loadFlags);

    ASSERT_TRUE(result.success) << "Failed to load image from IFile";
    ASSERT_NE(result.image, nullptr);

    const auto& imageDesc = result.image->GetImageDesc();
    EXPECT_EQ(imageDesc.width, 284u);
    EXPECT_EQ(imageDesc.height, 323u);
}

/**
 * @tc.name: LoadFromBuffer
 * @tc.desc: Load a JPEG from a raw byte buffer and verify dimensions.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_JpgLoaderTest, LoadFromBuffer, testing::ext::TestSize.Level1)
{
    auto& fileManager = UTest::GetTestEnv()->engine->GetFileManager();
    auto file = fileManager.OpenFile("test://image/canine_284x323.jpg");
    ASSERT_NE(file, nullptr) << "Could not open test://image/canine_284x323.jpg for buffer test";

    const uint64_t fileSize = file->GetLength();
    ASSERT_GT(fileSize, 0u);

    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    const uint64_t bytesRead = file->Read(buffer.data(), fileSize);
    ASSERT_EQ(bytesRead, fileSize);

    constexpr uint32_t loadFlags = IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
    auto result = m_imageLoaderManager->LoadImage(array_view<const uint8_t>(buffer.data(), buffer.size()), loadFlags);

    ASSERT_TRUE(result.success) << "Failed to load image from buffer";
    ASSERT_NE(result.image, nullptr);

    const auto& imageDesc = result.image->GetImageDesc();
    EXPECT_EQ(imageDesc.width, 284u);
    EXPECT_EQ(imageDesc.height, 323u);
}

/**
 * @tc.name: MetadataOnly
 * @tc.desc: Load only metadata (dimensions) without decoding pixel data.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_JpgLoaderTest, MetadataOnly, testing::ext::TestSize.Level1)
{
    constexpr uint32_t loadFlags =
        IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT | IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY;
    auto result = m_imageLoaderManager->LoadImage("test://image/canine_284x323.jpg", loadFlags);

    ASSERT_TRUE(result.success) << "Metadata-only load failed";
    ASSERT_NE(result.image, nullptr);

    const auto& imageDesc = result.image->GetImageDesc();
    EXPECT_EQ(imageDesc.width, 284u);
    EXPECT_EQ(imageDesc.height, 323u);
}

/**
 * @tc.name: LoadInvalidBytes
 * @tc.desc: Pass invalid (all-zero) bytes; expect graceful failure with no crash.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_JpgLoaderTest, LoadInvalidBytes, testing::ext::TestSize.Level1)
{
    const uint8_t invalidData[16] = {};
    auto result = m_imageLoaderManager->LoadImage(array_view<const uint8_t>(invalidData, sizeof(invalidData)),
        IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT);

    EXPECT_FALSE(result.success) << "Loading invalid bytes should fail gracefully";
    // No crash is also a requirement — reaching here satisfies it.
}

/**
 * @tc.name: SecurityLargeDimensions
 * @tc.desc: Security regression: malformed JPEG claiming huge dimensions must not crash or hang (VULN-032).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_JpgLoaderTest, SecurityLargeDimensions, testing::ext::TestSize.Level1)
{
    // This file contains a crafted JPEG header with extreme dimensions.
    // The loader must handle it gracefully: either fail or succeed, but never crash.
    auto result = m_imageLoaderManager->LoadImage("test://image/security/VULN-032_jpg_large_dimensions.jpg",
        IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT);

    // Reaching this point without a crash or hang satisfies the regression requirement.
    // We additionally expect the load to fail for such a corrupt/crafted file.
    EXPECT_FALSE(result.success) << "Crafted large-dimensions JPEG should be rejected gracefully";
}
