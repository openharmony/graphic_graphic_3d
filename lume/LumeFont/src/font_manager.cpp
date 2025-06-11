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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "font_manager.h"

#include <algorithm>
#include <freetype/ftsizes.h>
#include <freetype/tttables.h>
#include <numeric>

#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>

#include "face_data.h"
#include "font.h"
#include "font_buffer.h"

using namespace BASE_NS;
using namespace BASE_NS::Math;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace FONT_NS::FontDefs;

FONT_BEGIN_NAMESPACE()

namespace {
string GetErrorString()
{
    // Visual Studio lacks strerrorlen_s so need to define a fixed "large enough" buffer
    char buffer[64u];
#ifdef _WIN32
    strerror_s(buffer, sizeof(buffer), errno);
#else
    strerror_r(errno, buffer, sizeof(buffer));
#endif
    return string(buffer);
}

constexpr string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";

constexpr Width widths_os2_match[] = { Width(0), Width::UltraCondensed, Width::ExtraCondensed, Width::Condensed,
    Width::SemiCondensed, Width::Normal, Width::SemiExpanded, Width::Expanded, Width::ExtraExpanded,
    Width::UltraExpanded };

TypeFace InitTypeFace(FT_Face face, string_view path, string_view filename)
{
    TypeFace typeFace {
        string(path),            // font file path
        face->family_name,       // family name as defined in font file
        face->style_name,        // style name as defined in font file
        BASE_NS::hash(filename), // physical PATH hash as map key for font buffers map
        face->face_index,        // face index for quick access to face in font file
        Regular,                 // style
    };
    const TT_OS2* tblOS2 = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(face, FT_SFNT_OS2));
    if (tblOS2) {
        typeFace.style.Weight = static_cast<Weight>(tblOS2->usWeightClass);
        typeFace.style.Width = widths_os2_match[tblOS2->usWidthClass];
    } else {
        if (face->style_flags & FT_STYLE_FLAG_BOLD) {
            typeFace.style.Weight = Weight::Bold;
        }
    }
    if (face->style_flags & FT_STYLE_FLAG_ITALIC) {
        typeFace.style.SlantType = SlantType::Italic;
    }
    return typeFace;
}
} // namespace

FontManager::FontManager(IRenderContext& context) : renderContext_(context)
{
    if (FT_Init_FreeType(&fontLib_)) {
        CORE_LOG_E("failed to init freetype library: %s", GetErrorString().c_str());
    }

    GetTypeFacesByDir(typeFaces_, "fonts://");
}

FontManager::~FontManager()
{
    FT_Done_FreeType(fontLib_);
    fontLib_ = nullptr;
}

array_view<const TypeFace> FontManager::GetTypeFaces() const
{
    return typeFaces_;
}

const TypeFace* FontManager::GetTypeFace(string_view name, string_view styleName)
{
    for (auto& typeFace : typeFaces_) {
        if (typeFace.name == name) {
            if (styleName.empty() || typeFace.styleName == styleName) {
                return &typeFace;
            }
        }
    }
    return nullptr;
}

vector<TypeFace> FontManager::GetTypeFaces(const string_view filePath)
{
    vector<TypeFace> typeFaces;
    GetTypeFacesByFile(typeFaces, filePath);
    return typeFaces;
}

vector<TypeFace> FontManager::GetTypeFaces(array_view<const string_view> lookupDirs)
{
    vector<TypeFace> typeFaces;
    for (const auto& dir : lookupDirs) {
        GetTypeFacesByDir(typeFaces, dir);
    }
    return typeFaces;
}

