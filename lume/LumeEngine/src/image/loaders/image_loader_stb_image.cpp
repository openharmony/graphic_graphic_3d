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

#include "image/loaders/image_loader_stb_image.h"
#if defined(USE_STB_IMAGE) && (USE_STB_IMAGE == 1)
#include <cstddef>
#include <cstdint>
#include <limits>

//
// Enabling only formats that are actually used.
// Others available:
//        STBI_ONLY_BMP
//        STBI_ONLY_PSD
//        STBI_ONLY_TGA
//        STBI_ONLY_GIF
//        STBI_ONLY_HDR
//        STBI_ONLY_PIC
//        STBI_ONLY_PNM   (.ppm and .pgm)
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG

// Without this a few gif functions are still included.
#define STBI_NO_GIF

// Currently we always load from memory so disabling support for loading directly from a file.
#define STBI_NO_STDIO

// This makes the stb_image implementation local to this compilation unit
// (So it wont interfere if someone else is using stb_image too).
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION

// On x86, SSE2 will automatically be used when available based on a run-time
// test; if not, the generic C versions are used as a fall-back. On ARM targets,
// the typical path is to have separate builds for NEON and non-NEON devices
// Therefore, the NEON support is toggled by a build flag: define STBI_NEON to
// get NEON loops
//
// Assuming that NEON is supported by our target devices.
// (on NDK r21 and newer NEON is enabled by default anyway).
#if defined(__arm__) || defined(__aarch64__)
#define STBI_NEON
#endif

#include <stb/stb_image.h>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/math/mathf.h>
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
using BASE_NS::Format;
using BASE_NS::make_unique;
using BASE_NS::move;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::unique_ptr;
using BASE_NS::vector;
using BASE_NS::Math::pow;
using BASE_NS::Math::round;

// NOTE: Reading the stb error code is NOT THREADSAFE.
// Enable this if you really need to know the error message.
constexpr const bool CORE_ENABLE_STB_NON_THREADSAFE_ERROR_MSG = false;

void FreeStbImageBytes(void* imageBytes)
{
    stbi_image_free(imageBytes);
}

uint8_t g_sRgbPremultiplyLookup[256u * 256u] = { 0 };

void InitializeSRGBTable()
{
    // Generate lookup table to premultiply sRGB encoded image in linear space and reencoding it to sRGB
    // Formulas from https://en.wikipedia.org/wiki/SRGB
    for (uint32_t a = 0; a < 256u; a++) {
        const float alpha = static_cast<float>(a) / 255.f;
        for (uint32_t sRGB = 0; sRGB < 256u; sRGB++) {
            float color = static_cast<float>(sRGB) / 255.f;
            if (color <= 0.04045f) {
                color *= (1.f / 12.92f);
            } else {
                color = pow((color + 0.055f) * (1.f / 1.055f), 2.4f);
            }
            float premultiplied = color * alpha;
            if (premultiplied <= 0.0031308f) {
                premultiplied *= 12.92f;
            } else {
                premultiplied = 1.055f * pow(premultiplied, 1.f / 2.4f) - 0.055f;
            }
            g_sRgbPremultiplyLookup[a * 256u + sRGB] = static_cast<uint8_t>(round(premultiplied * 255.f));
        }
    }
}

bool PremultiplyAlpha(
    uint8_t* imageBytes, uint32_t width, uint32_t height, uint32_t channelCount, uint32_t bytesPerChannel, bool linear)
{
    // Only need to process images with color and alpha data. I.e. RGBA or grayscale + alpha.
    if (channelCount != 4u && channelCount != 2u) {
        return true;
    }

    const uint32_t pixelCount = width * height;

    if (bytesPerChannel == 1) {
        if (linear) {
            uint8_t* img = imageBytes;
            for (uint32_t i = 0; i < pixelCount; i++) {
                // We know the alpha value is always last.
                uint32_t alpha = img[channelCount - 1];
                for (uint32_t j = 0; j < channelCount - 1; j++) {
                    *img = static_cast<uint8_t>(*img * alpha / 0xff);
                    img++;
                }
                img++; // Skip over the alpha value.
            }
        } else {
            if (g_sRgbPremultiplyLookup[256u * 256u - 1] == 0) {
                InitializeSRGBTable();
            }
            uint8_t* img = imageBytes;
            for (uint32_t i = 0; i < pixelCount; i++) {
                uint8_t* p = &g_sRgbPremultiplyLookup[img[channelCount - 1] * 256u];
                for (uint32_t j = 0; j < channelCount - 1; j++) {
                    *img = p[*img];
                    img++;
                }
                img++;
            }
        }
    } else if (bytesPerChannel == 2u) {
        // Same for 16 bits per channel images.
        uint16_t* img = reinterpret_cast<uint16_t*>(imageBytes);
        for (uint32_t i = 0; i < pixelCount; i++) {
            uint32_t alpha = img[channelCount - 1];
            for (uint32_t j = 0; j < channelCount - 1; j++) {
                *img = static_cast<uint16_t>(*img * alpha / 0xffff);
                img++;
            }
            img++;
        }
    } else {
        CORE_LOG_E("Format not supported.");
        return false;
    }
    return true;
}

