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

#include "image/loaders/image_loader_libpng.h"
#if defined(USE_LIB_PNG_JPEG) && (USE_LIB_PNG_JPEG == 1)
#include "png.h"
#include "pngstruct.h"
#include "pnginfo.h"
#include <core/log.h>
#include "base/containers/string.h"

#include "image/loaders/image_loader_common.h"

CORE_BEGIN_NAMESPACE()
namespace {
using BASE_NS::array_view;
using BASE_NS::Format;
using BASE_NS::make_unique;
using BASE_NS::move;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::unique_ptr;

constexpr size_t IMG_SIZE_LIMIT_2GB = static_cast<size_t>(std::numeric_limits<int>::max());

void ArrayPNGReader(png_structp png_ptr, png_bytep png_data, png_size_t length)
{
    ArrayLoader<uint8_t> &loader = *static_cast<ArrayLoader<uint8_t> *>(png_get_io_ptr(png_ptr));
    loader.ArrayRead(reinterpret_cast<uint8_t *>(png_data), static_cast<uint64_t>(length));
}

void HandlePNGColorType(png_structp png_ptr, png_infop info_ptr, uint32_t loadFlags)
{
    if (png_ptr == nullptr || info_ptr == nullptr) {
        CORE_LOG_E("pass nullptr to HandlePNGColorType");
        return;
    }
    if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY && info_ptr->bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png_ptr);  // force to 8bit
    } else if (info_ptr->bit_depth == 16) {
        png_set_strip_16(png_ptr);  // force to 8bit
    }
    bool forceGray = (loadFlags & IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT) != 0;
    if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);  // force color index to RGB
        if (forceGray) {
            png_set_rgb_to_gray(png_ptr, 1, 0.299, 0.587);  // RGB->Gray from opencv
            png_set_strip_alpha(png_ptr);
        }
    }
    // Also convert 2 channel (grayscale + alpha) to 4 because r + a in not supported.
    else if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        // Force grayscale if requested.
        if (forceGray) {
            png_set_strip_alpha(png_ptr);
        } else {
            png_set_gray_to_rgb(png_ptr);
        }
    }
    // Convert 3 channels to 4 because 3 channel textures are not always supported.
    else if (info_ptr->color_type == PNG_COLOR_TYPE_RGB) {
        // Force grayscale if requested.
        if (forceGray) {
            png_set_rgb_to_gray(png_ptr, 1, 0.299, 0.587);  // RGB->Gray from opencv
        } else {
            png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
        }
    } else if (info_ptr->color_type == PNG_COLOR_TYPE_RGBA) {
        // Force grayscale if requested.
        if (forceGray) {
            png_set_rgb_to_gray(png_ptr, 1, 0.299, 0.587);  // RGB->Gray from opencv
            png_set_strip_alpha(png_ptr);
        }
    }
    png_read_update_info(png_ptr, info_ptr);
}

class LibPNGImage final : public LibBaseImage {
public:
    LibPNGImage() : LibBaseImage()
    {}

    static LibBaseImagePtr LoadFromMemory(png_structp png_ptr, png_infop info_ptr, uint32_t loadFlags, Info &info)
    {
        LibBaseImagePtr imageBytes = nullptr;
        if (png_ptr == nullptr || info_ptr == nullptr) {
            CORE_LOG_E("pass nullptr to LoadFromMemory");
            return imageBytes;
        }
        HandlePNGColorType(png_ptr, info_ptr, loadFlags);

        info.width = info_ptr->width;
        info.height = info_ptr->height;
        info.componentCount = info_ptr->channels;
        info.is16bpc = info_ptr->bit_depth == 16;

        size_t imgSize = info.width * info.height * info.componentCount;
        if (imgSize < 1 || imgSize >= IMG_SIZE_LIMIT_2GB) {
            CORE_LOG_E("imgSize more than limit!");
            return imageBytes;
        }

        RowPointers<uint8_t> rp(info.width, info.height, info.componentCount, sizeof(uint8_t));
        if (!rp.allocSucc) {
            CORE_LOG_E("RowPointers allocate fail");
            return imageBytes;
        }

        png_bytep buff = static_cast<png_bytep>(malloc(imgSize * sizeof(uint8_t)));
        if (buff == nullptr) {
            CORE_LOG_E("malloc fail return null");
            return imageBytes;
        }
        imageBytes = {buff, FreeLibBaseImageBytes};

        png_read_image(png_ptr, rp.rowPointers);
        png_read_end(png_ptr, info_ptr);
        // Flip vertically if requested.
        if (imageBytes && (loadFlags & IImageLoaderManager::IMAGE_LOADER_FLIP_VERTICALLY_BIT) != 0) {
            VerticalFlipRowPointers(rp.rowPointers, info.height, info.width, info.componentCount);
        }

        uint32_t pos = 0;
        for (uint32_t y = 0; y < info.height; ++y) {
            for (uint32_t x = 0; x < info.componentCount * info.width; x += info.componentCount) {
                for (uint32_t k = 0; k < info.componentCount; k++) {
                    buff[pos++] = rp.rowPointers[y][x + k];
                }
            }
        }
        return imageBytes;
    }