std::shared_ptr<FontBuffer> FontManager::CreateFontBuffer(const TypeFace& typeFace)
{
    {
        std::shared_lock readerLock(fontBuffersMutex_);

        if (auto it = fontBuffers_.find(typeFace.uid); it != fontBuffers_.end()) {
            if (auto fromWeak = it->second.lock()) {
                return fromWeak;
            }
        }
    }

    std::lock_guard writerLock(fontBuffersMutex_);

    if (auto it = fontBuffers_.find(typeFace.uid); it != fontBuffers_.end()) {
        if (auto fromWeak = it->second.lock()) {
            return fromWeak;
        }
    }

    auto& fileManager = renderContext_.GetEngine().GetFileManager();
    IFile::Ptr file = fileManager.OpenFile(typeFace.path);
    if (file == nullptr) {
        CORE_LOG_E("failed to open font %s | %s", typeFace.path.data(), GetErrorString().c_str());
        return nullptr;
    }

    const size_t len = static_cast<size_t>(file->GetLength());
    vector<uint8_t> bytes;
    bytes.resize(len);

    if (len != file->Read(bytes.data(), bytes.size())) {
        CORE_LOG_E("failed to read %zu bytes from %s | %s", len, typeFace.path.data(), GetErrorString().c_str());
        return nullptr;
    }

    auto fb = std::make_shared<FontBuffer>(this, typeFace.uid, bytes);

    fontBuffers_.insert({ typeFace.uid, fb });
    CORE_LOG_N("create FontBuffer");

    return fb;
}

IFont::Ptr FontManager::CreateFont(const TypeFace& typeFace)
{
    std::shared_ptr fontBuff = CreateFontBuffer(typeFace);
    if (!fontBuff) {
        return nullptr;
    }
    std::shared_ptr<FaceData> face = fontBuff->CreateFace(typeFace.index);
    if (!face) {
        return nullptr;
    }
    Font::Ptr font = Font::Ptr(new Font(std::move(face)));

    return font;
}

IFont::Ptr FontManager::CreateFontFromMemory(const TypeFace& typeFace, const BASE_NS::vector<uint8_t>& buffer)
{
    std::shared_ptr<FontBuffer> fontBuff;
    {
        std::shared_lock readerLock(fontBuffersMutex_);
        if (auto it = fontBuffers_.find(typeFace.uid); it != fontBuffers_.end()) {
            fontBuff = it->second.lock();
        }
    }
    if (!fontBuff) {
        std::lock_guard writerLock(fontBuffersMutex_);
        if (auto it = fontBuffers_.find(typeFace.uid); it != fontBuffers_.end()) {
            fontBuff = it->second.lock();
        }
        if (!fontBuff) {
            fontBuff = std::make_shared<FontBuffer>(this, typeFace.uid, buffer);
            fontBuffers_.insert({ typeFace.uid, fontBuff });
        }
    }
    if (!fontBuff) {
        return {};
    }

    return Font::Ptr(new Font(fontBuff->CreateFace(typeFace.index)));
}

uint32_t FontManager::GetGlyphIndex(const TypeFace& typeFace, uint32_t code)
{
    auto fontBuff = CreateFontBuffer(typeFace);
    if (!fontBuff) {
        return 0;
    }
    auto face = fontBuff->CreateFace(typeFace.index);
    if (!face) {
        return 0;
    }
    return face->GetGlyphIndex(code);
}

void FontManager::FlushCaches()
{
    UploadPending();
    std::lock_guard writerLock(atlasMutex_);
    atlasTextures_.clear();
    CORE_LOG_N("atlas textures flush");
}