using StbImagePtr = unique_ptr<void, decltype(&FreeStbImageBytes)>;
} // namespace

class StbImage final : public IImageContainer {
public:
    StbImage() : IImageContainer(), imageDesc_(), imageBuffer_() {}

    using Ptr = BASE_NS::unique_ptr<StbImage, Deleter>;

    const ImageDesc& GetImageDesc() const override
    {
        return imageDesc_;
    }

    array_view<const uint8_t> GetData() const override
    {
        return array_view<const uint8_t>(static_cast<const uint8_t*>(imageBytes_.get()), imageBytesLength_);
    }

    array_view<const SubImageDesc> GetBufferImageCopies() const override
    {
        return array_view<const SubImageDesc>(&imageBuffer_, 1);
    }

    static constexpr Format ResolveFormat(uint32_t loadFlags, uint32_t componentCount, bool is16bpc)
    {
        Format format {};
        const bool forceLinear = (loadFlags & IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT) != 0;

        switch (componentCount) {
            case 1u:
                format = is16bpc ? Format::BASE_FORMAT_R16_UNORM
                                 : (forceLinear ? Format::BASE_FORMAT_R8_UNORM : Format::BASE_FORMAT_R8_SRGB);
                break;
            case 2u:
                format = is16bpc ? Format::BASE_FORMAT_R16G16_UNORM
                                 : (forceLinear ? Format::BASE_FORMAT_R8G8_UNORM : Format::BASE_FORMAT_R8G8_SRGB);
                break;
            case 3u:
                format = is16bpc ? Format::BASE_FORMAT_R16G16B16_UNORM
                                 : (forceLinear ? Format::BASE_FORMAT_R8G8B8_UNORM : Format::BASE_FORMAT_R8G8B8_SRGB);
                break;
            case 4u:
                format = is16bpc
                             ? Format::BASE_FORMAT_R16G16B16A16_UNORM
                             : (forceLinear ? Format::BASE_FORMAT_R8G8B8A8_UNORM : Format::BASE_FORMAT_R8G8B8A8_SRGB);
                break;
            default:
                format = Format::BASE_FORMAT_UNDEFINED;
                break;
        }

        return format;
    }

    constexpr static ImageDesc ResolveImageDesc(Format format, uint32_t imageWidth, uint32_t imageHeight,
        uint32_t bitsPerPixel, bool generateMips, bool isPremultiplied)
    {
        uint32_t mipCount = 1;

        // 1D images not supported with stb loader
        constexpr ImageType imageType = ImageType::TYPE_2D;
        constexpr ImageViewType imageViewType = ImageViewType::VIEW_TYPE_2D;

        uint32_t imageFlags = isPremultiplied ? ImageFlags::FLAGS_PREMULTIPLIED_ALPHA_BIT : 0;
        if (generateMips) {
            imageFlags |= ImageFlags::FLAGS_REQUESTING_MIPMAPS_BIT;
            uint32_t mipsize = (imageWidth > imageHeight) ? imageWidth : imageHeight;
            mipCount = 0;
            while (mipsize > 0) {
                mipCount++;
                mipsize >>= 1;
            }
        }

        return ImageDesc {
            imageFlags,   // imageFlags
            1,            // blockPixelWidth
            1,            // blockPixelHeight
            1,            // blockPixelDepth
            bitsPerPixel, // bitsPerBlock

            imageType,     // imageType
            imageViewType, // imageViewType
            format,        // format

            static_cast<uint32_t>(imageWidth),  // width
            static_cast<uint32_t>(imageHeight), // height
            1,                                  // depth

            mipCount, // mipCount
            1,        // layerCount
        };
    }

