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

#include "image/loaders/image_loader_astc.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

#include <base/containers/allocator.h>
#include <base/containers/array_view.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <base/util/formats.h>
#include <core/image/intf_image_container.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/io/intf_file.h>
#include <core/log.h>
#include <core/namespace.h>

#include "image/image_loader_manager.h"

CORE_BEGIN_NAMESPACE()
namespace {
using BASE_NS::array_view;
using BASE_NS::CloneData;
using BASE_NS::Format;
using BASE_NS::make_unique;
using BASE_NS::move;
using BASE_NS::string_view;
using BASE_NS::unique_ptr;
using BASE_NS::vector;

constexpr const size_t ASTC_MAGIC_LENGTH = 4;
constexpr const uint8_t ASTC_MAGIC[ASTC_MAGIC_LENGTH] = { 0x13, 0xAB, 0xA1, 0x5C };
constexpr const uint8_t ASTC_HEADER_SIZE = 16u;
constexpr const size_t ASTC_BYTES_PER_BLOCK = 16;
constexpr const uint32_t MAX_DIMENSIONS = 16384u;

struct AstcHeader {
    uint8_t magic[4];
    uint8_t blockWidth;
    uint8_t blockHeight;
    uint8_t blockDepth;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};

inline Format GetAstcBlockSizeFormat(uint32_t x, uint32_t y, bool srgb)
{
    // We don't have any info if the format is unorm or srgb
    struct FormatPair {
        Format unorm;
        Format srgb;
    };

    const BASE_NS::unordered_map<uint64_t, FormatPair> formatMap = {
        { 0x0000000400000004U, { Format::BASE_FORMAT_ASTC_4x4_UNORM_BLOCK, Format::BASE_FORMAT_ASTC_4x4_SRGB_BLOCK } },
        { 0x0000000500000004U, { Format::BASE_FORMAT_ASTC_5x4_UNORM_BLOCK, Format::BASE_FORMAT_ASTC_5x4_SRGB_BLOCK } },
        { 0x0000000500000005U, { Format::BASE_FORMAT_ASTC_5x5_UNORM_BLOCK, Format::BASE_FORMAT_ASTC_5x5_SRGB_BLOCK } },
        { 0x0000000600000005U, { Format::BASE_FORMAT_ASTC_6x5_UNORM_BLOCK, Format::BASE_FORMAT_ASTC_6x5_SRGB_BLOCK } },
        { 0x0000000600000006U, { Format::BASE_FORMAT_ASTC_6x6_UNORM_BLOCK, Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK } },
        { 0x0000000800000005U, { Format::BASE_FORMAT_ASTC_8x5_UNORM_BLOCK, Format::BASE_FORMAT_ASTC_8x5_SRGB_BLOCK } },
        { 0x0000000800000006U, { Format::BASE_FORMAT_ASTC_8x6_UNORM_BLOCK, Format::BASE_FORMAT_ASTC_8x6_SRGB_BLOCK } },
        { 0x0000000A00000005U,
            { Format::BASE_FORMAT_ASTC_10x5_UNORM_BLOCK, Format::BASE_FORMAT_ASTC_10x5_SRGB_BLOCK } },
        { 0x0000000A00000006U,
            { Format::BASE_FORMAT_ASTC_10x6_UNORM_BLOCK, Format::BASE_FORMAT_ASTC_10x6_SRGB_BLOCK } },
        { 0x0000000800000008U, { Format::BASE_FORMAT_ASTC_8x8_UNORM_BLOCK, Format::BASE_FORMAT_ASTC_8x8_SRGB_BLOCK } },
        { 0x0000000A00000008U,
            { Format::BASE_FORMAT_ASTC_10x8_UNORM_BLOCK, Format::BASE_FORMAT_ASTC_10x8_SRGB_BLOCK } },
        { 0x0000000A0000000AU,
            { Format::BASE_FORMAT_ASTC_10x10_UNORM_BLOCK, Format::BASE_FORMAT_ASTC_10x10_SRGB_BLOCK } },
        { 0x0000000C0000000AU,
            { Format::BASE_FORMAT_ASTC_12x10_UNORM_BLOCK, Format::BASE_FORMAT_ASTC_12x10_SRGB_BLOCK } },
        { 0x0000000C0000000CU,
            { Format::BASE_FORMAT_ASTC_12x12_UNORM_BLOCK, Format::BASE_FORMAT_ASTC_12x12_SRGB_BLOCK } },
    };

    const uint64_t key = (static_cast<uint64_t>(x) << 32u) | y;
    const auto it = formatMap.find(key);
    return (it != formatMap.end()) ? (srgb ? it->second.srgb : it->second.unorm) : Format::BASE_FORMAT_UNDEFINED;
}

class AstcImage final : public IImageContainer {
public:
    AstcImage(unique_ptr<uint8_t[]>&& fileBytes, size_t fileBytesLength)
        : fileBytes_(move(fileBytes)), fileBytesLength_(fileBytesLength)
    {}
    using Ptr = unique_ptr<AstcImage, Deleter>;

