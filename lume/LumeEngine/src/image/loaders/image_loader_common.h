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
        uint32_t width;
        uint32_t height;
        uint32_t componentCount;
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
    T **rowPointers = nullptr;
    int allocHeight = 0;
    bool allocSucc = false;

    RowPointers(uint32_t width, uint32_t height, uint32_t channels, uint32_t channelSize)
    {
        if (height <= 0 || height > static_cast<size_t>(std::numeric_limits<int>::max())) {
            allocSucc = false;
            return;
        }
        size_t rowbytes = static_cast<size_t>(width * channels) * channelSize;
        if (rowbytes <= 0 || rowbytes > static_cast<size_t>(std::numeric_limits<int>::max())) {
            allocSucc = false;
            return;
        }
        rowPointers = (T **)malloc(height * sizeof(T *));
        if (rowPointers == nullptr) {
            CORE_LOG_E("malloc fail return null");
            return;
        }
        allocHeight = static_cast<int>(height);
        for (int i = 0; i < allocHeight; i++) {
            rowPointers[i] = nullptr; /* security precaution */
        }
        for (int i = 0; i < allocHeight; i++) {
            rowPointers[i] = (T *)malloc(rowbytes);
            if (rowPointers[i] == nullptr) {
                CORE_LOG_E("malloc fail return null");
                return;
            }
        }
        allocSucc = true;
    }

    ~RowPointers()
    {
        if (rowPointers) {
            for (int i = 0; i < allocHeight; i++) {
                if (rowPointers[i]) {
                    free(rowPointers[i]);
                }
            }
            free(rowPointers);
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
        int ret = memcpy_s(dest, length, static_cast<const T *>(buf.data()) + curr, length);
        if (ret != 0) {
            CORE_LOG_E("memcpy_s in ArrayLoader error.");
            return;
        }
        curr += length;
    }

private:
    BASE_NS::array_view<const T> buf;
    uint64_t curr;
    uint64_t arrSize;
};

template <typename T>
void VerticalFlipRowPointers(T **rowPointers, uint32_t height, uint32_t width, uint32_t channels)
{
    uint32_t halfWidth = width / 2;
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < halfWidth; x++) {
            for (uint32_t k = 0; k < channels; k++) {
                uint32_t idx = x * channels + k;
                uint32_t ridx = (width - x - 1) * channels + k;
                T tempPixel = rowPointers[y][idx];
                rowPointers[y][idx] = rowPointers[y][ridx];
                rowPointers[y][ridx] = tempPixel;
            }
        }
    }
}

template <typename T>
void VerticalFlipRow(T *rowPointer, uint32_t width, uint32_t channels)
{
    uint32_t halfWidth = width / 2;
    for (uint32_t x = 0; x < halfWidth; x++) {
        for (uint32_t k = 0; k < channels; k++) {
            uint32_t idx = x * channels + k;
            uint32_t ridx = (width - x - 1) * channels + k;
            T tempPixel = rowPointer[idx];
            rowPointer[idx] = rowPointer[ridx];
            rowPointer[ridx] = tempPixel;
        }
    }
}

CORE_END_NAMESPACE()

#endif  //  CORE_IMAGE_LOADERS_IMAGE_LOADER_COMMON_H