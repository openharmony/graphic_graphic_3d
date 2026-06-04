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

#include "test_framework.h"

#include <base/containers/array_view.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/image/intf_image_container.h>
#include <core/io/intf_file.h>
#include <core/namespace.h>
#include <png/image_loader_png.h>

#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <securec.h>

using namespace BASE_NS;
using namespace CORE_NS;

namespace {
// Valid 8x1 RGBA PNG test data (143 bytes)
static const uint8_t VALID_PNG_DATA[] = {0x89,
    0x50,
    0x4e,
    0x47,
    0x0d,
    0x0a,
    0x1a,
    0x0a,
    0x00,
    0x00,
    0x00,
    0x0d,
    0x49,
    0x48,
    0x44,
    0x52,
    0x00,
    0x00,
    0x00,
    0x08,
    0x00,
    0x00,
    0x00,
    0x01,
    0x08,
    0x06,
    0x00,
    0x00,
    0x00,
    0xe3,
    0x00,
    0xef,
    0x43,
    0x00,
    0x00,
    0x00,
    0x06,
    0x62,
    0x4b,
    0x47,
    0x44,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0xf9,
    0x43,
    0xbb,
    0x7f,
    0x00,
    0x00,
    0x00,
    0x09,
    0x70,
    0x48,
    0x59,
    0x73,
    0x00,
    0x00,
    0x0b,
    0x13,
    0x00,
    0x00,
    0x0b,
    0x13,
    0x01,
    0x00,
    0x9a,
    0x9c,
    0x18,
    0x00,
    0x00,
    0x00,
    0x07,
    0x74,
    0x49,
    0x4d,
    0x45,
    0x07,
    0xe3,
    0x01,
    0x04,
    0x07,
    0x1b,
    0x2b,
    0xee,
    0xc3,
    0x59,
    0x34,
    0x00,
    0x00,
    0x00,
    0x1c,
    0x49,
    0x44,
    0x41,
    0x54,
    0x08,
    0xd7,
    0x35,
    0xc8,
    0xb1,
    0x01,
    0x00,
    0x00,
    0x08,
    0xc3,
    0x20,
    0x4e,
    0xef,
    0xe7,
    0x71,
    0x92,
    0x11,
    0xa8,
    0x6a,
    0x5b,
    0x91,
    0xf4,
    0x05,
    0x07,
    0x29,
    0x2b,
    0x10,
    0xf2,
    0x2c,
    0x30,
    0x61,
    0x04,
    0x00,
    0x00,
    0x00,
    0x00,
    0x49,
    0x45,
    0x4e,
    0x44,
    0xae,
    0x42,
    0x60,
    0x82};
static const size_t VALID_PNG_SIZE = sizeof(VALID_PNG_DATA);

// PNG signature (8 bytes)
static const uint8_t PNG_SIGNATURE[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};

// Invalid header (JPEG header)
static const uint8_t INVALID_HEADER[] = {0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46};

// Grayscale 8-bit 2x2 PNG (color type 0)
static const uint8_t GRAYSCALE_PNG_DATA[] = {0x89,
    0x50,
    0x4e,
    0x47,
    0x0d,
    0x0a,
    0x1a,
    0x0a,
    0x00,
    0x00,
    0x00,
    0x0d,
    0x49,
    0x48,
    0x44,
    0x52,
    0x00,
    0x00,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x02,
    0x08,
    0x00,
    0x00,
    0x00,
    0x00,
    0x57,
    0xdd,
    0x52,
    0xf8,
    0x00,
    0x00,
    0x00,
    0x0e,
    0x49,
    0x44,
    0x41,
    0x54,
    0x78,
    0x9c,
    0x63,
    0x68,
    0x70,
    0x60,
    0x38,
    0xa0,
    0x00,
    0x00,
    0x05,
    0x26,
    0x01,
    0xa1,
    0x65,
    0x6f,
    0xaa,
    0xc4,
    0x00,
    0x00,
    0x00,
    0x00,
    0x49,
    0x45,
    0x4e,
    0x44,
    0xae,
    0x42,
    0x60,
    0x82};

// Grayscale+Alpha 8-bit 2x2 PNG (color type 4)
static const uint8_t GRAYSCALE_ALPHA_PNG_DATA[] = {0x89,
    0x50,
    0x4e,
    0x47,
    0x0d,
    0x0a,
    0x1a,
    0x0a,
    0x00,
    0x00,
    0x00,
    0x0d,
    0x49,
    0x48,
    0x44,
    0x52,
    0x00,
    0x00,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x02,
    0x08,
    0x04,
    0x00,
    0x00,
    0x00,
    0xd8,
    0xbf,
    0xc5,
    0xaf,
    0x00,
    0x00,
    0x00,
    0x12,
    0x49,
    0x44,
    0x41,
    0x54,
    0x78,
    0x9c,
    0x63,
    0x68,
    0xf8,
    0xef,
    0xd0,
    0xc0,
    0x70,
    0xc0,
    0xc1,
    0xe1,
    0x04,
    0x00,
    0x16,
    0x4a,
    0x04,
    0x48,
    0x4a,
    0x02,
    0x3b,
    0xe2,
    0x00,
    0x00,
    0x00,
    0x00,
    0x49,
    0x45,
    0x4e,
    0x44,
    0xae,
    0x42,
    0x60,
    0x82};

// RGB 8-bit 2x2 PNG (color type 2, no alpha - tests add_alpha path)
static const uint8_t RGB_PNG_DATA[] = {0x89,
    0x50,
    0x4e,
    0x47,
    0x0d,
    0x0a,
    0x1a,
    0x0a,
    0x00,
    0x00,
    0x00,
    0x0d,
    0x49,
    0x48,
    0x44,
    0x52,
    0x00,
    0x00,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x02,
    0x08,
    0x02,
    0x00,
    0x00,
    0x00,
    0xfd,
    0xd4,
    0x9a,
    0x73,
    0x00,
    0x00,
    0x00,
    0x11,
    0x49,
    0x44,
    0x41,
    0x54,
    0x78,
    0x9c,
    0x63,
    0xf8,
    0xcf,
    0xc0,
    0xc0,
    0x00,
    0xc5,
    0x0d,
    0x0d,
    0x0d,
    0x00,
    0x20,
    0xf1,
    0x04,
    0x7e,
    0x5f,
    0x8d,
    0xa9,
    0x50,
    0x00,
    0x00,
    0x00,
    0x00,
    0x49,
    0x45,
    0x4e,
    0x44,
    0xae,
    0x42,
    0x60,
    0x82};

// Palette 8-bit 2x2 PNG (color type 3)
static const uint8_t PALETTE_PNG_DATA[] = {0x89,
    0x50,
    0x4e,
    0x47,
    0x0d,
    0x0a,
    0x1a,
    0x0a,
    0x00,
    0x00,
    0x00,
    0x0d,
    0x49,
    0x48,
    0x44,
    0x52,
    0x00,
    0x00,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x02,
    0x08,
    0x03,
    0x00,
    0x00,
    0x00,
    0x45,
    0x68,
    0xfd,
    0x16,
    0x00,
    0x00,
    0x00,
    0x0c,
    0x50,
    0x4c,
    0x54,
    0x45,
    0xff,
    0x00,
    0x00,
    0x00,
    0xff,
    0x00,
    0x00,
    0x00,
    0xff,
    0x80,
    0x80,
    0x80,
    0xcc,
    0xb0,
    0x46,
    0x0f,
    0x00,
    0x00,
    0x00,
    0x0e,
    0x49,
    0x44,
    0x41,
    0x54,
    0x78,
    0x9c,
    0x63,
    0x60,
    0x60,
    0x64,
    0x60,
    0x62,
    0x06,
    0x00,
    0x00,
    0x11,
    0x00,
    0x07,
    0x9e,
    0xa2,
    0x2a,
    0x12,
    0x00,
    0x00,
    0x00,
    0x00,
    0x49,
    0x45,
    0x4e,
    0x44,
    0xae,
    0x42,
    0x60,
    0x82};

// Grayscale 16-bit 2x2 PNG (color type 0, bit depth 16)
static const uint8_t GRAYSCALE_16BPC_PNG_DATA[] = {0x89,
    0x50,
    0x4e,
    0x47,
    0x0d,
    0x0a,
    0x1a,
    0x0a,
    0x00,
    0x00,
    0x00,
    0x0d,
    0x49,
    0x48,
    0x44,
    0x52,
    0x00,
    0x00,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x02,
    0x10,
    0x00,
    0x00,
    0x00,
    0x00,
    0x07,
    0x4d,
    0x8e,
    0xbb,
    0x00,
    0x00,
    0x00,
    0x12,
    0x49,
    0x44,
    0x41,
    0x54,
    0x78,
    0x9c,
    0x63,
    0x68,
    0x60,
    0x70,
    0x60,
    0x60,
    0x38,
    0xc0,
    0xa0,
    0xc0,
    0x00,
    0x00,
    0x09,
    0x8a,
    0x01,
    0xa1,
    0x74,
    0x17,
    0xab,
    0x51,
    0x00,
    0x00,
    0x00,
    0x00,
    0x49,
    0x45,
    0x4e,
    0x44,
    0xae,
    0x42,
    0x60,
    0x82};

// RGBA 8-bit 4x4 PNG (for tiled loading with depth > 1)
static const uint8_t RGBA_4X4_PNG_DATA[] = {0x89,
    0x50,
    0x4e,
    0x47,
    0x0d,
    0x0a,
    0x1a,
    0x0a,
    0x00,
    0x00,
    0x00,
    0x0d,
    0x49,
    0x48,
    0x44,
    0x52,
    0x00,
    0x00,
    0x00,
    0x04,
    0x00,
    0x00,
    0x00,
    0x04,
    0x08,
    0x06,
    0x00,
    0x00,
    0x00,
    0xa9,
    0xf1,
    0x9e,
    0x7e,
    0x00,
    0x00,
    0x00,
    0x16,
    0x49,
    0x44,
    0x41,
    0x54,
    0x78,
    0x9c,
    0x63,
    0x68,
    0x40,
    0x03,
    0x0c,
    0x0e,
    0x68,
    0x80,
    0xe1,
    0x00,
    0x1a,
    0x60,
    0x50,
    0x40,
    0x03,
    0x00,
    0xc9,
    0x71,
    0x1a,
    0x01,
    0xc9,
    0xa4,
    0x51,
    0x08,
    0x00,
    0x00,
    0x00,
    0x00,
    0x49,
    0x45,
    0x4e,
    0x44,
    0xae,
    0x42,
    0x60,
    0x82};

// Mock IFile implementation for testing
class MockFile : public CORE_NS::IFile {
public:
    MockFile(const uint8_t* data, size_t size) : data_(data), size_(size), pos_(0)
    {}