    // Actual png loading implementation.
    static ImageLoaderManager::LoadResult Load(array_view<const uint8_t> imageFileBytes, uint32_t loadFlags)
    {
        CORE_LOG_D("ImageLoaderManager Load png start");
        if (imageFileBytes.empty()) {
            return ImageLoaderManager::ResultFailure("Input data must not be null.");
        }

        png_structp png_ptr;
        png_infop info_ptr;

        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr) {
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            return ImageLoaderManager::ResultFailure("Loading png_ptr failed");
        }
        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            return ImageLoaderManager::ResultFailure("Loading info_ptr failed");
        }
        if (setjmp(png_jmpbuf(png_ptr))) {
            /* If we get here, we had a problem reading the file. */
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            return ImageLoaderManager::ResultFailure("png_jmpbuf to fail");
        }

        // Load the image info without decoding the image data
        // (Just to check what the image format is so we can convert if necessary).
        Info info;
        ArrayLoader<uint8_t> aloader(imageFileBytes);
        png_set_read_fn(png_ptr, &aloader, ArrayPNGReader);
        png_read_info(png_ptr, info_ptr);
        info.width = info_ptr->width;
        info.height = info_ptr->height;
        info.componentCount = info_ptr->channels;
        info.is16bpc = info_ptr->bit_depth == 16;

        // Not supporting hdr images via pnglib.
        // libpng cannot check hdr

        LibBaseImagePtr imageBytes = nullptr;
        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) == 0) {
            imageBytes = LoadFromMemory(png_ptr, info_ptr, loadFlags, info);
            if (imageBytes == nullptr) {
                png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
                return ImageLoaderManager::ResultFailure("png LoadFromMemory fail");
            }
        } else {
            imageBytes = {nullptr, FreeLibBaseImageBytes};
        }
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

        // Success. Populate the image info and image data object.
        return CreateImage(
            CORE_NS::move(imageBytes), info.width, info.height, info.componentCount, loadFlags, info.is16bpc);
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    ImageDesc imageDesc_;
    SubImageDesc imageBuffer_;

    LibBaseImagePtr imageBytes_;
    size_t imageBytesLength_ = 0;
};

class ImageLoaderLibPNGImage final : public ImageLoaderManager::IImageLoader {
public:
    // Inherited via ImageManager::IImageLoader
    ImageLoaderManager::LoadResult Load(IFile &file, uint32_t loadFlags) const override
    {
        const uint64_t byteLength = file.GetLength();
        // uses int for file sizes. Don't even try to read a file if the size does not fit to int.
        if (byteLength > static_cast<uint64_t>(std::numeric_limits<int>::max())) {
            return ImageLoaderManager::ResultFailure("File too big to read.");
        }

        // Read the file to a buffer.
        unique_ptr<uint8_t[]> buffer = make_unique<uint8_t[]>(static_cast<size_t>(byteLength));
        const uint64_t read = file.Read(buffer.get(), byteLength);
        if (read != byteLength) {
            return ImageLoaderManager::ResultFailure("Reading file failed.");
        }

        return LibPNGImage::Load(array_view<const uint8_t>(buffer.get(), static_cast<size_t>(byteLength)), loadFlags);
    }

    ImageLoaderManager::LoadResult Load(array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) const override
    {
        // uses int for file sizes. Don't even try to read a file if the size does not fit to int.
        // Not writing a test for this :)
        if (imageFileBytes.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
            return ImageLoaderManager::ResultFailure("Data too big to read.");
        }

        return LibPNGImage::Load(imageFileBytes, loadFlags);
    }

    bool CanLoad(array_view<const uint8_t> imageFileBytes) const override
    {
        // uses int for file sizes. Don't even try to read a file if the size does not fit to int.
        // Not writing a test for this :)
        size_t maxFileSize = static_cast<size_t>(std::numeric_limits<int>::max());
        size_t pngHeaderSize = 8;
        if (imageFileBytes.size() > maxFileSize || imageFileBytes.size() < pngHeaderSize) {
            return false;
        }

        // Check for PNG
        if (imageFileBytes[0] == 137 && imageFileBytes[1] == 80 && imageFileBytes[2] == 78 && imageFileBytes[3] == 71 &&
            imageFileBytes[4] == 13 && imageFileBytes[5] == 10 && imageFileBytes[6] == 26 && imageFileBytes[7] == 10) {
            return true;
        }

        return false;
    }

    // No animation support
    ImageLoaderManager::LoadAnimatedResult LoadAnimatedImage(IFile &file, uint32_t loadFlags) override
    {
        return ImageLoaderManager::ResultFailureAnimated("Animation not supported.");
    }

    ImageLoaderManager::LoadAnimatedResult LoadAnimatedImage(
        array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) override
    {
        return ImageLoaderManager::ResultFailureAnimated("Animation not supported.");
    }

    BASE_NS::vector<IImageLoaderManager::ImageType> GetSupportedTypes() const override
    {
        return BASE_NS::vector<IImageLoaderManager::ImageType>(std::begin(PNG_IMAGE_TYPES), std::end(PNG_IMAGE_TYPES));
    }
protected:
    ~ImageLoaderLibPNGImage() = default;
    void Destroy() override
    {
        delete this;
    }
};
}  // namespace
IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderLibPNGImage(PluginToken)
{
    return ImageLoaderManager::IImageLoader::Ptr{new ImageLoaderLibPNGImage()};
}
CORE_END_NAMESPACE()
#endif // defined(USE_LIB_PNG_JPEG) && (USE_LIB_PNG_JPEG == 1)