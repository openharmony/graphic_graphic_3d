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

#include "image/loaders/image_loader_libjpeg.h"
#if defined(USE_LIB_PNG_JPEG) && (USE_LIB_PNG_JPEG == 1)
#include <csetjmp>
#include "jpeglib.h"
#include "jerror.h"

#include "image/loaders/image_loader_common.h"

#define LIBJPEG_SUPPORT_8_BIT

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

struct MyErrorMgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmpBuffer;
};

void MyErrorExit(j_common_ptr cinfo)
{
    if (cinfo == nullptr) {
        CORE_LOG_E("cinfo is nullptr in MyErrorExit");
        return;
    }
    MyErrorMgr *myerr = reinterpret_cast<MyErrorMgr *>(cinfo->err);
    if (cinfo->err->output_message == nullptr) {
        CORE_LOG_E("cinfo err output_message is nullptr in MyErrorExit");
        return;
    }
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmpBuffer, 1);
}

#ifdef LIBJPEG_SUPPORT_12_BIT
void HandleLittleEndian(J12SAMPARRAY rowPointers12, int len)
{
    int littleEndian = 1;
    char *ptr = reinterpret_cast<char *>(&littleEndian);
    if (*ptr == 1) {
        /* Swap MSB and LSB in each sample */
        for (int col = 0; col < len; col++) {
            rowPointers12[0][col] = ((rowPointers12[0][col] & 0xFF) << 8) | ((rowPointers12[0][col] >> 8) & 0xFF);
        }
    }
}
#endif

void HandleJPEGColorType(jpeg_decompress_struct &cinfo, uint32_t loadFlags, LibBaseImage::Info &info)
{
    // Convert 3 channels to 4 because 3 channel textures are not always supported.
    // Also convert 2 channel (grayscale + alpha) to 4 because r + a in not supported.
    if (cinfo.num_components == 2 || cinfo.num_components == 3) {
        cinfo.out_color_space = JCS_EXT_RGBA;
    }
    bool forceGray = (loadFlags & IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT) != 0;
    if (forceGray) {
        cinfo.out_color_space = JCS_GRAYSCALE;
    }

    jpeg_start_decompress(&cinfo);

    info.width = cinfo.output_width;
    info.height = cinfo.output_height;
    info.componentCount = static_cast<uint32_t>(cinfo.output_components);
    info.is16bpc = cinfo.data_precision > 8;
}

class LibJPEGImage final : public LibBaseImage {
public:
    LibJPEGImage() : LibBaseImage()
    {}

    static void ReadBuffer16Bit(jpeg_decompress_struct &cinfo, uint16_t *buff, int row_stride, bool needVerticalFlip)
    {
#ifdef LIBJPEG_SUPPORT_12_BIT
        J12SAMPARRAY rowPointers12 = static_cast<J12SAMPARRAY>((*cinfo.mem->alloc_sarray)(
            reinterpret_cast<j_common_ptr>(&cinfo), JPOOL_IMAGE, static_cast<uint32_t>(row_stride), 1));
        if (rowPointers12 == nullptr) {
            CORE_LOG_E("rowPointers12 is null");
            return;
        }
        int pos = 0;
        while (cinfo.output_scanline < cinfo.output_height) {
            jpeg12_read_scanlines(&cinfo, rowPointers12, 1);
            HandleLittleEndian(rowPointers12, row_stride);
            if (needVerticalFlip) {
                VerticalFlipRow(rowPointers12[0], cinfo.output_width, static_cast<uint32_t>(cinfo.output_components));
            }
            for (int k = 0; k < row_stride; k++) {
                buff[pos++] = rowPointers12[0][k];
            }
        }
#endif
    }
    static void ReadBuffer8Bit(jpeg_decompress_struct &cinfo, uint8_t *buff, int row_stride, bool needVerticalFlip)
    {
        JSAMPARRAY rowPointers8 = (*cinfo.mem->alloc_sarray)(
            reinterpret_cast<j_common_ptr>(&cinfo), JPOOL_IMAGE, static_cast<uint32_t>(row_stride), 1);
        if (rowPointers8 == nullptr) {
            CORE_LOG_E("rowPointers8 is null");
            return;
        }
        int pos = 0;
        while (cinfo.output_scanline < cinfo.output_height) {
            jpeg_read_scanlines(&cinfo, rowPointers8, 1);
            if (needVerticalFlip) {
                VerticalFlipRow(rowPointers8[0], cinfo.output_width, static_cast<uint32_t>(cinfo.output_components));
            }
            for (int k = 0; k < row_stride; k++) {
                buff[pos++] = rowPointers8[0][k];
            }
        }
    }

    static LibBaseImagePtr LoadFromMemory(jpeg_decompress_struct &cinfo, uint32_t loadFlags, Info &info)
    {
        LibBaseImagePtr imageBytes = nullptr;

        HandleJPEGColorType(cinfo, loadFlags, info);

        size_t imgSize = cinfo.output_width * cinfo.output_height * static_cast<uint32_t>(cinfo.output_components);
        if (imgSize < 1 || imgSize >= IMG_SIZE_LIMIT_2GB) {
            CORE_LOG_E("imgSize more than limit!");
            return imageBytes;
        }

        int row_stride = static_cast<int>(cinfo.output_width) * cinfo.output_components;
        bool needVerticalFlip = (loadFlags & IImageLoaderManager::IMAGE_LOADER_FLIP_VERTICALLY_BIT) != 0;
        if (info.is16bpc) {
#ifdef LIBJPEG_SUPPORT_12_BIT
            uint16_t *buff = static_cast<uint16_t *>(malloc(imgSize * sizeof(uint16_t)));
            if (buff == nullptr) {
                CORE_LOG_E("malloc fail return null");
                return imageBytes;
            }
            ReadBuffer16Bit(cinfo, buff, row_stride, needVerticalFlip);
#else
            uint16_t *buff = nullptr;
            CORE_LOG_E("libjpeg do not support 12/16 bit in current version");
#endif
            imageBytes = {buff, FreeLibBaseImageBytes};
        } else {
            uint8_t *buff = static_cast<uint8_t *>(malloc(imgSize * sizeof(uint8_t)));
            if (buff == nullptr) {
                CORE_LOG_E("malloc fail return null");
                return imageBytes;
            }
            ReadBuffer8Bit(cinfo, buff, row_stride, needVerticalFlip);
            imageBytes = {buff, FreeLibBaseImageBytes};
        }
        return imageBytes;
    }

