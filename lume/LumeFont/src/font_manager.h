/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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


#ifndef UTIL__FONT_MANAGER_H
#define UTIL__FONT_MANAGER_H

#include <atomic>
#include <ft2build.h>
#include <memory>
#include <shared_mutex>
#include FT_FREETYPE_H
#include <freetype/ftglyph.h>

#include <base/containers/unordered_map.h>
#include <font/intf_font_manager.h>
#include <render/resource_handle.h>

#include "font_defs.h"

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
RENDER_END_NAMESPACE()

FONT_BEGIN_NAMESPACE()
class FontBuffer;

class FontManager final : public IFontManager {
public:
    explicit FontManager(RENDER_NS::IRenderContext& context);
    ~FontManager();

    FontManager(FontManager const&) = delete;
    FontManager& operator=(FontManager const&) = delete;

    // IFontManager
    uint32_t GetGlyphIndex(const TypeFace&, uint32_t code) override;

    BASE_NS::array_view<const TypeFace> GetTypeFaces() const override;
    inline const TypeFace* GetTypeFace(BASE_NS::string_view name) override
    {
        return GetTypeFace(name, nullptr);
    }
    const TypeFace* GetTypeFace(BASE_NS::string_view name, BASE_NS::string_view styleName) override;
    BASE_NS::vector<TypeFace> GetTypeFaces(BASE_NS::string_view filePath) override;
    BASE_NS::vector<TypeFace> GetTypeFaces(BASE_NS::array_view<const BASE_NS::string_view> lookupDirs) override;

    IFont::Ptr CreateFont(const TypeFace& typeface) override;
    IFont::Ptr CreateFontFromMemory(const TypeFace& typeface, const BASE_NS::vector<uint8_t>& fontData) override;

    void FlushCaches() override;

    void UploadPending() override;

    // IInterface
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

    void Gc(uint64_t uid, FontBuffer* data);

    bool IsFont(BASE_NS::string_view uri);

    FT_Face OpenFtFace(BASE_NS::array_view<const uint8_t> buf, FT_Long index) const;

    // Called by FontData to request atlas slots
    /*
     * fit glyph to atlas
     *
     * @ret: -1 failure; 0 success; 1 atlas reset
     *
     */
    int UpdateAtlas(FontDefs::Glyph& glyph, const FT_Bitmap&, bool inColor);

    /* atlas partitioning:
     *
     *  | 10 px | 24 px | 12 px | NN px | (sizes are for example)
     *  +-------+-------+-------+-------+
     *  | A     | a     | F     | E     | <== row 0
     *  | a     | C     | f     | D     | <== row 1
     *    ...     ...     ...     ...     <== row N
     *    ...     ...     ...     ...
     *    ...     ...     ...     ...
     *
     * Atlas info contains info on free width left and list of columns which have info on their width and free height
     * left for that column.
     *
     * Filling algorithm works like this:
     *  - checks if the color formats match (mono vs color font) and if not skips that atlas
     *  - goes through all columns looking for an exact width match with enough free height left and uses that if found
     *  - simultaneously keeps track of the closest width column that has enough free height left
     *  - if no exact fit is found, checks if the closes width fit is close enough (GLYPH_FIT_THRESHOLD pixels) and uses
     *    that
     *  - if no close enough match is found, and atlas has enough free width left, creates a new column and puts the
     *    glyph as first element there
     *  - if none of the above conditions are fullfilled, creates a new atlas texture and creates a new column and puts
     *    the glyph there
     *
     * In pseudo code:
     * for ( atlas in atlases ) {
     *     if ( atlas format is glyph format ) {
     *         for ( column in atlas columns ) {
     *             if ( column has glyph height free and is wider or equal to glyph width ) {
     *                 if ( column width exactly glyph width ) {
     *                     insert glyph to this column and return
     *                 } else {
     *                     update closest column width
     *                 }
     *             }
     *         }
     *         if ( closest column width is at most GLYPH_FIT_THRESHOLD pixels ) {
     *                  insert glyph to closest width column and return
     *         }
     *         if ( atlas has glyph width free left ) {
     *             allocate new column, insert the glyph and return
     *         }
     *     }
     *  }
     *  allocate new atlas, allocate first column, insert glyph and return
     *
     *  Initially there are no atlases available so the first glyph creates one as it falls to the end right away. Same
     * happens when color format changes for the first time.
     */
    struct ColumnHeader {
        uint16_t colWidth;
        uint16_t heightLeft;
    };
    struct PendingGlyph {
        uint16_t posX;
        uint16_t posY;
        uint16_t width;
        uint16_t height;
        BASE_NS::vector<uint8_t> data;
    };
    struct AtlasTexture {
        RENDER_NS::RenderHandleReference handle;
        BASE_NS::vector<ColumnHeader> columns;
        BASE_NS::vector<PendingGlyph> pending;
        uint32_t widthLeft {};
        bool inColor {};
#if defined(FONT_VALIDATION_ENABLED) && (FONT_VALIDATION_ENABLED)
        BASE_NS::string name;
#endif
    };

    AtlasTexture* GetAtlas(size_t index)
    {
        std::shared_lock readerLock(atlasMutex_);

        if (index >= atlasTextures_.size()) {
            return nullptr;
        }
        return &atlasTextures_[index];
    }

private:
    bool OpenFtFace(BASE_NS::string_view uri, FT_Long index, FT_Face* face);

    void GetTypeFacesByFile(BASE_NS::vector<TypeFace>& typeFaces, BASE_NS::string_view path);
    void GetTypeFacesByDir(BASE_NS::vector<TypeFace>& typeFaces, BASE_NS::string_view path);

    std::shared_ptr<FontBuffer> CreateFontBuffer(const TypeFace& typeFace);

    // Atlas manager
    AtlasTexture* CreateAtlasTexture(bool color);
    void AddGlyphToColumn(
        FontDefs::Glyph& glyph, size_t atlasIndex, ColumnHeader& hdr, const FT_Bitmap& bitmap, uint32_t columnX);

    std::atomic_uint32_t refcnt_ { 0 };

    RENDER_NS::IRenderContext& renderContext_;

    FT_Library fontLib_;

    std::shared_mutex fontBuffersMutex_; // mutex for file data cache access
    BASE_NS::unordered_map<uint64_t, std::weak_ptr<FontBuffer>> fontBuffers_;
    BASE_NS::vector<TypeFace> typeFaces_;

    std::shared_mutex atlasMutex_; // mutex for atlas texture access
    BASE_NS::vector<AtlasTexture> atlasTextures_;
};

FONT_END_NAMESPACE()

#endif // UTIL__FONT_MANAGER_H
