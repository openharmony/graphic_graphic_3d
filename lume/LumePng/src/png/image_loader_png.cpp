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

#include "image_loader_png.h"

#include <functional>
#include <limits>
#include <memory>

#include <base/math/mathf.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <core/namespace.h>

#include "png.h"

using namespace BASE_NS;
using namespace CORE_NS;

namespace PNGPlugin {
namespace {
constexpr uint32_t MAX_IMAGE_EXTENT { 32767U };
constexpr int IMG_SIZE_LIMIT_2GB = std::numeric_limits<int>::max();
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

IImageLoaderManager::LoadResult ResultFailure(const string_view error)
{
    IImageLoaderManager::LoadResult result {
        false,  // success
        "",     // error[128];
        nullptr // image;
    };

    // Copy the error string
    const auto count = Math::min(error.size(), sizeof(result.error) - 1);
    error.copy(result.error, count);
    result.error[count] = '\0';

    return result;
}

IImageLoaderManager::LoadResult ResultSuccess(IImageContainer::Ptr image)
{
    return IImageLoaderManager::LoadResult {
        true,                // success
        "",                  // error[128];
        BASE_NS::move(image) // image;
    };
}

IImageLoaderManager::LoadAnimatedResult ResultFailureAnimated(const string_view error)
{
    IImageLoaderManager::LoadAnimatedResult result {
        false,  // success
        "",     // error[128];
        nullptr // image;
    };

    // Copy the error string
    const auto count = Math::min(error.size(), sizeof(result.error) - 1);
    error.copy(result.error, count);
    result.error[count] = '\0';

    return result;
}
} // namespace

class PNGImage final : public IImageContainer {
public:
    PNGImage() : IImageContainer(), imageDesc_(), imageBuffer_() {}

    ~PNGImage() override = default;

    using Ptr = BASE_NS::unique_ptr<PNGImage, Deleter>;

    const ImageDesc& GetImageDesc() const override
    {
        return imageDesc_;
    }

    array_view<const uint8_t> GetData() const override
    {
        return array_view<const uint8_t>(imageBytes_.get(), imageBytesLength_);
    }

    array_view<const SubImageDesc> GetBufferImageCopies() const override
    {
        return array_view<const SubImageDesc>(imageBuffer_);
    }

