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

#ifndef CORE__GLTF__GLTF2_DATA_STRUCTURES_H
#define CORE__GLTF__GLTF2_DATA_STRUCTURES_H

// Public data structures.
#include <3d/gltf/gltf_data.h>

// Extension defines for internal use (controls which extensions the parser handles).
#define GLTF2_EXTENSION_IGFX_COMPRESSED
#define GLTF2_EXTENSION_KHR_LIGHTS
#define GLTF2_EXTENSION_KHR_LIGHTS_PBR
#define GLTF2_EXTENSION_KHR_MATERIALS_CLEARCOAT
#define GLTF2_EXTENSION_KHR_MATERIALS_EMISSIVE_STRENGTH
#define GLTF2_EXTENSION_KHR_MATERIALS_IOR
#define GLTF2_EXTENSION_KHR_MATERIALS_PBRSPECULARGLOSSINESS
#define GLTF2_EXTENSION_KHR_MATERIALS_SHEEN
#define GLTF2_EXTENSION_KHR_MATERIALS_SPECULAR
#define GLTF2_EXTENSION_KHR_MATERIALS_TRANSMISSION
#define GLTF2_EXTENSION_KHR_MATERIALS_UNLIT
#define GLTF2_EXTENSION_KHR_MESH_QUANTIZATION
#define GLTF2_EXTENSION_KHR_TEXTURE_BASISU
#define GLTF2_EXTENSION_KHR_TEXTURE_TRANSFORM
#define GLTF2_EXTENSION_EXT_LIGHTS_IMAGE_BASED
#define GLTF2_EXTRAS_CLEAR_COAT_MATERIAL
#define GLTF2_EXTENSION_HW_XR_EXT
#define GLTF2_EXTRAS_RSDZ
#ifndef GLTF2_EXTENSION_EXT_MESHOPT_COMPRESSION
#define GLTF2_EXTENSION_EXT_MESHOPT_COMPRESSION
#endif

// Internal types not exposed in the public API.
CORE3D_BEGIN_NAMESPACE()
namespace GLTF2 {

constexpr const uint32_t GLTF_MAGIC = 0x46546C67;  // ASCII string "glTF"

enum class ChunkType : int {
    JSON = 0x4E4F534A,
    BIN = 0x004E4942,
};

struct GLBHeader {
    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t length = 0;
};

struct GLBChunk {
    uint32_t chunkLength = 0;
    uint32_t chunkType = 0;
};

}  // namespace GLTF2
CORE3D_END_NAMESPACE()

#endif  // CORE__GLTF__GLTF2_DATA_STRUCTURES_H
