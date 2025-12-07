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

#include <algorithm>
#include <string_view>

#include <base/util/formats.h>
#include <core/io/intf_file_manager.h>

#include "test_framework.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
#include "image/image_loader_manager.h"
#include "image/loaders/image_loader_ktx.h"
#if (USE_STB_IMAGE == 1)
#include "image/loaders/image_loader_stb_image.h"
#endif
#include "log/logger.h"

using namespace CORE_NS;
using BASE_NS::array_view;
using BASE_NS::Format;
using BASE_NS::string;
using BASE_NS::string_view;

// ImageManager copy construction and assignment should be prevented.
static_assert(std::is_copy_constructible<ImageLoaderManager>::value == false);
static_assert(std::is_copy_assignable<ImageLoaderManager>::value == false);

// Image copy construction and assignment should be prevented.
static_assert(std::is_copy_constructible<IImageContainer>::value == false);
static_assert(std::is_copy_assignable<IImageContainer>::value == false);

namespace {
void DestroyImageLoaderManager(IImageLoaderManager* inst)
{
    delete static_cast<ImageLoaderManager*>(inst);
}

BASE_NS::unique_ptr<IImageLoaderManager, void (*)(IImageLoaderManager*)> CreateImageLoaderManager()
{
    ImageLoaderManager* ret = new ImageLoaderManager(*CORE_NS::UTest::GetTestEnv()->fileManager);
    return BASE_NS::unique_ptr<IImageLoaderManager, void (*)(IImageLoaderManager*)>(ret, DestroyImageLoaderManager);
}
} // namespace