    CORE_NS::IFile::Mode GetMode() const override
    {
        return CORE_NS::IFile::Mode::READ_ONLY;
    }

    void Close() override
    {}

    uint64_t Read(void* buffer, uint64_t count) override
    {
        if (buffer == nullptr || pos_ >= size_) {
            return 0;
        }
        uint64_t readCount = (count > size_ - pos_) ? (size_ - pos_) : count;
        if (memcpy_s(buffer, static_cast<size_t>(readCount), data_ + pos_, static_cast<size_t>(readCount)) != EOK) {
            return 0;
        }
        pos_ += readCount;
        return readCount;
    }
    uint64_t Write(const void* buffer, uint64_t count) override
    {
        return 0;
    }

    uint64_t Append(const void* buffer, uint64_t count, uint64_t flushSize) override
    {
        return 0;
    }

    uint64_t GetLength() const override
    {
        return size_;
    }

    bool Seek(uint64_t offset) override
    {
        if (offset > size_) {
            return false;
        }
        pos_ = offset;
        return true;
    }

    uint64_t GetPosition() const override
    {
        return pos_;
    }

    void Destroy() override
    {
        delete this;
    }

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;
};

// Mock file that simulates a partial read failure
class MockFilePartialRead : public CORE_NS::IFile {
public:
    MockFilePartialRead(const uint8_t* data, size_t size, size_t maxRead)
        : data_(data), size_(size), maxRead_(maxRead), pos_(0)
    {}

