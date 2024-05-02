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

#include "image/loaders/image_loader_ktx.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

#include <base/containers/allocator.h>
#include <base/containers/array_view.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <base/util/formats.h>
#include <core/image/intf_image_container.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/io/intf_file.h>
#include <core/log.h>
#include <core/namespace.h>

#include "image/image_loader_manager.h"
#include "image/loaders/gl_util.h"

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

uint32_t ReadU32(const uint8_t** data)
{
    CORE_ASSERT(data);
    CORE_ASSERT(*data);

    uint32_t value = *(*data)++;
    value |= static_cast<uint32_t>(*(*data)++) << 8;
    value |= static_cast<uint32_t>(*(*data)++) << 16;
    value |= static_cast<uint32_t>(*(*data)++) << 24;
    return value;
}

uint32_t ReadU32FlipEndian(const uint8_t** data)
{
    CORE_ASSERT(data);
    CORE_ASSERT(*data);

    uint32_t value = static_cast<uint32_t>(*(*data)++) << 24;
    value |= static_cast<uint32_t>(*(*data)++) << 16;
    value |= static_cast<uint32_t>(*(*data)++) << 8;
    value |= static_cast<uint32_t>(*(*data)++);
    return value;
}

#ifdef CORE_READ_KTX_HEADER_STRING
// NOTE: Returns null if the value is not a valid null terminated string.
//       (i.e. maxBytes was reached before a null was found)
string_view ReadStringZ(const uint8_t** data, size_t maxBytes, size_t* bytesReadOut)
{
    CORE_ASSERT(data);
    CORE_ASSERT(*data);
    CORE_ASSERT(bytesReadOut);

    *bytesReadOut = 0;

    if (maxBytes == 0) {
        return {};
    }

    const auto start = *data;
    const auto end = start + maxBytes;

    if (auto const pos = std::find(start, end, 0); pos != end) {
        *data = pos + 1;
        *bytesReadOut = static_cast<size_t>(std::distance(start, pos + 1));
        return { reinterpret_cast<const char*>(start), *bytesReadOut };
    }

    return {};
}
#endif

// 12 byte ktx identifier.
constexpr const size_t KTX_IDENTIFIER_LENGTH = 12;
constexpr const char KTX_IDENTIFIER_REFERENCE[KTX_IDENTIFIER_LENGTH] = { '\xAB', 'K', 'T', 'X', ' ', '1', '1', '\xBB',
    '\r', '\n', '\x1A', '\n' };
constexpr const uint32_t KTX_FILE_ENDIANNESS = 0x04030201;
constexpr const uint32_t KTX_FILE_ENDIANNESS_FLIPPED = 0x01020304;

struct KtxHeader {
    int8_t identifier[KTX_IDENTIFIER_LENGTH];
    uint32_t endianness;
    uint32_t glType;
    uint32_t glTypeSize;
    uint32_t glFormat;
    uint32_t glInternalFormat;
    uint32_t glBaseInternalFormat;
    uint32_t pixelWidth;
    uint32_t pixelHeight;
    uint32_t pixelDepth;
    uint32_t numberOfArrayElements;
    uint32_t numberOfFaces;
    uint32_t numberOfMipmapLevels;
    uint32_t bytesOfKeyValueData;
};

constexpr const size_t KTX_HEADER_LENGTH = sizeof(KtxHeader);

IImageContainer::ImageType GetImageType(const KtxHeader& header)
{
    if (header.pixelHeight == 0 && header.pixelDepth == 0) {
        return IImageContainer::ImageType::TYPE_1D;
    }
    if (header.pixelDepth == 0) {
        return IImageContainer::ImageType::TYPE_2D;
    }

    return IImageContainer::ImageType::TYPE_3D;
}