    static constexpr Format ResolveFormat(uint32_t loadFlags, uint32_t componentCount, bool is16bpc) noexcept
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
        uint32_t bitsPerPixel, bool generateMips, bool isPremultiplied) noexcept
    {
        uint32_t mipCount = 1;

        // 1D images not supported with WebP loader
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
            imageFlags,    // imageFlags
            1,             // blockPixelWidth
            1,             // blockPixelHeight
            1,             // blockPixelDepth
            bitsPerPixel,  // bitsPerBlock
            imageType,     // imageType
            imageViewType, // imageViewType
            format,        // format
            imageWidth,    // width
            imageHeight,   // height
            1,             // depth
            mipCount,      // mipCount
            1,             // layerCount
        };
    }

    static void Handle3DTexture(PNGImage* image, BASE_NS::unique_ptr<uint8_t[]>& imageBytes, uint32_t sliceWidth,
        uint32_t sliceHeight, uint32_t depth, uint32_t bitsPerPixel)
    {
        const uint32_t oldImageWidth = image->imageDesc_.width;
        const uint32_t oldImageHeight = image->imageDesc_.height;
        const uint32_t tileCountX = oldImageWidth / sliceWidth;
        const uint8_t* orgimageBytes = reinterpret_cast<uint8_t*>(imageBytes.get());
        const size_t imageSizeInBytes = image->imageDesc_.width * image->imageDesc_.height * (bitsPerPixel / 8);
        uint8_t* sorted3dBytes = new uint8_t[imageSizeInBytes];
        const uint32_t sizeOfPixelInBytes = bitsPerPixel / 8;
        const uint32_t totalSizeOfOneTile = sliceHeight * sliceWidth * sizeOfPixelInBytes;
        for (uint32_t y = 0; y < oldImageHeight; ++y) {
            for (uint32_t x = 0; x < oldImageWidth; ++x) {
                const uint32_t currentTileX = x / sliceWidth;
                const uint32_t currentTileY = y / sliceHeight;
                const uint32_t currentSlice = currentTileY * tileCountX + currentTileX;
                uint32_t* dest = reinterpret_cast<uint32_t*>(
                    &sorted3dBytes[(currentSlice * totalSizeOfOneTile) +
                                   (((y % sliceHeight) * sliceWidth) + (x % sliceWidth)) * sizeOfPixelInBytes]);
                *dest =
                    *reinterpret_cast<const uint32_t*>(&orgimageBytes[((y * oldImageWidth) + x) * sizeOfPixelInBytes]);
            }
        }
        image->imageBytes_.reset(sorted3dBytes);
        image->imageDesc_.width = sliceWidth;
        image->imageDesc_.height = sliceHeight;
        image->imageDesc_.depth = depth;
        image->imageDesc_.imageViewType = IImageContainer::ImageViewType::VIEW_TYPE_3D;
        image->imageDesc_.imageType = IImageContainer::ImageType::TYPE_3D;
    }

    static IImageLoaderManager::LoadResult CreateImage(BASE_NS::unique_ptr<uint8_t[]> imageBytes, uint32_t imageWidth,
        uint32_t imageHeight, uint32_t componentCount, uint32_t loadFlags, bool is16bpc) noexcept
    {
        auto image = PNGImage::Ptr(new PNGImage);
        if (!image) {
            return ResultFailure("Loading image failed.");
        }
        const uint32_t bytesPerComponent = is16bpc ? 2u : 1u;

        // Premultiply alpha if requested.
        bool isPremultiplied = false;
        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) == 0) {
            if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA) != 0) {
                const bool forceLinear = (loadFlags & IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT) != 0;
                isPremultiplied = PremultiplyAlpha(static_cast<uint8_t*>(imageBytes.get()), imageWidth, imageHeight,
                    componentCount, bytesPerComponent, forceLinear);
            }
        }

        const Format format = ResolveFormat(loadFlags, componentCount, is16bpc);
        const uint32_t bitsPerPixel = bytesPerComponent * componentCount * 8u;
        const bool generateMips = (loadFlags & IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS) != 0;
        image->imageDesc_ =
            ResolveImageDesc(format, imageWidth, imageHeight, bitsPerPixel, generateMips, isPremultiplied);
        image->imageBytes_ = BASE_NS::move(imageBytes);
        image->imageBytesLength_ = imageWidth * imageHeight * componentCount * bytesPerComponent;

        image->imageBuffer_.resize(1);
        image->imageBuffer_.front() = SubImageDesc {
            0,           // uint32_t bufferOffset
            imageWidth,  // uint32_t bufferRowLength
            imageHeight, // uint32_t bufferImageHeight

            0, // uint32_t mipLevel
            1, // uint32_t layerCount

            static_cast<uint32_t>(imageWidth),
            static_cast<uint32_t>(imageHeight),
            1,
        };

        return ResultSuccess(BASE_NS::move(image));
    }

    static IImageLoaderManager::LoadResult CreateImage(BASE_NS::unique_ptr<uint8_t[]> imageBytes, uint32_t imageWidth,
        uint32_t imageHeight, uint32_t componentCount, uint32_t loadFlags, bool is16bpc, uint32_t sliceWidth,
        uint32_t sliceHeight, uint32_t depth) noexcept
    {
        auto image = PNGImage::Ptr(new PNGImage);
        if (!image) {
            return ResultFailure("Loading image failed.");
        }
        const uint32_t bytesPerComponent = is16bpc ? 2u : 1u;

        // Premultiply alpha if requested.
        bool isPremultiplied = false;
        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) == 0) {
            if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA) != 0) {
                const bool forceLinear = (loadFlags & IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT) != 0;
                isPremultiplied = PremultiplyAlpha(static_cast<uint8_t*>(imageBytes.get()), imageWidth, imageHeight,
                    componentCount, bytesPerComponent, forceLinear);
            }
        }

        const Format format = ResolveFormat(loadFlags, componentCount, is16bpc);
        const uint32_t bitsPerPixel = bytesPerComponent * componentCount * 8u;
        const bool generateMips = (loadFlags & IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS) != 0;
        image->imageDesc_ =
            ResolveImageDesc(format, imageWidth, imageHeight, bitsPerPixel, generateMips, isPremultiplied);

        // Handling 3d textures
        if (depth > 1) {
            Handle3DTexture(image.get(), imageBytes, sliceWidth, sliceHeight, depth, bitsPerPixel);
        } else {
            image->imageBytes_ = BASE_NS::move(imageBytes);
        }

        image->imageBytesLength_ = imageWidth * imageHeight * componentCount * bytesPerComponent;

        image->imageBuffer_.resize(depth);

        if (depth == 1) {
            sliceWidth = imageWidth;
            sliceHeight = imageHeight;
        }
        for (uint32_t i = 0; i < depth; i++) {
            image->imageBuffer_[i] = SubImageDesc {
                0,           // uint32_t bufferOffset
                sliceWidth,  // uint32_t bufferRowLength
                sliceHeight, // uint32_t bufferImageHeight

                0, // uint32_t mipLevel
                1, // uint32_t layerCount

                static_cast<uint32_t>(sliceWidth),
                static_cast<uint32_t>(sliceHeight),
                depth,
            };
        }

        return ResultSuccess(BASE_NS::move(image));
    }

    static void UpdateColorType(png_structp pngPtr, png_infop infoPtr, uint32_t loadFlags) noexcept
    {
        const auto colorType = png_get_color_type(pngPtr, infoPtr);
        const auto bitDepth = png_get_bit_depth(pngPtr, infoPtr);
        const bool forceGray = (loadFlags & IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT) ==
                               IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT;

        // convert gray scale to RGB if needed
        if ((colorType & (PNG_COLOR_MASK_PALETTE | PNG_COLOR_MASK_COLOR)) == 0) {
            if (bitDepth < 8) { // 8: depth
                png_set_expand_gray_1_2_4_to_8(pngPtr);
            } else if (!forceGray) {
                png_set_gray_to_rgb(pngPtr);
            }
        }
        // convert palette to RGB
        if (colorType & PNG_COLOR_MASK_PALETTE) {
            png_set_palette_to_rgb(pngPtr);
        }
        // limit RGB to 8 bit
        if (colorType & PNG_COLOR_MASK_COLOR) {
            if (bitDepth > 8) { // 8: depth
                png_set_scale_16(pngPtr);
            }
        }
        if (forceGray) {
            // conversion to gray scale requested
            if (colorType & PNG_COLOR_MASK_COLOR) {
                png_set_rgb_to_gray(pngPtr, 1, 0.299, 0.587); // 0.299、0.587: Specifications
            }
            if (colorType & PNG_COLOR_MASK_ALPHA) {
                png_set_strip_alpha(pngPtr);
            }
        } else if ((colorType & PNG_COLOR_MASK_ALPHA) == 0) {
            png_set_add_alpha(pngPtr, 0xff, PNG_FILLER_AFTER);
        }
        png_read_update_info(pngPtr, infoPtr);
    }

    // callback for fetching data from imageFileBytes to libpng
    static void ReadData(png_structp pngPtr, png_bytep outBytes, png_size_t byteCountToRead) noexcept
    {
        png_voidp ioPtr = png_get_io_ptr(pngPtr);
        if (!ioPtr) {
            png_error(pngPtr, "Failed to read data.");
            return;
        }
        array_view<const uint8_t>& imageFileBytes = *(array_view<const uint8_t>*)ioPtr;
        // copy either the requested amount or what's left.
        const auto read = BASE_NS::Math::min(imageFileBytes.size(), byteCountToRead);
        BASE_NS::CloneData(outBytes, byteCountToRead, imageFileBytes.data(), read);
        // move the start of imageFileBytes past the data which was copied.
        imageFileBytes = array_view(imageFileBytes.data() + read, imageFileBytes.size() - read);
        if (read != byteCountToRead) {
            png_error(pngPtr, "Not enough data.");
            return;
        }
    }

    static IImageLoaderManager::LoadResult Load(array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) noexcept
    {
        if (imageFileBytes.empty()) {
            return ResultFailure("Input data must not be null.");
        }

        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) {
            png_destroy_read_struct(&png, nullptr, nullptr);
            return ResultFailure("Initialization failed.");
        }
        png_infop info = png_create_info_struct(png);
        if (!info) {
            png_destroy_read_struct(&png, nullptr, nullptr);
            return ResultFailure("Initialization failed.");
        }

        // libpng uses longjmp for error handling. execution will jump to this if statement on error. it's not
        // guaranteed that stack is cleaned up with longjmp, so have the big buffer pointer and pointers to start of
        // each row available here, and we can call reset() to release memory if something fails.
        BASE_NS::unique_ptr<uint8_t[]> image;
        BASE_NS::unique_ptr<png_byte*[]> rows;
        if (setjmp(png_jmpbuf(png))) {
            rows.reset();
            image.reset();
            png_destroy_read_struct(&png, &info, nullptr);
            return ResultFailure("Decoding failed.");
        }

        // libpng will now call ReadData when it needs data.
        png_set_read_fn(png, &imageFileBytes, ReadData);

        // first ask for the image information.
        png_read_info(png, info);
        auto width = png_get_image_width(png, info);
        auto height = png_get_image_height(png, info);
        auto channels = png_get_channels(png, info);
        auto is16bpc = png_get_bit_depth(png, info) == 16;

        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) !=
            IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) {
            // ask libpng to do possible conversions according to loadFlags.
            UpdateColorType(png, info, loadFlags);

            // update the image information in case some conversions will be done.
            width = png_get_image_width(png, info);
            height = png_get_image_height(png, info);
            channels = png_get_channels(png, info);
            is16bpc = png_get_bit_depth(png, info) == 16; // 16: index
            if (channels <= 0 || channels > 4) {          // 0: invalid channel num, 4: RGBA
                png_destroy_read_struct(&png, &info, nullptr);
                return ResultFailure("Invalid number of color channels.");
            }

            const size_t imageSize = width * height * channels;
            if ((width > MAX_IMAGE_EXTENT) || (height > MAX_IMAGE_EXTENT) || (imageSize > IMG_SIZE_LIMIT_2GB)) {
                png_destroy_read_struct(&png, &info, nullptr);
                return ResultFailure("Image too large.");
            }
            // allocate space for the whole image and and array of row pointers. libpng writes data to each row pointer.
            // alternative would be to use a different api which writes only one row and feed it the correct address
            // every time.
            image = BASE_NS::make_unique<uint8_t[]>(imageSize);
            rows = BASE_NS::make_unique<png_byte*[]>(height);
            // fill rows depending on should there be a vertical flip or not.
            auto row = rows.get();
            if (loadFlags & IImageLoaderManager::IMAGE_LOADER_FLIP_VERTICALLY_BIT) {
                for (auto i = 0U; i < height; ++i) {
                    *row++ = image.get() + ((height - 1) - i) * (width * channels);
                }
            } else {
                for (auto i = 0U; i < height; ++i) {
                    *row++ = image.get() + i * (width * channels);
                }
            }

            // ask libpng to devoce the whole image.
            png_read_image(png, rows.get());
            png_read_end(png, info);
        }
        png_destroy_read_struct(&png, &info, nullptr);

        // Success. Populate the image info and image data object.
        return CreateImage(BASE_NS::move(image), width, height, channels, loadFlags, is16bpc);
    }

    static IImageLoaderManager::LoadResult Load(
        array_view<const uint8_t> imageFileBytes, uint32_t loadFlags, uint32_t rowCount, uint32_t columnCount) noexcept
    {
        if (imageFileBytes.empty()) {
            return ResultFailure("Input data must not be null.");
        }

        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) {
            png_destroy_read_struct(&png, nullptr, nullptr);
            return ResultFailure("Initialization failed.");
        }
        png_infop info = png_create_info_struct(png);
        if (!info) {
            png_destroy_read_struct(&png, nullptr, nullptr);
            return ResultFailure("Initialization failed.");
        }

        // libpng uses longjmp for error handling. execution will jump to this if statement on error. it's not
        // guaranteed that stack is cleaned up with longjmp, so have the big buffer pointer and pointers to start of
        // each row available here, and we can call reset() to release memory if something fails.
        BASE_NS::unique_ptr<uint8_t[]> image;
        BASE_NS::unique_ptr<png_byte *[]> rows;
        if (setjmp(png_jmpbuf(png))) {
            rows.reset();
            image.reset();
            png_destroy_read_struct(&png, &info, nullptr);
            return ResultFailure("Decoding failed.");
        }

        // libpng will now call ReadData when it needs data.
        png_set_read_fn(png, &imageFileBytes, ReadData);

        // first ask for the image information.
        png_read_info(png, info);
        auto width = png_get_image_width(png, info);
        auto height = png_get_image_height(png, info);
        auto channels = png_get_channels(png, info);
        auto is16bpc = png_get_bit_depth(png, info) == 16;

        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) !=
            IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) {
            // ask libpng to do possible conversions according to loadFlags.
            UpdateColorType(png, info, loadFlags);

            // update the image information in case some conversions will be done.
            width = png_get_image_width(png, info);
            height = png_get_image_height(png, info);
            channels = png_get_channels(png, info);
            is16bpc = png_get_bit_depth(png, info) == 16; // 16: index
            if (channels <= 0 || channels > 4) {          // 0: invalid channel num, 4: RGBA
                png_destroy_read_struct(&png, &info, nullptr);
                return ResultFailure("Invalid number of color channels.");
            }

            const size_t imageSize = width * height * channels;
            if ((width > MAX_IMAGE_EXTENT) || (height > MAX_IMAGE_EXTENT) || (imageSize > IMG_SIZE_LIMIT_2GB)) {
                png_destroy_read_struct(&png, &info, nullptr);
                return ResultFailure("Image too large.");
            }
            // allocate space for the whole image and and array of row pointers. libpng writes data to each row pointer.
            // alternative would be to use a different api which writes only one row and feed it the correct address
            // every time.
            image = BASE_NS::make_unique<uint8_t[]>(imageSize);
            rows = BASE_NS::make_unique<png_byte *[]>(height);
            // fill rows depending on should there be a vertical flip or not.
            auto row = rows.get();
            if (loadFlags & IImageLoaderManager::IMAGE_LOADER_FLIP_VERTICALLY_BIT) {
                for (auto i = 0U; i < height; ++i) {
                    *row++ = image.get() + ((height - 1) - i) * (width * channels);
                }
            } else {
                for (auto i = 0U; i < height; ++i) {
                    *row++ = image.get() + i * (width * channels);
                }
            }

            // ask libpng to devoce the whole image.
            png_read_image(png, rows.get());
            png_read_end(png, info);
        }
        png_destroy_read_struct(&png, &info, nullptr);

        uint32_t sliceWidth = width / rowCount;
        uint32_t sliceHeight = height / columnCount;
        uint32_t depth = rowCount * columnCount;

        // Success. Populate the image info and image data object.
        return CreateImage(
            BASE_NS::move(image), width, height, channels, loadFlags, is16bpc, sliceWidth, sliceHeight, depth);
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    ImageDesc imageDesc_;
    BASE_NS::vector<SubImageDesc> imageBuffer_;

    BASE_NS::unique_ptr<uint8_t[]> imageBytes_;
    size_t imageBytesLength_ = 0;
};

