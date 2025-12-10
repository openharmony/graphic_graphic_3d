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

#ifndef FONT_DEFS_H
#define FONT_DEFS_H

#include <cstdint>

#include <base/containers/unordered_map.h>
#include <base/math/matrix.h>
#include <core/log.h>
#include <font/namespace.h>

FONT_BEGIN_NAMESPACE()

#ifndef VERBOSE
#define CORE_LOG_N(...) ;
#endif
#ifndef CORE_LOG_N
#define CORE_LOG_N CORE_LOG_I
#endif

// class DrawBatcher;
// class IPaint;
using RenderDrawKey = uint64_t;

namespace FontDefs {
// implementation specific API
struct RenderData {
    float x;
    float y;
    BASE_NS::Math::Mat4X4 matrix;
    // DrawBatcher& drawBatcher;
    // const IPaint& paint;
    // RenderDrawKey renderDrawKey;
    // RENDER_NS::ScissorDesc scissor;
    // ClipData clipData;

    RenderData(const RenderData&) = delete;
    RenderData& operator=(const RenderData&) = delete;
};

struct IRect {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

struct AtlasSlot {
    IRect rect;
    uint16_t index;
};

struct Glyph {
    int16_t xMin;
    int16_t xMax;
    int16_t yMin;
    int16_t yMax;
    int16_t hlsb; // left side bearing
    int16_t htsb; // top side bearing
    int16_t hAdv;
    int16_t vlsb; // left side bearing
    int16_t vtsb; // top side bearing
    int16_t vAdv;
    AtlasSlot atlas;
};

using GlyphCache = BASE_NS::unordered_map<uint32_t, FontDefs::Glyph>;

constexpr uint32_t ATLAS_SIZE = 2048;

constexpr int ATLAS_ERROR = -1;
constexpr int ATLAS_OK = 0;
constexpr int ATLAS_RESET = 1;

constexpr uint8_t BORDER_WIDTH = 1u;
constexpr uint8_t BORDER_WIDTH_X2 = 2u * BORDER_WIDTH;

constexpr const int GLYPH_FIT_THRESHOLD = 4;

inline uint64_t rshift_u64(uint64_t val, uint8_t bits)
{
    return val >> bits;
}

inline uint64_t lshift_u64(uint64_t val, uint8_t bits)
{
    return val << bits;
}

inline uint32_t rshift_u32(uint32_t val, uint8_t bits)
{
    return val >> bits;
}

inline uint32_t lshift_u32(uint32_t val, uint8_t bits)
{
    return val << bits;
}

inline int32_t rshift_i32(int32_t val, uint8_t bits)
{
    if (val < 0) {
        return ~int32_t(~uint32_t(val) >> bits);
    }
    return int32_t(uint32_t(val) >> bits);
}

inline int32_t lshift_i32(int32_t val, uint8_t bits)
{
    return int32_t(uint32_t(val) << bits);
}

inline uint16_t lshift_u16(uint16_t val, uint8_t bits)
{
    return uint16_t(val << bits);
}

inline uint8_t GetStrength(uint64_t glyphKey)
{
    return rshift_u64(glyphKey, 48) & UINT8_MAX;
}

inline uint8_t GetSkewX(uint64_t glyphKey)
{
    return rshift_u64(glyphKey, 56) & UINT8_MAX;
}

inline int32_t IntToFp26(int32_t val)
{
    // convert to 26.6 fractional pixels values used by freetype library
    return lshift_i32(val, 6);
}

inline int32_t Fp26ToInt(int32_t val)
{
    // convert from 26.6 fractional pixels values used by freetype library
    return rshift_i32(val, 6);
}
// font size in pixel
inline uint64_t MakeGlyphKey(float size, uint32_t idx)
{
    return (static_cast<uint64_t>(size) << 32) | idx;
}

// FT_Pos is a 26.6 fixed point value, 2^6 = 64
constexpr float FLOAT_DIV = 64.f;

inline int32_t FloatToFTPos(float x)
{
    return static_cast<int32_t>(x * FLOAT_DIV);
}

inline float FTPosToFloat(int32_t x)
{
    return static_cast<float>(x) / FLOAT_DIV;
}

// 13.3 fixed point value, 2^3 = 8
// valid range is -4096.0...4,095.875
constexpr float FLOAT16_DIV = 8.f;

inline float Int16ToFloat(int16_t x)
{
    return static_cast<float>(x) / FLOAT16_DIV;
}

inline int16_t FTPosToInt16(int32_t x)
{
#if defined(CORE2D_VALIDATION_ENABLED) && (CORE2D_VALIDATION_ENABLED)
    CORE_ASSERT((x >= (INT16_MIN * static_cast<int32_t>(FLOAT16_DIV))) &&
                (x <= (INT16_MAX * static_cast<int32_t>(FLOAT16_DIV))));
#endif
    return static_cast<int16_t>(x / static_cast<int32_t>(FLOAT_DIV / FLOAT16_DIV));
}

inline int32_t Int16ToFTPos(int16_t x)
{
    return static_cast<int32_t>(static_cast<int32_t>(x) * static_cast<int32_t>(FLOAT_DIV / FLOAT16_DIV));
}
} // namespace FontDefs

FONT_END_NAMESPACE()
#endif // FONT_DEFS_H
