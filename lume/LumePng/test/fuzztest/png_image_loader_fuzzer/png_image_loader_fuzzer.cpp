/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

/* Fuzz test for PNGPlugin::ImageLoaderPng via <png/image_loader_png.h>. */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include <base/containers/array_view.h>
#include <base/containers/unique_ptr.h>
#include <core/image/intf_image_container.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/io/intf_file.h>
#include <core/namespace.h>
#include <core/plugin/intf_plugin.h>

#include <zlib.h>  // crc32() for synthetic PNG chunks

#include "fuzz_consumer.h"
#include "png/image_loader_png.h"
#include "png_image_loader_fuzzer.h"

// libpng's longjmp-based error path walks Windows x64 .pdata; catch any escaping
// SEH (missing unwind info etc.) as a C++ exception so the fuzz loop survives.
#if defined(_WIN32) && defined(_MSC_VER)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <eh.h>
#include <windows.h>

namespace {
class SEHException {
public:
    explicit SEHException(unsigned int c) noexcept : code(c)
    {}
    unsigned int code;
};

void TranslateSE(unsigned int code, EXCEPTION_POINTERS*)
{
    throw SEHException(code);
}

struct SETranslatorScope {
    _se_translator_function previous;
    SETranslatorScope() noexcept : previous(_set_se_translator(&TranslateSE))
    {}
    ~SETranslatorScope() noexcept
    {
        _set_se_translator(previous);
    }
};
}  // namespace
#endif

using namespace BASE_NS;
using namespace CORE_NS;