class ImageLoaderPng final : public IImageLoaderManager::IImageLoader {
public:
    // Inherited via ImageManager::IImageLoader
    IImageLoaderManager::LoadResult Load(IFile& file, uint32_t loadFlags) const override
    {
        const uint64_t byteLength = file.GetLength();
        if (byteLength > static_cast<uint64_t>(std::numeric_limits<int>::max())) {
            return ResultFailure("File too big.");
        }

        // Read the file to a buffer.
        unique_ptr<uint8_t[]> buffer = make_unique<uint8_t[]>(static_cast<size_t>(byteLength));
        const uint64_t read = file.Read(buffer.get(), byteLength);
        if (read != byteLength) {
            return ResultFailure("Reading file failed.");
        }

        return PNGImage::Load(array_view<const uint8_t>(buffer.get(), static_cast<size_t>(byteLength)), loadFlags);
    }

    IImageLoaderManager::LoadResult Load(
        IFile& file, uint32_t loadFlags, uint32_t rowCount, uint32_t columnCount) const override
    {
        const uint64_t byteLength = file.GetLength();
        if (byteLength > static_cast<uint64_t>(std::numeric_limits<int>::max())) {
            return ResultFailure("File too big.");
        }

        // Read the file to a buffer.
        unique_ptr<uint8_t[]> buffer = make_unique<uint8_t[]>(static_cast<size_t>(byteLength));
        const uint64_t read = file.Read(buffer.get(), byteLength);
        if (read != byteLength) {
            return ResultFailure("Reading file failed.");
        }

        return PNGImage::Load(
            array_view<const uint8_t>(buffer.get(), static_cast<size_t>(byteLength)), loadFlags, rowCount, columnCount);
    }

