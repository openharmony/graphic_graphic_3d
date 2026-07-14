/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_IMP_SRC_OBJECTS_MESH_BIN_FORMAT_H
#define SCENE_IMP_SRC_OBJECTS_MESH_BIN_FORMAT_H

#include <cstddef>
#include <cstdint>
#include <scene_importer/base/namespace.h>

SCENE_IMP_BEGIN_NAMESPACE()

/// Layout constants for the `.bin` companion file paired with a `.mesh` JSON
/// descriptor. The binary starts with a fixed 16-byte header so readers can
/// sanity-check the file independently of the JSON; region offsets in the
/// JSON are file-absolute, so writers place all vertex/index data after the
/// header.
///
/// The on-disk format is **little-endian** regardless of host byte order —
/// readers and writers must use ReadU32LE / WriteU32LE for multi-byte fields.
/// Header layout:
///   bytes  0..3 : magic ("LMSH")
///   bytes  4..7 : uint32 version (little-endian)
///   bytes  8..15: reserved, zero (future versions may repurpose this region)
namespace MeshBin {
constexpr char MAGIC[4] = {'L', 'M', 'S', 'H'};

/// Binary format version. Independent of the .mesh JSON schema version — bump
/// on any change to the binary layout itself (e.g. new compression scheme).
/// A reader rejects files whose version exceeds its known maximum.
constexpr uint32_t VERSION = 1;

constexpr size_t HEADER_SIZE = 16;
constexpr size_t MAGIC_OFFSET = 0;
constexpr size_t VERSION_OFFSET = 4;

/// Upper bound on accepted .bin file size (1 GiB). Files larger than this are
/// almost certainly malformed/malicious — refuse them before the importer
/// allocates a giant buffer. Bump only with a concrete need backed by a real
/// asset size, since the limit also caps blast radius of corrupt JSON
/// pointing at a runaway file.
constexpr uint64_t MAX_FILE_SIZE = uint64_t{1} << 30;

// Catch layout drift at compile time: the magic and version fields must not
// overlap, and the version field must fit within the declared header size.
static_assert(MAGIC_OFFSET + sizeof(MAGIC) <= VERSION_OFFSET);
static_assert(VERSION_OFFSET + sizeof(uint32_t) <= HEADER_SIZE);

/// Reads / writes the 4 bytes pointed to by `p` as little-endian regardless of
/// host byte order, so the on-disk format is identical on LE and BE targets.
inline uint32_t ReadU32LE(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

inline void WriteU32LE(uint8_t* p, uint32_t v)
{
    p[0] = static_cast<uint8_t>(v);
    p[1] = static_cast<uint8_t>(v >> 8);
    p[2] = static_cast<uint8_t>(v >> 16);
    p[3] = static_cast<uint8_t>(v >> 24);
}
}  // namespace MeshBin

SCENE_IMP_END_NAMESPACE()

#endif
