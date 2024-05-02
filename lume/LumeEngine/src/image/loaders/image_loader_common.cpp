/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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

#include "image/loaders/image_loader_common.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::Format;
using BASE_NS::make_unique;
using BASE_NS::move;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::unique_ptr;

ImageLoaderCommon &ImageLoaderCommon::GetInstance()
{
    static ImageLoaderCommon single;
    return single;
}

void ImageLoaderCommon::InitializeSRGBTable()
{
    // Generate lookup table to premultiply sRGB encoded image in linear space and reencoding it to sRGB
    // Formulas from https://en.wikipedia.org/wiki/SRGB
    for (uint32_t a = 0; a < 256u; a++) {
        const float alpha = a / 255.f;
        for (uint32_t sRGB = 0; sRGB < 256u; sRGB++) {
            float color = sRGB / 255.f;
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
            SRGBPremultiplyLookup[a * 256u + sRGB] = static_cast<uint8_t>(round(premultiplied * 255.f));
        }
    }
}

bool ImageLoaderCommon::PremultiplyAlpha(
    uint8_t *imageBytes, uint32_t width, uint32_t height, uint32_t channelCount, uint32_t bytesPerChannel, bool linear)
{
    // Only need to process images with color and alpha data. I.e. RGBA or grayscale + alpha.
    if (channelCount != 4u && channelCount != 2u) {
        return true;
    }

    const uint32_t pixelCount = width * height;

    if (bytesPerChannel == 1 && linear) {
        uint8_t *img = imageBytes;
        for (uint32_t i = 0; i < pixelCount; i++) {
            // We know the alpha value is always last.
            uint32_t alpha = img[channelCount - 1];
            for (uint32_t j = 0; j < channelCount - 1; j++) {
                *img = static_cast<uint8_t>(*img * alpha / 0xff);
                img++;
            }
            img++;  // Skip over the alpha value.
        }
    } else if (bytesPerChannel == 1) {
        if (SRGBPremultiplyLookup[256u * 256u - 1] == 0) {
            InitializeSRGBTable();
        }
        uint8_t *img = imageBytes;
        for (uint32_t i = 0; i < pixelCount; i++) {
            uint8_t *p = &SRGBPremultiplyLookup[img[channelCount - 1] * 256u];
            for (uint32_t j = 0; j < channelCount - 1; j++) {
                *img = p[*img];
                img++;
            }
            img++;
        }
    } else if (bytesPerChannel == 2u) {
        // Same for 16 bits per channel images.
        uint16_t *img = reinterpret_cast<uint16_t *>(imageBytes);
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

void FreeLibBaseImageBytes(void *imageBytes)
{
    free(imageBytes);
}

LibBaseImage::LibBaseImage() : IImageContainer(), imageDesc_(), imageBuffer_()
{}

const IImageContainer::ImageDesc &LibBaseImage::GetImageDesc() const
{
    return imageDesc_;
}

array_view<const uint8_t> LibBaseImage::GetData() const
{
    return array_view<const uint8_t>(static_cast<const uint8_t *>(imageBytes_.get()), imageBytesLength_);
}

array_view<const IImageContainer::SubImageDesc> LibBaseImage::GetBufferImageCopies() const
{
    return array_view<const IImageContainer::SubImageDesc>(&imageBuffer_, 1);
}

constexpr Format LibBaseImage::ResolveFormat(uint32_t loadFlags, uint32_t componentCount, bool is16bpc)
{
    Format format{};
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
            format = is16bpc ? Format::BASE_FORMAT_R16G16B16A16_UNORM
                             : (forceLinear ? Format::BASE_FORMAT_R8G8B8A8_UNORM : Format::BASE_FORMAT_R8G8B8A8_SRGB);
            break;
        default:
            format = Format::BASE_FORMAT_UNDEFINED;
            break;
    }

    return format;
}

constexpr IImageContainer::ImageDesc LibBaseImage::ResolveImageDesc(Format format, uint32_t imageWidth,
    uint32_t imageHeight, uint32_t bitsPerPixel, bool generateMips, bool isPremultiplied)
{
    uint32_t mipCount = 1;

    // 1D images not supported with img loader
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

    return ImageDesc{
        imageFlags,    // imageFlags
        1,             // blockPixelWidth
        1,             // blockPixelHeight
        1,             // blockPixelDepth
        bitsPerPixel,  // bitsPerBlock

        imageType,      // imageType
        imageViewType,  // imageViewType
        format,         // format

        static_cast<uint32_t>(imageWidth),   // width
        static_cast<uint32_t>(imageHeight),  // height
        1,                                   // depth

        mipCount,  // mipCount
        1,         // layerCount
    };
}

ImageLoaderManager::LoadResult LibBaseImage::CreateImage(LibBaseImagePtr imageBytes, uint32_t imageWidth,
    uint32_t imageHeight, uint32_t componentCount, uint32_t loadFlags, bool is16bpc)
{
    auto image = LibBaseImage::Ptr(new LibBaseImage);
    if (!image) {
        return ImageLoaderManager::ResultFailure("Loading image failed.");
    }

    // Premultiply alpha if requested.
    bool isPremultiplied = false;
    if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) == 0) {
        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA) != 0) {
            const uint32_t bytesPerChannel = is16bpc ? 2u : 1u;
            const bool forceLinear = (loadFlags & IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT) != 0;
            isPremultiplied =
                ImageLoaderCommon::GetInstance().PremultiplyAlpha(static_cast<uint8_t *>(imageBytes.get()),
                    imageWidth,
                    imageHeight,
                    componentCount,
                    bytesPerChannel,
                    forceLinear);
        }
    }

    const Format format = ResolveFormat(loadFlags, componentCount, is16bpc);

    const bool generateMips = (loadFlags & IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS) != 0;
    const uint32_t bytesPerComponent = is16bpc ? 2u : 1u;
    const uint32_t bitsPerPixel = bytesPerComponent * componentCount * 8u;

    image->imageDesc_ = ResolveImageDesc(format, imageWidth, imageHeight, bitsPerPixel, generateMips, isPremultiplied);
    image->imageBytes_ = BASE_NS::move(imageBytes);
    image->imageBytesLength_ = static_cast<size_t>(imageWidth * imageHeight * componentCount * bytesPerComponent);

    image->imageBuffer_ = SubImageDesc{
        0,            // uint32_t bufferOffset
        imageWidth,   // uint32_t bufferRowLength
        imageHeight,  // uint32_t bufferImageHeight

        0,  // uint32_t mipLevel
        1,  // uint32_t layerCount

        static_cast<uint32_t>(imageWidth),
        static_cast<uint32_t>(imageHeight),
        1,
    };
    return ImageLoaderManager::ResultSuccess(CORE_NS::move(image));
}

void LibBaseImage::Destroy()
{
    delete this;
}
CORE_END_NAMESPACE()
