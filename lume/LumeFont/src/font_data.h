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

#ifndef FONT_DATA_H
#define FONT_DATA_H

#include <cfloat>
#include <cstdint>
#include <freetype/freetype.h>
#include <memory>
#include <shared_mutex>

#include <base/containers/string_view.h>
#include <font/intf_font.h>

#include "font_defs.h"

namespace {
constexpr float POINT_SIZE = 72.f;
}

FONT_BEGIN_NAMESPACE()
struct UiText;
struct RenderVertexLayout0;

class FaceData;

class FontData final : public std::enable_shared_from_this<FontData> {
public:
    // interface API
    static std::unique_ptr<FontData> CreateFromFaceData(const std::weak_ptr<FaceData>& face, bool sdf);

    FontData(const std::weak_ptr<FaceData>& face, FT_Size size, bool sdf);
    ~FontData();

    FontSize GetSize();

    FontMetrics GetMetrics();
    GlyphMetrics GetGlyphMetrics(uint32_t glyphIndex);
    GlyphInfo GetGlyphInfo(uint32_t glyphIndex);
    BASE_NS::vector<GlyphContour> GetGlyphContours(uint32_t glyphIndex);

    // measure string dimension as if text would have been drawn by DrawString
    BASE_NS::Math::Vec2 MeasureString(BASE_NS::string_view);

private:
    friend class FaceData;

    std::weak_ptr<FaceData> faceData_; // weak reference to face which owns this data
    FT_Size sizeData_;                 // scaling size used by this instance
    bool sdfData_;                     // sdf usage by this instance

    std::shared_mutex mutex_; // mutex for glyph cache access
    FontDefs::GlyphCache glyphCache_;

    const FontDefs::Glyph* GetOrCreateCachedGlyph(uint32_t glyphIndex);
    const FontDefs::Glyph* UpdateGlyph(uint32_t glyphIndex);
};

FONT_END_NAMESPACE()
#endif // FONT_DATA_H