void FontManager::GetTypeFacesByFile(vector<TypeFace>& typeFaces, string_view path)
{
    auto& fileManager = renderContext_.GetEngine().GetFileManager();

    IFile::Ptr file = fileManager.OpenFile(path);
    if (file == nullptr) {
        CORE_LOG_E("failed to open font %s | %s", path.data(), GetErrorString().c_str());
        return;
    }

    auto entry = fileManager.GetEntry(path);
    const size_t len = static_cast<size_t>(file->GetLength());
    vector<uint8_t> buf(len);

    if (len != file->Read(buf.data(), buf.size())) {
        CORE_LOG_E("failed to read %zu bytes from %s | %s", len, path.data(), GetErrorString().c_str());
        return;
    }

    if (buf.empty()) {
        return;
    }

    FT_Long numFaces = 0;
    FT_Long numInstances = 0;
    FT_Long faceIndex = 0;
    FT_Long instanceIndex = 0;

    constexpr uint8_t INSTANCE_SHIFT = 16u;

    do {
        FT_Long faceId = (instanceIndex << INSTANCE_SHIFT) + faceIndex;
        auto face = OpenFtFace(buf, faceId);
        // lume api is utf8, unicode is the default charmap with freetype.
        if (face && face->charmap) {
            typeFaces.push_back(InitTypeFace(face, path, entry.name));

            numFaces = face->num_faces;
            numInstances = face->style_flags >> INSTANCE_SHIFT;

            CORE_LOG_N("%s", path.data());
            CORE_LOG_N("  number of faces:     %ld", numFaces);
            CORE_LOG_N("  number of instances: %ld", numInstances);

            if (instanceIndex < numInstances) {
                ++instanceIndex;
            } else {
                ++faceIndex;
                instanceIndex = 0;
            }
            FT_Done_Face(face);
        } else {
            CORE_LOG_W("failed to create face: %s", path.data());
        }
    } while (faceIndex < numFaces);
}

void FontManager::GetTypeFacesByDir(vector<TypeFace>& typeFaces, string_view path)
{
    auto& fileManager = renderContext_.GetEngine().GetFileManager();
    IDirectory::Ptr dir = fileManager.OpenDirectory(path);
    if (!dir) {
        return;
    }

    vector<IDirectory::Entry> const files = dir->GetEntries();
    CORE_LOG_N("check dir '%s' with %zu entries", string(path).c_str(), files.size());

    for (auto const& file : files) {
        if (file.type == IDirectory::Entry::Type::FILE) {
            auto fontUri = path + file.name;
            if (IsFont(fontUri)) {
                GetTypeFacesByFile(typeFaces, fontUri);
            }
        }
    }
}

/* Functions for streaming integration with freetype.
 * (used to allow not reading the whole file just to check if it's a supported font file).
 */
extern "C" {
static unsigned long FtRead(FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count)
{
    unsigned long result = 0;
    auto* file = reinterpret_cast<CORE_NS::IFile*>(stream->descriptor.pointer);
    if (!file) {
        return 0;
    }

    auto seekResult = file->Seek(offset);
    // Note: If count == 0, return the status of the seek operation (non-zero indicates an error).
    if (count == 0) {
        return !seekResult;
    }

    if (!seekResult) {
        // Seek failed.
        return 0;
    }
    result = static_cast<unsigned long>(file->Read(buffer, count));
    return result;
}

static void FtClose(FT_Stream stream)
{
    auto* file = reinterpret_cast<CORE_NS::IFile*>(stream->descriptor.pointer);
    if (file) {
        file->Close();
    }
}
} // extern "C"

namespace {
class FtFileReader {
public:
    FtFileReader(CORE_NS::IFileManager& fileManager, BASE_NS::string_view uri) : fileManager_(fileManager), uri_(uri) {}
    ~FtFileReader() {}

    FT_Stream Open()
    {
        file_ = fileManager_.OpenFile(uri_);
        if (!file_) {
            return {};
        }

        stream_.base = nullptr;
        stream_.size = static_cast<unsigned long>(file_->GetLength());
        stream_.pos = 0;
        stream_.descriptor.pointer = file_.get();
        stream_.pathname.pointer = (void*)uri_.data();
        stream_.read = FtRead;
        stream_.close = FtClose;

        return &stream_;
    }

private:
    CORE_NS::IFileManager& fileManager_;
    BASE_NS::string uri_;
    CORE_NS::IFile::Ptr file_ {};
    FT_StreamRec stream_ {};
};
} // namespace

bool FontManager::IsFont(BASE_NS::string_view uri)
{
    // FT_Open_Face and its siblings can be used to quickly check whether the font format of a given font resource
    // is supported by FreeType. In general, if the face_index argument is negative, the function's return value is
    // 0 if the font format is recognized, or non-zero otherwise.
    return OpenFtFace(uri, -1, nullptr);
}

