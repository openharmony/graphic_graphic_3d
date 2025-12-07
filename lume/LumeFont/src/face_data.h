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

#ifndef FACE_DATA_H
#define FACE_DATA_H

#include <ft2build.h>
#include <memory>
#include <mutex>
#include <shared_mutex>

#include FT_FREETYPE_H
#include <freetype/ftglyph.h>

#include <base/containers/vector.h>
#include <font/namespace.h>

#include "font_defs.h"

FONT_BEGIN_NAMESPACE()
class FontData;
class FontManager;
class FontBuffer;

class FaceData final : public std::enable_shared_from_this<FaceData> {
public:
    FaceData(std::shared_ptr<FontBuffer> buf, FT_Face ftFace);
    ~FaceData();
    FontManager& GetFontManager() const;
    FontData* CreateFontData(float sizeInPt, uint16_t xDpi, uint16_t yDpi, bool sdf);

    int UpdateGlyph(bool sdf, FT_Size ftSize, uint32_t glyphIndex, FontDefs::Glyph& glyph);

    uint32_t GetGlyphIndex(uint32_t code);

private:
    friend class FontBuffer;
    friend class FontData;
    friend class Font;

    std::shared_ptr<FontBuffer> fontBuffer_; // contains the data from font file

    std::shared_mutex mutex_;
    FT_Face face_; // face data
    struct Data {
        int64_t pixelSize;
        std::unique_ptr<FontData> fontData;
    };
    BASE_NS::vector<Data> datas_;
};
FONT_END_NAMESPACE()

#endif // FACE_DATA_H