    IImageLoaderManager::LoadResult Load(array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) const override
    {
        if (imageFileBytes.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
            return ResultFailure("File too big.");
        }
        return PNGImage::Load(imageFileBytes, loadFlags);
    }

    bool CanLoad(array_view<const uint8_t> imageFileBytes) const override
    {
        // Check for PNG
        return png_sig_cmp(imageFileBytes.data(), 0, imageFileBytes.size()) == 0;
    }

    IImageLoaderManager::LoadAnimatedResult LoadAnimatedImage(IFile& file, uint32_t loadFlags) override
    {
        return ResultFailureAnimated("");
    }

    IImageLoaderManager::LoadAnimatedResult LoadAnimatedImage(
        array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) override
    {
        return ResultFailureAnimated("");
    }

    vector<IImageLoaderManager::ImageType> GetSupportedTypes() const override
    {
        return vector<IImageLoaderManager::ImageType>(std::begin(IMAGE_TYPES), std::end(IMAGE_TYPES));
    }

protected:
    ~ImageLoaderPng() = default;
    void Destroy() override
    {
        delete this;
    }
};

IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderPng(CORE_NS::PluginToken)
{
    return IImageLoaderManager::IImageLoader::Ptr { new ImageLoaderPng() };
}

} // namespace PNGPlugin