bool FontManager::OpenFtFace(BASE_NS::string_view uri, FT_Long index, FT_Face* face)
{
    CORE_ASSERT_MSG(fontLib_, "font library is not initialized");

    auto& fileManager = renderContext_.GetEngine().GetFileManager();

    FT_Open_Args args {};
    args.flags = FT_OPEN_STREAM;

    FtFileReader reader(fileManager, uri);
    args.stream = reader.Open();
    if (!args.stream) {
        CORE_LOG_E("failed to open font: %s | %s", BASE_NS::string(uri).c_str(), GetErrorString().c_str());
        return false;
    }

    return FT_Open_Face(fontLib_, &args, index, face) == 0;
}

FT_Face FontManager::OpenFtFace(array_view<const uint8_t> buf, FT_Long index)
{
    CORE_ASSERT_MSG(fontLib_, "font library is not initialized");

    FT_Face face;
    FT_Error err = FT_New_Memory_Face(fontLib_, buf.data(), (FT_Long)buf.size(), index, &face);
    if (err) {
        CORE_LOG_E("failed to init font face");
        return nullptr;
    }
    if (!face->charmap) {
        CORE_LOG_E("failed to init font face, no unicode charmap available");
        FT_Done_Face(face);
        return nullptr;
    }

    CORE_ASSERT(face->num_glyphs > 0);

    return face;
}

FontManager::AtlasTexture* FontManager::CreateAtlasTexture(bool color)
{
    // Access locked from caller already
    auto tex = &atlasTextures_.emplace_back();
    const ComponentMapping colorSwizzle { CORE_COMPONENT_SWIZZLE_B, CORE_COMPONENT_SWIZZLE_G, CORE_COMPONENT_SWIZZLE_R,
        CORE_COMPONENT_SWIZZLE_A }; // BGRA swizzle
    const ComponentMapping monoSwizzle { CORE_COMPONENT_SWIZZLE_R, CORE_COMPONENT_SWIZZLE_R, CORE_COMPONENT_SWIZZLE_R,
        CORE_COMPONENT_SWIZZLE_R }; // RRRR swizzle
    const GpuImageDesc desc {
        ImageType::CORE_IMAGE_TYPE_2D,                                            // imageType
        ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,                                   // imageViewType
        color ? Format::BASE_FORMAT_R8G8B8A8_SRGB : Format::BASE_FORMAT_R8_UNORM, // format
        ImageTiling::CORE_IMAGE_TILING_OPTIMAL,                                   // imageTiling
        ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
            ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT,                     // usageFlags
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,            // memoryPropertyFlags
        0,                                                                        // ImageCreateFlags
        EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, // EngineImageCreationFlags
        FontDefs::ATLAS_SIZE,                                                     // width
        FontDefs::ATLAS_SIZE,                                                     // height
        1,                                                                        // depth
        1,                                                                        // mip count
        1,                                                                        // layer count
        SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,                             // sample count
        color ? colorSwizzle : monoSwizzle                                        // swizzle
    };
    auto& gpuResourceManager = renderContext_.GetDevice().GetGpuResourceManager();
    tex->handle = gpuResourceManager.Create(desc);
    tex->widthLeft = FontDefs::ATLAS_SIZE;
    tex->inColor = color;

#if defined(FONT_VALIDATION_ENABLED) && (FONT_VALIDATION_ENABLED)
    string atlasName = "FontAtlas:";
    atlasName += to_string(reinterpret_cast<uintptr_t>(this));
    atlasName += ':';
    atlasName += to_string(atlasTextures_.size());
    tex->name = atlasName;
    CORE_LOG_N("Created atlas '%s' gpuHnd: %llx", tex->name.data(), tex->handle.GetHandle().id);
#endif

    // Clear the atlas as we don't initialize borders when copying glyphs
    auto staging = refcnt_ptr<IRenderDataStoreDefaultStaging>(static_cast<IRenderDataStoreDefaultStaging*>(
        renderContext_.GetRenderDataStoreManager().GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING).get()));
    ClearColorValue zero { 0.f, 0.f, 0.f, 0.f };
    staging->ClearColorImage(tex->handle, zero);
    return tex;
}

