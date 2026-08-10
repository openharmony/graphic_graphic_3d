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

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>

#define JSON_IMPL
#include <core/json/json.h>

#define JSON2_IMPL
#include <core/json/json2.h>

namespace {

// Fuzz the JSON parsing layer that GLTF loading depends on.
// The full GLTF import path (IGltf2::LoadGLTF with array_view) requires
// an initialized IGraphicsContext, so we fuzz the underlying JSON parser
// with GLTF-shaped inputs and exercise GLB header validation.

// GLB magic: 0x46546C67 ("glTF")
constexpr uint32_t GLB_MAGIC = 0x46546C67;
constexpr uint32_t GLB_CHUNK_JSON = 0x4E4F534A;
constexpr uint32_t GLB_CHUNK_BIN = 0x004E4942;

const char* GLTF_KEYS[] = {"asset",
    "scene",
    "scenes",
    "nodes",
    "meshes",
    "materials",
    "textures",
    "images",
    "samplers",
    "accessors",
    "bufferViews",
    "buffers",
    "animations",
    "skins",
    "cameras",
    "extensions"};

void FuzzGltfJsonV1(const uint8_t* data, size_t size)
{
    BASE_NS::vector<char> buf(size + 1);
    if (memcpy_s(buf.data(), buf.size(), data, size) != EOK) {
        return;
    }
    buf[size] = '\0';

    auto value = CORE_NS::json::parse(buf.data());
    if (!value || value.type != CORE_NS::json::type::object) {
        return;
    }

    for (const auto* key : GLTF_KEYS) {
        auto* it = value.find(key);
        if (it && it->type == CORE_NS::json::type::array) {
            for (const auto& elem : it->array_) {
                (void)elem.type;
            }
        }
    }
}

void FuzzGltfJsonV2(const uint8_t* data, size_t size)
{
    BASE_NS::string_view sv(reinterpret_cast<const char*>(data), size);
    auto value = CORE_NS::json2::view::parse(sv);
    if (!value || !value.is_object()) {
        return;
    }

    for (const auto* key : GLTF_KEYS) {
        auto* it = value.find(key);
        if (it && it->is_array()) {
            for (const auto& elem : it->as_array()) {
                (void)elem.type();
            }
        }
    }
}

void FuzzGlbHeader(const uint8_t* data, size_t size)
{
    if (size < 12) {
        return;
    }

    uint32_t magic = 0;
    if (memcpy_s(&magic, sizeof(magic), data, 4) != EOK) {
        return;
    }
    if (magic != GLB_MAGIC) {
        return;
    }

    // Walk chunks and parse JSON chunks with both v1 and v2
    size_t offset = 12;
    while (offset + 8 <= size) {
        uint32_t chunkLength = 0;
        uint32_t chunkType = 0;
        if (memcpy_s(&chunkLength, sizeof(chunkLength), data + offset, 4) != EOK ||
            memcpy_s(&chunkType, sizeof(chunkType), data + offset + 4, 4) != EOK) {
            break;
        }
        offset += 8;

        if (chunkLength > size - offset) {
            break;
        }

        if (chunkType == GLB_CHUNK_JSON && chunkLength > 0) {
            // v1
            BASE_NS::vector<char> jsonBuf(chunkLength + 1);
            if (memcpy_s(jsonBuf.data(), jsonBuf.size(), data + offset, chunkLength) != EOK) {
                break;
            }
            jsonBuf[chunkLength] = '\0';
            auto v1 = CORE_NS::json::parse(jsonBuf.data());
            (void)v1;

            // v2
            BASE_NS::string_view sv(reinterpret_cast<const char*>(data + offset), chunkLength);
            auto v2 = CORE_NS::json2::view::parse(sv);
            (void)v2;
        }

        offset += chunkLength;
    }
}

}  // anonymous namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size == 0 || size > 131072) {
        return 0;
    }

    FuzzGltfJsonV1(data, size);
    FuzzGltfJsonV2(data, size);
    FuzzGlbHeader(data, size);
    return 0;
}
