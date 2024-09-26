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

#include "font.h"

#include <cfloat>

#include "face_data.h"
#include "font_buffer.h"
#include "font_data.h"

using namespace BASE_NS;

FONT_BEGIN_NAMESPACE()
Font::Font(std::shared_ptr<FaceData>&& faceData) : faceData_(faceData) {}

void Font::SetSize(FontSize sizeInPt)
{
    this->fontSize_ = sizeInPt;
    this->data_ = nullptr;
}

FontSize Font::GetSize()
{
    return fontSize_;
}

void Font::SetDpi(uint16_t x, uint16_t y)
{
    xDpi_ = x;
    yDpi_ = y;
    this->data_ = nullptr;
}
void Font::GetDpi(uint16_t& x, uint16_t& y)
{
    x = xDpi_;
    y = yDpi_;
}

FontData* Font::GetData()
{
    if (!data_) {
        data_ = faceData_->CreateFontData(fontSize_, xDpi_, yDpi_);
    }
    return data_;
}

BASE_NS::array_view<uint8_t> Font::GetFontData()
{
    return faceData_->fontBuffer_->bytes_;
}

void Font::DrawGlyphs(BASE_NS::array_view<const GlyphInfo> glyphs, const FontDefs::RenderData& renderData)
{
    GetData()->DrawGlyphs(glyphs, renderData);
}

void Font::DrawString(const BASE_NS::string_view string, const FontDefs::RenderData& renderData)
{
    GetData()->DrawString(string, renderData);
}

BASE_NS::Math::Vec2 Font::MeasureString(const BASE_NS::string_view string)
{
    return GetData()->MeasureString(string);
}

FontMetrics Font::GetMetrics()
{
    return GetData()->GetMetrics();
}

GlyphMetrics Font::GetGlyphMetrics(uint32_t glyphIndex)
{
    return GetData()->GetGlyphMetrics(glyphIndex);
}

GlyphInfo Font::GetGlyphInfo(uint32_t glyphIndex)
{
    return GetData()->GetGlyphInfo(glyphIndex);
}

uint32_t Font::GetGlyphIndex(uint32_t code)
{
    return faceData_->GetGlyphIndex(code);
}

const CORE_NS::IInterface* Font::GetInterface(const BASE_NS::Uid& uid) const
{
    if (uid == CORE_NS::IInterface::UID) {
        return this;
    }
    if (uid == IFont::UID) {
        return this;
    }
    return nullptr;
}

CORE_NS::IInterface* Font::GetInterface(const BASE_NS::Uid& uid)
{
    if (uid == CORE_NS::IInterface::UID) {
        return this;
    }
    if (uid == IFont::UID) {
        return this;
    }
    return nullptr;
}

void Font::Ref()
{
    refcnt_.fetch_add(1, std::memory_order_relaxed);
}

void Font::Unref()
{
    if (refcnt_.fetch_sub(1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
}

FONT_END_NAMESPACE()