int FontManager::UpdateAtlas(FontDefs::Glyph& glyph, const FT_Bitmap& bitmap, bool inColor)
{
    uint32_t width = bitmap.width + BORDER_WIDTH_X2;
    uint32_t height = bitmap.rows + BORDER_WIDTH_X2;

    // Find best fit (but larger) column in atlases
    uint32_t bestFitAtlas = 0;
    uint32_t bestFitColumn = 0;
    uint32_t bestFitColumnPos = 0;
    uint32_t minDiff = UINT32_MAX;

    std::lock_guard writerLock(atlasMutex_);

    for (uint32_t i = 0; i < atlasTextures_.size(); ++i) {
        auto& atlas = atlasTextures_[i];
        if (inColor == atlas.inColor) {
            uint32_t colPos = 0;
            for (uint32_t col = 0; col < atlas.columns.size(); ++col) {
                auto& hdr = atlas.columns[col];

                if (hdr.colWidth >= width && hdr.heightLeft >= height) {
                    if (hdr.colWidth == width) { // Exact match
                        AddGlyphToColumn(glyph, i, hdr, bitmap, colPos);
                        return ATLAS_OK;
                    }
                    uint32_t wDiff = hdr.colWidth - width;
                    if (wDiff < minDiff) {
                        minDiff = wDiff;
                        bestFitAtlas = i;
                        bestFitColumn = col;
                        bestFitColumnPos = colPos;
                    }
                }
                colPos += hdr.colWidth;
            }
        }
    }

    if (minDiff <= GLYPH_FIT_THRESHOLD) {
        // Close enough
        auto& atlas = atlasTextures_[bestFitAtlas];
        auto& hdr = atlas.columns[bestFitColumn];
        AddGlyphToColumn(glyph, bestFitAtlas, hdr, bitmap, bestFitColumnPos);
        return ATLAS_OK;
    }

    // No matching column is found try to create one ...
    for (uint32_t i = 0; i < atlasTextures_.size(); ++i) {
        auto& atlas = atlasTextures_[i];
        if (inColor == atlas.inColor && atlas.widthLeft >= width) {
            uint32_t colPos = FontDefs::ATLAS_SIZE - atlas.widthLeft;
            atlas.widthLeft -= width;
            auto& hdr = atlas.columns.emplace_back();
            hdr.colWidth = static_cast<uint16_t>(width);
            hdr.heightLeft = FontDefs::ATLAS_SIZE;
            AddGlyphToColumn(glyph, i, hdr, bitmap, colPos);
            return ATLAS_OK;
        }
    }

    // Need to create new atlas
    size_t i = atlasTextures_.size();
    auto atlas = CreateAtlasTexture(inColor);
    atlas->widthLeft -= width;
    auto& hdr = atlas->columns.emplace_back();
    hdr.colWidth = static_cast<uint16_t>(width);
    hdr.heightLeft = static_cast<uint16_t>(ATLAS_SIZE);
    AddGlyphToColumn(glyph, i, hdr, bitmap, 0);
    return ATLAS_OK;
}