IImageContainer::ImageViewType GetImageViewType(const KtxHeader& header, IImageContainer::ImageType imageType)
{
    const bool isArray = (header.numberOfArrayElements != 0);
    const bool isCubeMap = (header.numberOfFaces == 6);

    if (isCubeMap) {
        if (imageType == IImageContainer::ImageType::TYPE_3D || imageType == IImageContainer::ImageType::TYPE_1D) {
            // Cubemaps must be 2d textures.
            return IImageContainer::ImageViewType::VIEW_TYPE_MAX_ENUM;
        }
        return (isArray ? IImageContainer::ImageViewType::VIEW_TYPE_CUBE_ARRAY
                        : IImageContainer::ImageViewType::VIEW_TYPE_CUBE);
    }
    if (isArray) {
        switch (imageType) {
            case IImageContainer::ImageType::TYPE_1D:
                return IImageContainer::ImageViewType::VIEW_TYPE_1D_ARRAY;
            case IImageContainer::ImageType::TYPE_2D:
                return IImageContainer::ImageViewType::VIEW_TYPE_2D_ARRAY;
            case IImageContainer::ImageType::TYPE_3D:
                // 3d arrays are not supported.
                [[fallthrough]];
            case IImageContainer::ImageType::TYPE_MAX_ENUM:
                return IImageContainer::ImageViewType::VIEW_TYPE_MAX_ENUM;
        }
    } else {
        switch (imageType) {
            case IImageContainer::ImageType::TYPE_1D:
                return IImageContainer::ImageViewType::VIEW_TYPE_1D;
            case IImageContainer::ImageType::TYPE_2D:
                return IImageContainer::ImageViewType::VIEW_TYPE_2D;
            case IImageContainer::ImageType::TYPE_3D:
                return IImageContainer::ImageViewType::VIEW_TYPE_3D;
            case IImageContainer::ImageType::TYPE_MAX_ENUM:
                return IImageContainer::ImageViewType::VIEW_TYPE_MAX_ENUM;
        }
    }

    return IImageContainer::ImageViewType::VIEW_TYPE_MAX_ENUM;
}

class KtxImage final : public IImageContainer {
public:
    KtxImage() = default;

    KtxImage(unique_ptr<uint8_t[]>&& fileBytes, size_t fileBytesLength)
        : fileBytes_(CORE_NS::move(fileBytes)), fileBytesLength_(fileBytesLength)
    {}

    using Ptr = BASE_NS::unique_ptr<KtxImage, Deleter>;

    const ImageDesc& GetImageDesc() const override
    {
        return imageDesc_;
    }

    array_view<const uint8_t> GetData() const override
    {
        return array_view<const uint8_t>(imageBytes_, imageBytesLength_);
    }

    array_view<const SubImageDesc> GetBufferImageCopies() const override
    {
        return imageBuffers_;
    }

    static void ProcessMipmapLevel(KtxImage::Ptr& image, const size_t imageBufferIndex,
        const uint32_t currentImageElementOffset, const GlImageFormatInfo& formatInfo, const uint32_t elementWidth,
        const uint32_t elementHeight, const uint32_t mipmapLevel, const uint32_t faceCount,
        const uint32_t arrayElementCount, const uint32_t elementDepth, const size_t subelementLength)
    {
        image->imageBuffers_[imageBufferIndex].bufferOffset = currentImageElementOffset;

        // Vulkan requires the bufferRowLength and bufferImageHeight to be multiple of block width / height.
        const auto blockWidth = formatInfo.blockWidth;
        const auto blockHeight = formatInfo.blockHeight;
        const auto widthBlockCount = (elementWidth + (blockWidth - 1)) / blockWidth;
        const auto heightBlockCount = (elementHeight + (blockHeight - 1)) / blockHeight;
        image->imageBuffers_[imageBufferIndex].bufferRowLength = widthBlockCount * blockWidth;
        image->imageBuffers_[imageBufferIndex].bufferImageHeight = heightBlockCount * blockHeight;

        image->imageBuffers_[imageBufferIndex].mipLevel = mipmapLevel;

        image->imageBuffers_[imageBufferIndex].layerCount = faceCount * arrayElementCount;

        image->imageBuffers_[imageBufferIndex].width = elementWidth;
        image->imageBuffers_[imageBufferIndex].height = elementHeight;
        image->imageBuffers_[imageBufferIndex].depth = elementDepth;

        //
        // Vulkan requires that: "If the calling command's VkImage parameter's
        // format is not a depth/stencil format or a multi-planar format, then
        // bufferOffset must be a multiple of the format's texel block size."
        //
        // This is a bit problematic as the ktx format requires padding only to
        // 4 bytes and contains a 4 byte "lodsize" value between each data section.
        // this causes all formats with bytesPerBlock > 4 to be misaligned.
        //
        // NOTE: try to figure out if there is a better way.
        //
        const uint32_t bytesPerBlock = formatInfo.bitsPerBlock / 8u;
        if (mipmapLevel > 0 && bytesPerBlock > 4u) {
            // We can assume that moving the data to the previous valid position
            // is ok as it will only overwrite the now unnecessary "lodsize" value.
            const uint32_t validOffset =
                static_cast<uint32_t>(currentImageElementOffset / bytesPerBlock * bytesPerBlock);
            uint8_t* imageBytes = const_cast<uint8_t*>(image->imageBytes_);
            if (memmove_s(imageBytes + validOffset, image->imageBytesLength_ - validOffset,
                    imageBytes + currentImageElementOffset, subelementLength) != EOK) {
                CORE_LOG_E("memmove failed.");
            }
            image->imageBuffers_[imageBufferIndex].bufferOffset = validOffset;
        }
    }

