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

#include <algorithm>
#include <cstdint>
#include <png/implementation_uids.h>
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

bool EnsurePngPlugin()
{
    static bool loaded = []() {
        constexpr Uid png[]{PNGPlugin::UID_PNG_PLUGIN};
        return GetPluginRegister().LoadPlugins(png);
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
    string messageStr = "Test: '" + aTest;
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
constexpr uint32_t srgb = IImageLoaderManager::IMAGE_LOADER_FORCE_SRGB_BIT;
constexpr uint32_t premult =
    IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT | IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA;
constexpr uint32_t premultS =
    IImageLoaderManager::IMAGE_LOADER_FORCE_SRGB_BIT | IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA;
}  // namespace

class API_PngLoaderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(EnsurePngPlugin());
        m_imageLoaderManager = &UTest::GetTestEnv()->engine->GetImageLoaderManager();
        ASSERT_NE(m_imageLoaderManager, nullptr);
    }

    IImageLoaderManager* m_imageLoaderManager{nullptr};
};

/**
 * @tc.name: PngPluginLoaded
 * @tc.desc: Verify that the PNG plugin loads successfully and registers expected MIME types.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_PngLoaderTest, PngPluginLoaded, testing::ext::TestSize.Level1)
{
    EXPECT_TRUE(EnsurePngPlugin());

    auto& loaderManager = *m_imageLoaderManager;
    const auto types = loaderManager.GetSupportedTypes();

    bool foundPng = false;
    for (const auto& type : types) {
        if (type.mimeType == string_view("image/png")) {
            if (type.fileExtension == string_view("png")) {
                foundPng = true;
            }
        }
    }
    EXPECT_TRUE(foundPng) << "Expected 'png' extension not found in supported types";
}

/**
 * @tc.name: LoadImages
 * @tc.desc: Load a variety of PNG images covering different channel counts, bit depths, and loader flags.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_PngLoaderTest, LoadImages, testing::ext::TestSize.Level1)
{
    // The test_image contains the following RGBA pixel data:
    const uint8_t pixelDataRGBA8[] = {
        0x00,
        0x00,
        0x00,
        0xff,  // [black]
        0xff,
        0xff,
        0xff,
        0xff,  // [white]
        0x80,
        0x80,
        0x80,
        0xff,  // [gray]
        0xff,
        0x00,
        0x00,
        0xff,  // [red]
        0x00,
        0xff,
        0x00,
        0xff,  // [green]
        0x00,
        0x00,
        0xff,
        0xff,  // [blue]
        0xff,
        0xff,
        0xff,
        0x80,  // [50% transparent white] (not premultiplied)
        0x00,
        0x00,
        0x00,
        0x00,  // [100% transparent]
    };

    const uint16_t pixelDataRGBA16[] = {
        0x0000,
        0x0000,
        0x0000,
        0xffff,  // [black]
        0xffff,
        0xffff,
        0xffff,
        0xffff,  // [white]
        0x80 << 8,
        0x80 << 8,
        0x80 << 8,
        0xffff,  // [gray]
        0xffff,
        0x0000,
        0x0000,
        0xffff,  // [red]
        0x0000,
        0xffff,
        0x0000,
        0xffff,  // [green]
        0x0000,
        0x0000,
        0xffff,
        0xffff,  // [blue]
        0xffff,
        0xffff,
        0xffff,
        0x80 << 8,  // [50% transparent white] (not premultiplied)
        0x0000,
        0x0000,
        0x0000,
        0x00 << 8,  // [100% transparent]
    };

    const uint8_t pixelDataRGBA8Premultiplied[] = {
        0x00,
        0x00,
        0x00,
        0xff,  // [black]
        0xff,
        0xff,
        0xff,
        0xff,  // [white]
        0x80,
        0x80,
        0x80,
        0xff,  // [gray]
        0xff,
        0x00,
        0x00,
        0xff,  // [red]
        0x00,
        0xff,
        0x00,
        0xff,  // [green]
        0x00,
        0x00,
        0xff,
        0xff,  // [blue]
        0x80,
        0x80,
        0x80,
        0x80,  // [50% transparent white] (premultiplied)
        0x00,
        0x00,
        0x00,
        0x00,  // [100% transparent]
    };

    const uint16_t pixelDataRGBA16Premultiplied[] = {
        0x0000,
        0x0000,
        0x0000,
        0xffff,  // [black]
        0xffff,
        0xffff,
        0xffff,
        0xffff,  // [white]
        0x80 << 8,
        0x80 << 8,
        0x80 << 8,
        0xffff,  // [gray]
        0xffff,
        0x0000,
        0x0000,
        0xffff,  // [red]
        0x0000,
        0xffff,
        0x0000,
        0xffff,  // [green]
        0x0000,
        0x0000,
        0xffff,
        0xffff,  // [blue]
        0x8000,
        0x8000,
        0x8000,
        0x8000,  // [50% transparent white] (premultiplied)
        0x0000,
        0x0000,
        0x0000,
        0x0000,  // [100% transparent]
    };

    const uint8_t pixelDataSRGBA8Premultiplied[] = {
        0x00,
        0x00,
        0x00,
        0xff,  // [black]
        0xff,
        0xff,
        0xff,
        0xff,  // [white]
        0x80,
        0x80,
        0x80,
        0xff,  // [gray]
        0xff,
        0x00,
        0x00,
        0xff,  // [red]
        0x00,
        0xff,
        0x00,
        0xff,  // [green]
        0x00,
        0x00,
        0xff,
        0xff,  // [blue]
        0xBC,
        0xBC,
        0xBC,
        0x80,  // [50% transparent white] (premultiplied)
        0x00,
        0x00,
        0x00,
        0x00,  // [100% transparent]
    };

    // 256 pixel width gradient. The first pixel value in the image is 0x00 and the last 0xff.
    uint8_t gradientPixelDataR8[256];
    for (size_t i = 0; i < 256; i++) {
        gradientPixelDataR8[i] = static_cast<uint8_t>(i);
    }

    // 256x256 16bpc gradient. The first pixel value in the image is 0x0000 and the last 0xffff.
    uint16_t gradientPixelDataR16[256 * 256];
    for (size_t i = 0; i < 256 * 256; i++) {
        gradientPixelDataR16[i] = static_cast<uint16_t>(i);
    }

    const TestImage testImages[] = {
        // Pixel-verified 8bpc and 16bpc RGBA images
        {"test://image/test_image_8x1_RGBA.png",
            8,
            1,
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
            lrgb,
            {{0, (void*)pixelDataRGBA8}}},
        {"test://image/test_image_8x1_RGBA.png",
            8,
            1,
            1,
            1,
            1,
            1,
            32,
            1,
            1,
            0,
            Format::BASE_FORMAT_R8G8B8A8_SRGB,
            TYPE_2D,
            VTYPE_2D,
            srgb,
            {{0, (void*)pixelDataRGBA8}}},
        {"test://image/test_image_8x1_RGBA16.png",
            8,
            1,
            1,
            1,
            1,
            1,
            64,
            1,
            1,
            0,
            Format::BASE_FORMAT_R16G16B16A16_UNORM,
            TYPE_2D,
            VTYPE_2D,
            lrgb,
            {{0, (void*)pixelDataRGBA16}}},
        {"test://image/gradient_256x1_R8.png",
            256,
            1,
            1,
            1,
            1,
            1,
            8,
            1,
            1,
            0,
            Format::BASE_FORMAT_R8_UNORM,
            TYPE_2D,
            VTYPE_2D,
            lrgb,
            {{0, (void*)gradientPixelDataR8}}},
        {"test://image/gradient_256x256_R16.png",
            256,
            256,
            1,
            1,
            1,
            1,
            16,
            1,
            1,
            0,
            Format::BASE_FORMAT_R16_UNORM,
            TYPE_2D,
            VTYPE_2D,
            lrgb,
            {{0, (void*)gradientPixelDataR16}}},
        // Premultiplied alpha
        {"test://image/test_image_8x1_RGBA.png",
            8,
            1,
            1,
            1,
            1,
            1,
            32,
            1,
            1,
            IImageContainer::ImageFlags::FLAGS_PREMULTIPLIED_ALPHA_BIT,
            Format::BASE_FORMAT_R8G8B8A8_UNORM,
            TYPE_2D,
            VTYPE_2D,
            premult,
            {{0, (void*)pixelDataRGBA8Premultiplied}}},
        {"test://image/test_image_8x1_RGBA16.png",
            8,
            1,
            1,
            1,
            1,
            1,
            64,
            1,
            1,
            IImageContainer::ImageFlags::FLAGS_PREMULTIPLIED_ALPHA_BIT,
            Format::BASE_FORMAT_R16G16B16A16_UNORM,
            TYPE_2D,
            VTYPE_2D,
            premult,
            {{0, (void*)pixelDataRGBA16Premultiplied}}},
        {"test://image/test_image_8x1_RGBA.png",
            8,
            1,
            1,
            1,
            1,
            1,
            32,
            1,
            1,
            IImageContainer::ImageFlags::FLAGS_PREMULTIPLIED_ALPHA_BIT,
            Format::BASE_FORMAT_R8G8B8A8_SRGB,
            TYPE_2D,
            VTYPE_2D,
            premultS,
            {{0, (void*)pixelDataSRGBA8Premultiplied}}},
        // 512x512 canine
        {"test://image/canine_512x512.png",
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
        // 8bpc channel variants (default flags → sRGB)
        {"test://image/png/canine_128x128_R8.png",
            128,
            128,
            1,
            1,
            1,
            1,
            8,
            1,
            1,
            0,
            Format::BASE_FORMAT_R8_SRGB,
            TYPE_2D,
            VTYPE_2D,
            0},
        {"test://image/png/canine_128x128_RG8.png",
            128,
            128,
            1,
            1,
            1,
            1,
            32,
            1,
            1,
            0,
            Format::BASE_FORMAT_R8G8B8A8_SRGB,
            TYPE_2D,
            VTYPE_2D,
            0},  // gray+alpha → RGBA
        {"test://image/png/canine_128x128_RGB8.png",
            128,
            128,
            1,
            1,
            1,
            1,
            32,
            1,
            1,
            0,
            Format::BASE_FORMAT_R8G8B8A8_SRGB,
            TYPE_2D,
            VTYPE_2D,
            0},  // RGB → RGBA
        {"test://image/png/canine_128x128_RGBA8.png",
            128,
            128,
            1,
            1,
            1,
            1,
            32,
            1,
            1,
            0,
            Format::BASE_FORMAT_R8G8B8A8_SRGB,
            TYPE_2D,
            VTYPE_2D,
            0},
        // 16bpc channel variants
        {"test://image/png/canine_128x128_R16.png",
            128,
            128,
            1,
            1,
            1,
            1,
            16,
            1,
            1,
            0,
            Format::BASE_FORMAT_R16_UNORM,
            TYPE_2D,
            VTYPE_2D,
            0},
        {"test://image/png/canine_128x128_RG16.png",
            128,
            128,
            1,
            1,
            1,
            1,
            64,
            1,
            1,
            0,
            Format::BASE_FORMAT_R16G16B16A16_UNORM,
            TYPE_2D,
            VTYPE_2D,
            0},  // gray+alpha → RGBA
        {"test://image/png/canine_128x128_RGB16.png",
            128,
            128,
            1,
            1,
            1,
            1,
            64,
            1,
            1,
            0,
            Format::BASE_FORMAT_R16G16B16A16_UNORM,
            TYPE_2D,
            VTYPE_2D,
            0},  // RGB → RGBA
        {"test://image/png/canine_128x128_RGBA16.png",
            128,
            128,
            1,
            1,
            1,
            1,
            64,
            1,
            1,
            0,
            Format::BASE_FORMAT_R16G16B16A16_UNORM,
            TYPE_2D,
            VTYPE_2D,
            0},
        // Force grayscale
        {"test://image/png/canine_128x128_R8.png",
            128,
            128,
            1,
            1,
            1,
            1,
            8,
            1,
            1,
            0,
            Format::BASE_FORMAT_R8_SRGB,
            TYPE_2D,
            VTYPE_2D,
            IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT},
        {"test://image/png/canine_128x128_RG8.png",
            128,
            128,
            1,
            1,
            1,
            1,
            8,
            1,
            1,
            0,
            Format::BASE_FORMAT_R8_SRGB,
            TYPE_2D,
            VTYPE_2D,
            IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT},
        {"test://image/png/canine_128x128_RGB8.png",
            128,
            128,
            1,
            1,
            1,
            1,
            8,
            1,
            1,
            0,
            Format::BASE_FORMAT_R8_SRGB,
            TYPE_2D,
            VTYPE_2D,
            IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT},
        {"test://image/png/canine_128x128_RGBA8.png",
            128,
            128,
            1,
            1,
            1,
            1,
            8,
            1,
            1,
            0,
            Format::BASE_FORMAT_R8_SRGB,
            TYPE_2D,
            VTYPE_2D,
            IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT},
    };

    for (const auto& testInput : testImages) {
        IImageContainer::Ptr image;
        ASSERT_NO_FATAL_FAILURE(CheckImage(m_imageLoaderManager, "LoadImages", testInput, image))
            << testInput.imageUri.data();
        ASSERT_TRUE(image != nullptr) << testInput.imageUri.data();

        for (auto& pixelData : testInput.pixelDatas) {
            ASSERT_NO_FATAL_FAILURE(CheckImageData("LoadImages", *image, pixelData.elementIndex, pixelData.data))
                << testInput.imageUri.data();
        }
    }
}

/**
 * @tc.name: LoadFromFile
 * @tc.desc: Load a PNG via IFile handle and verify dimensions.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_PngLoaderTest, LoadFromFile, testing::ext::TestSize.Level1)
{
    auto& fileManager = UTest::GetTestEnv()->engine->GetFileManager();
    auto file = fileManager.OpenFile("test://image/canine_512x512.png");
    ASSERT_NE(file, nullptr) << "Could not open test://image/canine_512x512.png";

    constexpr uint32_t loadFlags = IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
    auto result = m_imageLoaderManager->LoadImage(*file, loadFlags);

    ASSERT_TRUE(result.success) << "Failed to load image from IFile";
    ASSERT_NE(result.image, nullptr);

    const auto& imageDesc = result.image->GetImageDesc();
    EXPECT_EQ(imageDesc.width, 512u);
    EXPECT_EQ(imageDesc.height, 512u);
}

/**
 * @tc.name: LoadFromBuffer
 * @tc.desc: Load a PNG from a raw byte buffer and verify dimensions.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_PngLoaderTest, LoadFromBuffer, testing::ext::TestSize.Level1)
{
    auto& fileManager = UTest::GetTestEnv()->engine->GetFileManager();
    auto file = fileManager.OpenFile("test://image/canine_512x512.png");
    ASSERT_NE(file, nullptr) << "Could not open test://image/canine_512x512.png for buffer test";

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
    EXPECT_EQ(imageDesc.width, 512u);
    EXPECT_EQ(imageDesc.height, 512u);
}

/**
 * @tc.name: MetadataOnly
 * @tc.desc: Load only metadata (dimensions) without decoding pixel data.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_PngLoaderTest, MetadataOnly, testing::ext::TestSize.Level1)
{
    constexpr uint32_t loadFlags =
        IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT | IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY;
    auto result = m_imageLoaderManager->LoadImage("test://image/canine_512x512.png", loadFlags);

    ASSERT_TRUE(result.success) << "Metadata-only load failed";
    ASSERT_NE(result.image, nullptr);

    const auto& imageDesc = result.image->GetImageDesc();
    EXPECT_EQ(imageDesc.width, 512u);
    EXPECT_EQ(imageDesc.height, 512u);
}

/**
 * @tc.name: LoadAnimatedImage
 * @tc.desc: PNG does not support animation; LoadAnimatedImage must return success=false.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_PngLoaderTest, LoadAnimatedImage, testing::ext::TestSize.Level1)
{
    auto result = m_imageLoaderManager->LoadAnimatedImage(
        "test://image/canine_512x512.png", IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT);

    EXPECT_FALSE(result.success) << "PNG does not support animation; LoadAnimatedImage should return success=false";
}

/**
 * @tc.name: LoadInvalidBytes
 * @tc.desc: Pass invalid (all-zero) bytes; expect graceful failure with no crash.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_PngLoaderTest, LoadInvalidBytes, testing::ext::TestSize.Level1)
{
    const uint8_t invalidData[16] = {};
    auto result = m_imageLoaderManager->LoadImage(array_view<const uint8_t>(invalidData, sizeof(invalidData)),
        IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT);

    EXPECT_FALSE(result.success) << "Loading invalid bytes should fail gracefully";
    // No crash is also a requirement — reaching here satisfies it.
}

/**
 * @tc.name: SecurityLargeDimensions
 * @tc.desc: Security regression: malformed PNG claiming huge dimensions must not crash or hang (VULN-032).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_PngLoaderTest, SecurityLargeDimensions, testing::ext::TestSize.Level1)
{
    // This file contains a crafted PNG header with extreme dimensions.
    // The loader must handle it gracefully: either fail or succeed, but never crash.
    auto result = m_imageLoaderManager->LoadImage(
        "test://image/security/VULN-032_large_16bpc_rgba.png", IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT);

    // Reaching this point without a crash or hang satisfies the regression requirement.
    // We additionally expect the load to fail for such a corrupt/crafted file.
    EXPECT_FALSE(result.success) << "Crafted large-dimensions PNG should be rejected gracefully";
}
