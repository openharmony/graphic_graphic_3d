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

#include <algorithm>
#include <string_view>

#include <gtest/gtest.h>

#include <base/util/formats.h>
#include <core/io/intf_file_manager.h>

#include "image/image_loader_manager.h"
#include "image/loaders/image_loader_ktx.h"
#include "image/loaders/image_loader_stb_image.h"
#include "log/logger.h"
#include "TestRunner.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;
using BASE_NS::array_view;
using BASE_NS::Format;
using BASE_NS::string;
using BASE_NS::string_view;
using namespace testing;
using namespace testing::ext;

// ImageManager copy construction and assignment should be prevented.
static_assert(std::is_copy_constructible<ImageLoaderManager>::value == false);
static_assert(std::is_copy_assignable<ImageLoaderManager>::value == false);

// Image copy construction and assignment should be prevented.
static_assert(std::is_copy_constructible<IImageContainer>::value == false);
static_assert(std::is_copy_assignable<IImageContainer>::value == false);

namespace {
struct TestContext {
    std::shared_ptr<ISceneInit> sceneInit_ = nullptr;
    CORE_NS::IEcs::Ptr ecs_;
};
static TestContext g_context;
void DestroyImageLoaderManager(IImageLoaderManager* inst)
{
    delete static_cast<ImageLoaderManager*>(inst);
}

BASE_NS::unique_ptr<IImageLoaderManager, void (*)(IImageLoaderManager*)> CreateImageLoaderManager()
{
    ImageLoaderManager* ret = new ImageLoaderManager(context.sceneInit_->GetEngineInstance().engine_->GetFileManager());
    return BASE_NS::unique_ptr<IImageLoaderManager, void (*)(IImageLoaderManager*)>(ret, DestroyImageLoaderManager);
}

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
    auto factory = GetInstance<ISystemGraphLoaderFactory>(UID_SYSTEM_GRAPH_LOADER);
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

class ImageManagerTest : public testing::Test {
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
 * @tc.name: BasicLoadImage
 * @tc.desc: test BasicLoadImage
 * @tc.type: FUNC
 */
HWTEST_F(ImageManagerTest, BasicLoadImage, TestSize.Level1)
{
    auto& files = context.sceneInit_->GetEngineInstance().engine_->GetFileManager();

    auto imageManager = CreateImageLoaderManager();
    ASSERT_TRUE(imageManager != nullptr);
    {
        //*.ktx
        auto file = files.OpenFile("file:///data/local/test_data/image/cubemap_yokohama_RGBA8.ktx");
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

struct TestImageData {
    uint32_t elementIndex;
    void* data;
};

struct TestImage {
    BASE_NS::string imageUri;

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

namespace {
// NOTE: gtest asserts can only be called in a void function.
void checkImage(const string_view aTest, TestImage& aTestImage, IImageContainer::Ptr& aImageOut)
{
    BASE_NS::string messageStr =
        "Test: '" +
        aTest; //+"' image: '" + BASE_NS::string_view(aTestImage.imageUri.data(), aTestImage.imageUri.size()) + "'";
    auto message = BASE_NS::string_view(messageStr.data(), messageStr.size());
    auto imageManager = CreateImageLoaderManager();
    ASSERT_TRUE(imageManager != nullptr);
    auto& fileManager = g_context.sceneInit_->GetEngineInstance().engine_->GetFileManager();

    // Check that the file exists first.
    {
        auto file = fileManager.OpenFile(aTestImage.imageUri);
        ASSERT_TRUE(file != nullptr);
    }

    auto result = imageManager->LoadImage(aTestImage.imageUri.c_str(), aTestImage.loaderFlags);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);

    auto imageData = result.image->GetData();
    ASSERT_TRUE(imageData.size() != 0);

    // Check that the loaded ImageDesc data matched with the expected.
    const IImageContainer::ImageDesc& imageDesc = result.image->GetImageDesc();

    const TestImage& expected = aTestImage;

    ASSERT_EQ(imageDesc.imageFlags, expected.imageFlags);
    ASSERT_EQ(imageDesc.blockPixelWidth, expected.blockPixelWidth);
    ASSERT_EQ(imageDesc.blockPixelHeight, expected.blockPixelHeight);
    ASSERT_EQ(imageDesc.blockPixelDepth, expected.blockPixelDepth);
    ASSERT_EQ(imageDesc.bitsPerBlock, expected.bitsPerBlock);

    ASSERT_EQ(imageDesc.width, expected.width);
    ASSERT_EQ(imageDesc.height, expected.height);
    ASSERT_EQ(imageDesc.depth, expected.depth);

    ASSERT_EQ(imageDesc.mipCount, expected.mipCount);
    ASSERT_EQ(imageDesc.layerCount, expected.layerCount);

    ASSERT_EQ(imageDesc.format, expected.format);
    ASSERT_EQ(imageDesc.imageType, expected.imageType);
    ASSERT_EQ(imageDesc.imageViewType, expected.imageViewType);

    // Check image element data.
    auto imageBuffers = result.image->GetBufferImageCopies();
    const bool cubeMap = (imageDesc.imageFlags & IImageContainer::ImageFlags::FLAGS_CUBEMAP_BIT) != 0;

    const uint32_t bytesPerBlock = imageDesc.bitsPerBlock / 8;

    ASSERT_EQ(imageBuffers.size(), imageDesc.mipCount);

    for (auto& imageSubelement : imageBuffers) {
        ASSERT_LT(imageSubelement.bufferOffset, imageData.size());

        // Ktx requires alignment to 4 bytes.
        ASSERT_EQ(imageSubelement.bufferOffset % 4, 0)
            << " Offset=" << imageSubelement.bufferOffset << " BytesPerBlock=" << bytesPerBlock << " ";

        // Vulkan requires that: "If the calling command's VkImage parameter's
        // format is not a depth/stencil format or a multi-planar format, then
        // bufferOffset must be a multiple of the format's texel block size."

        // Also take into account that the alignment to 4 bytes.
        const uint32_t alignment = ((bytesPerBlock - 1) / 4 + 1) * 4;

        ASSERT_EQ(imageSubelement.bufferOffset % alignment, 0)
            << " Offset=" << imageSubelement.bufferOffset << " BytesPerBlock=" << bytesPerBlock << " ";

        // For uncompressed formats check that the total buffer size matches with the format information.
        if ((imageDesc.imageFlags & IImageContainer::ImageFlags::FLAGS_COMPRESSED_BIT) == 0) {
            // NOTE: This is only correct for uncompressed formats.
            const uint32_t bytesPerPixel = bytesPerBlock / imageDesc.blockPixelWidth;

            const size_t subelementDataLength = bytesPerPixel * imageSubelement.bufferRowLength *
                                                imageSubelement.bufferImageHeight * imageSubelement.depth;
            ASSERT_LE(imageSubelement.bufferOffset + subelementDataLength, imageData.size());
        }
    }

    // Only checking the first element size for now.
    uint32_t bufferIndex = 0;
    ASSERT_EQ(imageBuffers[bufferIndex].layerCount, expected.layerCount);
    ASSERT_EQ(imageBuffers[bufferIndex].width, expected.width);
    ASSERT_EQ(imageBuffers[bufferIndex].height, expected.height);
    ASSERT_EQ(imageBuffers[bufferIndex].depth, expected.depth);

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
} // namespace

/**
 * @tc.name: LoadImages
 * @tc.desc: test LoadImages
 * @tc.type: FUNC
 */
HWTEST_F(ImageManagerTest, LoadImages, TestSize.Level1)
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
    testImages.emplace_back(
        TestImage {
            "file:///data/local/test_data/image/ktx-test/ktx/valid/composition/array_2_elements_cubemap_6_faces.ktx",
            64, 64, 1, 1, 1, 1, 24, 7, 12, cube, Format::BASE_FORMAT_R8G8B8_UNORM, TYPE_2D, VTYPE_ARRAY_CUBE, 0 });

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
namespace {
void TestInvalidFiles(const string_view dirUri)
{
    auto imageManager = CreateImageLoaderManager();
    ASSERT_TRUE(imageManager != nullptr);
    auto& fileManager = g_context.sceneInit_->GetEngineInstance().engine_->GetFileManager();

    IDirectory::Ptr dir = fileManager.OpenDirectory(dirUri);
    if (dir) {
        const auto entries = dir->GetEntries();
        for (IDirectory::Entry entry : entries) {
            if (entry.type == IDirectory::Entry::FILE) {
                const BASE_NS::string uri = dirUri + entry.name;
                {
                    // Check that the file can be opened first.
                    auto file = fileManager.OpenFile(uri);
                    ASSERT_TRUE(file != nullptr) << "Could not open file: " << uri.data();
                }

                auto result = imageManager->LoadImage(uri, 0);
                ASSERT_FALSE(result.success) << uri.data() << " error: " << result.error;
                ASSERT_EQ(result.image, nullptr) << uri.data();
            }
        }
    }
}
} // namespace

/**
 * @tc.name: InvalidImages
 * @tc.desc: test InvalidImages
 * @tc.type: FUNC
 */
HWTEST_F(ImageManagerTest, InvalidImages, TestSize.Level1)
{
    TestInvalidFiles("file:///data/local/test_data/image/ktx-test/ktx/invalid/header/");
    TestInvalidFiles("file:///data/local/test_data/image/ktx-test/ktx/invalid/pixeldata/");
}

/**
 * @tc.name: ImageNotLoaded
 * @tc.desc: test ImageNotLoaded
 * @tc.type: FUNC
 */
HWTEST_F(ImageManagerTest, ImageNotLoaded, TestSize.Level1)
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
        string_view filename = "file:///data/local/test_data/image/source.txt";
        auto result = imageManager->LoadImage(filename, 0);
        ASSERT_FALSE(result.success);
    }
    {
        // File is not an image 2.
        string_view filename = "file:///data/local/test_data/image/source.txt";
        auto result = imageManager->LoadImage(filename, 0);
        ASSERT_FALSE(result.success);
    }
}