    static bool ResolveGpuImageDesc(ImageDesc& desc, const KtxHeader& ktx, const CORE_NS::GlImageFormatInfo& formatInfo,
        const uint32_t loadFlags, const uint32_t inputMipCount, const uint32_t arrayElementCount,
        const uint32_t faceCount)
    {
        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_FORCE_SRGB_BIT) != 0) {
            desc.format = formatInfo.coreFormatForceSrgb;
        } else if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT) != 0) {
            desc.format = formatInfo.coreFormatForceLinear;
        } else {
            desc.format = formatInfo.coreFormat;
        }

        if ((desc.imageFlags & ImageFlags::FLAGS_CUBEMAP_BIT) != 0) {
        }

        desc.imageType = GetImageType(ktx);
        desc.imageViewType = GetImageViewType(ktx, desc.imageType);
        if (desc.format == Format::BASE_FORMAT_UNDEFINED || desc.imageType == ImageType::TYPE_MAX_ENUM ||
            desc.imageViewType == ImageViewType::VIEW_TYPE_MAX_ENUM) {
            CORE_LOG_D(
                "glFormat=%u imageType=%u imageViewType=%u", ktx.glInternalFormat, desc.imageType, desc.imageViewType);
            return false;
        }

        desc.mipCount = inputMipCount;

        // NOTE: depth here means 3D textures, not color channels.
        // In 1D and 2D textures the height and depth might be 0.
        desc.width = ktx.pixelWidth;
        desc.height = ((ktx.pixelHeight == 0) ? 1 : ktx.pixelHeight);
        desc.depth = ((ktx.pixelDepth == 0) ? 1 : ktx.pixelDepth);
        desc.layerCount = arrayElementCount * faceCount;

        const bool compressed = (desc.imageFlags & ImageFlags::FLAGS_COMPRESSED_BIT) != 0;
        const bool imageRequestingMips = (desc.imageFlags & ImageFlags::FLAGS_REQUESTING_MIPMAPS_BIT) != 0;
        const bool loaderRequestingMips = (loadFlags & IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS) != 0;
        if (!compressed && (imageRequestingMips || loaderRequestingMips)) {
            desc.imageFlags |= ImageFlags::FLAGS_REQUESTING_MIPMAPS_BIT;
            uint32_t mipsize = (desc.width > desc.height) ? desc.width : desc.height;
            desc.mipCount = 0;
            while (mipsize > 0) {
                desc.mipCount++;
                mipsize >>= 1;
            }
        } else {
            desc.imageFlags &= ~ImageFlags::FLAGS_REQUESTING_MIPMAPS_BIT;
        }

        return true;
    }

    static bool ResolveImageDesc(const KtxHeader& ktx, const GlImageFormatInfo& formatInfo, uint32_t loadFlags,
        uint32_t inputMipCount, const uint32_t arrayElementCount, uint32_t faceCount, ImageDesc& outImageDesc)
    {
        ImageDesc desc;

        desc.blockPixelWidth = formatInfo.blockWidth;
        desc.blockPixelHeight = formatInfo.blockHeight;
        desc.blockPixelDepth = formatInfo.blockDepth;
        desc.bitsPerBlock = formatInfo.bitsPerBlock;

        // If there are six faces this is a cube.
        if (faceCount == 6u) {
            desc.imageFlags |= ImageFlags::FLAGS_CUBEMAP_BIT;
        }

        // Is compressed?
        if ((ktx.glType == 0) && (ktx.glFormat == 0)) {
            desc.imageFlags |= ImageFlags::FLAGS_COMPRESSED_BIT;
        } else {
            // Mipmap generation works only if image is not using a compressed format.
            // In ktx mip count of 0 (instead of 1) means requesting generating full chain of mipmaps.
            if ((ktx.numberOfMipmapLevels == 0)) {
                desc.imageFlags |= ImageFlags::FLAGS_REQUESTING_MIPMAPS_BIT;
            }
        }

        if (!ResolveGpuImageDesc(desc, ktx, formatInfo, loadFlags, inputMipCount, arrayElementCount, faceCount)) {
            return false;
        }

        outImageDesc = desc;
        return true;
    }

    static bool VerifyKtxInfo(const KtxHeader& ktx, const GlImageFormatInfo& formatInfo)
    {
        if (formatInfo.compressed) {
            if (ktx.glTypeSize != 1) {
                CORE_LOG_D("Invalid typesize for a compressed image.");
                return false;
            }
            if (ktx.glFormat != 0) {
                CORE_LOG_D("Invalid glFormat for a compressed image.");
                return false;
            }
            if (ktx.glType != 0) {
                CORE_LOG_D("Invalid glType for a compressed image.");
                return false;
            }
        }

        if (ktx.pixelDepth != 0 && ktx.pixelHeight == 0) {
            CORE_LOG_D("No pixelHeight defined for a 3d texture.");
            return false;
        }

        return true;
    }

    static ImageLoaderManager::LoadResult CreateImage(
        KtxImage::Ptr image, const KtxHeader& ktx, uint32_t loadFlags, const uint8_t* data, bool isEndianFlipped)
    {
        // Mark this as the image data starting position.
        const uint8_t* ktxDataSection = data;

        const uint32_t inputMipCount = ktx.numberOfMipmapLevels == 0 ? 1 : ktx.numberOfMipmapLevels;
        const uint32_t arrayElementCount = ktx.numberOfArrayElements == 0 ? 1 : ktx.numberOfArrayElements;

        //
        // Populate the image descriptor.
        //
        const GlImageFormatInfo formatInfo = GetFormatInfo(ktx.glInternalFormat);
        if (!ResolveImageDesc(
                ktx, formatInfo, loadFlags, inputMipCount, arrayElementCount, ktx.numberOfFaces, image->imageDesc_)) {
            return ImageLoaderManager::ResultFailure("Image not supported.");
        }

        if (!VerifyKtxInfo(ktx, formatInfo)) {
            return ImageLoaderManager::ResultFailure("Invalid ktx data.");
        }

        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) == 0) {
            uint32_t elementWidth = image->imageDesc_.width;
            uint32_t elementHeight = image->imageDesc_.height;
            uint32_t elementDepth = image->imageDesc_.depth;

            // Create buffer info for each mipmap level.
            // NOTE: One BufferImageCopy can copy all the layers and faces in one step.
            image->imageBuffers_.resize(static_cast<size_t>(inputMipCount));

            const auto myReadU32 = isEndianFlipped ? ReadU32FlipEndian : ReadU32;

            // for non-array cubemaps imageSize is the size of one face, but for other types the total size.
            const auto iterations = (ktx.numberOfArrayElements == 0 && ktx.numberOfFaces == 6u) ? 6u : 1u;

            // Fill the image subelement buffer info.
            size_t imageBufferIndex = 0;
            for (uint32_t mipmapLevel = 0; mipmapLevel < inputMipCount; ++mipmapLevel) {
                if (data < ktxDataSection) {
                    CORE_LOG_D("Trying to jump out of the parsed data.");
                    return ImageLoaderManager::ResultFailure("Invalid ktx data.");
                }
                if (sizeof(uint32_t) >=
                    image->fileBytesLength_ - static_cast<uintptr_t>(data - image->fileBytes_.get())) {
                    CORE_LOG_D("Not enough data in the bytearray.");
                    return ImageLoaderManager::ResultFailure("Invalid ktx data.");
                }

                const size_t lodSize = myReadU32(&data);
                // Pad to to a multiple of 4.
                const size_t lodSizePadded = lodSize + ((~lodSize + 1) & (4u - 1u));

                const uint64_t totalSizePadded = static_cast<uint64_t>(lodSizePadded) * iterations;
                if (totalSizePadded >= UINT32_MAX) {
                    CORE_LOG_D("imageSize too big");
                    return ImageLoaderManager::ResultFailure("Invalid ktx data.");
                }

                const auto fileBytesLeft =
                    image->fileBytesLength_ - static_cast<uintptr_t>(data - image->fileBytes_.get());
                if (totalSizePadded > fileBytesLeft) {
                    CORE_LOG_D("Not enough data for the element");
                    CORE_LOG_V(
                        "  mips=%u faces=%u arrayElements=%u", inputMipCount, ktx.numberOfFaces, arrayElementCount);
                    CORE_LOG_V("  mipmapLevel=%u lodsize=%zu end=%zu filesize=%zu.", mipmapLevel, lodSize,
                        static_cast<size_t>(data - image->fileBytes_.get() + static_cast<ptrdiff_t>(lodSize)),
                        image->fileBytesLength_);
                    return ImageLoaderManager::ResultFailure("Invalid ktx data.");
                }

                const uint32_t currentImageElementOffset = static_cast<uint32_t>(data - image->imageBytes_);
                CORE_ASSERT_MSG(currentImageElementOffset % 4u == 0, "Offset must be aligned to 4 bytes");
                ProcessMipmapLevel(image, imageBufferIndex, currentImageElementOffset, formatInfo, elementWidth,
                    elementHeight, mipmapLevel, ktx.numberOfFaces, arrayElementCount, elementDepth,
                    static_cast<uint32_t>(totalSizePadded));

                // Move to the next buffer if any.
                imageBufferIndex++;

                // Figure out the next mipmap level sizes. The dimentions of each level are half of the previous.
                elementWidth /= 2u;
                elementWidth = (elementWidth <= 1) ? 1 : elementWidth;

                elementHeight /= 2u;
                elementHeight = (elementHeight <= 1) ? 1 : elementHeight;

                elementDepth /= 2u;
                elementDepth = (elementDepth <= 1) ? 1 : elementDepth;

                // Skip to the next lod level.
                // NOTE: in theory there could be packing here for each face. in that case we would need to
                // make a separate subelement of each face.
                data += totalSizePadded;
            }

            if (data != (image->fileBytes_.get() + image->fileBytesLength_)) {
                CORE_LOG_D("File data left over.");
                return ImageLoaderManager::ResultFailure("Invalid ktx data.");
            }
        }
        return ImageLoaderManager::ResultSuccess(CORE_NS::move(image));
    }

    // Actual ktx loading implementation.
    static ImageLoaderManager::LoadResult Load(
        unique_ptr<uint8_t[]> fileBytes, uint64_t fileBytesLength, uint32_t loadFlags)
    {
        if (!fileBytes) {
            return ImageLoaderManager::ResultFailure("Input data must not be null.");
        }
        if (fileBytesLength < KTX_HEADER_LENGTH) {
            return ImageLoaderManager::ResultFailure("Not enough data for parsing ktx.");
        }

        // Populate the image object.
        auto image = KtxImage::Ptr(new KtxImage(move(fileBytes), static_cast<size_t>(fileBytesLength)));
        if (!image) {
            return ImageLoaderManager::ResultFailure("Loading image failed.");
        }

        const uint8_t* data = image->fileBytes_.get();
        const auto ktxHeader = ReadHeader(&data);
        // Check that the header values make sense.
        if (memcmp(ktxHeader.identifier, KTX_IDENTIFIER_REFERENCE, KTX_IDENTIFIER_LENGTH) != 0) {
            CORE_LOG_D("Ktx invalid file identifier.");
            return ImageLoaderManager::ResultFailure("Invalid ktx data.");
        }
        if (ktxHeader.endianness != KTX_FILE_ENDIANNESS && ktxHeader.endianness != KTX_FILE_ENDIANNESS_FLIPPED) {
            CORE_LOG_D("Ktx invalid endian marker.");
            return ImageLoaderManager::ResultFailure("Invalid ktx data.");
        }
        if (ktxHeader.numberOfFaces != 1 && ktxHeader.numberOfFaces != 6u) { // 1 for regular, 6 for cubemaps
            CORE_LOG_D("Ktx invalid numberOfFaces.");
            return ImageLoaderManager::ResultFailure("Invalid ktx data.");
        }
        if (ktxHeader.pixelWidth == 0) {
            CORE_LOG_D("Ktx pixelWidth can't be 0.");
            return ImageLoaderManager::ResultFailure("Invalid ktx data.");
        }

        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) == 0) {
            if (ktxHeader.bytesOfKeyValueData >
                image->fileBytesLength_ - static_cast<uintptr_t>(data - image->fileBytes_.get())) {
                CORE_LOG_D("Ktx bytesOfKeyValueData too large.");
                return ImageLoaderManager::ResultFailure("Invalid ktx data.");
            }

            ReadKeyValueData(ktxHeader, &data);
            // NOTE: Point to the start of the actual data of the first texture
            // (Jump over the first lod offset uint32_t (4 bytes)
            const size_t headerLength = static_cast<size_t>(data - image->fileBytes_.get()) + sizeof(uint32_t);
            image->imageBytes_ = data + sizeof(uint32_t);
            image->imageBytesLength_ = image->fileBytesLength_ - headerLength;
        }
        const bool isEndianFlipped = (ktxHeader.endianness == KTX_FILE_ENDIANNESS_FLIPPED);
        if (isEndianFlipped && ktxHeader.glTypeSize > 1) {
            CORE_ASSERT_MSG(true, "NOTE: must convert all data to correct endianness");
            return ImageLoaderManager::ResultFailure("Image not supported.");
        }

        return CreateImage(move(image), ktxHeader, loadFlags, data, isEndianFlipped);
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    static KtxHeader ReadHeader(const uint8_t** data)
    {
        // Read the identifier.
        KtxHeader ktxHeader = {};

        CloneData(ktxHeader.identifier, sizeof(ktxHeader.identifier), *data, KTX_IDENTIFIER_LENGTH);
        *data += KTX_IDENTIFIER_LENGTH;

        // Check file endianness.
        ktxHeader.endianness = ReadU32(data);

        const bool isEndianFlipped = (ktxHeader.endianness == KTX_FILE_ENDIANNESS_FLIPPED);

        const auto myReadU32 = isEndianFlipped ? ReadU32FlipEndian : ReadU32;

        ktxHeader.glType = myReadU32(data);
        ktxHeader.glTypeSize = myReadU32(data);
        ktxHeader.glFormat = myReadU32(data);
        ktxHeader.glInternalFormat = myReadU32(data);
        ktxHeader.glBaseInternalFormat = myReadU32(data);
        ktxHeader.pixelWidth = myReadU32(data);
        ktxHeader.pixelHeight = myReadU32(data);
        ktxHeader.pixelDepth = myReadU32(data);
        ktxHeader.numberOfArrayElements = myReadU32(data);
        ktxHeader.numberOfFaces = myReadU32(data);
        ktxHeader.numberOfMipmapLevels = myReadU32(data);
        ktxHeader.bytesOfKeyValueData = myReadU32(data);

        return ktxHeader;
    }

    static void ReadKeyValueData(const KtxHeader& ktxHeader, const uint8_t** data)
    {
#ifndef CORE_READ_KTX_HEADER_STRING
        // Skip reading the key-value data for now.
        *data += ktxHeader.bytesOfKeyValueData;
#else
        const bool isEndianFlipped = (ktxHeader.endianness == KTX_FILE_ENDIANNESS_FLIPPED);
        const auto myReadU32 = isEndianFlipped ? ReadU32FlipEndian : ReadU32;

        // Read KTX key-value data.
        size_t keyValueDataRead = 0;
        while (keyValueDataRead < ktxHeader.bytesOfKeyValueData) {
            uint32_t keyAndValueByteSize = myReadU32(data);
            keyValueDataRead += sizeof(uint32_t);

            size_t keyBytesRead;
            const auto key = ReadStringZ(data, keyAndValueByteSize, &keyBytesRead);
            keyValueDataRead += keyBytesRead;

            size_t valueBytesRead;
            const auto value = ReadStringZ(data, keyAndValueByteSize - keyBytesRead, &valueBytesRead);
            keyValueDataRead += valueBytesRead;

            if (!key.empty() && !value.empty()) {
                // NOTE: The key-value data is not used for anything. Just printing to log.
                CORE_LOG_V("KTX metadata: '%s' : '%s'", key.data(), value.data());
            }

            // Pad to a multiple of 4 bytes.
            const size_t padding = (~keyAndValueByteSize + 1) & (4u - 1u);
            keyValueDataRead += padding;
            *data += padding;
        }
#endif
    }

    // Saving the whole image file data in one big chunk. This way we don't
    // need to copy the data to a separate buffer after reading the file. We
    // will be pointing to the file data anyway. Only downside is the wasted
    // memory for the file header.
    unique_ptr<uint8_t[]> fileBytes_;
    size_t fileBytesLength_ { 0 };

    // The actual image data part of the file;
    const uint8_t* imageBytes_ { nullptr };
    size_t imageBytesLength_ { 0 };

    ImageDesc imageDesc_;
    vector<SubImageDesc> imageBuffers_;
};

