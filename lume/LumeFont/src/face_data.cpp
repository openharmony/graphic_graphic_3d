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

#include "face_data.h"

#include <algorithm>
#include <freetype/ftbitmap.h>
#include <freetype/ftsizes.h>

#include <core/log.h>

#include "font_buffer.h"
#include "font_data.h"
#include "font_manager.h"

using namespace BASE_NS;

FONT_BEGIN_NAMESPACE()

using namespace FontDefs;

FaceData::FaceData(std::shared_ptr<FontBuffer> buf, FT_Face ftFace) : fontBuffer_(buf), face_(ftFace) {}

FaceData::~FaceData()
{
    {
        std::lock_guard writerLock(mutex_);
        // FontData must be destroyed first as it has a FT_Size which is used by FT_Face.
        datas_.clear();
        FT_Done_Face(face_);
    }
    fontBuffer_->Gc();
}

FontManager& FaceData::GetFontManager() const
{
    return *(fontBuffer_->fontManager);
}

FontData* FaceData::CreateFontData(float sizeInPt, uint16_t xDpi, uint16_t yDpi, bool sdf)
{
    const FT_Pos pixelSize26Dot6 = FloatToFTPos(sizeInPt * yDpi / 72.f);
    const int64_t pixelSize = Fp26ToInt(pixelSize26Dot6) | (sdf ? (1ll << 32) : 0);

    {
        std::shared_lock readerLock(mutex_);
        if (auto it = std::find_if(
                datas_.cbegin(), datas_.cend(), [pixelSize](const Data& data) { return data.pixelSize == pixelSize; });
            it != datas_.cend()) {
            return it->fontData.get();
        }
    }

    std::lock_guard writerLock(mutex_);
    if (auto it = std::find_if(
            datas_.cbegin(), datas_.cend(), [pixelSize](const Data& data) { return data.pixelSize == pixelSize; });
        it != datas_.cend()) {
        return it->fontData.get();
    }

    auto fontData = FontData::CreateFromFaceData(weak_from_this(), sdf);
    if (!fontData) {
        return nullptr;
    }

    FT_Error err = FT_Err_Ok;
    if (FT_IS_SCALABLE(face_)) {
        err = FT_Set_Char_Size(face_, 0, FloatToFTPos(sizeInPt), xDpi, yDpi);
    } else if (FT_HAS_FIXED_SIZES(face_)) {
        auto* sizes = face_->available_sizes;
        int closestIdx = 0;
        auto smallestDiff = std::abs(sizes[0].y_ppem - pixelSize26Dot6);
        if (smallestDiff > 0) {
            for (FT_Int i = 1; i < face_->num_fixed_sizes; i++) {
                auto diff = std::abs(sizes[i].y_ppem - pixelSize26Dot6);
                if (diff < smallestDiff) {
                    smallestDiff = diff;
                    closestIdx = i;
                    if (!smallestDiff) { // exact
                        break;
                    }
                }
            }
            if (smallestDiff) {
                CORE_LOG_N("use of closest match for bitmap font, request: %dpx , selected: %dpx", pixelSize26Dot6 >> 6,
                    sizes[closestIdx].y_ppem >> 6);
            }
        }
        err = FT_Select_Size(face_, closestIdx);
    } else {
        CORE_LOG_E("face not scallable and no bitmap size available");
    }

    if (err) {
        CORE_LOG_E("failed to select font face size: %d", err);
        return nullptr;
    }

    auto fontDataPtr = fontData.get();
    datas_.push_back(Data { pixelSize, std::move(fontData) });

    CORE_LOG_N("create FontData PT: %f dpi: %d (pix: %d, yppem: %d, h: %d) %p", sizeInPt, yDpi, pixelSize,
        fontDataPtr->sizeData_->metrics.y_ppem, fontDataPtr->sizeData_->metrics.height >> 6, this);

    return fontDataPtr;
}