    const ImageDesc& GetImageDesc() const override
    {
        return imageDesc_;
    };

    array_view<const uint8_t> GetData() const override
    {
        return { fileBytes_.get() + ASTC_HEADER_SIZE, fileBytesLength_ - ASTC_HEADER_SIZE };
    };

    array_view<const SubImageDesc> GetBufferImageCopies() const override
    {
        return { &imageBuffer_, 1 };
    };

    static AstcHeader ReadHeader(const uint8_t* data)
    {
        struct RawHeader {
            uint8_t magic[4u]; //  4 byte file magic
            uint8_t block_x;
            uint8_t block_y;
            uint8_t block_z;
            uint8_t dim_x[3u];
            uint8_t dim_y[3u];
            uint8_t dim_z[3u];
        };

        AstcHeader astcHeader = {};
        const RawHeader* rawHeader = reinterpret_cast<const RawHeader*>(data);

        CloneData(astcHeader.magic, sizeof(astcHeader.magic), rawHeader->magic, ASTC_MAGIC_LENGTH);
        astcHeader.blockWidth = rawHeader->block_x;
        astcHeader.blockHeight = rawHeader->block_y;
        astcHeader.blockDepth = rawHeader->block_z;

        // Image dimension integers are unsigned and packed to 3-bytes, so need to unpack to 4-byte uint32_t.
        // Castnig a 24-bit int to uint32_t is safe because it doesn't change the sign bit.
        astcHeader.width = (uint32_t)(rawHeader->dim_x[0] + (rawHeader->dim_x[1] << 8u) + (rawHeader->dim_x[2] << 16u));
        astcHeader.height =
            (uint32_t)(rawHeader->dim_y[0] + (rawHeader->dim_y[1] << 8u) + (rawHeader->dim_y[2] << 16u));
        astcHeader.depth = (uint32_t)(rawHeader->dim_z[0] + (rawHeader->dim_z[1] << 8u) + (rawHeader->dim_z[2] << 16u));

        return astcHeader;
    }

    static ImageLoaderManager::LoadResult CreateImage(
        AstcImage::Ptr image, const AstcHeader& header, uint32_t loadFlags, const uint8_t* data)
    {
        // IMAGE_LOADER_GENERATE_MIPS flag not supported, because you can't really generate mips for ASTC in the engine,
        // need the arm cli to do it.
        // IMAGE_LOADER_FORCE_LINEAR_RGB_BIT is "default", because there is no format metadata so we assume linear rgb
        // (UNORM) format.

        bool srgb = loadFlags & IImageLoaderManager::IMAGE_LOADER_FORCE_SRGB_BIT;
        // Fill image descriptions, dimensions are already validated to be 2D before
        image->imageDesc_ = {
            ImageFlags::FLAGS_COMPRESSED_BIT, // imageFlags
            header.blockWidth,                // blockPixelWidth
            header.blockHeight,               // blockPixelHeight
            header.blockDepth,                // blockPixelDepth
            ASTC_BYTES_PER_BLOCK * 8u,        // bitsPerBlock

            ImageType::TYPE_2D,                                                  // imageType, only 2D support
            ImageViewType::VIEW_TYPE_2D,                                         // imageViewType
            GetAstcBlockSizeFormat(header.blockWidth, header.blockHeight, srgb), // format, defaults to linear rgb so
                                                                                 // only check for srgb load flag

            header.width,  // width
            header.height, // height
            header.depth,  // depth

            1, // mipCount
            1, // layerCount
        };

        image->imageBuffer_ = SubImageDesc {
            0,             // uint32_t bufferOffset
            header.width,  // uint32_t bufferRowLength
            header.height, // uint32_t bufferImageHeight

            0, // uint32_t mipLevel
            1, // uint32_t layerCount

            header.width,
            header.height,
            header.depth,
        };

        return ImageLoaderManager::ResultSuccess(move(image));
    }