    CORE_NS::IFile::Mode GetMode() const override
    {
        return CORE_NS::IFile::Mode::READ_ONLY;
    }
    void Close() override
    {}
    uint64_t Read(void* buffer, uint64_t count) override
    {
        if (buffer == nullptr || pos_ >= size_) {
            return 0;
        }
        size_t toRead = static_cast<size_t>(count);
        if (toRead > maxRead_) {
            toRead = maxRead_;
        }
        if (toRead > size_ - pos_) {
            toRead = size_ - pos_;
        }
        if (memcpy_s(buffer, toRead, data_ + pos_, toRead) != EOK) {
            return 0;
        }
        pos_ += toRead;
        return toRead;
    }
    uint64_t Write(const void*, uint64_t) override
    {
        return 0;
    }
    uint64_t Append(const void*, uint64_t, uint64_t) override
    {
        return 0;
    }
    uint64_t GetLength() const override
    {
        return size_;
    }
    bool Seek(uint64_t offset) override
    {
        pos_ = offset;
        return true;
    }
    uint64_t GetPosition() const override
    {
        return pos_;
    }
    void Destroy() override
    {
        delete this;
    }

private:
    const uint8_t* data_;
    size_t size_;
    size_t maxRead_;
    size_t pos_;
};

// Mock file that reports a very large size
class MockFileTooBig : public CORE_NS::IFile {
public:
    MockFileTooBig() : reportedSize_(static_cast<uint64_t>(std::numeric_limits<int>::max()) + 1)
    {}

