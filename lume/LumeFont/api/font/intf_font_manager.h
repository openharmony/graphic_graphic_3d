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

#ifndef API_FONT_IFONT_MANAGER_H
#define API_FONT_IFONT_MANAGER_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <core/plugin/intf_interface.h>
#include <font/intf_font.h>
#include <font/namespace.h>

FONT_BEGIN_NAMESPACE()

enum class Weight : uint64_t {
    Invisible = 0,
    Thin = 100,
    ExtraLight = 200,
    Light = 300,
    Normal = 400,
    Medium = 500,
    SemiBold = 600,
    Bold = 700,
    ExtraBold = 800,
    Black = 900,
    ExtraBlack = 1000,
};

enum class Width : uint64_t {
    UltraCondensed = 500,
    ExtraCondensed = 625,
    Condensed = 750,
    SemiCondensed = 875,
    Normal = 1000,
    SemiExpanded = 1125,
    Expanded = 1250,
    ExtraExpanded = 1500,
    UltraExpanded = 2000,
};

enum class SlantType : uint64_t {
    Upright,
    Italic,
    Oblique,
};

union FontStyle {
    uint64_t bits;
    struct {
        Weight Weight : 10;
        uint64_t Slant : 7;
        uint64_t BackSlant : 1;
        SlantType SlantType : 2;
        Width Width : 11;
        uint64_t OpticalSize : 8;
        uint64_t Pad : 25;
    };

    FontStyle& operator=(uint64_t _bits)
    {
        bits = _bits;
        return *this;
    }
    inline constexpr operator uint64_t() const
    {
        return bits;
    }
    inline constexpr bool operator==(const FontStyle& style) const
    {
        return bits == uint64_t(style);
    }
};

static constexpr FontStyle Regular = { 0x73E800190 };
static constexpr FontStyle Bold = { 0x73E8002BC };
static constexpr FontStyle Italic = { 0x73E800190 | (1 << 18) };
static constexpr FontStyle Oblique = { 0x73E800190 | (1 << 19) };

/**
 * Font face identification structure.
 *
 * FontManager will build a TypeFace list from font files gathered in lookup directories.
 *
 */
struct TypeFace {
    BASE_NS::string path;      // font file path
    BASE_NS::string name;      // family name as defined in font file
    BASE_NS::string styleName; // style name as defined in font file
    uint64_t uid;              // physical PATH hash as map key for font buffers map
    long index;                // face index for quick access to face in font file
    FontStyle style;           //
};

class IFontManager : public CORE_NS::IInterface {
public:
    static constexpr BASE_NS::Uid UID { "0ee45da4-ef82-423b-88ff-c203dcc51a56" };
    using Ptr = BASE_NS::refcnt_ptr<IFontManager>;

    // check if provided typeface has a glyph with corresponding code;
    // return glyph's index or 0 for undefined character code ("missing glyph")
    // NOTE: this function will also update charmap index in given typeface
    // This function is useful for checking glyph's existence without creating
    // new font
    virtual uint32_t GetGlyphIndex(const TypeFace&, uint32_t code) = 0;

    /**
    return all typefaces from font files found in default lookup directories.
    */
    virtual BASE_NS::array_view<const TypeFace> GetTypeFaces() const = 0;
    /**
    return first matching typeface given a face name in default lookup directories.
    */
    virtual const TypeFace* GetTypeFace(BASE_NS::string_view name) = 0;
    /**
    return first matching typeface given a face and a style name in default lookup directories.
    */
    virtual const TypeFace* GetTypeFace(BASE_NS::string_view name, BASE_NS::string_view styleName) = 0;
    /**
    return all typefaces from provided font file path.
    */
    virtual BASE_NS::vector<TypeFace> GetTypeFaces(BASE_NS::string_view filePath) = 0;
    /**
    return all typefaces from font files found in provided lookup directories.
    */
    virtual BASE_NS::vector<TypeFace> GetTypeFaces(BASE_NS::array_view<const BASE_NS::string_view> lookupDirs) = 0;

    /**
    Create a new font for typeface given in argument.
    Note that font cache key is the typeface.uid.
    */
    virtual IFont::Ptr CreateFont(const TypeFace&) = 0;
    /**
    Create a new font object from font blob given in argument, TypeFace uid member must be pre-initialized with
    a value that uniquely identify this font.
    */
    virtual IFont::Ptr CreateFontFromMemory(const TypeFace&, const BASE_NS::vector<uint8_t>& fontData) = 0;

    // flush caches
    virtual void FlushCaches() = 0;

    /**
    Triggers an update for atlas textures. All pending changes will be copied to GPU memory.
    */
    virtual void UploadPending() = 0;

protected:
    IFontManager() = default;
    virtual ~IFontManager() = default;
    IFontManager(IFontManager const&) = delete;
    IFontManager& operator=(IFontManager const&) = delete;
};
FONT_END_NAMESPACE()

#endif // API_FONT_IFONT_MANAGER_H