int FaceData::UpdateGlyph(bool sdf, FT_Size ftSize, uint32_t glyphIndex, FontDefs::Glyph& glyph)
{
    std::lock_guard writerLock(mutex_);

    // if face's active size is different, try to activate this one
    if (face_->size != ftSize && FT_Activate_Size(ftSize)) {
        return FontDefs::ATLAS_ERROR;
    }

    FT_Int32 flags = FT_LOAD_RENDER;

    if (sdf == true) {
        flags |= FT_LOAD_TARGET_(FT_RENDER_MODE_SDF);
    } else if (FT_HAS_COLOR(face_)) {
        flags |= FT_LOAD_COLOR;
    }

    if (FT_Load_Glyph(face_, glyphIndex, flags)) {
        return FontDefs::ATLAS_ERROR;
    }

    FT_Glyph bmp;
    if (FT_Get_Glyph(face_->glyph, &bmp)) {
        return FontDefs::ATLAS_ERROR;
    }

    // sanity check
    if (bmp->format != FT_GLYPH_FORMAT_BITMAP) {
        FT_Done_Glyph(bmp);
        return FontDefs::ATLAS_ERROR;
    }

    FT_BBox bbox;
    FT_Glyph_Get_CBox(bmp, FT_GLYPH_BBOX_UNSCALED, &bbox);

    glyph.xMin = FontDefs::FTPosToInt16(bbox.xMin);
    glyph.xMax = FontDefs::FTPosToInt16(bbox.xMax);
    glyph.yMin = FontDefs::FTPosToInt16(bbox.yMin);
    glyph.yMax = FontDefs::FTPosToInt16(bbox.yMax);
    glyph.hlsb = FontDefs::FTPosToInt16(face_->glyph->metrics.horiBearingX);
    glyph.htsb = FontDefs::FTPosToInt16(face_->glyph->metrics.horiBearingY);
    glyph.hAdv = FontDefs::FTPosToInt16(face_->glyph->metrics.horiAdvance);

    if (FT_HAS_VERTICAL(face_)) {
        glyph.vlsb = FontDefs::FTPosToInt16(face_->glyph->metrics.vertBearingX);
        glyph.vtsb = FontDefs::FTPosToInt16(face_->glyph->metrics.vertBearingY);
        glyph.vAdv = FontDefs::FTPosToInt16(face_->glyph->metrics.vertAdvance);
    }

    glyph.atlas.rect.w = 0;
    bool isColor = false;
    bool needsConversion = false;
    switch (((FT_BitmapGlyph)bmp)->bitmap.pixel_mode) {
        case FT_PIXEL_MODE_MONO:
            needsConversion = true;
            break;

        case FT_PIXEL_MODE_GRAY:
            needsConversion = false;
            break;

        case FT_PIXEL_MODE_GRAY2:
            needsConversion = true;
            break;

        case FT_PIXEL_MODE_GRAY4:
            needsConversion = true;
            break;

        case FT_PIXEL_MODE_BGRA:
            needsConversion = false;
            isColor = true;
            break;

        case FT_PIXEL_MODE_NONE:
        case FT_PIXEL_MODE_LCD:
        case FT_PIXEL_MODE_LCD_V:
        case FT_PIXEL_MODE_MAX:
            CORE_LOG_E("unhandled pixel mode");
            return FontDefs::ATLAS_ERROR;
    }

    FT_Bitmap converted;
    if (needsConversion) {
        FT_Bitmap_Init(&converted);
        FT_Bitmap_Convert(bmp->library, &((FT_BitmapGlyph)bmp)->bitmap, &converted, 1);

        if (converted.num_grays == 2U) {
            // convert created a binary bitmap, expand to 0..255 range.
            std::transform(converted.buffer, converted.buffer + converted.pitch * converted.rows, converted.buffer,
                [](const uint8_t& c) { return (c) ? uint8_t(0xffU) : uint8_t(0x00U); });
        } else if (converted.num_grays == 4U) {
            // convert created a 2 bit bitmap, expand to 0..255 range.
            std::transform(converted.buffer, converted.buffer + converted.pitch * converted.rows, converted.buffer,
                [](const uint8_t& c) {
                    auto half = (c << 2U) | c;
                    return uint8_t((half << 4U) | half);
                });
        } else if (converted.num_grays == 16U) {
            // convert created a 4 bit bitmap, expand to 0..255 range.
            std::transform(converted.buffer, converted.buffer + converted.pitch * converted.rows, converted.buffer,
                [](const uint8_t& c) { return uint8_t((c << 4U) | c); });
        }
    }

    GetFontManager().UpdateAtlas(glyph, ((FT_BitmapGlyph)bmp)->bitmap, isColor);

    if (needsConversion) {
        FT_Bitmap_Done(bmp->library, &converted);
    }

    FT_Done_Glyph(bmp);
    return 0;
}

uint32_t FaceData::GetGlyphIndex(uint32_t utfCode)
{
    std::shared_lock readerLock(mutex_);

    return FT_Get_Char_Index(face_, utfCode);
}
FONT_END_NAMESPACE()
