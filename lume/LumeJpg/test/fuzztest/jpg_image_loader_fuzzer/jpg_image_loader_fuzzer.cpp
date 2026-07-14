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

/* Fuzz test for JPGPlugin::ImageLoaderJPG via <jpg/image_loader_jpg.h>. */

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

#include "fuzz_consumer.h"
#include "jpg/image_loader_jpg.h"
#include "jpg_image_loader_fuzzer.h"

// libjpeg-turbo's longjmp-based error path walks Windows x64 .pdata; catch any
// escaping SEH (missing unwind info etc.) as a C++ exception so the fuzz loop survives.
#if defined(_WIN32) && defined(_MSC_VER)
#ifndef NOMINMAX
#define NOMINMAX  // keep std::numeric_limits<>::max working through <windows.h>
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
// input is passed as JPEG bytes (never consumed as mode/flag prefix) so real
// seeds keep their SOI marker.
constexpr size_t FUZZ_MIN_INPUT_SIZE = 0;
constexpr uint8_t FUZZ_CASE_COUNT = 8;

constexpr uint32_t MAX_FUZZ_DIM = 4096;

constexpr uint8_t MARKER_PREFIX = 0xFF;
constexpr uint8_t MARKER_SOI = 0xD8;
constexpr uint8_t MARKER_SOF0 = 0xC0;
constexpr uint8_t MARKER_DQT = 0xDB;
constexpr uint8_t MARKER_DHT = 0xC4;
constexpr uint8_t MARKER_SOS = 0xDA;
constexpr uint8_t MARKER_EOI = 0xD9;
constexpr uint8_t MARKER_APP0 = 0xE0;

void AppendU16BE(std::vector<uint8_t>& out, uint16_t value)
{
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(value & 0xFF));
}

// Minimal baseline-JPEG stream with fuzz-controlled SOF0 width/height/precision/channels.
// Most variants are rejected by libjpeg-turbo; the point is to push fuzz values into the
// wrapper's MAX_IMAGE_EXTENT / channel guards that sit above libjpeg's own validation.
bool BuildSyntheticJpeg(const uint8_t* fuzzPayload, size_t fuzzRemaining, std::vector<uint8_t>& out)
{
    FuzzConsumer fc(fuzzPayload, fuzzRemaining);
    uint16_t rawW = 0;
    uint16_t rawH = 0;
    uint8_t precision = 8;
    uint8_t channels = 3;
    if (!fc.Consume(rawW) || !fc.Consume(rawH) || !fc.Consume(precision) || !fc.Consume(channels)) {
        return false;
    }
    const uint16_t width = static_cast<uint16_t>((rawW % MAX_FUZZ_DIM) + 1);
    const uint16_t height = static_cast<uint16_t>((rawH % MAX_FUZZ_DIM) + 1);
    const uint8_t prec = static_cast<uint8_t>((precision % 16) + 1);
    const uint8_t nComp = static_cast<uint8_t>((channels % 5) + 1);  // 5 is invalid -> error path

    out.clear();
    out.reserve(64 + fuzzRemaining);

    out.push_back(MARKER_PREFIX);
    out.push_back(MARKER_SOI);

    // JFIF APP0 so CanLoad's JFIF branch is reachable.
    out.push_back(MARKER_PREFIX);
    out.push_back(MARKER_APP0);
    AppendU16BE(out, 16);
    static const uint8_t jfif[] = {'J', 'F', 'I', 'F', 0x00, 0x01, 0x02, 0x00, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00};
    out.insert(out.end(), jfif, jfif + sizeof(jfif));

    out.push_back(MARKER_PREFIX);
    out.push_back(MARKER_SOF0);
    const uint16_t sof0Len = static_cast<uint16_t>(8 + 3 * nComp);
    AppendU16BE(out, sof0Len);
    out.push_back(prec);
    AppendU16BE(out, height);
    AppendU16BE(out, width);
    out.push_back(nComp);
    for (uint8_t c = 0; c < nComp; ++c) {
        out.push_back(static_cast<uint8_t>(c + 1));
        out.push_back(0x11);
        out.push_back(0x00);
    }

    // Whatever the fuzzer hands us tail-end -- DQT/DHT/SOS bytes -- mostly garbage,
    // which drives libjpeg-turbo's error_exit / longjmp path.
    size_t tail = fc.Remaining();
    if (tail > 0) {
        const uint8_t* p = fc.Tail();
        out.insert(out.end(), p, p + tail);
    }

    out.push_back(MARKER_PREFIX);
    out.push_back(MARKER_EOI);
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
// fakeLength > 0 lies to GetLength() (drives "File too big"); shortRead makes Read
// return 0 (drives "Reading file failed").
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
    const uint8_t* payload, size_t payloadSize)
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
            std::vector<uint8_t> jpg;
            if (BuildSyntheticJpeg(payload, payloadSize, jpg) && !jpg.empty()) {
                const array_view<const uint8_t> input(jpg.data(), jpg.size());
                (void)loader->CanLoad(input);
                auto result = loader->Load(input, loadFlags);
                if (result.success && result.image) {
                    ExerciseImage(*result.image);
                }
            }
            break;
        }
        case 2: {
            const array_view<const uint8_t> input(payload, payloadSize);
            (void)loader->CanLoad(input);
            break;
        }
        case 3: {
            MemoryFile file(payload, payloadSize);
            auto result = loader->Load(file, loadFlags);
            if (result.success && result.image) {
                ExerciseImage(*result.image);
            }
            break;
        }
        case 4: {
            MemoryFile file(payload, payloadSize);
            (void)loader->LoadAnimatedImage(file, loadFlags);
            break;
        }
        case 5: {
            const array_view<const uint8_t> input(payload, payloadSize);
            (void)loader->LoadAnimatedImage(input, loadFlags);
            break;
        }
        case 6: {
            // GetLength() > INT_MAX -> "File too big" rejection.
            const uint64_t fakeLen = static_cast<uint64_t>(std::numeric_limits<int>::max()) + 1ull;
            MemoryFile file(payload, payloadSize, fakeLen, false);
            (void)loader->Load(file, loadFlags);
            break;
        }
        case 7: {
            // Short-read -> "Reading file failed".
            MemoryFile file(payload, payloadSize, 0, true);
            (void)loader->Load(file, loadFlags);
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
    // METADATA_ONLY keeps cinfo.num_components raw, so a 2-component synthetic JPEG
    // reaches ResolveFormat's case 2u (unreachable via the live decode path which
    // converts 2/3-component sources to JCS_EXT_RGBA).
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

    auto loader = JPGPlugin::CreateImageLoaderJPG(nullptr);
    if (!loader) {
        return 0;
    }
    (void)loader->GetSupportedTypes();

    const uint32_t hash = HashInput(data, size);
    const uint32_t loadFlags = FLAG_VARIANTS[hash % FLAG_VARIANT_COUNT];

    for (uint8_t mode = 0; mode < FUZZ_CASE_COUNT; ++mode) {
#if defined(_WIN32) && defined(_MSC_VER)
        SETranslatorScope sehScope;
        try {
            RunOneCase(loader.get(), mode, loadFlags, data, size);
        } catch (const SEHException&) {
        } catch (...) {
        }
#else
        RunOneCase(loader.get(), mode, loadFlags, data, size);
#endif
    }

    return 0;
}
