/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef API_FONT_IFONT_H
#define API_FONT_IFONT_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/refcnt_ptr.h>
#include <base/math/vector.h>
#include <core/plugin/intf_interface.h>
#include <font/namespace.h>
#include <render/resource_handle.h>
#include <base/containers/vector.h>

FONT_BEGIN_NAMESPACE()
using FontSize = float;

enum class FontMethod { RASTER = 0, SDF = 1, TEXT3D = 2 };

constexpr FontSize DEFAULT_FONT_PT_SIZE = 12.f;
constexpr uint16_t DEFAULT_XDPI = 72u;
constexpr uint16_t DEFAULT_YDPI = 72u;

struct GlyphInfo {
    RENDER_NS::RenderHandleReference atlas;
    BASE_NS::Math::Vec2 tl;
    BASE_NS::Math::Vec2 br;
};

struct GlyphMetrics {
    float advance;
    // bounds
    float left;
    float top;
    float right;
    float bottom;
    float leftBearing;
    float topBearing;
};

struct FontMetrics {
    float ascent;
    float descent;
    /** Line height, distance between two consecutive baselines, to get
     * the global font height, compute: descent - ascent.
     */
    float height;
    /** Line Gap, difference between line height and sum of ascender and descender */
    float leading;
    float x_height;
};

struct GlyphContour {
    BASE_NS::vector<BASE_NS::Math::Vec2> points;
};

/** Fonts interface.
 *
 */
class IFont : public CORE_NS::IInterface {
public:
    static constexpr BASE_NS::Uid UID { "138e9acd-3ee1-4b75-a169-c7724f63b061" };
    using Ptr = BASE_NS::refcnt_ptr<IFont>;

    /** Set current font size in point units. */
    virtual void SetSize(FontSize) = 0;
    /** Get current font size in point units. */
    virtual FontSize GetSize() = 0;

    /** Set current font method. */
    virtual void SetMethod(FontMethod) = 0;
    /** Get current font method. */
    virtual FontMethod GetMethod() = 0;

    /** Set current DPI for direct calls to font measure and draw api. */
    virtual void SetDpi(uint16_t x, uint16_t y) = 0;
    /** Get current DPI of this font.*/
    virtual void GetDpi(uint16_t& x, uint16_t& y) = 0;

    /** Get metrics of the font. */
    virtual FontMetrics GetMetrics() = 0;
    /** Get metrics of the glyph at given index. */
    virtual GlyphMetrics GetGlyphMetrics(uint32_t glyphIndex) = 0;
    virtual GlyphInfo GetGlyphInfo(uint32_t glyphIndex) = 0;
    virtual BASE_NS::vector<GlyphContour> GetGlyphContours(uint32_t glyphIndex) = 0;

    /** Convert character code to glyph index.
     * @return glyph's index or 0 if requested glyph is not found.
     */
    virtual uint32_t GetGlyphIndex(uint32_t codepoint) = 0;

    /** This function returns dimensions of UTF-8 string by positioning glyphs based on their advances and kerning (if
     * kerning is supported by font.
     */
    virtual BASE_NS::Math::Vec2 MeasureString(const BASE_NS::string_view) = 0;

    virtual BASE_NS::array_view<uint8_t> GetFontData() = 0;

protected:
    IFont() = default;
    virtual ~IFont() = default;
    IFont(const IFont&) = delete;
    IFont& operator=(const IFont&) = delete;
};
FONT_END_NAMESPACE()
#endif // API_FONT_IFONT_H
