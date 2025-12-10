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

#include "image_loader_jpg.h"

#include <csetjmp>
#include <functional>
#include <jpeglib.h>
#include <limits>
#include <memory>

#include <base/math/mathf.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <core/namespace.h>

using namespace BASE_NS;
using namespace CORE_NS;

namespace JPGPlugin {
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

class JPGImage final : public IImageContainer {
public:
    JPGImage() : IImageContainer(), imageDesc_(), imageBuffer_() {}

    ~JPGImage() override = default;

    using Ptr = BASE_NS::unique_ptr<JPGImage, Deleter>;

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
        return array_view<const SubImageDesc>(&imageBuffer_, 1);
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

    static IImageLoaderManager::LoadResult CreateImage(BASE_NS::unique_ptr<uint8_t[]> imageBytes, uint32_t imageWidth,
        uint32_t imageHeight, uint32_t componentCount, uint32_t loadFlags, bool is16bpc) noexcept
    {
        auto image = JPGImage::Ptr(new JPGImage);
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

        return ResultSuccess(BASE_NS::move(image));
    }

    static void ErrorExit(j_common_ptr cinfo) noexcept
    {
        if (!cinfo) {
            CORE_LOG_I("No info");
            return;
        }
        if (cinfo->err && cinfo->err->output_message) {
            (*cinfo->err->output_message)(cinfo);
        }
        jmp_buf* jmpBuffer = static_cast<jmp_buf*>(cinfo->client_data);
        longjmp(*jmpBuffer, 1);
    }

    static IImageLoaderManager::LoadResult Load(array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) noexcept
    {
        if (imageFileBytes.empty()) {
            return ResultFailure("Input data must not be null.");
        }
        jmp_buf jmpBuffer;
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;
        // initialize the error handler and override the exit handler which calls exit(). instead we'll call longjmp to
        // return here for cleanup.
        cinfo.err = jpeg_std_error(&jerr);
        jerr.error_exit = ErrorExit;
        cinfo.client_data = &jmpBuffer;

        BASE_NS::unique_ptr<uint8_t[]> image;
        BASE_NS::unique_ptr<uint8_t* []> rows;
        if (setjmp(jmpBuffer)) {
            jpeg_destroy_decompress(&cinfo);
            rows.reset();
            image.reset();
            return ResultFailure("Failed to load.");
        }

        // initialize the decompress struct
        jpeg_create_decompress(&cinfo);
        jpeg_mem_src(&cinfo, imageFileBytes.data(), static_cast<unsigned long>(imageFileBytes.size()));

        // first ask for the image information.
        auto ret = jpeg_read_header(&cinfo, TRUE);
        if (ret != JPEG_HEADER_OK) {
            jpeg_destroy_decompress(&cinfo);
            return ResultFailure("Failed to load.");
        }

        auto width = cinfo.image_width;
        auto height = cinfo.image_height;
        auto channels = static_cast<uint32_t>(cinfo.num_components);
        auto is16bpc = cinfo.data_precision > 8;

        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) !=
            IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) {
            // ask libjpg to do possible conversions according to loadFlags.
            if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT) ==
                IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT) {
                cinfo.out_color_space = JCS_GRAYSCALE;
            } else if (cinfo.num_components == 2 || cinfo.num_components == 3) { // 2: index  3: index
                cinfo.out_color_space = JCS_EXT_RGBA;
            }
            if (!jpeg_start_decompress(&cinfo)) {
                jpeg_abort_decompress(&cinfo);
            }

            // update the image information in case some conversions will be done.
            width = cinfo.output_width;
            height = cinfo.output_height;
            channels = static_cast<uint32_t>(cinfo.output_components);
            is16bpc = cinfo.data_precision > 8;  // 8: index
            if (channels <= 0 || channels > 4) { // 0: invalid channel num, 4: RGBA
                jpeg_destroy_decompress(&cinfo);
                return ResultFailure("Invalid number of color channels.");
            }

            const size_t imageSize = width * height * channels;
            if ((width > MAX_IMAGE_EXTENT) || (height > MAX_IMAGE_EXTENT) || (imageSize > IMG_SIZE_LIMIT_2GB)) {
                jpeg_destroy_decompress(&cinfo);
                return ResultFailure("Image too large.");
            }
            // allocate space for the whole image and and array of row pointers. libjpg writes data to each row pointer.
            // alternative would be to use a different api which writes only one row and feed it the correct address
            // every time.
            image = BASE_NS::make_unique<uint8_t[]>(imageSize);
            rows = BASE_NS::make_unique<uint8_t* []>(height);
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
            while (cinfo.output_scanline < cinfo.output_height) {
                jpeg_read_scanlines(
                    &cinfo, rows.get() + cinfo.output_scanline, cinfo.output_height - cinfo.output_scanline);
            }
            jpeg_finish_decompress(&cinfo);
        }
        jpeg_destroy_decompress(&cinfo);

        // Success. Populate the image info and image data object.
        return CreateImage(BASE_NS::move(image), width, height, channels, loadFlags, is16bpc);
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    ImageDesc imageDesc_;
    SubImageDesc imageBuffer_;

    BASE_NS::unique_ptr<uint8_t[]> imageBytes_;
    size_t imageBytesLength_ = 0;
};

class ImageLoaderJPG final : public IImageLoaderManager::IImageLoader {
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

        return JPGImage::Load(array_view<const uint8_t>(buffer.get(), static_cast<size_t>(byteLength)), loadFlags);
    }

    IImageLoaderManager::LoadResult Load(array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) const override
    {
        if (imageFileBytes.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
            return ResultFailure("File too big.");
        }
        return JPGImage::Load(imageFileBytes, loadFlags);
    }

    bool CanLoad(array_view<const uint8_t> imageFileBytes) const override
    {
        // Check for JPEG / DQT / JFIF / Exif / ICC_PROFILE tag
        // 10：size
        if ((imageFileBytes.size() >= 10) && imageFileBytes[0] == 0xff && imageFileBytes[1] == 0xd8 &&
            imageFileBytes[2] == 0xff &&
            (imageFileBytes[3] == 0xdb ||
                (imageFileBytes[3] == 0xe0 && imageFileBytes[6] == 'J' && imageFileBytes[7] == 'F' &&
                    imageFileBytes[8] == 'I' && imageFileBytes[9] == 'F') || // JFIF
                (imageFileBytes[3] == 0xe1 && imageFileBytes[6] == 'E' && imageFileBytes[7] == 'x' &&
                    imageFileBytes[8] == 'i' && imageFileBytes[9] == 'f') || // Exif
                (imageFileBytes[3] == 0xe2 && imageFileBytes[6] == 'I' && imageFileBytes[7] == 'C' &&
                    imageFileBytes[8] == 'C' && imageFileBytes[9] == '_'))) { // ICC_PROFILE
            return true;
        }

        return false;
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
    ~ImageLoaderJPG() = default;
    void Destroy() override
    {
        delete this;
    }
};

IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderJPG(CORE_NS::PluginToken)
{
    return IImageLoaderManager::IImageLoader::Ptr { new ImageLoaderJPG() };
}

} // namespace JPGPlugin
