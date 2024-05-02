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

#ifndef CORE__GLTF__GLTF2_UTIL_H
#define CORE__GLTF__GLTF2_UTIL_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <core/namespace.h>

#include "gltf/data.h"

CORE3D_BEGIN_NAMESPACE()
namespace GLTF2 {
// Helper functions to access GLTF2 data.
bool GetAttributeType(BASE_NS::string_view dataType, AttributeBase& out);
bool GetMimeType(BASE_NS::string_view type, MimeType& out);

bool GetDataType(BASE_NS::string_view dataType, DataType& out);
bool GetCameraType(BASE_NS::string_view type, CameraType& out);

bool GetAlphaMode(BASE_NS::string_view dataType, AlphaMode& out);
bool GetBlendMode(BASE_NS::string_view dataType, BlendMode& out);

bool GetAnimationInterpolation(BASE_NS::string_view interpolation, AnimationInterpolation& out);
bool GetAnimationPath(BASE_NS::string_view path, AnimationPath& out);

BASE_NS::string_view GetAttributeType(AttributeBase type);
BASE_NS::string_view GetMimeType(MimeType type);
BASE_NS::string_view GetDataType(DataType type);
BASE_NS::string_view GetCameraType(CameraType type);

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
bool GetLightType(BASE_NS::string_view type, LightType& out);
BASE_NS::string_view GetLightType(LightType type);
#endif

BASE_NS::string_view GetAlphaMode(AlphaMode mode);
BASE_NS::string_view GetBlendMode(BlendMode mode);
BASE_NS::string_view GetAnimationInterpolation(AnimationInterpolation interpolation);
BASE_NS::string_view GetAnimationPath(AnimationPath path);

uint32_t GetComponentByteSize(ComponentType component);
uint32_t GetComponentsCount(DataType type);
ComponentType GetAlternativeType(ComponentType component, size_t newByteCount);

#if defined(GLTF2_EXTENSION_KHR_MESH_QUANTIZATION)
BASE_NS::string_view ValidatePrimitiveAttributeQuatization(
    AttributeType attribute, DataType accessorType, ComponentType accessorComponentType);
BASE_NS::string_view ValidateMorphTargetAttributeQuantization(
    AttributeType attribute, DataType accessorType, ComponentType accessorComponentType);
#endif
BASE_NS::string_view ValidatePrimitiveAttribute(
    AttributeType attribute, DataType accessorType, ComponentType accessorComponentType);
BASE_NS::string_view ValidateMorphTargetAttribute(
    AttributeType attribute, DataType accessorType, ComponentType accessorComponentType);

void SplitFilename(BASE_NS::string_view source, BASE_NS::string_view& base, BASE_NS::string_view& path);
void SplitBaseFilename(BASE_NS::string_view source, BASE_NS::string_view& name, BASE_NS::string_view& extension);

BASE_NS::string_view ParseDataUri(const BASE_NS::string_view in, size_t& offsetToData);
bool DecodeDataURI(BASE_NS::vector<uint8_t>& out, BASE_NS::string_view in, size_t reqBytes, bool checkSize,
    BASE_NS::string_view& mimeType);
bool IsDataURI(BASE_NS::string_view in);

// Buffer / data helpers
struct GLTFLoadDataResult {
    GLTFLoadDataResult() = default;
    GLTFLoadDataResult(const GLTFLoadDataResult& other) = delete;
    GLTFLoadDataResult(GLTFLoadDataResult&& other) noexcept;

    GLTFLoadDataResult& operator=(GLTFLoadDataResult&& other) noexcept;

    bool success { false };
    bool normalized { false };
    BASE_NS::string error;

    ComponentType componentType;
    size_t componentByteSize { 0 };
    size_t componentCount { 0 };

    size_t elementSize { 0 };
    size_t elementCount { 0 };

    BASE_NS::vector<float> min;
    BASE_NS::vector<float> max;

    BASE_NS::vector<uint8_t> data;
};

struct BufferLoadResult {
    bool success { true };
    BASE_NS::string error;
};

// Populate GLTF buffers with data.
BufferLoadResult LoadBuffers(const Data& data, CORE_NS::IFileManager& fileManager);

enum UriLoadResult {
    URI_LOAD_SUCCESS,
    URI_LOAD_FAILED_INVALID_MIME_TYPE,
    URI_LOAD_FAILED_TO_DECODE_BASE64,
    URI_LOAD_FAILED_TO_READ_FILE
};

/** Load URI to data buffer.
 * @param aUri Uri to data, this can point either to data uri or external file.
 * @param mimeType Requested mime type, such as "image", can be empty to allow all mime types.
 * @param aFilePath Root path that can be used for relative uri/file lookup.
 * @param aFileManager File manager to access external files.
 * @param aOutExtension File extension of the actual file.
 * @param aOutData Output data.
 * @return Loading result, URI_LOAD_SUCCESS if data was successfully loaded.
 */
UriLoadResult LoadUri(BASE_NS::string_view uri, BASE_NS::string_view mimeType, BASE_NS::string_view filepath,
    CORE_NS::IFileManager& fileManager, BASE_NS::string_view& outExtension, BASE_NS::vector<uint8_t>& outData);

// Load accessor data.
GLTFLoadDataResult LoadData(Accessor const& accessor);
} // namespace GLTF2
CORE3D_END_NAMESPACE()

#endif // CORE__GLTF__GLTF2_UTIL_H