void FontManager::UploadPending()
{
    std::lock_guard writerLock(atlasMutex_);
    if (std::all_of(atlasTextures_.cbegin(), atlasTextures_.cend(),
        [](const AtlasTexture& atlas) { return atlas.pending.empty(); })) {
        return;
    }

    struct ColumnSize {
        uint32_t index;
        uint32_t count;
        uint16_t width;
        uint16_t height;
        uint32_t byteSize;
    };
    BASE_NS::vector<BASE_NS::vector<ColumnSize>> allColumnWidths;
    allColumnWidths.reserve(atlasTextures_.size());
    uint32_t totalSize = 0U;
    for (auto& atlas : atlasTextures_) {
        // sort so that columns can be identified
        std::sort(atlas.pending.begin(), atlas.pending.end(), [](const PendingGlyph& lhs, const PendingGlyph& rhs) {
            if (lhs.posX < rhs.posX) {
                return true;
            }
            if (lhs.posX > rhs.posX) {
                return false;
            }
            if (lhs.posY < rhs.posY) {
                return true;
            }
            if (lhs.posY > rhs.posY) {
                return false;
            }
            return false;
        });

        auto& columnWidths = allColumnWidths.emplace_back();
        columnWidths.reserve(atlas.pending.size());

        auto first = atlas.pending.begin();
        auto last = atlas.pending.end();
        for (; first != last;) {
            // find the first glyph in the column
            first = std::lower_bound(first, last, *(last - 1),
                [](const PendingGlyph& value, const PendingGlyph& element) { return value.posX < element.posX; });
            // check the maximum width of the column
            const auto width = std::accumulate(first, last, uint16_t(0u),
                [](const uint16_t a, const PendingGlyph& b) { return std::max(a, b.width); });
            const auto height = static_cast<uint16_t>(((last - 1)->posY + (last - 1)->height) - first->posY);
            const auto bufferSize = static_cast<uint32_t>(width * height * (atlas.inColor ? 4u : 1u));
            totalSize += bufferSize;
            columnWidths.push_back({ static_cast<uint32_t>(std::distance(atlas.pending.begin(), first)),
                static_cast<uint32_t>(std::distance(first, last)), width, height, bufferSize });
            last = first;
            first = atlas.pending.begin();
        }
    }
    if (!totalSize) {
        return;
    }

    // create a staging buffer big enough for all the columns
    const GpuBufferDesc desc {
        BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_SRC_BIT, // usageFlags
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT, // memoryPropertyFlags
        EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE |
            EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DEFERRED_DESTROY |
            EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER, // engineCreationFlags
        totalSize,                                                                          // byteSize
        BASE_NS::Format::BASE_FORMAT_UNDEFINED,                                             // format
    };
    auto& gpuResMan = renderContext_.GetDevice().GetGpuResourceManager();
    auto bufferHandle = gpuResMan.Create(desc);
    auto* ptr = static_cast<uint8_t*>(gpuResMan.MapBufferMemory(bufferHandle));
    if (!ptr) {
        return;
    }
    auto buffer = array_view(ptr, totalSize);
    std::fill(buffer.begin(), buffer.end(), uint8_t(0u));

    auto staging = refcnt_ptr<IRenderDataStoreDefaultStaging>(static_cast<IRenderDataStoreDefaultStaging*>(
        renderContext_.GetRenderDataStoreManager().GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING).get()));

    auto allColumnWidthsIter = allColumnWidths.cbegin();
    uint32_t columnOffset = 0U;
    for (auto& atlas : atlasTextures_) {
        const auto bytesPerPixel = (atlas.inColor ? 4u : 1u);
        for (auto& columnWidths : *allColumnWidthsIter) {
            // copy each glyph in this column into the staging buffer
            auto first = atlas.pending.begin() + columnWidths.index;
            auto last = first + columnWidths.count;
            std::for_each(first, last,
                [buffer = buffer.data() + columnOffset, width = columnWidths.width, bytesPerPixel,

                    firstPosY = first->posY](const PendingGlyph& glyph) mutable {
                    if (glyph.width == width) {
                        std::copy(glyph.data.begin().ptr(), glyph.data.end().ptr(),
                            buffer + (width * bytesPerPixel * (glyph.posY - firstPosY)));
                    } else {
                        auto src = glyph.data.data();
                        auto dst = buffer + (width * bytesPerPixel * (glyph.posY - firstPosY));
                        const auto srcPitch = glyph.width * bytesPerPixel;
                        const auto dstPitch = width * bytesPerPixel;
                        for (auto i = 0u; i < glyph.height; ++i) {
                            auto srcFirst = src + (i * srcPitch);
                            auto srcLast = src + ((i + 1) * srcPitch);
                            std::copy(srcFirst, srcLast, dst + (i * dstPitch));
                        }
                    }
                });

            // request to copy the column into the atlas texture
            const BufferImageCopy bufferImageCopy {
                columnOffset,        // bufferOffset
                columnWidths.width,  // bufferRowLength
                columnWidths.height, // bufferImageHeight
                {
                    CORE_IMAGE_ASPECT_COLOR_BIT,               // imageAspectFlags
                    0,                                         // mipLevel
                    0,                                         // baseArrayLayer
                    1u,                                        // layerCount
                },                                             // imageSubresource
                { first->posX, first->posY, 0 },               // imageOffset
                { columnWidths.width, columnWidths.height, 1 } // imageExtend
            };
            staging->CopyBufferToImage(bufferHandle, atlas.handle, bufferImageCopy);

            columnOffset += columnWidths.byteSize;
        }
        ++allColumnWidthsIter;
        atlas.pending.clear();
    }

    gpuResMan.UnmapBuffer(bufferHandle);
}