    static ImageLoaderManager::LoadResult CreateImage(StbImagePtr imageBytes, uint32_t imageWidth, uint32_t imageHeight,
        uint32_t componentCount, uint32_t loadFlags, bool is16bpc)
    {
        auto image = StbImage::Ptr(new StbImage);
        if (!image) {
            return ImageLoaderManager::ResultFailure("Loading image failed.");
        }

        // Premultiply alpha if requested.
        bool isPremultiplied = false;
        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) == 0) {
            if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA) != 0) {
                const uint32_t bytesPerChannel = is16bpc ? 2u : 1u;
                const bool forceLinear = (loadFlags & IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT) != 0;
                isPremultiplied = PremultiplyAlpha(static_cast<uint8_t*>(imageBytes.get()), imageWidth, imageHeight,
                    componentCount, bytesPerChannel, forceLinear);
            }
        }

        const Format format = ResolveFormat(loadFlags, componentCount, is16bpc);

        const bool generateMips = (loadFlags & IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS) != 0;
        const uint32_t bytesPerComponent = is16bpc ? 2u : 1u;
        const uint32_t bitsPerPixel = bytesPerComponent * componentCount * 8u;

        image->imageDesc_ =
            ResolveImageDesc(format, imageWidth, imageHeight, bitsPerPixel, generateMips, isPremultiplied);
        image->imageBytes_ = BASE_NS::move(imageBytes);
        image->imageBytesLength_ = imageWidth * imageHeight * componentCount * bytesPerComponent;

        image->imageBuffer_ = SubImageDesc {
            0,           // uint32_t bufferOffset
            imageWidth,  // uint32_t bufferRowLength
            imageHeight, // uint32_t bufferImageHeight

            0, // uint32_t mipLevel
            1, // uint32_t layerCount

            static_cast<uint32_t>(imageWidth),
            static_cast<uint32_t>(imageHeight),
            1,
        };

        return ImageLoaderManager::ResultSuccess(CORE_NS::move(image));
    }

    struct Info {
        int width;
        int height;
        int componentCount;
        bool is16bpc;
    };

    static StbImagePtr LoadFromMemory(array_view<const uint8_t> imageFileBytes, uint32_t loadFlags, Info& info)
    {
        StbImagePtr imageBytes = nullptr;
        // Convert 3 channels to 4 because 3 channel textures are not always supported.
        // Also convert 2 channel (grayscale + alpha) to 4 because r + a in not supported.
        int requestedComponentCount = (info.componentCount == 3 || info.componentCount == 2) ? 4 : 0;

        // Force grayscale if requested.
        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT) != 0) {
            requestedComponentCount = 1;
        }

        // Load the image byte data.
        if (info.is16bpc) {
            auto image = stbi_load_16_from_memory(imageFileBytes.data(), static_cast<int>(imageFileBytes.size()),
                &info.width, &info.height, &info.componentCount, requestedComponentCount);
            imageBytes = { image, FreeStbImageBytes };
        } else {
            auto image = stbi_load_from_memory(imageFileBytes.data(), static_cast<int>(imageFileBytes.size()),
                &info.width, &info.height, &info.componentCount, requestedComponentCount);
            imageBytes = { image, FreeStbImageBytes };
        }

        if (requestedComponentCount != 0) {
            info.componentCount = requestedComponentCount;
        }
        return imageBytes;
    }

    // Actual stb_image loading implementation.
    static ImageLoaderManager::LoadResult Load(array_view<const uint8_t> imageFileBytes, uint32_t loadFlags)
    {
        if (imageFileBytes.empty()) {
            return ImageLoaderManager::ResultFailure("Input data must not be null.");
        }

        // Load the image info without decoding the image data
        // (Just to check what the image format is so we can convert if necessary).
        Info info {};

        const int result = stbi_info_from_memory(imageFileBytes.data(), static_cast<int>(imageFileBytes.size()),
            &info.width, &info.height, &info.componentCount);

        info.is16bpc =
            (stbi_is_16_bit_from_memory(imageFileBytes.data(), static_cast<int>(imageFileBytes.size())) != 0);

        // Not supporting hdr images via stb_image.
#if !defined(NDEBUG)
        if (stbi_is_hdr_from_memory(imageFileBytes.data(), static_cast<int>(imageFileBytes.size()))) {
            CORE_LOG_D("HDR format detected.");
        }
#endif
        StbImagePtr imageBytes = nullptr;
        if (result) {
            if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) == 0) {
                imageBytes = LoadFromMemory(imageFileBytes, loadFlags, info);
                // Flip vertically if requested.
                if (imageBytes && (loadFlags & IImageLoaderManager::IMAGE_LOADER_FLIP_VERTICALLY_BIT) != 0) {
                    stbi__vertical_flip(imageBytes.get(), info.width, info.height, info.componentCount);
                }
            } else {
                imageBytes = { nullptr, FreeStbImageBytes };
            }
        }

        if (!result || (((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) == 0) && !imageBytes)) {
            if (CORE_ENABLE_STB_NON_THREADSAFE_ERROR_MSG) {
                const string errorString = string_view("Loading image failed: ") + string_view(stbi_failure_reason());
                return ImageLoaderManager::ResultFailure(errorString);
            }
            return ImageLoaderManager::ResultFailure("Loading image failed");
        }

        // Success. Populate the image info and image data object.
        return CreateImage(CORE_NS::move(imageBytes), static_cast<uint32_t>(info.width),
            static_cast<uint32_t>(info.height), static_cast<uint32_t>(info.componentCount), loadFlags, info.is16bpc);
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    ImageDesc imageDesc_;
    SubImageDesc imageBuffer_;

    StbImagePtr imageBytes_;
    size_t imageBytesLength_ = 0;
};