    static ImageLoaderManager::LoadResult Load(
        unique_ptr<uint8_t[]> fileBytes, uint64_t fileBytesLength, uint32_t loadFlags)
    {
        if (!fileBytes) {
            return ImageLoaderManager::ResultFailure("Input data must not be null.");
        }
        if (fileBytesLength < ASTC_HEADER_SIZE) {
            return ImageLoaderManager::ResultFailure("Not enough data for parsing astc.");
        }

        auto image = AstcImage::Ptr(new AstcImage(move(fileBytes), static_cast<size_t>(fileBytesLength)));

        const uint8_t* data = image->fileBytes_.get();

        const AstcHeader header = ReadHeader(data);
        const Format format = GetAstcBlockSizeFormat(header.blockWidth, header.blockHeight, false);

        // Validate header values
        if (memcmp(header.magic, ASTC_MAGIC, ASTC_MAGIC_LENGTH) != 0) {
            CORE_LOG_D("Astc invalid file magic sequence.");
            return ImageLoaderManager::ResultFailure("Invalid astc data.");
        }
        if (header.blockDepth > 1 || header.depth > 1) {
            CORE_LOG_D("Astc no support for 3D images.");
            return ImageLoaderManager::ResultFailure("Invalid astc data.");
        }
        if (header.blockWidth == 0 || header.blockHeight == 0 || header.blockDepth == 0) {
            CORE_LOG_D("Astc any of the image block size dimensions cannot be 0.");
            return ImageLoaderManager::ResultFailure("Invalid astc data.");
        }
        if (header.width == 0 || header.height == 0 || header.depth == 0) {
            CORE_LOG_D("Astc any of the image dimensions cannot be 0.");
            return ImageLoaderManager::ResultFailure("Invalid astc data.");
        }
        if (header.width > MAX_DIMENSIONS || header.height > MAX_DIMENSIONS || header.depth > MAX_DIMENSIONS) {
            CORE_LOG_D("Astc image dimensions too big.");
            return ImageLoaderManager::ResultFailure("Invalid astc data.");
        }
        if (format == Format::BASE_FORMAT_UNDEFINED) {
            CORE_LOG_D("Astc invalid block size.");
            return ImageLoaderManager::ResultFailure("Invalid astc data.");
        }

        return CreateImage(move(image), header, loadFlags, data);
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    unique_ptr<uint8_t[]> fileBytes_;
    size_t fileBytesLength_ { 0 };

    ImageDesc imageDesc_;
    SubImageDesc imageBuffer_;
};

class ImageLoaderAstc final : public IImageLoaderManager::IImageLoader {
public:
    bool CanLoad(array_view<const uint8_t> imageFileBytes) const override
    {
        return (imageFileBytes.size() >= ASTC_MAGIC_LENGTH &&
                memcmp(imageFileBytes.data(), ASTC_MAGIC, ASTC_MAGIC_LENGTH) == 0);
    }

    /** Load Image file from provided file with passed flags
     * @param file File where to load from
     * @param loadFlags Load flags. Combination of #ImageLoaderFlags
     * @return Result of the loading operation.
     */
    ImageLoaderManager::LoadResult Load(IFile& file, uint32_t loadFlags) const override
    {
        uint64_t byteLength = file.GetLength();
        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) != 0) {
            // Only load header
            byteLength = ASTC_HEADER_SIZE;
        }
        // Read the file to a buffer.
        unique_ptr<uint8_t[]> buffer = make_unique<uint8_t[]>(static_cast<size_t>(byteLength));
        const uint64_t read = file.Read(buffer.get(), byteLength);
        if (read != byteLength) {
            return ImageLoaderManager::ResultFailure("Reading file failed.");
        }

        return AstcImage::Load(std::move(buffer), byteLength, loadFlags);
    }

    /** Load image file from given data bytes
     * @param imageFileBytes Image data.
     * @param loadFlags Load flags. Combination of #ImageLoaderFlags
     * @return Result of the loading operation.
     */
    ImageLoaderManager::LoadResult Load(array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) const override
    {
        // NOTE: could reuse this and remove the extra copy here if the data would be given as a unique_ptr.
        unique_ptr<uint8_t[]> buffer = make_unique<uint8_t[]>(static_cast<size_t>(imageFileBytes.size()));
        if (buffer) {
            std::copy(imageFileBytes.begin(), imageFileBytes.end(), buffer.get());
        }

        return AstcImage::Load(move(buffer), imageFileBytes.size(), loadFlags);
    }

    /** Load animated image with given parameters
     * @param file File to load image from
     * @param loadFlags Load flags. Combination of #ImageLoaderFlags
     * @return Result of the loading operation.
     */
    ImageLoaderManager::LoadAnimatedResult LoadAnimatedImage(IFile& file, uint32_t loadFlags) override
    {
        return ImageLoaderManager::ResultFailureAnimated("Animated Astc not supported.");
    }

    /** Load animated image with given parameters
     * @param imageFileBytes Image data
     * @param loadFlags Load flags. Combination of #ImageLoaderFlags
     * @return Result of the loading operation.
     */
    ImageLoaderManager::LoadAnimatedResult LoadAnimatedImage(
        array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) override
    {
        return ImageLoaderManager::ResultFailureAnimated("Animated Astc not supported.");
    }

    /** Return a list of supported image formats.
     * @return List of supported image formats.
     */
    vector<ImageLoaderManager::ImageType> GetSupportedTypes() const override
    {
        return { std::begin(ASTC_IMAGE_TYPES), std::end(ASTC_IMAGE_TYPES) };
    }

protected:
    void Destroy() final
    {
        delete this;
    }
};
} // namespace

IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderAstc(PluginToken)
{
    return ImageLoaderManager::IImageLoader::Ptr { new ImageLoaderAstc() };
}

CORE_END_NAMESPACE()
