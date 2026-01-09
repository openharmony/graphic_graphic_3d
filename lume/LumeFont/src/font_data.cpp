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

#include "font_data.h"

#include <freetype/ftsizes.h>
#include <freetype/ftstroke.h>

#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/util/utf8_decode.h>
#include <render/resource_handle.h>

#include "face_data.h"
#include "font.h"
#include "font_manager.h"

using namespace BASE_NS;
using namespace RENDER_NS;

namespace {
FT_Pos CalcKerning(FT_Face face, uint32_t prevGlyphIndex, uint32_t glyphIndex)
{
    FT_Vector delta;
    FT_Get_Kerning(face, prevGlyphIndex, glyphIndex, FT_KERNING_UNFITTED, &delta);
    return delta.x;
}
} // namespace

FONT_BEGIN_NAMESPACE()
std::unique_ptr<FontData> FontData::CreateFromFaceData(const std::weak_ptr<FaceData>& face, bool sdf)
{
    auto faceFromWeak = face.lock();
    if (!faceFromWeak) {
        return {};
    }
    FT_Size size {};
    FT_Error err = FT_New_Size(faceFromWeak->face_, &size);
    if (err) {
        return {};
    }
    err = FT_Activate_Size(size);
    return std::make_unique<FontData>(face, size, sdf);
}

FontData::FontData(const std::weak_ptr<FaceData>& face, FT_Size size, bool sdf)
    : faceData_(face), sizeData_(size), sdfData_(sdf)
{}

FontData::~FontData()
{
    FT_Done_Size(sizeData_);
}

FontSize FontData::GetSize() const
{
    return sizeData_->metrics.y_ppem;
}

Math::Vec2 FontData::MeasureString(const string_view string)
{
    auto faceData = faceData_.lock();
    if (!faceData) {
        return {};
    }

    const char* str = string.data();
    const char* strEnd = string.data() + string.length();

    uint32_t prevGlyphIndex = 0;
    bool kerning = false; // FT_HAS_KERNING(face_);
    int32_t penX = 0;
    int16_t maxDescent = INT16_MAX;
    int16_t maxAscent = INT16_MIN;

    for (uint32_t code = 0; (str < strEnd) && (code = GetCharUtf8(&str), code);) {
        const uint32_t glyphIndex = FT_Get_Char_Index(faceData->face_, code);
        const FontDefs::Glyph* glyph = GetOrCreateCachedGlyph(glyphIndex);
        if (!glyph) {
            continue;
        }

        if (prevGlyphIndex && kerning) {
            penX += CalcKerning(faceData->face_, prevGlyphIndex, glyphIndex);
        }

        prevGlyphIndex = glyphIndex;

        penX += FontDefs::Int16ToFTPos(glyph->hAdv);
        if (maxDescent > glyph->yMin) {
            maxDescent = glyph->yMin;
        }
        if (maxAscent < glyph->yMax) {
            maxAscent = glyph->yMax;
        }
    }
    return { FontDefs::FTPosToFloat(penX),
        FontDefs::Int16ToFloat(int16_t(Math::clamp(maxAscent - maxDescent, int(INT16_MIN), int(INT16_MAX)))) };
}

FontMetrics FontData::GetMetrics()
{
    FontMetrics fontMetrics {};
    auto faceData = faceData_.lock();
    if (!faceData) {
        return {};
    }

    if (faceData->face_->units_per_EM == 0) {
        fontMetrics.ascent = -FontDefs::FTPosToFloat(sizeData_->metrics.ascender);
        fontMetrics.descent = -FontDefs::FTPosToFloat(sizeData_->metrics.descender);
        fontMetrics.height = FontDefs::FTPosToFloat(sizeData_->metrics.height);
        fontMetrics.leading = FontDefs::FTPosToFloat(
            sizeData_->metrics.height + (sizeData_->metrics.descender - sizeData_->metrics.ascender));
    } else {
        fontMetrics.ascent = static_cast<float>(-FontDefs::Fp26ToInt(sizeData_->metrics.ascender));
        fontMetrics.descent = static_cast<float>(-FontDefs::Fp26ToInt(sizeData_->metrics.descender));
        fontMetrics.height = static_cast<float>(FontDefs::Fp26ToInt(sizeData_->metrics.height));
        fontMetrics.leading = static_cast<float>(FontDefs::Fp26ToInt(
            sizeData_->metrics.height - sizeData_->metrics.ascender + sizeData_->metrics.descender));
    }

    uint32_t xIndex = faceData->GetGlyphIndex('x');
    if (xIndex) {
        GlyphMetrics glyphMetrics = GetGlyphMetrics(xIndex);
        fontMetrics.x_height = glyphMetrics.top - glyphMetrics.bottom;
    } else {
        // double check what to do if 'x' char is not found in face
        fontMetrics.x_height = -fontMetrics.ascent;
    }

    return fontMetrics;
}