class ImageLoaderStbImage final : public ImageLoaderManager::IImageLoader {
public:
    // Inherited via ImageManager::IImageLoader
    ImageLoaderManager::LoadResult Load(IFile& file, uint32_t loadFlags) const override
    {
        const uint64_t byteLength = file.GetLength();
        // stb_image uses int for file sizes. Don't even try to read a file if the size does not fit to int.
        if (byteLength > static_cast<uint64_t>(std::numeric_limits<int>::max())) {
            return ImageLoaderManager::ResultFailure("File too big to read.");
        }

        // Read the file to a buffer.
        unique_ptr<uint8_t[]> buffer = make_unique<uint8_t[]>(static_cast<size_t>(byteLength));
        const uint64_t read = file.Read(buffer.get(), byteLength);
        if (read != byteLength) {
            return ImageLoaderManager::ResultFailure("Reading file failed.");
        }

        return StbImage::Load(array_view<const uint8_t>(buffer.get(), static_cast<size_t>(byteLength)), loadFlags);
    }

    ImageLoaderManager::LoadResult Load(array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) const override
    {
        // stb_image uses int for file sizes. Don't even try to read a file if the size does not fit to int.
        // Not writing a test for this :)
        if (imageFileBytes.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
            return ImageLoaderManager::ResultFailure("Data too big to read.");
        }

        return StbImage::Load(imageFileBytes, loadFlags);
    }

    bool CanLoad(array_view<const uint8_t> imageFileBytes) const override
    {
        // stb_image uses int for file sizes. Don't even try to read a file if the size does not fit to int.
        // Not writing a test for this :)
        if (imageFileBytes.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
            return false;
        }

        // Check for PNG
        if ((imageFileBytes.size() >= 8) && imageFileBytes[0] == 137 && imageFileBytes[1] == 80 &&
            imageFileBytes[2] == 78 && imageFileBytes[3] == 71 && imageFileBytes[4] == 13 && imageFileBytes[5] == 10 &&
            imageFileBytes[6] == 26 && imageFileBytes[7] == 10) { // 6:index 26: pixle data 7:index 10: pixle
            return true;
        }

        // Check for JPEG / JFIF / Exif / ICC_PROFILE tag
        if ((imageFileBytes.size() >= 10) && imageFileBytes[0] == 0xff && imageFileBytes[1] == 0xd8 &&
            imageFileBytes[2] == 0xff && // 2:index
            ((imageFileBytes[3] == 0xe0 && imageFileBytes[6] == 'J' && imageFileBytes[7] == 'F' &&
                 imageFileBytes[8] == 'I' && imageFileBytes[9] == 'F') || // JFIF
                (imageFileBytes[3] == 0xe1 && imageFileBytes[6] == 'E' && imageFileBytes[7] == 'x' &&
                    imageFileBytes[8] == 'i' && imageFileBytes[9] == 'f') || // Exif
                (imageFileBytes[3] == 0xe2 && imageFileBytes[6] == 'I' && imageFileBytes[7] == 'C' &&
                    imageFileBytes[8] == 'C' && imageFileBytes[9] == '_'))) { // ICC_PROFILE
            return true;
        }

        return false;
    }

    // No animation support
    ImageLoaderManager::LoadAnimatedResult LoadAnimatedImage(IFile& /* file */, uint32_t /* loadFlags */) override
    {
        return ImageLoaderManager::ResultFailureAnimated("Animation not supported.");
    }

    ImageLoaderManager::LoadAnimatedResult LoadAnimatedImage(
        array_view<const uint8_t> /* imageFileBytes */, uint32_t /* loadFlags */) override
    {
        return ImageLoaderManager::ResultFailureAnimated("Animation not supported.");
    }

    vector<IImageLoaderManager::ImageType> GetSupportedTypes() const override
    {
        return vector<IImageLoaderManager::ImageType>(std::begin(STB_IMAGE_TYPES), std::end(STB_IMAGE_TYPES));
    }

protected:
    void Destroy() override
    {
        delete this;
    }
};

IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderStbImage(PluginToken)
{
    return ImageLoaderManager::IImageLoader::Ptr { new ImageLoaderStbImage() };
}
CORE_END_NAMESPACE()
#endif // defined(USE_STB_IMAGE) && (USE_STB_IMAGE == 1)