void FontManager::AddGlyphToColumn(
    Glyph& glyph, size_t atlasIndex, ColumnHeader& hdr, const FT_Bitmap& bitmap, uint32_t columnX)
{
    uint32_t width = bitmap.width;
    uint32_t height = bitmap.rows;

    uint32_t x = columnX + BORDER_WIDTH;
    uint32_t y = ATLAS_SIZE - hdr.heightLeft + BORDER_WIDTH;

    hdr.heightLeft -= static_cast<uint16_t>(height + BORDER_WIDTH_X2);

    glyph.atlas.index = static_cast<uint16_t>(atlasIndex);
    glyph.atlas.rect.x = static_cast<uint16_t>(x);
    glyph.atlas.rect.y = static_cast<uint16_t>(y);
    glyph.atlas.rect.w = static_cast<uint16_t>(width);
    glyph.atlas.rect.h = static_cast<uint16_t>(height);

    // Access locked by caller
    uint32_t bpp = (atlasTextures_[atlasIndex].inColor ? 4u : 1u);

    if (width > 0 && height > 0 && bitmap.pitch / bpp >= width) {
        atlasTextures_[atlasIndex].pending.push_back({
            static_cast<uint16_t>(x),
            static_cast<uint16_t>(y),
            static_cast<uint16_t>(width),
            static_cast<uint16_t>(height),
            { bitmap.buffer, bitmap.buffer + bitmap.pitch * bitmap.rows },
        });
    }
}

void FontManager::Gc(uint64_t uid, FontBuffer* data)
{
    // Check if file data cache has this font
    std::lock_guard writerLock(fontBuffersMutex_);

    if (!fontBuffers_.erase(uid)) {
        for (auto it = fontBuffers_.cbegin(); it != fontBuffers_.cend();) {
            if (it->second.expired()) {
                fontBuffers_.erase(it);
            } else {
                ++it;
            }
        }
    }
    if (fontBuffers_.empty()) {
        FlushCaches();
    }
}

const CORE_NS::IInterface* FontManager::GetInterface(const BASE_NS::Uid& uid) const
{
    if (uid == CORE_NS::IInterface::UID) {
        return this;
    }
    if (uid == IFontManager::UID) {
        return this;
    }
    return nullptr;
}

CORE_NS::IInterface* FontManager::GetInterface(const BASE_NS::Uid& uid)
{
    if (uid == CORE_NS::IInterface::UID) {
        return this;
    }
    if (uid == IFontManager::UID) {
        return this;
    }
    return nullptr;
}

void FontManager::Ref()
{
    refcnt_.fetch_add(1, std::memory_order_relaxed);
}

void FontManager::Unref()
{
    if (refcnt_.fetch_sub(1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        CORE_LOG_N("delete FontManager");
        delete this;
    }
}
FONT_END_NAMESPACE()