GlyphMetrics FontData::GetGlyphMetrics(uint32_t glyphIndex)
{
    GlyphMetrics glyphMetrics = {};

    const FontDefs::Glyph* glyph = GetOrCreateCachedGlyph(glyphIndex);
    if (glyph) {
        glyphMetrics.left = FontDefs::Int16ToFloat(glyph->xMin);
        glyphMetrics.top = FontDefs::Int16ToFloat(glyph->yMax);
        glyphMetrics.right = FontDefs::Int16ToFloat(glyph->xMax);
        glyphMetrics.bottom = FontDefs::Int16ToFloat(glyph->yMin);

        glyphMetrics.advance = FontDefs::Int16ToFloat(glyph->hAdv);
        glyphMetrics.leftBearing = FontDefs::Int16ToFloat(glyph->hlsb);
        glyphMetrics.topBearing = FontDefs::Int16ToFloat(glyph->htsb);
    }
    return glyphMetrics;
}

BASE_NS::vector<GlyphContour> FontData::GetGlyphContours(uint32_t glyphIndex)
{
    BASE_NS::vector<GlyphContour> contours;

    const auto faceData = faceData_.lock();
    if (!faceData || !faceData->face_) {
        return contours;
    }

    if (FT_Load_Glyph(faceData->face_, glyphIndex, FT_LOAD_DEFAULT) != 0) {
        return contours;
    }

    const FT_Outline* outline = &faceData->face_->glyph->outline;
    if (outline->n_points <= 0) {
        return contours;
    }

    short contourStart = 0U;

    for (short ii = 0U; ii < outline->n_contours; ii++) {
        const short contourEnd = outline->contours[ii];
        GlyphContour contour;
        contour.points.reserve(static_cast<size_t>(contourEnd - contourStart));

        for (short jj = contourStart; jj <= contourEnd; jj++) {
            const FT_Vector& point = outline->points[jj];
            contour.points.emplace_back(FontDefs::FTPosToFloat(point.x), FontDefs::FTPosToFloat(point.y));
        }
        contours.push_back(BASE_NS::move(contour));
        contourStart = contourEnd + 1;
    }

    return contours;
}

GlyphInfo FontData::GetGlyphInfo(uint32_t glyphIndex)
{
    GlyphInfo glyphInfo = {};

    const FontDefs::Glyph* glyph = GetOrCreateCachedGlyph(glyphIndex);
    if (!glyph) {
        return glyphInfo;
    }
    auto faceData = faceData_.lock();
    if (!faceData) {
        return glyphInfo;
    }

    auto& fontMgr = faceData->GetFontManager();
    if (auto atlas = fontMgr.GetAtlas(glyph->atlas.index); atlas) {
        glyphInfo.atlas = atlas->handle;
    }

    glyphInfo.tl.x = glyph->atlas.rect.x;
    glyphInfo.tl.y = glyph->atlas.rect.y;
    glyphInfo.br.x = glyphInfo.tl.x + glyph->atlas.rect.w;
    glyphInfo.br.y = glyphInfo.tl.y + glyph->atlas.rect.h;
    glyphInfo.tl /= FontDefs::ATLAS_SIZE;
    glyphInfo.br /= FontDefs::ATLAS_SIZE;

    return glyphInfo;
}

const FontDefs::Glyph* FontData::GetOrCreateCachedGlyph(uint32_t glyphIndex)
{
    std::shared_lock readerLock(mutex_);

    auto glyphPos = glyphCache_.find(glyphIndex);
    if (glyphPos != glyphCache_.end()) {
        return &glyphPos->second;
    }

    readerLock.unlock();
    return UpdateGlyph(glyphIndex);
}

const FontDefs::Glyph* FontData::UpdateGlyph(uint32_t glyphIndex)
{
    std::lock_guard writerLock(mutex_);
    if (auto glyphPos = glyphCache_.find(glyphIndex); glyphPos != glyphCache_.end()) {
        return &glyphPos->second;
    }
    auto faceData = faceData_.lock();
    if (!faceData) {
        return {};
    }

    FontDefs::Glyph& glyph = glyphCache_[glyphIndex];
    if (faceData->UpdateGlyph(sdfData_, sizeData_, glyphIndex, glyph)) {
        return nullptr;
    }
    return &glyph;
}
FONT_END_NAMESPACE()
