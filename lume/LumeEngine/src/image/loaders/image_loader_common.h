/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 */

#ifndef CORE_IMAGE_LOADERS_IMAGE_LOADER_COMMON_H
#define CORE_IMAGE_LOADERS_IMAGE_LOADER_COMMON_H

#include <functional>
#include <limits>
#include <base/math/mathf.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <core/namespace.h>

#include "image/image_loader_manager.h"

CORE_BEGIN_NAMESPACE()

class ImageLoaderCommon {
public:
    static ImageLoaderCommon &GetInstance();

    bool PremultiplyAlpha(uint8_t *imageBytes, uint32_t width, uint32_t height, uint32_t channelCount,
        uint32_t bytesPerChannel, bool linear);

private:
    ImageLoaderCommon() = default;
    ~ImageLoaderCommon() = default;

    ImageLoaderCommon(const ImageLoaderCommon &imgLoaderCommon) = delete;
    const ImageLoaderCommon &operator=(const ImageLoaderCommon &imgLoaderCommon) = delete;

    void InitializeSRGBTable();

    uint8_t SRGBPremultiplyLookup[256u * 256u] = {0};
};

void FreeLibBaseImageBytes(void *imageBytes);

using LibBaseImageDeleter = std::function<void(void *)>;
using LibBaseImagePtr = BASE_NS::unique_ptr<void, LibBaseImageDeleter>;

class LibBaseImage : public IImageContainer {
public:
    LibBaseImage();

    virtual ~LibBaseImage() = default;

    using Ptr = BASE_NS::unique_ptr<LibBaseImage, Deleter>;

    const ImageDesc &GetImageDesc() const override;

    BASE_NS::array_view<const uint8_t> GetData() const override;

    BASE_NS::array_view<const SubImageDesc> GetBufferImageCopies() const override;

    static constexpr BASE_NS::Format ResolveFormat(uint32_t loadFlags, uint32_t componentCount, bool is16bpc);

    constexpr static ImageDesc ResolveImageDesc(BASE_NS::Format format, uint32_t imageWidth, uint32_t imageHeight,
        uint32_t bitsPerPixel, bool generateMips, bool isPremultiplied);

    static ImageLoaderManager::LoadResult CreateImage(LibBaseImagePtr imageBytes, uint32_t imageWidth,
        uint32_t imageHeight, uint32_t componentCount, uint32_t loadFlags, bool is16bpc);

    struct Info {
        int width;
        int height;
        int componentCount;
        bool is16bpc;
    };

protected:
    virtual void Destroy() override;
    ImageDesc imageDesc_;
    SubImageDesc imageBuffer_;

    LibBaseImagePtr imageBytes_;
    size_t imageBytesLength_ = 0;
};

template <typename T>
class RowPointers {
public:
    T **row_pointers = nullptr;
    int allocHeight = 0;
    bool allocSucc = false;

    RowPointers(int width, int height, int channels, int channelSize)
    {
        if (height <= 0 || height > static_cast<size_t>(std::numeric_limits<int>::max())) {
            allocSucc = false;
            return;
        }
        row_pointers = (T **)malloc(height * sizeof(T *));
        allocHeight = height;

        size_t rowbytes = width * channels * channelSize;
        if (rowbytes <= 0 || rowbytes > static_cast<size_t>(std::numeric_limits<int>::max())) {
            allocSucc = false;
            return;
        }
        for (int i = 0; i < allocHeight; i++) {
            row_pointers[i] = nullptr; /* security precaution */
        }
        for (int i = 0; i < allocHeight; i++) {
            row_pointers[i] = (T *)malloc(rowbytes);
        }
        allocSucc = true;
    }

    ~RowPointers()
    {
        if (row_pointers) {
            for (int i = 0; i < allocHeight; i++) {
                if (row_pointers[i]) {
                    free(row_pointers[i]);
                }
            }
            free(row_pointers);
        }
    }
};

template <typename T>
class ArrayLoader {
public:
    ArrayLoader(BASE_NS::array_view<const T> imageFileBytes)
    {
        buf = imageFileBytes;
        arrSize = imageFileBytes.size();
        curr = 0;
    }
    void ArrayRead(T *dest, uint64_t length)
    {
        if (curr + length > arrSize) {
            CORE_LOG_E("ArrayLoader out of range.");
            return;
        }
        memcpy_s(dest, length, static_cast<const T *>(buf.data()) + curr, length);
        curr += length;
    }

private:
    BASE_NS::array_view<const T> buf;
    uint64_t curr;
    uint64_t arrSize;
};

template <typename T>
void VerticalFlipRowPointers(T **row_pointers, uint32_t height, uint32_t width, uint32_t channels)
{
    int halfWidth = width / 2;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < halfWidth; x++) {
            for (int k = 0; k < channels; k++) {
                int idx = x * channels + k;
                int ridx = (width - x - 1) * channels + k;
                T temp_pixel = row_pointers[y][idx];
                row_pointers[y][idx] = row_pointers[y][ridx];
                row_pointers[y][ridx] = temp_pixel;
            }
        }
    }
}

template <typename T>
void VerticalFlipRow(T *row_pointer, uint32_t width, uint32_t channels)
{
    int halfWidth = width / 2;
    for (int x = 0; x < halfWidth; x++) {
        for (int k = 0; k < channels; k++) {
            int idx = x * channels + k;
            int ridx = (width - x - 1) * channels + k;
            T temp_pixel = row_pointer[idx];
            row_pointer[idx] = row_pointer[ridx];
            row_pointer[ridx] = temp_pixel;
        }
    }
}

CORE_END_NAMESPACE()

#endif  //  CORE_IMAGE_LOADERS_IMAGE_LOADER_COMMON_H