/**
 * @tc.name: basicLoadImage
 * @tc.desc: Tests for Basic Load Image. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
#if (USE_STB_IMAGE == 0)
UNIT_TEST(SRC_ImageManagerTest, DISABLED_basicLoadImage, testing::ext::TestSize.Level1)
{
    // Skip
}
#else
UNIT_TEST(SRC_ImageManagerTest, basicLoadImage, testing::ext::TestSize.Level1)
{
    auto& files = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(files != nullptr);
    // Check that the files exist first.
    {
        IFile::Ptr file;
        file = files->OpenFile("test://image/png/canine_128x128_RGB16.png");
        ASSERT_TRUE(file != nullptr);
        file = files->OpenFile("test://image/cubemap_yokohama_RGBA8.ktx");
        ASSERT_TRUE(file != nullptr);
        file = files->OpenFile("test://image/astc/canine_128x128_ASTC_4x4.astc");
        ASSERT_TRUE(file != nullptr);
    }

    auto imageManager = CreateImageLoaderManager();
    ASSERT_TRUE(imageManager != nullptr);
    {
        const auto types = imageManager->GetSupportedTypes();
        EXPECT_TRUE(std::any_of(types.cbegin(), types.cend(), [](const IImageLoaderManager::ImageType& type) {
            return (type.mimeType == "image/jpeg") && (type.fileExtension == "jpeg");
        }));
        EXPECT_TRUE(std::any_of(types.cbegin(), types.cend(), [](const IImageLoaderManager::ImageType& type) {
            return (type.mimeType == "image/jpeg") && (type.fileExtension == "jpg");
        }));
        EXPECT_TRUE(std::any_of(types.cbegin(), types.cend(), [](const IImageLoaderManager::ImageType& type) {
            return (type.mimeType == "image/ktx") && (type.fileExtension == "ktx");
        }));
        EXPECT_TRUE(std::any_of(types.cbegin(), types.cend(), [](const IImageLoaderManager::ImageType& type) {
            return (type.mimeType == "image/png") && (type.fileExtension == "png");
        }));
        EXPECT_TRUE(std::any_of(types.cbegin(), types.cend(), [](const IImageLoaderManager::ImageType& type) {
            return (type.mimeType == "image/astc") && (type.fileExtension == "astc");
        }));
    }

    {
        auto result = imageManager->LoadImage("test://image/png/canine_128x128_RGB16.png", 0);
        ASSERT_TRUE(result.success) << result.error;
        ASSERT_TRUE(result.image != nullptr);
    }

    {
        auto result = imageManager->LoadImage("test://image/png/canine_128x128_RGB16.png", 0);
        ASSERT_TRUE(result.success) << result.error;
        ASSERT_TRUE(result.image != nullptr);
    }

    {
        // Open file, read to a byte array all at once. Then read the image.
        auto file = files->OpenFile("test://image/png/canine_128x128_RGB16.png");
        ASSERT_TRUE(file != nullptr);
        auto length = file->GetLength();
        ASSERT_TRUE(length > 0);
        auto buffer = std::make_unique<uint8_t[]>(static_cast<size_t>(length));
        auto read = file->Read(buffer.get(), length);
        ASSERT_EQ(read, length);

        auto metaResult = imageManager->LoadImage(
            array_view<const uint8_t>(buffer.get(), length), IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY);
        ASSERT_TRUE(metaResult.success) << metaResult.error;
        ASSERT_TRUE(metaResult.image != nullptr);

        auto result = imageManager->LoadImage(array_view<const uint8_t>(buffer.get(), length), 0);
        ASSERT_TRUE(result.success) << result.error;
        ASSERT_TRUE(result.image != nullptr);
    }

    {
        //*.ktx
        auto file = files->OpenFile("test://image/cubemap_yokohama_RGBA8.ktx");
        ASSERT_TRUE(file != nullptr);
        auto length = file->GetLength();
        ASSERT_TRUE(length > 0);
        auto buffer = std::make_unique<uint8_t[]>(static_cast<size_t>(length));
        auto read = file->Read(buffer.get(), length);
        ASSERT_EQ(read, length);

        auto metaResult = imageManager->LoadImage(
            array_view<const uint8_t>(buffer.get(), length), IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY);
        ASSERT_TRUE(metaResult.success) << metaResult.error;
        ASSERT_TRUE(metaResult.image != nullptr);

        auto result = imageManager->LoadImage(array_view<const uint8_t>(buffer.get(), length), 0);
        ASSERT_TRUE(result.success) << result.error;
        ASSERT_TRUE(result.image != nullptr);
    }

    {
        //*.astc
        auto file = files->OpenFile("test://image/astc/canine_128x128_ASTC_4x4.astc");
        ASSERT_TRUE(file != nullptr);
        auto length = file->GetLength();
        ASSERT_TRUE(length > 0);
        auto buffer = std::make_unique<uint8_t[]>(static_cast<size_t>(length));
        auto read = file->Read(buffer.get(), length);
        ASSERT_EQ(read, length);

        auto metaResult = imageManager->LoadImage(
            array_view<const uint8_t>(buffer.get(), length), IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY);
        ASSERT_TRUE(metaResult.success) << metaResult.error;
        ASSERT_TRUE(metaResult.image != nullptr);

        auto result = imageManager->LoadImage(array_view<const uint8_t>(buffer.get(), length), 0);
        ASSERT_TRUE(result.success) << result.error;
        ASSERT_TRUE(result.image != nullptr);
    }
}
#endif

/**
 * @tc.name: loadAnimatedImage
 * @tc.desc: Tests for Load Animated Image. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ImageManagerTest, loadAnimatedImage, testing::ext::TestSize.Level1)
{
    auto& files = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(files != nullptr);
    // Check that the file exists first.
    {
        auto pngFile = files->OpenFile("test://image/png/canine_128x128_RGB16.png");
        ASSERT_TRUE(pngFile != nullptr);
        auto ktxFile = files->OpenFile("test://image/cubemap_yokohama_RGBA8.ktx");
        ASSERT_TRUE(ktxFile != nullptr);
    }

    auto imageManager = CreateImageLoaderManager();
    ASSERT_TRUE(imageManager != nullptr);

    {
        auto result = imageManager->LoadAnimatedImage("test://image/png/canine_128x128_RGB16.png", 0);
        EXPECT_FALSE(result.success) << result.error;
        EXPECT_FALSE(result.image);
    }

    {
        auto result = imageManager->LoadAnimatedImage("test://image/cubemap_yokohama_RGBA8.ktx", 0);
        EXPECT_FALSE(result.success) << result.error;
        EXPECT_FALSE(result.image);
    }

    {
        auto file = files->OpenFile("test://image/cubemap_yokohama_RGBA8.ktx");
        ASSERT_TRUE(file != nullptr);
        auto length = file->GetLength();
        ASSERT_TRUE(length > 0);
        auto buffer = std::make_unique<uint8_t[]>(static_cast<size_t>(length));
        auto read = file->Read(buffer.get(), length);
        ASSERT_EQ(read, length);
        auto aniRes = imageManager->LoadAnimatedImage(array_view<const uint8_t>(buffer.get(), length), 0);
        EXPECT_FALSE(aniRes.success) << aniRes.error;
        EXPECT_FALSE(aniRes.image);
    }

    {
        auto result = imageManager->LoadAnimatedImage("test://image/png/canine_128x128_RGB16.png", 0);
        EXPECT_FALSE(result.success) << result.error;
        EXPECT_FALSE(result.image);
    }

    {
        auto file = files->OpenFile("test://image/png/canine_128x128_RGB16.png");
        ASSERT_TRUE(file != nullptr);
        auto length = file->GetLength();
        ASSERT_TRUE(length > 0);
        auto buffer = std::make_unique<uint8_t[]>(static_cast<size_t>(length));
        auto read = file->Read(buffer.get(), length);
        ASSERT_EQ(read, length);

        auto metaResult = imageManager->LoadAnimatedImage(
            array_view<const uint8_t>(buffer.get(), length), IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY);
        EXPECT_FALSE(metaResult.success) << metaResult.error;
        EXPECT_FALSE(metaResult.image);

        auto result = imageManager->LoadAnimatedImage(array_view<const uint8_t>(buffer.get(), length), 0);
        EXPECT_FALSE(result.success) << result.error;
        EXPECT_FALSE(result.image);
    }
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
void checkImage(const string_view aTest, TestImage& aTestImage, IImageContainer::Ptr& aImageOut)
{
    string messageStr =
        "Test: '" +
        aTest; //+"' image: '" + std::string_view(aTestImage.imageUri.data(), aTestImage.imageUri.size()) + "'";
    auto message = std::string_view(messageStr.data(), messageStr.size());
    auto imageManager = CreateImageLoaderManager();
    ASSERT_TRUE(imageManager != nullptr) << message;
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

void checkImageData(const char* aTest, const IImageContainer& aImage, uint32_t aElementIndex, void* pixelData)
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

/**
 * @tc.name: loadImages
 * @tc.desc: Tests for Load Images. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
#if (USE_STB_IMAGE == 0)
UNIT_TEST(SRC_ImageManagerTest, DISABLED_loadImages, testing::ext::TestSize.Level1)
{
    // Skip
}
#else
UNIT_TEST(SRC_ImageManagerTest, loadImages, testing::ext::TestSize.Level1)
{
    using namespace CORE_NS;

    // The test_image contains the following RGBA pixel data:
    const uint8_t pixelDataRGBA8[] = {
        0x00, 0x00, 0x00, 0xff, // [black]
        0xff, 0xff, 0xff, 0xff, // [white]
        0x80, 0x80, 0x80, 0xff, // [gray]
        0xff, 0x00, 0x00, 0xff, // [red]
        0x00, 0xff, 0x00, 0xff, // [green]
        0x00, 0x00, 0xff, 0xff, // [blue]
        0xff, 0xff, 0xff, 0x80, // [50% transparent white] (not premultiplied)
        0x00, 0x00, 0x00, 0x00, // [100% transparent]
    };

    const uint16_t pixelDataRGBA16[] = {
        0x0000, 0x0000, 0x0000, 0xffff,          // [black]
        0xffff, 0xffff, 0xffff, 0xffff,          // [white]
        0x80 << 8, 0x80 << 8, 0x80 << 8, 0xffff, // [gray]
        0xffff, 0x0000, 0x0000, 0xffff,          // [red]
        0x0000, 0xffff, 0x0000, 0xffff,          // [green]
        0x0000, 0x0000, 0xffff, 0xffff,          // [blue]
        0xffff, 0xffff, 0xffff, 0x80 << 8,       // [50% transparent white] (not premultiplied)
        0x0000, 0x0000, 0x0000, 0x00 << 8,       // [100% transparent]
    };

    const uint8_t pixelDataRGBA8Premultiplied[] = {
        0x00, 0x00, 0x00, 0xff, // [black]
        0xff, 0xff, 0xff, 0xff, // [white]
        0x80, 0x80, 0x80, 0xff, // [gray]
        0xff, 0x00, 0x00, 0xff, // [red]
        0x00, 0xff, 0x00, 0xff, // [green]
        0x00, 0x00, 0xff, 0xff, // [blue]
        0x80, 0x80, 0x80, 0x80, // [50% transparent white] (premultiplied)
        0x00, 0x00, 0x00, 0x00, // [100% transparent]
    };

    const uint16_t pixelDataRGBA16Premultiplied[] = {
        0x0000, 0x0000, 0x0000, 0xffff,          // [black]
        0xffff, 0xffff, 0xffff, 0xffff,          // [white]
        0x80 << 8, 0x80 << 8, 0x80 << 8, 0xffff, // [gray]
        0xffff, 0x0000, 0x0000, 0xffff,          // [red]
        0x0000, 0xffff, 0x0000, 0xffff,          // [green]
        0x0000, 0x0000, 0xffff, 0xffff,          // [blue]
        0x8000, 0x8000, 0x8000, 0x8000,          // [50% transparent white] (premultiplied)
        0x0000, 0x0000, 0x0000, 0x0000,          // [100% transparent]
    };

    const uint8_t pixelDataSRGBA8Premultiplied[] = {
        0x00, 0x00, 0x00, 0xff, // [black]
        0xff, 0xff, 0xff, 0xff, // [white]
        0x80, 0x80, 0x80, 0xff, // [gray]
        0xff, 0x00, 0x00, 0xff, // [red]
        0x00, 0xff, 0x00, 0xff, // [green]
        0x00, 0x00, 0xff, 0xff, // [blue]
        0xBC, 0xBC, 0xBC, 0x80, // [50% transparent white] (premultiplied)
        0x00, 0x00, 0x00, 0x00, // [100% transparent]
    };

    // 256 pixel width gradient. The first pixel value in the image is 0x00 and the last 0xff.
    uint8_t gradientPixelDataR8[256];
    for (size_t i = 0; i < 256; i++) {
        gradientPixelDataR8[i] = static_cast<uint8_t>(i);
    }

    // 256x256 16bpc gradient. The first pixel value in the image is 0x00 and the last 0xff.
    uint16_t gradientPixelDataR16[256 * 256];
    for (size_t i = 0; i < 256 * 256; i++) {
        gradientPixelDataR16[i] = static_cast<uint16_t>(i);
    }

    constexpr IImageContainer::ImageType TYPE_1D = IImageContainer::ImageType::TYPE_1D;
    constexpr IImageContainer::ImageType TYPE_2D = IImageContainer::ImageType::TYPE_2D;
    constexpr IImageContainer::ImageType TYPE_3D = IImageContainer::ImageType::TYPE_3D;

    constexpr IImageContainer::ImageViewType VTYPE_1D = IImageContainer::ImageViewType::VIEW_TYPE_1D;
    constexpr IImageContainer::ImageViewType VTYPE_2D = IImageContainer::ImageViewType::VIEW_TYPE_2D;
    constexpr IImageContainer::ImageViewType VTYPE_ARRAY_2D = IImageContainer::ImageViewType::VIEW_TYPE_2D_ARRAY;
    constexpr IImageContainer::ImageViewType VTYPE_CUBE = IImageContainer::ImageViewType::VIEW_TYPE_CUBE;
    constexpr IImageContainer::ImageViewType VTYPE_ARRAY_CUBE = IImageContainer::ImageViewType::VIEW_TYPE_CUBE_ARRAY;

    // Image flags
    uint32_t cube = IImageContainer::ImageFlags::FLAGS_CUBEMAP_BIT;
    uint32_t compressed = IImageContainer::ImageFlags::FLAGS_COMPRESSED_BIT;

    // Loader flags
    uint32_t lrgb = IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
    uint32_t srgb = IImageLoaderManager::IMAGE_LOADER_FORCE_SRGB_BIT;

    // Premultiply
    uint32_t premult =
        IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT | IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA;

    uint32_t premultS =
        IImageLoaderManager::IMAGE_LOADER_FORCE_SRGB_BIT | IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA;

    std::vector<TestImage> testImages;
    testImages.reserve(84);

    // Each TestImage contains:
    // {
    //   imageUri, width, height, depth, blockPixelWidth, blockPixelHeight, blockPixelDepth, bitsPerBlock,
    //   mipCount, layerCount, imageFlags, format, imageType, imageViewType, loaderFlags, pixelDatas,
    // }

    testImages.emplace_back(TestImage { "test://image/canine_284x323.jpg", 284, 323, 1, 1, 1, 1, 32, 1, 1, 0,
        Format::BASE_FORMAT_R8G8B8A8_UNORM, TYPE_2D, VTYPE_2D, lrgb }); // NOTE: We are Loading RGB as RGBA.
    testImages.emplace_back(TestImage { "test://image/canine_512x512.jpg", 512, 512, 1, 1, 1, 1, 32, 1, 1, 0,
        Format::BASE_FORMAT_R8G8B8A8_UNORM, TYPE_2D, VTYPE_2D, lrgb }); // NOTE: We are Loading RGB as RGBA.
    testImages.emplace_back(TestImage { "test://image/canine_512x512.png", 512, 512, 1, 1, 1, 1, 32, 1, 1, 0,
        Format::BASE_FORMAT_R8G8B8A8_UNORM, TYPE_2D, VTYPE_2D, lrgb }); // NOTE: We are Loading RGB as RGBA.
    testImages.emplace_back(TestImage { "test://image/test_image_8x1_RGBA.png", 8, 1, 1, 1, 1, 1, 32, 1, 1, 0,
        Format::BASE_FORMAT_R8G8B8A8_UNORM, TYPE_2D, VTYPE_2D, lrgb,
        std::vector<TestImageData>({ { 0, (void*)pixelDataRGBA8 } }) });
    testImages.emplace_back(TestImage { "test://image/test_image_8x1_RGBA.png", 8, 1, 1, 1, 1, 1, 32, 1, 1, 0,
        Format::BASE_FORMAT_R8G8B8A8_SRGB, TYPE_2D, VTYPE_2D, srgb,
        std::vector<TestImageData>({ { 0, (void*)pixelDataRGBA8 } }) });
    testImages.emplace_back(TestImage { "test://image/test_image_8x1_RGBA.ktx", 8, 1, 1, 1, 1, 1, 32, 1, 1, 0,
        Format::BASE_FORMAT_R8G8B8A8_UNORM, TYPE_1D, VTYPE_1D, lrgb,
        std::vector<TestImageData>({ { 0, (void*)pixelDataRGBA8 } }) });
    testImages.emplace_back(TestImage { "test://image/test_image_8x1_RGBA16.png", 8, 1, 1, 1, 1, 1, 64, 1, 1, 0,
        Format::BASE_FORMAT_R16G16B16A16_UNORM, TYPE_2D, VTYPE_2D, lrgb,
        std::vector<TestImageData>({ { 0, (void*)pixelDataRGBA16 } }) });
    testImages.emplace_back(
        TestImage { "test://image/gradient_256x1_R8.png", 256, 1, 1, 1, 1, 1, 8, 1, 1, 0, Format::BASE_FORMAT_R8_UNORM,
            TYPE_2D, VTYPE_2D, lrgb, std::vector<TestImageData>({ { 0, (void*)gradientPixelDataR8 } }) });
    testImages.emplace_back(TestImage { "test://image/gradient_256x256_R16.png", 256, 256, 1, 1, 1, 1, 16, 1, 1, 0,
        Format::BASE_FORMAT_R16_UNORM, TYPE_2D, VTYPE_2D, lrgb,
        std::vector<TestImageData>({ { 0, (void*)gradientPixelDataR16 } }) });

    // Premultiplied images.
    testImages.emplace_back(TestImage { "test://image/test_image_8x1_RGBA.png", 8, 1, 1, 1, 1, 1, 32, 1, 1,
        IImageContainer::ImageFlags::FLAGS_PREMULTIPLIED_ALPHA_BIT, Format::BASE_FORMAT_R8G8B8A8_UNORM, TYPE_2D,
        VTYPE_2D, premult, std::vector<TestImageData>({ { 0, (void*)pixelDataRGBA8Premultiplied } }) });
    testImages.emplace_back(TestImage { "test://image/test_image_8x1_RGBA16.png", 8, 1, 1, 1, 1, 1, 64, 1, 1,
        IImageContainer::ImageFlags::FLAGS_PREMULTIPLIED_ALPHA_BIT, Format::BASE_FORMAT_R16G16B16A16_UNORM, TYPE_2D,
        VTYPE_2D, premult, std::vector<TestImageData>({ { 0, (void*)pixelDataRGBA16Premultiplied } }) });

    testImages.emplace_back(TestImage { "test://image/test_image_8x1_RGBA.png", 8, 1, 1, 1, 1, 1, 32, 1, 1,
        IImageContainer::ImageFlags::FLAGS_PREMULTIPLIED_ALPHA_BIT, Format::BASE_FORMAT_R8G8B8A8_SRGB, TYPE_2D,
        VTYPE_2D, premultS, std::vector<TestImageData>({ { 0, (void*)pixelDataSRGBA8Premultiplied } }) });

    // Cubemaps.
    testImages.emplace_back(TestImage { "test://image/cubemap_yokohama_RGBA8.ktx", 512, 512, 1, 1, 1, 1, 32, 1, 6, cube,
        Format::BASE_FORMAT_R8G8B8A8_UNORM, TYPE_2D, VTYPE_CUBE, lrgb });
    testImages.emplace_back(TestImage { "test://image/cubemap_yokohama_RGBA8_mipmap.ktx", 512, 512, 1, 1, 1, 1, 32, 10,
        6, cube, Format::BASE_FORMAT_R8G8B8A8_UNORM, TYPE_2D, VTYPE_CUBE, lrgb });
    testImages.emplace_back(TestImage { "test://image/envmaps/cubemap_yokohama_irradiance_RGBA8.ktx", 256, 256, 1, 1, 1,
        1, 32, 1, 6, cube, Format::BASE_FORMAT_R8G8B8A8_UINT, TYPE_2D, VTYPE_CUBE, lrgb });
    testImages.emplace_back(TestImage { "test://image/envmaps/cubemap_yokohama_irradiance_RGBA16F.ktx", 256, 256, 1, 1,
        1, 1, 64, 1, 6, cube, Format::BASE_FORMAT_R16G16B16A16_SFLOAT, TYPE_2D, VTYPE_CUBE, lrgb });
    testImages.emplace_back(TestImage { "test://image/envmaps/cubemap_yokohama_radiance_RGBA8.ktx", 256, 256, 1, 1, 1,
        1, 32, 9, 6, cube, Format::BASE_FORMAT_R8G8B8A8_UINT, TYPE_2D, VTYPE_CUBE, lrgb });
    testImages.emplace_back(TestImage { "test://image/envmaps/cubemap_yokohama_radiance_RGBA16F.ktx", 256, 256, 1, 1, 1,
        1, 64, 9, 6, cube, Format::BASE_FORMAT_R16G16B16A16_SFLOAT, TYPE_2D, VTYPE_CUBE, lrgb });

    // Uncompressed basic image formats. 8bpc and 16bpc
    testImages.emplace_back(TestImage { "test://image/png/canine_128x128_R8.png", 128, 128, 1, 1, 1, 1, 8, 1, 1, 0,
        Format::BASE_FORMAT_R8_SRGB, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/png/canine_128x128_RG8.png", 128, 128, 1, 1, 1, 1, 32, 1, 1, 0,
        Format::BASE_FORMAT_R8G8B8A8_SRGB, TYPE_2D, VTYPE_2D, 0 }); // NOTE: We are Loading gray + alpha as RGBA.
    testImages.emplace_back(TestImage { "test://image/png/canine_128x128_RGB8.png", 128, 128, 1, 1, 1, 1, 32, 1, 1, 0,
        Format::BASE_FORMAT_R8G8B8A8_SRGB, TYPE_2D, VTYPE_2D, 0 }); // NOTE: We are Loading RGB as RGBA.
    testImages.emplace_back(TestImage { "test://image/png/canine_128x128_RGBA8.png", 128, 128, 1, 1, 1, 1, 32, 1, 1, 0,
        Format::BASE_FORMAT_R8G8B8A8_SRGB, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/png/canine_128x128_R16.png", 128, 128, 1, 1, 1, 1, 16, 1, 1, 0,
        Format::BASE_FORMAT_R16_UNORM, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/png/canine_128x128_RG16.png", 128, 128, 1, 1, 1, 1, 64, 1, 1, 0,
        Format::BASE_FORMAT_R16G16B16A16_UNORM, TYPE_2D, VTYPE_2D, 0 }); // NOTE: We are Loading gray + alpha as RGBA.
    testImages.emplace_back(TestImage { "test://image/png/canine_128x128_RGB16.png", 128, 128, 1, 1, 1, 1, 64, 1, 1, 0,
        Format::BASE_FORMAT_R16G16B16A16_UNORM, TYPE_2D, VTYPE_2D, 0 }); // NOTE: We are Loading RGB as RGBA.
    testImages.emplace_back(TestImage { "test://image/png/canine_128x128_RGBA16.png", 128, 128, 1, 1, 1, 1, 64, 1, 1, 0,
        Format::BASE_FORMAT_R16G16B16A16_UNORM, TYPE_2D, VTYPE_2D, 0 });

    // BCx
    testImages.emplace_back(TestImage { "test://image/bc/canine_128x128_RGB8_BC1_UB_lRGB.ktx", 128, 128, 1, 4, 4, 1, 64,
        8, 1, compressed, Format::BASE_FORMAT_BC1_RGB_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/bc/canine_128x128_RGBA8_BC2_UB_lRGB.ktx", 128, 128, 1, 4, 4, 1,
        128, 8, 1, compressed, Format::BASE_FORMAT_BC2_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/bc/canine_128x128_RGBA8_BC3_UB_lRGB.ktx", 128, 128, 1, 4, 4, 1,
        128, 8, 1, compressed, Format::BASE_FORMAT_BC3_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });

    // Force linear/srbg
    testImages.emplace_back(TestImage { "test://image/bc/canine_128x128_RGB8_BC1_UB_lRGB.ktx", 128, 128, 1, 4, 4, 1, 64,
        8, 1, compressed, Format::BASE_FORMAT_BC1_RGB_SRGB_BLOCK, TYPE_2D, VTYPE_2D, srgb });
    testImages.emplace_back(TestImage { "test://image/bc/canine_128x128_RGBA8_BC2_UB_lRGB.ktx", 128, 128, 1, 4, 4, 1,
        128, 8, 1, compressed, Format::BASE_FORMAT_BC2_SRGB_BLOCK, TYPE_2D, VTYPE_2D, srgb });
    testImages.emplace_back(TestImage { "test://image/bc/canine_128x128_RGBA8_BC3_UB_lRGB.ktx", 128, 128, 1, 4, 4, 1,
        128, 8, 1, compressed, Format::BASE_FORMAT_BC3_SRGB_BLOCK, TYPE_2D, VTYPE_2D, srgb });

    // EAC / ETC
    testImages.emplace_back(TestImage { "test://image/etc/canine_128x128_RGB8_ETC1_UB_lRGB.ktx", 128, 128, 1, 4, 4, 1,
        64, 8, 1, compressed, Format::BASE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });

    testImages.emplace_back(TestImage { "test://image/etc/canine_128x128_RGB8_ETC2_RGB_UB_sRGB.ktx", 128, 128, 1, 4, 4,
        1, 64, 8, 1, compressed, Format::BASE_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/etc/canine_128x128_RGBA8_ETC2_RGBA_UB_sRGB.ktx", 128, 128, 1, 4,
        4, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/etc/canine_128x128_RGBA8_ETC2_RGB_A1_UB_sRGB.ktx", 128, 128, 1, 4,
        4, 1, 64, 8, 1, compressed, Format::BASE_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });

    testImages.emplace_back(TestImage { "test://image/etc/canine_128x128_RGB8_ETC2_RGB_UB_lRGB.ktx", 128, 128, 1, 4, 4,
        1, 64, 8, 1, compressed, Format::BASE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/etc/canine_128x128_RGBA8_ETC2_RGBA_UB_lRGB.ktx", 128, 128, 1, 4,
        4, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/etc/canine_128x128_RGBA8_ETC2_RGB_A1_UB_lRGB.ktx", 128, 128, 1, 4,
        4, 1, 64, 8, 1, compressed, Format::BASE_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });

    testImages.emplace_back(TestImage { "test://image/etc/canine_128x128_R8_EAC_R11_UB_lRGB.ktx", 128, 128, 1, 4, 4, 1,
        64, 8, 1, compressed, Format::BASE_FORMAT_EAC_R11_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/etc/canine_128x128_R8_EAC_R11_SB_lRGB.ktx", 128, 128, 1, 4, 4, 1,
        64, 8, 1, compressed, Format::BASE_FORMAT_EAC_R11_SNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/etc/canine_128x128_RG8_EAC_RG11_UB_lRGB.ktx", 128, 128, 1, 4, 4,
        1, 128, 8, 1, compressed, Format::BASE_FORMAT_EAC_R11G11_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/etc/canine_128x128_RG8_EAC_RG11_SB_lRGB.ktx", 128, 128, 1, 4, 4,
        1, 128, 8, 1, compressed, Format::BASE_FORMAT_EAC_R11G11_SNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });

    // Force linear/srbg
    testImages.emplace_back(TestImage { "test://image/etc/canine_128x128_RGB8_ETC1_UB_lRGB.ktx", 128, 128, 1, 4, 4, 1,
        64, 8, 1, compressed, Format::BASE_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, TYPE_2D, VTYPE_2D, srgb });
    testImages.emplace_back(TestImage { "test://image/etc/canine_128x128_RGB8_ETC2_RGB_UB_sRGB.ktx", 128, 128, 1, 4, 4,
        1, 64, 8, 1, compressed, Format::BASE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, TYPE_2D, VTYPE_2D, lrgb });
    testImages.emplace_back(TestImage { "test://image/etc/canine_128x128_RGB8_ETC2_RGB_UB_lRGB.ktx", 128, 128, 1, 4, 4,
        1, 64, 8, 1, compressed, Format::BASE_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, TYPE_2D, VTYPE_2D, srgb });

    // ASTC
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_4x4_UB_sRGB.ktx", 128, 128, 1, 4,
        4, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_4x4_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_5x4_UB_sRGB.ktx", 128, 128, 1, 5,
        4, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_5x4_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_5x5_UB_sRGB.ktx", 128, 128, 1, 5,
        5, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_5x5_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_6x5_UB_sRGB.ktx", 128, 128, 1, 6,
        5, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_6x5_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_6x6_UB_sRGB.ktx", 128, 128, 1, 6,
        6, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_8x5_UB_sRGB.ktx", 128, 128, 1, 8,
        5, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_8x5_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_8x6_UB_sRGB.ktx", 128, 128, 1, 8,
        6, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_8x6_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_10x5_UB_sRGB.ktx", 128, 128, 1, 10,
        5, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_10x5_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_10x6_UB_sRGB.ktx", 128, 128, 1, 10,
        6, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_10x6_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_8x8_UB_sRGB.ktx", 128, 128, 1, 8,
        8, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_8x8_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_10x8_UB_sRGB.ktx", 128, 128, 1, 10,
        8, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_10x8_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_10x10_UB_sRGB.ktx", 128, 128, 1,
        10, 10, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_10x10_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_12x10_UB_sRGB.ktx", 128, 128, 1,
        12, 10, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_12x10_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_12x12_UB_sRGB.ktx", 128, 128, 1,
        12, 12, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_12x12_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });

    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_R8_ASTC_6x6_UB_sRGB.ktx", 128, 128, 1, 6, 6,
        1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RG8_ASTC_6x6_UB_sRGB.ktx", 128, 128, 1, 6, 6,
        1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGB8_ASTC_6x6_UB_sRGB.ktx", 128, 128, 1, 6, 6,
        1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_R16_ASTC_6x6_UB_sRGB.ktx", 128, 128, 1, 6, 6,
        1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RG16_ASTC_6x6_UB_sRGB.ktx", 128, 128, 1, 6, 6,
        1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGB16_ASTC_6x6_UB_sRGB.ktx", 128, 128, 1, 6,
        6, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA16_ASTC_6x6_UB_sRGB.ktx", 128, 128, 1, 6,
        6, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });

    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_6x6_UB_lRGB.ktx", 128, 128, 1, 6,
        6, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_6x6_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA16_ASTC_6x6_UB_lRGB.ktx", 128, 128, 1, 6,
        6, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_6x6_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });

    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_nomipmap_ASTC_6x6.ktx", 128, 128, 1, 6,
        6, 1, 128, 1, 1, compressed, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_pad8_ASTC_6x6.ktx", 128, 128, 1, 6, 6,
        1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, TYPE_2D, VTYPE_2D, 0 });

    // .astc ASTC
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_4x4.astc", 128, 128, 1, 4, 4, 1, 128, 1,
        1, compressed, Format::BASE_FORMAT_ASTC_4x4_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_5x4.astc", 128, 128, 1, 5, 4, 1, 128, 1,
        1, compressed, Format::BASE_FORMAT_ASTC_5x4_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_5x5.astc", 128, 128, 1, 5, 5, 1, 128, 1,
        1, compressed, Format::BASE_FORMAT_ASTC_5x5_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_6x5.astc", 128, 128, 1, 6, 5, 1, 128, 1,
        1, compressed, Format::BASE_FORMAT_ASTC_6x5_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_6x6.astc", 128, 128, 1, 6, 6, 1, 128, 1,
        1, compressed, Format::BASE_FORMAT_ASTC_6x6_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_8x5.astc", 128, 128, 1, 8, 5, 1, 128, 1,
        1, compressed, Format::BASE_FORMAT_ASTC_8x5_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_8x6.astc", 128, 128, 1, 8, 6, 1, 128, 1,
        1, compressed, Format::BASE_FORMAT_ASTC_8x6_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_8x8.astc", 128, 128, 1, 8, 8, 1, 128, 1,
        1, compressed, Format::BASE_FORMAT_ASTC_8x8_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_10x5.astc", 128, 128, 1, 10, 5, 1, 128,
        1, 1, compressed, Format::BASE_FORMAT_ASTC_10x5_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_10x6.astc", 128, 128, 1, 10, 6, 1, 128,
        1, 1, compressed, Format::BASE_FORMAT_ASTC_10x6_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_10x8.astc", 128, 128, 1, 10, 8, 1, 128,
        1, 1, compressed, Format::BASE_FORMAT_ASTC_10x8_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_10x10.astc", 128, 128, 1, 10, 10, 1, 128,
        1, 1, compressed, Format::BASE_FORMAT_ASTC_10x10_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_12x10.astc", 128, 128, 1, 12, 10, 1, 128,
        1, 1, compressed, Format::BASE_FORMAT_ASTC_12x10_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_12x12.astc", 128, 128, 1, 12, 12, 1, 128,
        1, 1, compressed, Format::BASE_FORMAT_ASTC_12x12_UNORM_BLOCK, TYPE_2D, VTYPE_2D, 0 });

    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_12x10.astc", 128, 128, 1, 12, 10, 1, 128,
        1, 1, compressed, Format::BASE_FORMAT_ASTC_12x10_SRGB_BLOCK, TYPE_2D, VTYPE_2D, srgb });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_ASTC_12x12.astc", 128, 128, 1, 12, 12, 1, 128,
        1, 1, compressed, Format::BASE_FORMAT_ASTC_12x12_SRGB_BLOCK, TYPE_2D, VTYPE_2D, srgb });

    // Texture arrays
    testImages.emplace_back(TestImage { "test://image/arrays/canine_128x128_arrayx2_R8_ASTC_6x6_UB_sRGB.ktx", 128, 128,
        1, 6, 6, 1, 128, 1, 2, compressed, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, TYPE_2D, VTYPE_ARRAY_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/arrays/canine_128x128_arrayx3_R8_ASTC_6x6_UB_sRGB.ktx", 128, 128,
        1, 6, 6, 1, 128, 1, 3, compressed, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, TYPE_2D, VTYPE_ARRAY_2D, 0 });
    testImages.emplace_back(TestImage { "test://image/arrays/canine_128x128_arrayx3mips_R8_ASTC_6x6_UB_sRGB.ktx", 128,
        128, 1, 6, 6, 1, 128, 8, 3, compressed, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, TYPE_2D, VTYPE_ARRAY_2D, 0 });

    // Cubemap array
    testImages.emplace_back(
        TestImage { "test://image/ktx-test/ktx/valid/composition/array_2_elements_cubemap_6_faces.ktx", 64, 64, 1, 1, 1,
            1, 24, 7, 12, cube, Format::BASE_FORMAT_R8G8B8_UNORM, TYPE_2D, VTYPE_ARRAY_CUBE, 0 });

    // Force linear/srbg
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_6x6_UB_sRGB.ktx", 128, 128, 1, 6,
        6, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_6x6_UNORM_BLOCK, TYPE_2D, VTYPE_2D, lrgb });
    testImages.emplace_back(TestImage { "test://image/astc/canine_128x128_RGBA8_ASTC_6x6_UB_lRGB.ktx", 128, 128, 1, 6,
        6, 1, 128, 8, 1, compressed, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, TYPE_2D, VTYPE_2D, srgb });

    // Converting image to grayscale
    testImages.emplace_back(TestImage { "test://image/png/canine_128x128_R8.png", 128, 128, 1, 1, 1, 1, 8, 1, 1, 0,
        Format::BASE_FORMAT_R8_SRGB, TYPE_2D, VTYPE_2D, IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT });
    testImages.emplace_back(TestImage { "test://image/png/canine_128x128_RG8.png", 128, 128, 1, 1, 1, 1, 8, 1, 1, 0,
        Format::BASE_FORMAT_R8_SRGB, TYPE_2D, VTYPE_2D, IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT });
    testImages.emplace_back(TestImage { "test://image/png/canine_128x128_RGB8.png", 128, 128, 1, 1, 1, 1, 8, 1, 1, 0,
        Format::BASE_FORMAT_R8_SRGB, TYPE_2D, VTYPE_2D, IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT });
    testImages.emplace_back(TestImage { "test://image/png/canine_128x128_RGBA8.png", 128, 128, 1, 1, 1, 1, 8, 1, 1, 0,
        Format::BASE_FORMAT_R8_SRGB, TYPE_2D, VTYPE_2D, IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT });

    for (size_t i = 0; i < testImages.size(); ++i) {
        IImageContainer::Ptr image;
        ASSERT_NO_FATAL_FAILURE(checkImage("loadImages", testImages[i], image)) << testImages[i].imageUri.data();
        ASSERT_TRUE(image != nullptr) << testImages[i].imageUri.data();

        for (auto& pixelData : testImages[i].pixelDatas) {
            ASSERT_NO_FATAL_FAILURE(checkImageData("loadImages", *image, pixelData.elementIndex, pixelData.data))
                << testImages[i].imageUri.data();
        }
    }
}
#endif

namespace {
void TestInvalidFiles(const string_view dirUri)
{
    auto imageManager = CreateImageLoaderManager();
    ASSERT_TRUE(imageManager != nullptr);
    auto& fileManager = CORE_NS::UTest::GetTestEnv()->fileManager;
    ASSERT_TRUE(fileManager != nullptr);

    IDirectory::Ptr dir = fileManager->OpenDirectory(dirUri);
    if (dir) {
        const auto entries = dir->GetEntries();
        for (IDirectory::Entry entry : entries) {
            if (entry.type == IDirectory::Entry::FILE) {
                const string uri = dirUri + entry.name;
                {
                    // Check that the file can be opened first.
                    auto file = fileManager->OpenFile(uri);
                    ASSERT_TRUE(file != nullptr) << "Could not open file: " << uri.data();
                }

                // Don't show logs about invalid ktx files (we don't want to see the errors this test generates).
                ::Test::LogLevelScope level(GetLogger(), ILogger::LogLevel::LOG_INFO);

                auto result = imageManager->LoadImage(uri, 0);
                ASSERT_FALSE(result.success) << uri.data() << " error: " << result.error;
                ASSERT_EQ(result.image, nullptr) << uri.data();
            }
        }
    }
}
} // namespace

/**
 * @tc.name: invalidImages
 * @tc.desc: Tests for Invalid Images. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ImageManagerTest, invalidImages, testing::ext::TestSize.Level1)
{
    TestInvalidFiles("test://image/ktx-test/ktx/invalid/header/");
    TestInvalidFiles("test://image/ktx-test/ktx/invalid/pixeldata/");
    TestInvalidFiles("test://image/astc-test/invalid/header/");
}

/**
 * @tc.name: imageNotLoaded
 * @tc.desc: Tests for Image Not Loaded. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ImageManagerTest, imageNotLoaded, testing::ext::TestSize.Level1)
{
    auto imageManager = CreateImageLoaderManager();
    ASSERT_TRUE(imageManager != nullptr);

    {
        // File does not exist (empty path).
        string_view filename;
        auto result = imageManager->LoadImage(filename, 0);
        ASSERT_FALSE(result.success);
    }

    {
        // File is not an image 1.
        string_view filename = "test://image/source.txt";
        auto result = imageManager->LoadImage(filename, 0);
        ASSERT_FALSE(result.success);
    }
    {
        // File is not an image 2.
        string_view filename = "test://image/source.txt";
        auto result = imageManager->LoadImage(filename, 0);
        ASSERT_FALSE(result.success);
    }
}