class ImageLoaderKtx final : public IImageLoaderManager::IImageLoader {
public:
    // Inherited via ImageManager::IImageLoader
    ImageLoaderManager::LoadResult Load(IFile& file, uint32_t loadFlags) const override
    {
        size_t byteLength = static_cast<size_t>(file.GetLength());

        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) != 0) {
            // Only load header
            byteLength = KTX_HEADER_LENGTH;
        }

        // Read the file to a buffer.
        unique_ptr<uint8_t[]> buffer = make_unique<uint8_t[]>(byteLength);
        const uint64_t read = file.Read(buffer.get(), byteLength);
        if (read != byteLength) {
            CORE_LOG_D("Error loading image");
            return ImageLoaderManager::ResultFailure("Reading file failed.");
        }

        return KtxImage::Load(move(buffer), byteLength, loadFlags);
    }

    ImageLoaderManager::LoadResult Load(array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) const override
    {
        // NOTE: could reuse this and remove the extra copy here if the data would be given as a unique_ptr.
        unique_ptr<uint8_t[]> buffer = make_unique<uint8_t[]>(static_cast<size_t>(imageFileBytes.size()));
        if (buffer) {
            std::copy(imageFileBytes.begin(), imageFileBytes.end(), buffer.get());
        }

        return KtxImage::Load(move(buffer), imageFileBytes.size(), loadFlags);
    }

    bool CanLoad(array_view<const uint8_t> imageFileBytes) const override
    {
        // Check for KTX
        return (imageFileBytes.size() >= KTX_IDENTIFIER_LENGTH) &&
               (memcmp(imageFileBytes.data(), KTX_IDENTIFIER_REFERENCE, KTX_IDENTIFIER_LENGTH) == 0);
    }

    // No animated KTX
    ImageLoaderManager::LoadAnimatedResult LoadAnimatedImage(IFile& /* file */, uint32_t /* loadFlags */) override
    {
        return ImageLoaderManager::ResultFailureAnimated("Animated KTX not supported.");
    }

    ImageLoaderManager::LoadAnimatedResult LoadAnimatedImage(
        array_view<const uint8_t> /* imageFileBytes */, uint32_t /* loadFlags */) override
    {
        return ImageLoaderManager::ResultFailureAnimated("Animated KTX not supported.");
    }

    vector<IImageLoaderManager::ImageType> GetSupportedTypes() const override
    {
        return vector<IImageLoaderManager::ImageType>(std::begin(KTX_IMAGE_TYPES), std::end(KTX_IMAGE_TYPES));
    }

protected:
    void Destroy() final
    {
        delete this;
    }
};
} // namespace

IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderKtx(PluginToken)
{
    return ImageLoaderManager::IImageLoader::Ptr { new ImageLoaderKtx() };
}
CORE_END_NAMESPACE()
