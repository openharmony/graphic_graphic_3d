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

#ifndef UTIL__FONT_H
#define UTIL__FONT_H

#include <atomic>
#include <memory>

#include <font/intf_font_manager.h>

#include "font_defs.h"

FONT_BEGIN_NAMESPACE()
class FaceData;
class FontData;

class Font final : public IFont {
public:
    // interface API
    explicit Font(std::shared_ptr<FaceData>&& faceData);
    ~Font() = default;

    // IInterface
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

    // IFont
    // set current size in Points for all consequent font operations
    void SetSize(FontSize sizeInPt) override;
    // get current font size in Points
    FontSize GetSize() override;
    void SetDpi(uint16_t x, uint16_t y) override;
    void GetDpi(uint16_t& x, uint16_t& y) override;

    BASE_NS::array_view<uint8_t> GetFontData() override;

    FontMetrics GetMetrics() override;
    GlyphMetrics GetGlyphMetrics(uint32_t glyphIndex) override;
    GlyphInfo GetGlyphInfo(uint32_t glyphIndex) override;

    uint32_t GetGlyphIndex(uint32_t codepoint) override;

    // measure string dimension as if text would have been drawn by DrawString
    BASE_NS::Math::Vec2 MeasureString(const BASE_NS::string_view) override;

    // implementation specific API
    void DrawGlyphs(BASE_NS::array_view<const GlyphInfo>, const FontDefs::RenderData&);

    // This function draws UTF-8 string by positioning corresponding glyphs
    // based on their advances and kerning (if kerning is supported by font).
    // NOTE: this function does not perform any font shaping nor guarantees
    // accurate glyphs positioning.
    void DrawString(const BASE_NS::string_view, const FontDefs::RenderData&);

private:
    std::atomic_uint32_t refcnt_ { 0 };

    std::shared_ptr<FaceData> faceData_;
    FontData* data_ { nullptr };                 // FontData is owned by FaceData
    FontSize fontSize_ { DEFAULT_FONT_PT_SIZE }; // in PT
    uint16_t xDpi_ = DEFAULT_XDPI;
    uint16_t yDpi_ = DEFAULT_YDPI;

    FontData* GetData();
};
FONT_END_NAMESPACE()
#endif