    // Actual jpeg loading implementation.
    static ImageLoaderManager::LoadResult Load(array_view<const uint8_t> imageFileBytes, uint32_t loadFlags)
    {
        CORE_LOG_D("ImageLoaderManager Load jpeg start");
        if (imageFileBytes.empty()) {
            return ImageLoaderManager::ResultFailure("Input data must not be null.");
        }

        struct jpeg_decompress_struct cinfo;
        struct MyErrorMgr jerr;

        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = MyErrorExit;
        if (setjmp(jerr.setjmpBuffer)) {
            jpeg_destroy_decompress(&cinfo);
            return ImageLoaderManager::ResultFailure("jpeg_jmpbuf to fail");
        }
        jpeg_create_decompress(&cinfo);

        // Load the image info without decoding the image data
        // (Just to check what the image format is so we can convert if necessary).
        Info info;
        jpeg_mem_src(&cinfo, imageFileBytes.data(), imageFileBytes.size());
        jpeg_read_header(&cinfo, TRUE);
        info.width = cinfo.image_width;
        info.height = cinfo.image_height;
        info.componentCount = static_cast<uint32_t>(cinfo.num_components);
        info.is16bpc = cinfo.data_precision > 8;

        // Not supporting hdr images via libjpeg.
        // LibJPEG cannot check hdr

        LibBaseImagePtr imageBytes = nullptr;
        if ((loadFlags & IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY) == 0) {
            imageBytes = LoadFromMemory(cinfo, loadFlags, info);
            if (imageBytes == nullptr) {
                jpeg_finish_decompress(&cinfo);
                jpeg_destroy_decompress(&cinfo);
                return ImageLoaderManager::ResultFailure("jpeg LoadFromMemory fail");
            }
        } else {
            imageBytes = {nullptr, FreeLibBaseImageBytes};
        }
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

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

class ImageLoaderLibJPEGImage final : public ImageLoaderManager::IImageLoader {
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

        return LibJPEGImage::Load(array_view<const uint8_t>(buffer.get(), static_cast<size_t>(byteLength)), loadFlags);
    }

    ImageLoaderManager::LoadResult Load(array_view<const uint8_t> imageFileBytes, uint32_t loadFlags) const override
    {
        // uses int for file sizes. Don't even try to read a file if the size does not fit to int.
        // Not writing a test for this :)
        if (imageFileBytes.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
            return ImageLoaderManager::ResultFailure("Data too big to read.");
        }

        return LibJPEGImage::Load(imageFileBytes, loadFlags);
    }

    bool CanLoad(array_view<const uint8_t> imageFileBytes) const override
    {
        // uses int for file sizes. Don't even try to read a file if the size does not fit to int.
        // Not writing a test for this :)
        size_t maxFileSize = static_cast<size_t>(std::numeric_limits<int>::max());
        size_t jpegHeaderSize = 10;
        if (imageFileBytes.size() > maxFileSize || imageFileBytes.size() < jpegHeaderSize) {
            return false;
        }

        // Check for JPEG / JFIF / Exif / ICC_PROFILE tag
        bool isJFIF = (imageFileBytes[3] == 0xe0 && imageFileBytes[6] == 'J' && imageFileBytes[7] == 'F' &&
                       imageFileBytes[8] == 'I' && imageFileBytes[9] == 'F');  // JFIF
        bool isExif = (imageFileBytes[3] == 0xe1 && imageFileBytes[6] == 'E' && imageFileBytes[7] == 'x' &&
                       imageFileBytes[8] == 'i' && imageFileBytes[9] == 'f');  // Exif
        bool isICC = (imageFileBytes[3] == 0xe2 && imageFileBytes[6] == 'I' && imageFileBytes[7] == 'C' &&
                      imageFileBytes[8] == 'C' && imageFileBytes[9] == '_');  // ICC_PROFILE
        if (imageFileBytes[0] == 0xff && imageFileBytes[1] == 0xd8 && imageFileBytes[2] == 0xff &&
            (isJFIF || isExif || isICC)) {
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
        return BASE_NS::vector<IImageLoaderManager::ImageType>(std::begin(JPEG_IMAGE_TYPES),
            std::end(JPEG_IMAGE_TYPES));
    }

protected:
    ~ImageLoaderLibJPEGImage() = default;
    void Destroy() override
    {
        delete this;
    }
};
}  // namespace
IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderLibJPEGImage(PluginToken)
{
    return ImageLoaderManager::IImageLoader::Ptr{new ImageLoaderLibJPEGImage()};
}
CORE_END_NAMESPACE()
#endif // defined(USE_LIB_PNG_JPEG) && (USE_LIB_PNG_JPEG == 1)