namespace {

// Every input is run through every mode, with mode/flag selection derived from
// a hash so libFuzzer's coverage attribution stays deterministic; the whole
// input is passed as PNG bytes (never consumed as mode/flag prefix) so real
// seeds keep their signature.
constexpr size_t FUZZ_MIN_INPUT_SIZE = 0;
constexpr uint8_t FUZZ_CASE_COUNT = 9;

constexpr uint32_t MAX_FUZZ_DIM = 256;
constexpr uint32_t MAX_FUZZ_3D_ROWS = 16;
constexpr uint32_t MAX_FUZZ_3D_COLS = 16;

constexpr uint8_t PNG_SIGNATURE[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

void AppendBE32(std::vector<uint8_t>& out, uint32_t value)
{
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(value & 0xFF));
}

void AppendChunk(std::vector<uint8_t>& out, const char type[4], const uint8_t* payload, uint32_t payloadLen)
{
    AppendBE32(out, payloadLen);
    const size_t typeStart = out.size();
    out.insert(out.end(), type, type + 4);
    if (payloadLen > 0 && payload != nullptr) {
        out.insert(out.end(), payload, payload + payloadLen);
    }
    const uint32_t crc = static_cast<uint32_t>(crc32(0L, out.data() + typeStart, 4 + payloadLen));
    AppendBE32(out, crc);
}

// Synthetic PNG with fuzz-controlled IHDR fields + IDAT carrying fuzz bytes.
// The IDAT is almost always invalid deflate -- that's the point: drives libpng's
// error_handler -> longjmp path the loader's setjmp cleanup must survive.
bool BuildSyntheticPng(const uint8_t* fuzzPayload, size_t fuzzRemaining, std::vector<uint8_t>& out)
{
    FuzzConsumer fc(fuzzPayload, fuzzRemaining);
    uint16_t rawW = 0;
    uint16_t rawH = 0;
    uint8_t bitDepthByte = 0;
    uint8_t colorTypeByte = 0;
    uint8_t interlaceByte = 0;
    if (!fc.Consume(rawW) || !fc.Consume(rawH) || !fc.Consume(bitDepthByte) || !fc.Consume(colorTypeByte) ||
        !fc.Consume(interlaceByte)) {
        return false;
    }
    const uint32_t width = (rawW % MAX_FUZZ_DIM) + 1;
    const uint32_t height = (rawH % MAX_FUZZ_DIM) + 1;
    static const uint8_t bitDepths[] = {1, 2, 4, 8, 16};
    const uint8_t bitDepth = bitDepths[bitDepthByte % (sizeof(bitDepths) / sizeof(bitDepths[0]))];
    static const uint8_t colorTypes[] = {0, 2, 3, 4, 6};  // gray, rgb, palette, gray+alpha, rgba
    const uint8_t colorType = colorTypes[colorTypeByte % (sizeof(colorTypes) / sizeof(colorTypes[0]))];
    const uint8_t interlace = (interlaceByte & 1) ? 1 : 0;

    out.clear();
    out.reserve(64 + fuzzRemaining);
    out.insert(out.end(), PNG_SIGNATURE, PNG_SIGNATURE + sizeof(PNG_SIGNATURE));

    uint8_t ihdr[13];
    ihdr[0] = static_cast<uint8_t>((width >> 24) & 0xFF);
    ihdr[1] = static_cast<uint8_t>((width >> 16) & 0xFF);
    ihdr[2] = static_cast<uint8_t>((width >> 8) & 0xFF);
    ihdr[3] = static_cast<uint8_t>(width & 0xFF);
    ihdr[4] = static_cast<uint8_t>((height >> 24) & 0xFF);
    ihdr[5] = static_cast<uint8_t>((height >> 16) & 0xFF);
    ihdr[6] = static_cast<uint8_t>((height >> 8) & 0xFF);
    ihdr[7] = static_cast<uint8_t>(height & 0xFF);
    ihdr[8] = bitDepth;
    ihdr[9] = colorType;
    ihdr[10] = 0;  // compression method (only 0 is defined by spec)
    ihdr[11] = 0;  // filter method (only 0 is defined by spec)
    ihdr[12] = interlace;
    AppendChunk(out, "IHDR", ihdr, sizeof(ihdr));

    if (colorType == 3) {  // palette type needs a PLTE chunk
        uint8_t plte[3 * 4] = {255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 0};
        AppendChunk(out, "PLTE", plte, sizeof(plte));
    }

    const size_t tail = fc.Remaining();
    const uint8_t* tailPtr = fc.Tail();
    if (tail > 0 && tailPtr != nullptr) {
        AppendChunk(out, "IDAT", tailPtr, static_cast<uint32_t>(std::min(tail, size_t{8192})));
    } else {
        AppendChunk(out, "IDAT", nullptr, 0);
    }

    AppendChunk(out, "IEND", nullptr, 0);
    return true;
}

void ExerciseImage(const CORE_NS::IImageContainer& image)
{
    (void)image.GetImageDesc();
    auto data = image.GetData();
    (void)data.size();
    (void)image.GetBufferImageCopies();
}

// Stand-in IFile so we can drive Load(IFile&, ...) without the OHOS file manager.
// fakeLength > 0 lies to GetLength() (drives "File too big"); shortRead makes
// Read return 0 (drives "Reading file failed").
class MemoryFile final : public CORE_NS::IFile {
public:
    MemoryFile(const uint8_t* data, size_t size, uint64_t fakeLength = 0, bool shortRead = false)
        : data_(data), size_(size), fakeLength_(fakeLength), shortRead_(shortRead)
    {}
    Mode GetMode() const override
    {
        return Mode::READ_ONLY;
    }
    void Close() override
    {
        pos_ = 0;
    }
    uint64_t Read(void* buffer, uint64_t count) override
    {
        if (shortRead_) {
            return 0;
        }
        if (!buffer || count == 0 || pos_ >= size_) {
            return 0;
        }
        const uint64_t remaining = static_cast<uint64_t>(size_ - pos_);
        const uint64_t n = (count < remaining) ? count : remaining;
        std::memcpy(buffer, data_ + pos_, static_cast<size_t>(n));
        pos_ += static_cast<size_t>(n);
        return n;
    }
    uint64_t Write(const void*, uint64_t) override
    {
        return 0;
    }
    uint64_t Append(const void*, uint64_t, uint64_t) override
    {
        return 0;
    }
    uint64_t GetLength() const override
    {
        return (fakeLength_ != 0) ? fakeLength_ : static_cast<uint64_t>(size_);
    }
    bool Seek(uint64_t offset) override
    {
        if (offset > size_) {
            return false;
        }
        pos_ = static_cast<size_t>(offset);
        return true;
    }
    uint64_t GetPosition() const override
    {
        return pos_;
    }

private:
    const uint8_t* data_;
    size_t size_;
    uint64_t fakeLength_;
    bool shortRead_;
    size_t pos_ = 0;
    void Destroy() override
    {}  // stack-allocated, never via heap-Ptr
};

void RunOneCase(CORE_NS::IImageLoaderManager::IImageLoader* loader, uint8_t selector, uint32_t loadFlags,
    uint32_t rowCount, uint32_t columnCount, const uint8_t* payload, size_t payloadSize)
{
    switch (selector % FUZZ_CASE_COUNT) {
        case 0: {
            const array_view<const uint8_t> input(payload, payloadSize);
            (void)loader->CanLoad(input);
            auto result = loader->Load(input, loadFlags);
            if (result.success && result.image) {
                ExerciseImage(*result.image);
            }
            break;
        }
        case 1: {
            MemoryFile file(payload, payloadSize);
            auto result = loader->Load(file, loadFlags);
            if (result.success && result.image) {
                ExerciseImage(*result.image);
            }
            break;
        }
        case 2: {
            MemoryFile file(payload, payloadSize);
            auto result = loader->Load(file, loadFlags, rowCount, columnCount);
            if (result.success && result.image) {
                ExerciseImage(*result.image);
            }
            break;
        }
        case 3: {
            MemoryFile file(payload, payloadSize);
            (void)loader->LoadAnimatedImage(file, loadFlags);
            break;
        }
        case 4: {
            const array_view<const uint8_t> input(payload, payloadSize);
            (void)loader->LoadAnimatedImage(input, loadFlags);
            break;
        }
        case 5: {
            const array_view<const uint8_t> input(payload, payloadSize);
            (void)loader->CanLoad(input);
            break;
        }
        case 6: {
            std::vector<uint8_t> png;
            if (BuildSyntheticPng(payload, payloadSize, png) && !png.empty()) {
                const array_view<const uint8_t> input(png.data(), png.size());
                (void)loader->CanLoad(input);
                auto result = loader->Load(input, loadFlags);
                if (result.success && result.image) {
                    ExerciseImage(*result.image);
                }
                MemoryFile synthFile(png.data(), png.size());
                auto result3d = loader->Load(synthFile, loadFlags, rowCount, columnCount);
                if (result3d.success && result3d.image) {
                    ExerciseImage(*result3d.image);
                }
            }
            break;
        }
        case 7: {
            // GetLength() > INT_MAX -> "File too big" rejection on both IFile overloads.
            const uint64_t fakeLen = static_cast<uint64_t>(std::numeric_limits<int>::max()) + 1ull;
            MemoryFile file(payload, payloadSize, fakeLen, false);
            (void)loader->Load(file, loadFlags);
            MemoryFile file3d(payload, payloadSize, fakeLen, false);
            (void)loader->Load(file3d, loadFlags, rowCount, columnCount);
            break;
        }
        case 8: {
            // Short-read -> "Reading file failed" on both IFile overloads.
            MemoryFile file(payload, payloadSize, 0, true);
            (void)loader->Load(file, loadFlags);
            MemoryFile file3d(payload, payloadSize, 0, true);
            (void)loader->Load(file3d, loadFlags, rowCount, columnCount);
            break;
        }
        default:
            break;
    }
}

constexpr uint32_t FLAG_VARIANTS[] = {
    0u,
    CORE_NS::IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT,
    CORE_NS::IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT,
    CORE_NS::IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA,
    CORE_NS::IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA |
        CORE_NS::IImageLoaderManager::IMAGE_LOADER_FORCE_LINEAR_RGB_BIT,
    // PREMULTIPLY + GRAYSCALE is the only combination that reaches PremultiplyAlpha's
    // channelCount != 4 && != 2 early return.
    CORE_NS::IImageLoaderManager::IMAGE_LOADER_PREMULTIPLY_ALPHA |
        CORE_NS::IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT,
    CORE_NS::IImageLoaderManager::IMAGE_LOADER_FLIP_VERTICALLY_BIT,
    CORE_NS::IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS,
    CORE_NS::IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY,
    CORE_NS::IImageLoaderManager::IMAGE_LOADER_METADATA_ONLY |
        CORE_NS::IImageLoaderManager::IMAGE_LOADER_FORCE_GRAYSCALE_BIT,
};
constexpr size_t FLAG_VARIANT_COUNT = sizeof(FLAG_VARIANTS) / sizeof(FLAG_VARIANTS[0]);

uint32_t HashInput(const uint8_t* data, size_t size)
{
    uint32_t h = 5381;
    const size_t lim = (size < 64) ? size : 64;
    for (size_t i = 0; i < lim; ++i) {
        h = ((h << 5) + h) ^ data[i];
    }
    return h;
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < FUZZ_MIN_INPUT_SIZE) {
        return 0;
    }

    auto loader = PNGPlugin::CreateImageLoaderPng(nullptr);
    if (!loader) {
        return 0;
    }
    (void)loader->GetSupportedTypes();

    const uint32_t hash = HashInput(data, size);
    const uint32_t loadFlags = FLAG_VARIANTS[hash % FLAG_VARIANT_COUNT];
    // 0 is included so Resolve3DTextureInfo's invalid-count rejection fires.
    const uint32_t rowCount = (hash >> 8) % (MAX_FUZZ_3D_ROWS + 1);
    const uint32_t columnCount = (hash >> 16) % (MAX_FUZZ_3D_COLS + 1);

    for (uint8_t mode = 0; mode < FUZZ_CASE_COUNT; ++mode) {
#if defined(_WIN32) && defined(_MSC_VER)
        SETranslatorScope sehScope;
        try {
            RunOneCase(loader.get(), mode, loadFlags, rowCount, columnCount, data, size);
        } catch (const SEHException&) {
        } catch (...) {
        }
#else
        RunOneCase(loader.get(), mode, loadFlags, rowCount, columnCount, data, size);
#endif
    }

    return 0;
}