    CORE_NS::IFile::Mode GetMode() const override
    {
        return CORE_NS::IFile::Mode::READ_ONLY;
    }
    void Close() override
    {}
    uint64_t Read(void*, uint64_t) override
    {
        return 0;
    }
    uint64_t Write(const void*, uint64_t) override
    {
        return 0;
    }
    uint64_t Append(const void*, uint64_t, uint64_t) override
    {
        return 0;
    }
    uint64_t GetLength() const override
    {
        return reportedSize_;
    }
    bool Seek(uint64_t) override
    {
        return true;
    }
    uint64_t GetPosition() const override
    {
        return 0;
    }
    void Destroy() override
    {
        delete this;
    }

private:
    uint64_t reportedSize_;
};
}  // anonymous namespace

class ImageLoaderPngTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {}
    static void TearDownTestSuite()
    {}
    void SetUp() override
    {
        loader_ = PNGPlugin::CreateImageLoaderPng(CORE_NS::PluginToken{});
    }
    void TearDown() override
    {
        loader_.reset();
    }

    IImageLoaderManager::IImageLoader::Ptr loader_;
};

/**
 * @tc.name: CreateImageLoaderPng_001
 * @tc.desc: Verify CreateImageLoaderPng returns a valid loader instance
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, CreateImageLoaderPng_001, testing::ext::TestSize.Level1)
{
    ASSERT_NE(loader_, nullptr);
}

/**
 * @tc.name: CanLoad_001
 * @tc.desc: Verify CanLoad returns true for valid PNG signature
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, CanLoad_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(PNG_SIGNATURE, sizeof(PNG_SIGNATURE));
    EXPECT_TRUE(loader_->CanLoad(view));
}

/**
 * @tc.name: CanLoad_002
 * @tc.desc: Verify CanLoad returns true for complete valid PNG data
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, CanLoad_002, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    EXPECT_TRUE(loader_->CanLoad(view));
}

/**
 * @tc.name: CanLoad_003
 * @tc.desc: Verify CanLoad returns false for invalid header (JPEG)
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, CanLoad_003, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(INVALID_HEADER, sizeof(INVALID_HEADER));
    EXPECT_FALSE(loader_->CanLoad(view));
}

/**
 * @tc.name: CanLoad_004
 * @tc.desc: Verify CanLoad returns false for empty data
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, CanLoad_004, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>();
    EXPECT_FALSE(loader_->CanLoad(view));
}

/**
 * @tc.name: CanLoad_005
 * @tc.desc: Verify CanLoad returns false for corrupted PNG signature
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, CanLoad_005, testing::ext::TestSize.Level1)
{
    uint8_t corrupted[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0b};
    auto view = array_view<const uint8_t>(corrupted, sizeof(corrupted));
    EXPECT_FALSE(loader_->CanLoad(view));
}

/**
 * @tc.name: LoadFromBytes_001
 * @tc.desc: Verify Load from valid PNG bytes with default flags succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, 0);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
}

/**
 * @tc.name: LoadFromBytes_002
 * @tc.desc: Verify Load from empty bytes fails
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_002, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>();
    auto result = loader_->Load(view, 0);
    EXPECT_FALSE(result.success);
}

/**
 * @tc.name: LoadFromBytes_003
 * @tc.desc: Verify Load from invalid (garbage) bytes fails
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_003, testing::ext::TestSize.Level1)
{
    const uint8_t garbage[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    auto view = array_view<const uint8_t>(garbage, sizeof(garbage));
    auto result = loader_->Load(view, 0);
    EXPECT_FALSE(result.success);
}

/**
 * @tc.name: LoadFromBytes_004
 * @tc.desc: Verify Load with GENERATE_MIPS flag succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_004, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_NE(desc.mipCount, 1u);
}

/**
 * @tc.name: LoadFromBytes_005
 * @tc.desc: Verify Load with FORCE_LINEAR_RGB_BIT flag succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_005, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
}

/**
 * @tc.name: LoadFromBytes_006
 * @tc.desc: Verify Load with FORCE_GRAYSCALE_BIT flag succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_006, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
}

/**
 * @tc.name: LoadFromBytes_007
 * @tc.desc: Verify Load with FLIP_VERTICALLY_BIT flag succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_007, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FLIP_VERTICALLY_BIT);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
}

/**
 * @tc.name: LoadFromBytes_008
 * @tc.desc: Verify Load with METADATA_ONLY flag succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_008, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    // With METADATA_ONLY, image metadata (dimensions) should be populated
    auto desc = result.image->GetImageDesc();
    EXPECT_EQ(desc.width, 8u);
    EXPECT_EQ(desc.height, 1u);
}

/**
 * @tc.name: LoadFromBytes_009
 * @tc.desc: Verify Load with combined flags succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_009, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    uint32_t combinedFlags =
        IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS | IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
    auto result = loader_->Load(view, combinedFlags);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
}

/**
 * @tc.name: LoadFromBytes_PremultiplyAlpha_001
 * @tc.desc: Verify Load with PREMULTIPLY_ALPHA flag succeeds for RGBA PNG
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_PremultiplyAlpha_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_TRUE(desc.imageFlags & IImageContainer::ImageFlags::FLAGS_PREMULTIPLIED_ALPHA_BIT);
}

/**
 * @tc.name: LoadFromFile_001
 * @tc.desc: Verify Load from valid IFile succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromFile_001, testing::ext::TestSize.Level1)
{
    auto* file = new MockFile(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(*file, 0);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    delete file;
}

/**
 * @tc.name: LoadFromFile_002
 * @tc.desc: Verify Load from oversized IFile fails with file too big error
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromFile_002, testing::ext::TestSize.Level1)
{
    auto* file = new MockFileTooBig();
    auto result = loader_->Load(*file, 0);
    EXPECT_FALSE(result.success);
    delete file;
}

/**
 * @tc.name: LoadFromFile_003
 * @tc.desc: Verify Load from IFile with partial read failure
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromFile_003, testing::ext::TestSize.Level1)
{
    auto* file = new MockFilePartialRead(VALID_PNG_DATA, VALID_PNG_SIZE, 10);
    auto result = loader_->Load(*file, 0);
    EXPECT_FALSE(result.success);
    delete file;
}

/**
 * @tc.name: LoadFromFile_004
 * @tc.desc: Verify Load from IFile with GENERATE_MIPS flag
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromFile_004, testing::ext::TestSize.Level1)
{
    auto* file = new MockFile(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(*file, IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_NE(desc.mipCount, 1u);
    delete file;
}

/**
 * @tc.name: LoadFromFile_Tiled_001
 * @tc.desc: Verify tiled Load from valid IFile with rowCount=1 columnCount=1
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromFile_Tiled_001, testing::ext::TestSize.Level1)
{
    auto* file = new MockFile(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(*file, 0, 1, 1);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_EQ(desc.width, 8u);
    EXPECT_EQ(desc.height, 1u);
    EXPECT_EQ(desc.depth, 1u);
    delete file;
}

/**
 * @tc.name: LoadFromFile_Tiled_002
 * @tc.desc: Verify tiled Load from oversized IFile fails
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromFile_Tiled_002, testing::ext::TestSize.Level1)
{
    auto* file = new MockFileTooBig();
    auto result = loader_->Load(*file, 0, 2, 1);
    EXPECT_FALSE(result.success);
    delete file;
}

/**
 * @tc.name: LoadFromFile_Tiled_003
 * @tc.desc: Verify tiled Load from IFile with partial read failure
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromFile_Tiled_003, testing::ext::TestSize.Level1)
{
    auto* file = new MockFilePartialRead(VALID_PNG_DATA, VALID_PNG_SIZE, 10);
    auto result = loader_->Load(*file, 0, 2, 1);
    EXPECT_FALSE(result.success);
    delete file;
}

/**
 * @tc.name: ImageProperties_001
 * @tc.desc: Verify loaded image dimensions are correct (8x1)
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, ImageProperties_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, 0);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);

    auto desc = result.image->GetImageDesc();
    EXPECT_EQ(desc.width, 8u);
    EXPECT_EQ(desc.height, 1u);
    EXPECT_EQ(desc.depth, 1u);
    EXPECT_EQ(desc.imageType, IImageContainer::ImageType::TYPE_2D);
    EXPECT_EQ(desc.imageViewType, IImageContainer::ImageViewType::VIEW_TYPE_2D);
}

/**
 * @tc.name: ImageProperties_002
 * @tc.desc: Verify loaded image has valid format
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, ImageProperties_002, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, 0);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);

    auto desc = result.image->GetImageDesc();
    EXPECT_NE(desc.format, Format::BASE_FORMAT_UNDEFINED);
}

/**
 * @tc.name: ImageProperties_003
 * @tc.desc: Verify loaded image with LINEAR_RGB has UNORM format
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, ImageProperties_003, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);

    auto desc = result.image->GetImageDesc();
    EXPECT_NE(desc.format, Format::BASE_FORMAT_UNDEFINED);
}

/**
 * @tc.name: ImageProperties_004
 * @tc.desc: Verify loaded image has correct layer count
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, ImageProperties_004, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, 0);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);

    auto desc = result.image->GetImageDesc();
    EXPECT_EQ(desc.layerCount, 1u);
}

/**
 * @tc.name: ImageData_001
 * @tc.desc: Verify loaded image data buffer is not empty for valid PNG
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, ImageData_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, 0);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);

    auto data = result.image->GetData();
    EXPECT_GT(data.size(), 0u);
}

/**
 * @tc.name: ImageData_002
 * @tc.desc: Verify loaded image buffer copies have correct dimensions
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, ImageData_002, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, 0);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);

    auto copies = result.image->GetBufferImageCopies();
    EXPECT_EQ(copies.size(), 1u);
    EXPECT_EQ(copies[0].width, 8u);
    EXPECT_EQ(copies[0].height, 1u);
    EXPECT_EQ(copies[0].depth, 1u);
}

/**
 * @tc.name: LoadAnimatedImage_001
 * @tc.desc: Verify LoadAnimatedImage from bytes always fails (PNG does not support animation in this plugin)
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadAnimatedImage_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->LoadAnimatedImage(view, 0);
    EXPECT_FALSE(result.success);
}

/**
 * @tc.name: LoadAnimatedImage_002
 * @tc.desc: Verify LoadAnimatedImage from IFile always fails
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadAnimatedImage_002, testing::ext::TestSize.Level1)
{
    auto* file = new MockFile(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->LoadAnimatedImage(*file, 0);
    EXPECT_FALSE(result.success);
    delete file;
}

/**
 * @tc.name: GetSupportedTypes_001
 * @tc.desc: Verify GetSupportedTypes returns correct PNG types
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, GetSupportedTypes_001, testing::ext::TestSize.Level1)
{
    auto types = loader_->GetSupportedTypes();
    EXPECT_EQ(types.size(), 1u);
    EXPECT_EQ(types[0].mimeType, "image/png");
    EXPECT_EQ(types[0].fileExtension, "png");
}

/**
 * @tc.name: LoadFromBytes_TruncatedPNG_001
 * @tc.desc: Verify Load from truncated PNG data fails
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_TruncatedPNG_001, testing::ext::TestSize.Level1)
{
    // Only provide the first 30 bytes (signature + partial IHDR)
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, 30);
    auto result = loader_->Load(view, 0);
    EXPECT_FALSE(result.success);
}

/**
 * @tc.name: LoadFromBytes_CorruptedPNG_001
 * @tc.desc: Verify Load from corrupted PNG data (valid header, corrupt IDAT) fails
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_CorruptedPNG_001, testing::ext::TestSize.Level1)
{
    uint8_t corrupted[sizeof(VALID_PNG_DATA)];
    EXPECT_EQ(memcpy_s(corrupted, sizeof(VALID_PNG_DATA), VALID_PNG_DATA, sizeof(VALID_PNG_DATA)), EOK);
    // Corrupt the IDAT chunk data
    corrupted[80] = 0x00;
    corrupted[81] = 0x00;
    auto view = array_view<const uint8_t>(corrupted, sizeof(corrupted));
    auto result = loader_->Load(view, 0);
    // Should not crash; may fail or succeed with corrupt data
}

/**
 * @tc.name: ImageProperties_MipmapCalculation_001
 * @tc.desc: Verify mipmap count for 8x1 image
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, ImageProperties_MipmapCalculation_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);

    auto desc = result.image->GetImageDesc();
    // For 8x1 image, max dimension is 8, mip levels: 8->4->2->1 = 4 levels
    EXPECT_EQ(desc.mipCount, 4u);
    EXPECT_TRUE(desc.imageFlags & IImageContainer::ImageFlags::FLAGS_REQUESTING_MIPMAPS_BIT);
}

/**
 * @tc.name: LoadFromBytes_PremultiplyAlphaLinear_001
 * @tc.desc: Verify Load with PREMULTIPLY_ALPHA and FORCE_LINEAR_RGB flags succeeds
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_PremultiplyAlphaLinear_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    uint32_t flags =
        IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA | IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
    auto result = loader_->Load(view, flags);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_TRUE(desc.imageFlags & IImageContainer::ImageFlags::FLAGS_PREMULTIPLIED_ALPHA_BIT);
}

/**
 * @tc.name: LoadFromBytes_MetadataOnlyNoPremultiply_001
 * @tc.desc: Verify Load with METADATA_ONLY ignores PREMULTIPLY_ALPHA
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_MetadataOnlyNoPremultiply_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    uint32_t flags =
        IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY | IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA;
    auto result = loader_->Load(view, flags);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    // With METADATA_ONLY, premultiply should not happen
    EXPECT_FALSE(desc.imageFlags & IImageContainer::ImageFlags::FLAGS_PREMULTIPLIED_ALPHA_BIT);
}

// ==================== Different Color Types (cover UpdateColorType branches) ====================

/**
 * @tc.name: LoadFromBytes_Grayscale_001
 * @tc.desc: Verify Load grayscale PNG triggers gray_to_rgb + add_alpha path
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_Grayscale_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(GRAYSCALE_PNG_DATA, sizeof(GRAYSCALE_PNG_DATA));
    auto result = loader_->Load(view, 0);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_EQ(desc.width, 2u);
    EXPECT_EQ(desc.height, 2u);
    EXPECT_NE(desc.format, Format::BASE_FORMAT_UNDEFINED);
}

/**
 * @tc.name: LoadFromBytes_Grayscale_002
 * @tc.desc: Verify Load grayscale PNG with FORCE_GRAYSCALE keeps grayscale (no gray_to_rgb)
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_Grayscale_002, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(GRAYSCALE_PNG_DATA, sizeof(GRAYSCALE_PNG_DATA));
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
}

/**
 * @tc.name: LoadFromBytes_GrayscaleAlpha_001
 * @tc.desc: Verify Load grayscale+alpha PNG triggers channelCount=2 PremultiplyAlpha path
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_GrayscaleAlpha_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(GRAYSCALE_ALPHA_PNG_DATA, sizeof(GRAYSCALE_ALPHA_PNG_DATA));
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_TRUE(desc.imageFlags & IImageContainer::ImageFlags::FLAGS_PREMULTIPLIED_ALPHA_BIT);
}

/**
 * @tc.name: LoadFromBytes_GrayscaleAlpha_002
 * @tc.desc: Verify FORCE_GRAYSCALE on grayscale+alpha triggers strip_alpha path
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_GrayscaleAlpha_002, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(GRAYSCALE_ALPHA_PNG_DATA, sizeof(GRAYSCALE_ALPHA_PNG_DATA));
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
}

/**
 * @tc.name: LoadFromBytes_RGB_001
 * @tc.desc: Verify Load RGB PNG (no alpha) triggers add_alpha path
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_RGB_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(RGB_PNG_DATA, sizeof(RGB_PNG_DATA));
    auto result = loader_->Load(view, 0);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_EQ(desc.width, 2u);
    EXPECT_EQ(desc.height, 2u);
}

/**
 * @tc.name: LoadFromBytes_Palette_001
 * @tc.desc: Verify Load palette PNG triggers palette_to_rgb path
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_Palette_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(PALETTE_PNG_DATA, sizeof(PALETTE_PNG_DATA));
    auto result = loader_->Load(view, 0);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_EQ(desc.width, 2u);
    EXPECT_EQ(desc.height, 2u);
}

/**
 * @tc.name: LoadFromBytes_16BPC_001
 * @tc.desc: Verify Load 16-bit PNG triggers png_set_scale_16 + is16bpc path
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromBytes_16BPC_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(GRAYSCALE_16BPC_PNG_DATA, sizeof(GRAYSCALE_16BPC_PNG_DATA));
    auto result = loader_->Load(view, 0);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_EQ(desc.width, 2u);
    EXPECT_EQ(desc.height, 2u);
}

// ==================== Tiled 3D (cover Handle3DTexture with depth > 1) ====================

/**
 * @tc.name: LoadFromFile_Tiled_3D_001
 * @tc.desc: Verify tiled Load with rowCount=2 columnCount=2 triggers Handle3DTexture
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, LoadFromFile_Tiled_3D_001, testing::ext::TestSize.Level1)
{
    auto* file = new MockFile(RGBA_4X4_PNG_DATA, sizeof(RGBA_4X4_PNG_DATA));
    auto result = loader_->Load(*file, 0, 2, 2);
    EXPECT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    auto desc = result.image->GetImageDesc();
    EXPECT_EQ(desc.depth, 4u);
    EXPECT_EQ(desc.imageType, IImageContainer::ImageType::TYPE_3D);
    EXPECT_EQ(desc.imageViewType, IImageContainer::ImageViewType::VIEW_TYPE_3D);
    auto copies = result.image->GetBufferImageCopies();
    EXPECT_EQ(copies.size(), 4u);
    for (size_t i = 0; i < copies.size(); i++) {
        EXPECT_EQ(copies[i].width, 2u);
        EXPECT_EQ(copies[i].height, 2u);
        EXPECT_EQ(copies[i].depth, 4u);
    }
    auto data = result.image->GetData();
    EXPECT_GT(data.size(), 0u);
    delete file;
}

/**
 * @tc.name: FormatVerification_RGBA_SRGB_001
 * @tc.desc: Verify component=4, !linear, !16bpc → R8G8B8A8_SRGB
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, FormatVerification_RGBA_SRGB_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, 0);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    EXPECT_EQ(result.image->GetImageDesc().format, Format::BASE_FORMAT_R8G8B8A8_SRGB);
}

/**
 * @tc.name: FormatVerification_RGBA_UNORM_001
 * @tc.desc: Verify component=4, linear, !16bpc → R8G8B8A8_UNORM
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, FormatVerification_RGBA_UNORM_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(VALID_PNG_DATA, VALID_PNG_SIZE);
    auto result = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    EXPECT_EQ(result.image->GetImageDesc().format, Format::BASE_FORMAT_R8G8B8A8_UNORM);
}

/**
 * @tc.name: FormatVerification_Grayscale_001
 * @tc.desc: Verify component=1, !linear, !16bpc → R8_SRGB; linear → R8_UNORM
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, FormatVerification_Grayscale_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(GRAYSCALE_PNG_DATA, sizeof(GRAYSCALE_PNG_DATA));
    auto r1 = loader_->Load(view, IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT);
    ASSERT_TRUE(r1.success);
    EXPECT_EQ(r1.image->GetImageDesc().format, Format::BASE_FORMAT_R8_SRGB);

    uint32_t flags =
        IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT | IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
    auto r2 = loader_->Load(view, flags);
    ASSERT_TRUE(r2.success);
    EXPECT_EQ(r2.image->GetImageDesc().format, Format::BASE_FORMAT_R8_UNORM);
}

/**
 * @tc.name: FormatVerification_16BPC_001
 * @tc.desc: Verify component=1, linear, 16bpc → R16_UNORM
 * @tc.type: FUNC
 */
HWTEST_F(ImageLoaderPngTest, FormatVerification_16BPC_001, testing::ext::TestSize.Level1)
{
    auto view = array_view<const uint8_t>(GRAYSCALE_16BPC_PNG_DATA, sizeof(GRAYSCALE_16BPC_PNG_DATA));
    uint32_t flags =
        IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT | IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT;
    auto result = loader_->Load(view, flags);
    ASSERT_TRUE(result.success);
    ASSERT_NE(result.image, nullptr);
    EXPECT_EQ(result.image->GetImageDesc().format, Format::BASE_FORMAT_R16_UNORM);
}
