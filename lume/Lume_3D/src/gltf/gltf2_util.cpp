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

#include "gltf/gltf2_util.h"

#include <algorithm>
#include <cinttypes>
#include <cstring>

#include <base/containers/fixed_string.h>
#include <base/util/base64_decode.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <core/namespace.h>

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;

namespace GLTF2 {
namespace {
using ByteBuffer = vector<uint8_t>;

vector<uint8_t> Read(const uint8_t* src, uint32_t componentByteSize, uint32_t componentCount, uint32_t elementSize,
    size_t byteStride, uint32_t count)
{
    vector<uint8_t> result;

    if (elementSize > 0u) {
        const size_t sourceSize = componentCount * componentByteSize * count;
        result.reserve(sourceSize);

        if (elementSize == byteStride || byteStride == 0u) {
            result.insert(result.end(), src, src + sourceSize);
        } else {
            const uint8_t* source = src;

            for (size_t i = 0u; i < count; ++i) {
                result.insert(result.end(), source, source + elementSize);
                source += byteStride;
            }
        }
    }

    return result;
}

vector<uint8_t> Read(Accessor const& accessor)
{
    if (!accessor.bufferView->data) {
        return {};
    }

    const size_t startOffset = accessor.bufferView->byteOffset + accessor.byteOffset;
    const size_t bufferLength = accessor.bufferView->buffer->byteLength;
    const size_t bufferRemaining = bufferLength - startOffset;

    const uint8_t* src = accessor.bufferView->data + accessor.byteOffset;

    const uint32_t componentByteSize = GetComponentByteSize(accessor.componentType);
    const uint32_t componentCount = GetComponentsCount(accessor.type);
    const uint32_t elementSize = componentCount * componentByteSize;
    const size_t byteStride = accessor.bufferView->byteStride;
    const uint32_t count = accessor.count;

    size_t readBytes = 0u;

    if (elementSize == byteStride || byteStride == 0u) {
        readBytes = accessor.count * elementSize;
    } else {
        readBytes = (accessor.count - 1) * byteStride + elementSize;
    }

    if (bufferRemaining < readBytes) {
        // Avoid buffer overflow.
        return vector<uint8_t>();
    }

    return Read(src, componentByteSize, componentCount, elementSize, byteStride, count);
}

template<class T>
void CopySparseElements(
    ByteBuffer& destination, ByteBuffer& source, ByteBuffer& indices, uint32_t elementSize, size_t count)
{
    T* indicesPtr = reinterpret_cast<T*>(indices.data());
    auto const end = ptrdiff_t(destination.data() + destination.size());
    for (size_t i = 0u; i < count; ++i) {
        const uint8_t* sourcePtr = source.data() + (i * elementSize);
        uint8_t* destinationPtr = destination.data() + (indicesPtr[i] * elementSize);
        auto const left = end - ptrdiff_t(destinationPtr);
        if (left > 0) {
            if (!CloneData(destinationPtr, size_t(left), sourcePtr, elementSize)) {
                CORE_LOG_E("Copying of sparseElements failed.");
            }
        }
    }
}

BufferLoadResult LoadBuffer(Data const& data, Buffer& buffer, IFileManager& fileManager)
{
    if (IsDataURI(buffer.uri)) {
        string_view type;
        if (!DecodeDataURI(buffer.data, buffer.uri, 0u, false, type)) {
            return BufferLoadResult { false, "Failed to decode data uri: " + buffer.uri + '\n' };
        }
    } else {
        uint32_t offset;
        string_view uri;
        if (buffer.uri.size()) {
            uri = buffer.uri;
            offset = 0u;
        } else if (data.defaultResourcesOffset >= 0) {
            uri = data.defaultResources;
            offset = static_cast<uint32_t>(data.defaultResourcesOffset);
        } else {
            return BufferLoadResult { false, "Failed to open buffer: " + buffer.uri + '\n' };
        }

        IFile::Ptr file;
        IFile* filePtr = nullptr;
        if (data.memoryFile_) {
            filePtr = data.memoryFile_.get();
        } else {
            string fileName;
            fileName.reserve(data.filepath.size() + 1u + uri.size());
            fileName.append(data.filepath);
            fileName.append("/");
            fileName.append(uri);

            file = fileManager.OpenFile(fileName);
            filePtr = file.get();
        }
        if (!filePtr) {
            return BufferLoadResult { false, "Failed open uri: " + buffer.uri + '\n' };
        }

        filePtr->Seek(offset);

        // Check that buffer does not overflow over the file boundaries.
        const auto length = filePtr->GetLength();
        const auto remaining = length - filePtr->GetPosition();
        if (remaining < buffer.byteLength) {
            // Clamp buffer to file boundary.
            CORE_LOG_W("Buffer size %zu larger than file size (%" PRIu64 " remaining).", buffer.byteLength, remaining);
            buffer.byteLength = static_cast<size_t>(remaining);
        }

        buffer.data.resize(buffer.byteLength);

        if (filePtr->Read(buffer.data.data(), buffer.byteLength) != buffer.byteLength) {
            return BufferLoadResult { false, "Failed to read buffer: " + buffer.uri + '\n' };
        }
    }

    return BufferLoadResult {};
}

void LoadSparseAccessor(Accessor const& accessor, GLTFLoadDataResult& result)
{
    auto const& sparseIndicesBufferView = accessor.sparse.indices.bufferView;
    vector<uint8_t> sparseIndicesData;
    if (sparseIndicesBufferView->buffer) {
        const uint8_t* src = sparseIndicesBufferView->data + accessor.sparse.indices.byteOffset;

        auto const componentCount = 1u;
        auto const componentByteSize = GetComponentByteSize(accessor.sparse.indices.componentType);
        auto const elementSize = componentCount * componentByteSize;
        auto const byteStride = accessor.bufferView->byteStride;
        auto const count = accessor.sparse.count;

        sparseIndicesData = Read(src, componentByteSize, componentCount, elementSize, byteStride, count);
    }

    auto const& sparseValuesBufferView = accessor.sparse.values.bufferView;
    if (sparseValuesBufferView->buffer) {
        vector<uint8_t> sourceData;

        const uint8_t* src = sparseValuesBufferView->data + accessor.sparse.indices.byteOffset;
        auto const componentCount = GetComponentsCount(accessor.type);
        auto const componentByteSize = GetComponentByteSize(accessor.componentType);
        auto const elementSize = componentCount * componentByteSize;
        auto const byteStride = accessor.bufferView->byteStride;
        auto const count = accessor.sparse.count;

        sourceData = Read(src, componentByteSize, componentCount, elementSize, byteStride, count);

        switch (accessor.sparse.indices.componentType) {
            case ComponentType::UNSIGNED_BYTE: {
                CopySparseElements<uint8_t>(
                    result.data, sourceData, sparseIndicesData, elementSize, accessor.sparse.count);
                break;
            }

            case ComponentType::UNSIGNED_SHORT: {
                CopySparseElements<uint16_t>(
                    result.data, sourceData, sparseIndicesData, elementSize, accessor.sparse.count);
                break;
            }

            case ComponentType::UNSIGNED_INT: {
                CopySparseElements<uint32_t>(
                    result.data, sourceData, sparseIndicesData, elementSize, accessor.sparse.count);
                break;
            }

            case ComponentType::BYTE:
            case ComponentType::SHORT:
            case ComponentType::INT:
            case ComponentType::FLOAT:
            default:
                result.error += "invalid accessor.sparse.indices.componentType\n";
                result.success = false;
                break;
        }
    }
}

bool ReadUriToVector(
    const string_view filePath, IFileManager& fileManager, string_view const& uri, vector<uint8_t>& out)
{
    string filepath;
    filepath.reserve(filePath.size() + 1u + uri.size());
    filepath += filePath;
    filepath += '/';
    filepath += uri;
    auto file = fileManager.OpenFile(filepath);
    if (file) {
        const size_t count = static_cast<size_t>(file->GetLength());
        out.resize(count);
        return file->Read(out.data(), count) == count;
    }
    return false;
}
} // namespace

// Helper functions to access GLTF2 data
bool GetAttributeType(const string_view dataType, AttributeBase& out)
{
    const char* data = dataType.data();
    const unsigned int size = static_cast<unsigned int>(dataType.size());

    AttributeBase attribute;
    attribute.type = AttributeType::INVALID;
    attribute.index = 0u;
    if (data == nullptr) {
        return false;
    }

    /*
    POSITION, NORMAL, TANGENT, TEXCOORD_0, TEXCOORD_1, COLOR_0, JOINTS_0, and WEIGHTS_0
    */
    if (dataType == "POSITION") {
        attribute.type = AttributeType::POSITION;
    } else if (dataType == "NORMAL") {
        attribute.type = AttributeType::NORMAL;
    } else if (dataType == "TANGENT") {
        attribute.type = AttributeType::TANGENT;
    } else if (dataType.find("TEXCOORD_") == 0u) {
        attribute.type = AttributeType::TEXCOORD;
        attribute.index = 0u;

        if (size > 9u) {
            attribute.index = (unsigned int)(data[9u] - '0'); // NOTE: check if size is over 10 => index more than 9
        }
    } else if (dataType.find("COLOR_") == 0u) {
        attribute.type = AttributeType::COLOR;
        attribute.index = 0u;

        if (size > 6u) {
            attribute.index = (unsigned int)(data[6u] - '0'); // NOTE: check if size is over 7 => index more than 9
        }
    } else if (dataType.find("JOINTS_") == 0u) {
        attribute.type = AttributeType::JOINTS;
        attribute.index = 0u;

        if (size > 7u) {
            attribute.index = (unsigned int)(data[7u] - '0'); // NOTE: check if size is over 8 => index more than 9
        }
    } else if (dataType.find("WEIGHTS_") == 0u) {
        attribute.type = AttributeType::WEIGHTS;
        attribute.index = 0u;

        if (size > 8u) {
            attribute.index = (unsigned int)(data[8u] - '0'); // NOTE: check if size is over 9 => index more than 9
        }
    } else {
        return false;
    }

    if (attribute.index > 9u) {
        return false;
    }

    out = attribute;
    return true;
}

bool GetMimeType(const string_view type, MimeType& out)
{
    out = MimeType::INVALID;

    if (type == "image/jpeg") {
        out = MimeType::JPEG;
    } else if (type == "image/png") {
        out = MimeType::PNG;
    } else if (type == "image/ktx") {
        out = MimeType::KTX;
    } else if (type == "image/ktx2") {
        out = MimeType::KTX2;
    } else if (type == "image/vnd-ms.dds") {
        out = MimeType::DDS;
    } else if (type == "application/octet-stream") {
        out = MimeType::KTX; // Probably it's a KTX file, bundled by using an external application.
    }
    return out != MimeType::INVALID;
}

bool GetDataType(const string_view dataType, DataType& out)
{
    out = DataType::INVALID;

    if (dataType == "SCALAR") {
        out = DataType::SCALAR;
    } else if (dataType == "VEC2") {
        out = DataType::VEC2;
    } else if (dataType == "VEC3") {
        out = DataType::VEC3;
    } else if (dataType == "VEC4") {
        out = DataType::VEC4;
    } else if (dataType == "MAT2") {
        out = DataType::MAT2;
    } else if (dataType == "MAT3") {
        out = DataType::MAT3;
    } else if (dataType == "MAT4") {
        out = DataType::MAT4;
    }
    return out != DataType::INVALID;
}

bool GetCameraType(const string_view type, CameraType& out)
{
    out = CameraType::INVALID;

    if (type == "perspective") {
        out = CameraType::PERSPECTIVE;
    } else if (type == "orthographic") {
        out = CameraType::ORTHOGRAPHIC;
    }
    return out != CameraType::INVALID;
}

bool GetAlphaMode(const string_view dataType, AlphaMode& out)
{
    out = AlphaMode::OPAQUE;

    bool result = true;

    if (dataType == "BLEND") {
        out = AlphaMode::BLEND;
    } else if (dataType == "MASK") {
        out = AlphaMode::MASK;
    } else if (dataType == "OPAQUE") {
        out = AlphaMode::OPAQUE;
    } else {
        result = false;
    }
    return result;
}

bool GetBlendMode(const string_view dataType, BlendMode& out)
{
    out = BlendMode::REPLACE;

    bool result = true;

    if (dataType == "transparentColor") {
        out = BlendMode::TRANSPARENT_COLOR;
    } else if (dataType == "transparentAlpha") {
        out = BlendMode::TRANSPARENT_ALPHA;
    } else if (dataType == "add") {
        out = BlendMode::ADD;
    } else if (dataType == "modulate") {
        out = BlendMode::MODULATE;
    } else if (dataType == "replace") {
        out = BlendMode::REPLACE;
    } else if (dataType == "none") {
        out = BlendMode::NONE;
    } else {
        result = false;
    }
    return result;
}

bool GetAnimationInterpolation(string_view interpolation, AnimationInterpolation& out)
{
    // Default type is linear, this is not required attribute.
    out = AnimationInterpolation::LINEAR;

    if (interpolation == "LINEAR") {
        out = AnimationInterpolation::LINEAR;
    } else if (interpolation == "STEP") {
        out = AnimationInterpolation::STEP;
    } else if (interpolation == "CUBICSPLINE") {
        out = AnimationInterpolation::SPLINE;
    }
    return true;
}

bool GetAnimationPath(string_view path, AnimationPath& out)
{
    bool result = true;

    if (path == "translation") {
        out = AnimationPath::TRANSLATION;
    } else if (path == "rotation") {
        out = AnimationPath::ROTATION;
    } else if (path == "scale") {
        out = AnimationPath::SCALE;
    } else if (path == "weights") {
        out = AnimationPath::WEIGHTS;
    } else if (path == "visible") {
        out = AnimationPath::VISIBLE;
    } else if (path == "material.opacity") {
        out = AnimationPath::OPACITY;
    } else {
        result = false;
    }
    return result;
}

namespace {
constexpr const char* ATTRIBUTE_TYPES[] = { "NORMAL", "POSITION", "TANGENT" };
constexpr const char* TEXCOORD_ATTRIBUTES[] = { "TEXCOORD_0", "TEXCOORD_1", "TEXCOORD_2", "TEXCOORD_3", "TEXCOORD_4" };
constexpr const char* COLOR_ATTRIBUTES[] = { "COLOR_0", "COLOR_1", "COLOR_2", "COLOR_3", "COLOR_4" };
constexpr const char* JOINTS_ATTRIBUTES[] = { "JOINTS_0", "JOINTS_1", "JOINTS_2", "JOINTS_3", "JOINTS_4" };
constexpr const char* WEIGHTS_ATTRIBUTES[] = { "WEIGHTS_0", "WEIGHTS_1", "WEIGHTS_2", "WEIGHTS_3", "WEIGHTS_4" };
constexpr const char* ATTRIBUTE_INVALID = "INVALID";
} // namespace

string_view GetAttributeType(AttributeBase dataType)
{
    if ((dataType.type < AttributeType::NORMAL)) {
    } else if (dataType.type <= AttributeType::TANGENT) {
        return ATTRIBUTE_TYPES[static_cast<int>(dataType.type)];
    } else if (dataType.type == AttributeType::TEXCOORD) {
        if (dataType.index < countof(TEXCOORD_ATTRIBUTES)) {
            return TEXCOORD_ATTRIBUTES[dataType.index];
        }
    } else if (dataType.type == AttributeType::COLOR) {
        if (dataType.index < countof(COLOR_ATTRIBUTES)) {
            return COLOR_ATTRIBUTES[dataType.index];
        }
    } else if (dataType.type == AttributeType::JOINTS) {
        if (dataType.index < countof(JOINTS_ATTRIBUTES)) {
            return JOINTS_ATTRIBUTES[dataType.index];
        }
    } else if (dataType.type == AttributeType::WEIGHTS) {
        if (dataType.index < countof(WEIGHTS_ATTRIBUTES)) {
            return WEIGHTS_ATTRIBUTES[dataType.index];
        }
    }
    return ATTRIBUTE_INVALID;
}

string_view GetMimeType(MimeType type)
{
    switch (type) {
        case MimeType::JPEG:
            return "image/jpeg";
        case MimeType::PNG:
            return "image/png";
        case MimeType::KTX:
            return "image/ktx";
        case MimeType::DDS:
            return "image/vnd-ms.dds";
        case MimeType::KTX2:
            return "image/ktx2";

        case MimeType::INVALID:
        default:
            break;
    }
    return "INVALID";
}

string_view GetDataType(DataType dataType)
{
    switch (dataType) {
        case DataType::SCALAR:
            return "SCALAR";
        case DataType::VEC2:
            return "VEC2";
        case DataType::VEC3:
            return "VEC3";
        case DataType::VEC4:
            return "VEC4";
        case DataType::MAT2:
            return "MAT2";
        case DataType::MAT3:
            return "MAT3";
        case DataType::MAT4:
            return "MAT4";
        case DataType::INVALID:
        default:
            break;
    }
    return "INVALID";
}

string_view GetCameraType(CameraType type)
{
    switch (type) {
        case CameraType::PERSPECTIVE:
            return "perspective";
        case CameraType::ORTHOGRAPHIC:
            return "orthographic";
        case CameraType::INVALID:
        default:
            break;
    }

    return "INVALID";
}

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
bool GetLightType(const string_view type, LightType& out)
{
    out = LightType::INVALID;

    if (type == "directional") {
        out = LightType::DIRECTIONAL;
    } else if (type == "point") {
        out = LightType::POINT;
    } else if (type == "spot") {
        out = LightType::SPOT;
    } else if (type == "ambient") {
        out = LightType::AMBIENT;
    }
    return out != LightType::INVALID;
}

string_view GetLightType(LightType type)
{
    switch (type) {
        case LightType::DIRECTIONAL:
            return "directional";
        case LightType::POINT:
            return "point";
        case LightType::SPOT:
            return "spot";
        case LightType::AMBIENT:
            return "ambient";
        case LightType::INVALID:
        default:
            break;
    }

    return "INVALID";
}
#endif

string_view GetAlphaMode(AlphaMode aMode)
{
    switch (aMode) {
        case AlphaMode::BLEND:
            return "BLEND";
        case AlphaMode::MASK:
            return "MASK";
        case AlphaMode::OPAQUE:
        default:
            return "OPAQUE";
    }
}

string_view GetBlendMode(BlendMode data)
{
    switch (data) {
        case BlendMode::TRANSPARENT_ALPHA:
            return "transparentAlpha";
        case BlendMode::TRANSPARENT_COLOR:
            return "transparentColor";
        case BlendMode::ADD:
            return "add";
        case BlendMode::MODULATE:
            return "modulate";

        case BlendMode::NONE:
        case BlendMode::REPLACE:
        default:
            return "replace";
    }
}

string_view GetAnimationInterpolation(AnimationInterpolation interpolation)
{
    switch (interpolation) {
        case AnimationInterpolation::STEP:
            return "STEP";
        default:
            [[fallthrough]];
        case AnimationInterpolation::INVALID:
            [[fallthrough]];
        case AnimationInterpolation::LINEAR:
            return "LINEAR";
        case AnimationInterpolation::SPLINE:
            return "CUBICSPLINE";
    }
}

string_view GetAnimationPath(AnimationPath path)
{
    switch (path) {
        default:
            [[fallthrough]];
        case AnimationPath::INVALID:
            CORE_LOG_W("invalid animation path %d", path);
            return "translation";
        case AnimationPath::TRANSLATION:
            return "translation";
        case AnimationPath::ROTATION:
            return "rotation";
        case AnimationPath::SCALE:
            return "scale";
        case AnimationPath::WEIGHTS:
            return "weights";
    }
}

uint32_t GetComponentByteSize(ComponentType component)
{
    switch (component) {
        case ComponentType::BYTE:
        case ComponentType::UNSIGNED_BYTE:
            return 1u;

        case ComponentType::SHORT:
        case ComponentType::UNSIGNED_SHORT:
            return 2u;

        case ComponentType::UNSIGNED_INT:
        case ComponentType::INT:
        case ComponentType::FLOAT:
            return 4u;

        default:
            return 0u;
    }
}

uint32_t GetComponentsCount(DataType type)
{
    switch (type) {
        case DataType::SCALAR:
            return 1u;
        case DataType::VEC2:
            return 2u;
        case DataType::VEC3:
            return 3u;
        case DataType::VEC4:
            return 4u;
        case DataType::MAT2:
            return 4u;
        case DataType::MAT3:
            return 9u;
        case DataType::MAT4:
            return 16u;

        case DataType::INVALID:
        default:
            return 0u;
    }
}

ComponentType GetAlternativeType(ComponentType component, size_t aNewByteCount)
{
    switch (component) {
        case ComponentType::UNSIGNED_SHORT:
        case ComponentType::UNSIGNED_INT:
        case ComponentType::UNSIGNED_BYTE: {
            switch (aNewByteCount) {
                case 1u:
                    return ComponentType::UNSIGNED_BYTE;
                case 2u:
                    return ComponentType::UNSIGNED_SHORT;
                case 4u:
                    return ComponentType::UNSIGNED_INT;
                default:
                    break;
            }
            break;
        }

        case ComponentType::BYTE:
        case ComponentType::SHORT:
        case ComponentType::INT: {
            switch (aNewByteCount) {
                case 1u:
                    return ComponentType::BYTE;
                case 2u:
                    return ComponentType::SHORT;
                case 4u:
                    return ComponentType::INT;
                default:
                    break;
            }
            break;
        }

        case ComponentType::FLOAT:
        default:
            // do nothing
            break;
    }

    return component;
}

namespace {
struct AttributeValidation {
    array_view<const DataType> dataTypes;
    array_view<const ComponentType> componentTypes;
};
struct AttributeValidationErrors {
    string_view dataTypeError;
    string_view componentTypeError;
};

constexpr const DataType NORMAL_DATA_TYPES[] = { DataType::VEC3 };
constexpr const ComponentType NORMAL_COMPONENT_TYPES[] = { ComponentType::FLOAT };
constexpr const DataType POSITION_DATA_TYPES[] = { DataType::VEC3 };
constexpr const ComponentType POSITION_COMPONENT_TYPES[] = { ComponentType::FLOAT };
constexpr const DataType TANGENT_DATA_TYPES[] = { DataType::VEC4 };
constexpr const ComponentType TANGENT_COMPONENT_TYPES[] = { ComponentType::FLOAT };
constexpr const DataType TEXCOORD_DATA_TYPES[] = { DataType::VEC2 };
constexpr const ComponentType TEXCOORD_COMPONENT_TYPES[] = { ComponentType::FLOAT, ComponentType::UNSIGNED_BYTE,
    ComponentType::UNSIGNED_SHORT };
constexpr const DataType COLOR_DATA_TYPES[] = { DataType::VEC3, DataType::VEC4 };
constexpr const ComponentType COLOR_COMPONENT_TYPES[] = { ComponentType::FLOAT, ComponentType::UNSIGNED_BYTE,
    ComponentType::UNSIGNED_SHORT };
constexpr const DataType JOINTS_DATA_TYPES[] = { DataType::VEC4 };
constexpr const ComponentType JOINTS_COMPONENT_TYPES[] = { ComponentType::UNSIGNED_BYTE,
    ComponentType::UNSIGNED_SHORT };
constexpr const DataType WEIGHTS_DATA_TYPES[] = { DataType::VEC4 };
constexpr const ComponentType WEIGHTS_COMPONENT_TYPES[] = { ComponentType::FLOAT, ComponentType::UNSIGNED_BYTE,
    ComponentType::UNSIGNED_SHORT };

constexpr const AttributeValidation ATTRIBUTE_VALIDATION[] = {
    { NORMAL_DATA_TYPES, NORMAL_COMPONENT_TYPES },
    { POSITION_DATA_TYPES, POSITION_COMPONENT_TYPES },
    { TANGENT_DATA_TYPES, TANGENT_COMPONENT_TYPES },
    { TEXCOORD_DATA_TYPES, TEXCOORD_COMPONENT_TYPES },
    { COLOR_DATA_TYPES, COLOR_COMPONENT_TYPES },
    { JOINTS_DATA_TYPES, JOINTS_COMPONENT_TYPES },
    { WEIGHTS_DATA_TYPES, WEIGHTS_COMPONENT_TYPES },
};

#if defined(GLTF2_EXTENSION_KHR_MESH_QUANTIZATION)
constexpr const ComponentType NORMAL_COMPONENT_TYPES_Q[] = { ComponentType::BYTE, ComponentType::SHORT,
    ComponentType::FLOAT };
constexpr const ComponentType POSITION_COMPONENT_TYPES_Q[] = { ComponentType::BYTE, ComponentType::UNSIGNED_BYTE,
    ComponentType::SHORT, ComponentType::UNSIGNED_SHORT, ComponentType::FLOAT };
constexpr const ComponentType TANGENT_COMPONENT_TYPES_Q[] = { ComponentType::BYTE, ComponentType::SHORT,
    ComponentType::FLOAT };
constexpr const ComponentType TEXCOORD_COMPONENT_TYPES_Q[] = { ComponentType::BYTE, ComponentType::UNSIGNED_BYTE,
    ComponentType::SHORT, ComponentType::UNSIGNED_SHORT, ComponentType::FLOAT };

constexpr const AttributeValidation ATTRIBUTE_VALIDATION_Q[] = {
    { NORMAL_DATA_TYPES, NORMAL_COMPONENT_TYPES_Q },
    { POSITION_DATA_TYPES, POSITION_COMPONENT_TYPES_Q },
    { TANGENT_DATA_TYPES, TANGENT_COMPONENT_TYPES_Q },
    { TEXCOORD_DATA_TYPES, TEXCOORD_COMPONENT_TYPES_Q },
    { COLOR_DATA_TYPES, COLOR_COMPONENT_TYPES },
    { JOINTS_DATA_TYPES, JOINTS_COMPONENT_TYPES },
    { WEIGHTS_DATA_TYPES, WEIGHTS_COMPONENT_TYPES },
};
#endif

constexpr const AttributeValidationErrors ATTRIBUTE_VALIDATION_ERRORS[] = {
    { "NORMAL accessor must use VEC3.", "NORMAL accessor must use FLOAT." },
    { "POSITION accessor must use VEC3.", "POSITION accessor must use FLOAT." },
    { "TANGENT accessor must use VEC4.", "TANGENT accessor must use FLOAT." },
    { "TEXCOORD accessor must use VEC2.", "TEXCOORD accessor must use FLOAT, UNSIGNED_BYTE, or UNSIGNED_SHORT." },
    { "COLOR accessor must use VEC3 or VEC4.", "COLOR accessor must use FLOAT, UNSIGNED_BYTE, or UNSIGNED_SHORT." },
    { "JOINTS accessor must use VEC4.", "JOINTS accessor must use UNSIGNED_BYTE or UNSIGNED_SHORT." },
    { "WEIGHTS accessor must use VEC4.", "WEIGHTS accessor must use FLOAT, UNSIGNED_BYTE, or UNSIGNED_SHORT." },
};
} // namespace

string_view ValidatePrimitiveAttribute(
    AttributeType attribute, DataType accessorType, ComponentType accessorComponentType)
{
    const auto attributeIndex = static_cast<size_t>(attribute);
    if (attributeIndex <= countof(ATTRIBUTE_VALIDATION)) {
        auto& validation = ATTRIBUTE_VALIDATION[attributeIndex];
        if (std::none_of(validation.dataTypes.begin(), validation.dataTypes.end(),
                [accessorType](const DataType& validType) { return validType == accessorType; })) {
            return ATTRIBUTE_VALIDATION_ERRORS[attributeIndex].dataTypeError;
        } else if (std::none_of(validation.componentTypes.begin(), validation.componentTypes.end(),
                        [accessorComponentType](
                            const ComponentType& validType) { return validType == accessorComponentType; })) {
            return ATTRIBUTE_VALIDATION_ERRORS[attributeIndex].componentTypeError;
        }
    } else {
        return "invalid attribute";
    }
    return string_view();
}

string_view ValidateMorphTargetAttribute(
    AttributeType attribute, DataType accessorType, ComponentType accessorComponentType)
{
    switch (attribute) {
        case AttributeType::NORMAL:
            if (accessorType != DataType::VEC3) {
                return "ValidateMorphTargetAttribute: NORMAL accessor must use VEC3.";
            }
            if (accessorComponentType != ComponentType::FLOAT) {
                return "ValidateMorphTargetAttribute: NORMAL accessor must use FLOAT.";
            }
            break;

        case AttributeType::POSITION:
            if (accessorType != DataType::VEC3) {
                return "ValidateMorphTargetAttribute: POSITION accessor must use VEC3.";
            }
            if (accessorComponentType != ComponentType::FLOAT && accessorComponentType != ComponentType::SHORT) {
                return "ValidateMorphTargetAttribute: POSITION accessor must use FLOAT or SHORT.";
            }
            break;

        case AttributeType::TANGENT:
            if (accessorType != DataType::VEC3) {
                return "ValidateMorphTargetAttribute: TANGENT accessor must use VEC3.";
            }
            if (accessorComponentType != ComponentType::FLOAT) {
                return "ValidateMorphTargetAttribute: TANGENT accessor must use FLOAT.";
            }
            break;

        case AttributeType::INVALID:
        case AttributeType::TEXCOORD:
        case AttributeType::COLOR:
        case AttributeType::JOINTS:
        case AttributeType::WEIGHTS:
        default:
            return "ValidateMorphTargetAttribute: invalid attribute";
    }

    return string_view();
}

#if defined(GLTF2_EXTENSION_KHR_MESH_QUANTIZATION)
string_view ValidatePrimitiveAttributeQuatization(
    AttributeType attribute, DataType accessorType, ComponentType accessorComponentType)
{
    const auto attributeIndex = static_cast<size_t>(attribute);
    if (attributeIndex <= countof(ATTRIBUTE_VALIDATION_Q)) {
        auto& validation = ATTRIBUTE_VALIDATION_Q[attributeIndex];
        if (std::none_of(validation.dataTypes.begin(), validation.dataTypes.end(),
                [accessorType](const DataType& validType) { return validType == accessorType; })) {
            return ATTRIBUTE_VALIDATION_ERRORS[attributeIndex].dataTypeError;
        } else if (std::none_of(validation.componentTypes.begin(), validation.componentTypes.end(),
                        [accessorComponentType](
                            const ComponentType& validType) { return validType == accessorComponentType; })) {
            return ATTRIBUTE_VALIDATION_ERRORS[attributeIndex].componentTypeError;
        }
    } else {
        return "invalid attribute";
    }
    return string_view();
}

string_view ValidateMorphTargetAttributeQuantization(
    AttributeType attribute, DataType accessorType, ComponentType accessorComponentType)
{
    switch (attribute) {
        case AttributeType::NORMAL:
            if (accessorType != DataType::VEC3) {
                return "ValidateMorphTargetAttribute: NORMAL accessor must use VEC3.";
            }
            if (accessorComponentType != ComponentType::FLOAT && accessorComponentType != ComponentType::BYTE &&
                accessorComponentType != ComponentType::SHORT) {
                return "ValidateMorphTargetAttribute: NORMAL accessor must use FLOAT, BYTE or SHORT.";
            }
            break;

        case AttributeType::POSITION:
            if (accessorType != DataType::VEC3) {
                return "ValidateMorphTargetAttribute: POSITION accessor must use VEC3.";
            }
            if (accessorComponentType != ComponentType::FLOAT && accessorComponentType != ComponentType::BYTE &&
                accessorComponentType != ComponentType::SHORT) {
                return "ValidateMorphTargetAttribute: POSITION accessor must use FLOAT, BYTE or SHORT.";
            }
            break;

        case AttributeType::TANGENT:
            if (accessorType != DataType::VEC3) {
                return "ValidateMorphTargetAttribute: TANGENT accessor must use VEC3.";
            }
            if (accessorComponentType != ComponentType::FLOAT && accessorComponentType != ComponentType::BYTE &&
                accessorComponentType != ComponentType::SHORT) {
                return "ValidateMorphTargetAttribute: TANGENT accessor must use FLOAT, BYTE or SHORT.";
            }
            break;

        case AttributeType::INVALID:
        case AttributeType::TEXCOORD:
        case AttributeType::COLOR:
        case AttributeType::JOINTS:
        case AttributeType::WEIGHTS:
        default:
            return "ValidateMorphTargetAttribute: invalid attribute";
    }

    return string_view();
}
#endif

void SplitFilename(const string_view source, string_view& base, string_view& path)
{
    size_t slash = source.rfind('\\');
    if (slash == string_view::npos) {
        slash = source.rfind('/');
    }
    if (slash == string_view::npos) {
        base = source;
        path = {};
        return;
    }
    path = source.substr(0u, slash);
    base = source.substr(slash + 1u);
}

void SplitBaseFilename(const string_view source, string_view& name, string_view& extension)
{
    const size_t slash = source.rfind('.');
    if (slash == string_view::npos) {
        extension = {};
        name = source;
        return;
    }
    name = source.substr(0u, slash);
    extension = source.substr(slash + 1u);
}

string_view ParseDataUri(const string_view in, size_t& offsetToData)
{
    offsetToData = 0u;
    // see: https://en.wikipedia.org/wiki/Data_URI_scheme
    auto const scheme = in.substr(0u, 5u);
    if (scheme != "data:") {
        return {}; // scheme is not data:
    }
    auto pos = in.find(",");
    if (pos == string_view::npos) {
        return {}; // no start of data.
    }
    auto const mediaType = in.substr(5u, pos - 5u);
    offsetToData = pos + 1u;
    pos = mediaType.find_last_of(
        ';'); // technically media-type could contain parameters. but the last parameter should be base64 here
    if (pos == string_view::npos) {
        return {}; // no encoding defined.
    }
    auto const encoding = mediaType.substr(pos + 1u);
    if (encoding != "base64") {
        return {};
    }

    // NOTE: 0 to pos would return media-type without the base64 encoding option. (which leaves all the optional
    // parameters to be handled by user)
    pos = mediaType.find_first_of(';');
    return mediaType.substr(0u, pos); // NOTE: return media-type without any of the parameters.
}

bool DecodeDataURI(vector<uint8_t>& out, string_view in, size_t reqBytes, bool checkSize, string_view& outMimeType)
{
    size_t offsetToData = 0u;
    out.clear();
    if (auto const mimeType = ParseDataUri(in, offsetToData); mimeType.empty()) {
        return false;
    } else {
        outMimeType = mimeType;
    }
    in.remove_prefix(offsetToData);
    out = BASE_NS::Base64Decode(in);

    if (out.empty()) {
        return false;
    }

    if (checkSize) {
        if (out.size() != reqBytes) {
            return false;
        }
    }
    return true;
}

bool IsDataURI(const string_view in)
{
    size_t offsetToData;
    if (auto const mimeType = ParseDataUri(in, offsetToData); !mimeType.empty()) {
        if (mimeType == "application/octet-stream") {
            return true;
        }
        if (mimeType == "image/jpeg") {
            return true;
        }
        if (mimeType == "image/png") {
            return true;
        }
        if (mimeType == "image/bmp") {
            return true;
        }
        if (mimeType == "image/gif") {
            return true;
        }
        if (mimeType == "text/plain") {
            return true;
        }
        if (mimeType == "application/gltf-buffer") {
            return true;
        }
    }
    return false;
}

// Buffer / data helpers
GLTFLoadDataResult::GLTFLoadDataResult(GLTFLoadDataResult&& other) noexcept
    : success(other.success), normalized(other.normalized), error(move(other.error)),
      componentType(other.componentType), componentByteSize(other.componentByteSize),
      componentCount(other.componentCount), elementSize(other.elementSize), elementCount(other.elementCount),
      min(move(other.min)), max(move(other.max)), data(move(other.data))
{}

GLTFLoadDataResult& GLTFLoadDataResult::operator=(GLTFLoadDataResult&& other) noexcept
{
    success = other.success;
    normalized = other.normalized;
    error = move(other.error);
    componentType = other.componentType;
    componentByteSize = other.componentByteSize;
    componentCount = other.componentCount;
    elementSize = other.elementSize;
    elementCount = other.elementCount;
    min = move(other.min);
    max = move(other.max);
    data = move(other.data);

    return *this;
}

// Populate GLTF buffers with data.
BufferLoadResult LoadBuffers(const Data& data, IFileManager& fileManager)
{
    BufferLoadResult result;
    // Load data to all buffers.
    for (size_t i = 0u; i < data.buffers.size(); ++i) {
        Buffer* buffer = data.buffers[i].get();
        if (buffer->data.empty()) {
            result = LoadBuffer(data, *buffer, fileManager);
            if (!result.success) {
                return result;
            }
        }
    }

    // Set up bufferview data pointers.
    for (size_t i = 0u; i < data.bufferViews.size(); ++i) {
        BufferView* view = data.bufferViews[i].get();
        view->data = &(view->buffer->data[view->byteOffset]);
    }

    return result;
}

UriLoadResult LoadUri(const string_view uri, const string_view expectedMimeType, const string_view filePath,
    IFileManager& fileManager, string_view& outExtension, vector<uint8_t>& outData)
{
    size_t offsetToData;
    if (auto const mimeType = ParseDataUri(uri, offsetToData); !mimeType.empty()) {
        bool isValidMimeType = true;
        const auto pos = mimeType.find_first_of('/');
        if (pos != string_view::npos) {
            auto const type = mimeType.substr(0u, pos);

            if (!expectedMimeType.empty()) {
                if (type != expectedMimeType) {
                    isValidMimeType = false;
                }
            }
            outExtension = mimeType.substr(pos + 1u);
        }
        if (isValidMimeType) {
            string_view outMimeType;
            DecodeDataURI(outData, uri, 0u, false, outMimeType);
            if (outData.empty()) {
                return URI_LOAD_FAILED_TO_DECODE_BASE64;
            }
        } else {
            return URI_LOAD_FAILED_INVALID_MIME_TYPE;
        }
    } else {
        string_view baseName, extension;
        SplitBaseFilename(uri, baseName, extension);
        outExtension = extension;
        if (!ReadUriToVector(filePath, fileManager, uri, outData)) {
            return URI_LOAD_FAILED_TO_READ_FILE;
        }
    }

    return URI_LOAD_SUCCESS;
}

// Load accessor data.
GLTFLoadDataResult LoadData(Accessor const& accessor)
{
    GLTFLoadDataResult result;
    result.success = true;

    const auto* bufferView = accessor.bufferView;
    const auto dataType = accessor.type;
    const auto component = accessor.componentType;

    result.normalized = accessor.normalized;
    result.componentType = accessor.componentType;
    result.componentByteSize = GetComponentByteSize(component);
    result.componentCount = GetComponentsCount(dataType);
    result.elementSize = result.componentCount * result.componentByteSize;
    result.elementCount = accessor.count;
    result.min = accessor.min;
    result.max = accessor.max;

    if (bufferView) {
        if (bufferView->buffer) {
            vector<uint8_t> fileData = Read(accessor);
            if (fileData.empty()) {
                result.error = "Failed to load attribute data.\n";
                result.success = false;
            }

            result.data.swap(fileData);
        }
        if (accessor.sparse.count) {
            LoadSparseAccessor(accessor, result);
        }
    }

    return result;
}

// class Data from GLTFData.h
Data::Data(IFileManager& fileManager) : fileManager_(fileManager) {}

bool Data::LoadBuffers()
{
    BufferLoadResult result = GLTF2::LoadBuffers(*this, fileManager_);
    return result.success;
}

void Data::ReleaseBuffers()
{
    // Reset bufferview pointers.
    for (size_t i = 0u; i < bufferViews.size(); ++i) {
        BufferView* view = bufferViews[i].get();
        view->data = nullptr;
    }

    // Release buffer data.
    for (size_t i = 0u; i < buffers.size(); ++i) {
        Buffer* buffer = buffers[i].get();
        buffer->data = vector<uint8_t>();
    }
}

vector<string> Data::GetExternalFileUris()
{
    vector<string> result;

    // All images in this glTF.
    for (auto const& image : images) {
        if (!IsDataURI(image->uri)) {
            result.push_back(image->uri);
        }
    }

    // All buffers in this glTF (in case there are several .bin files).
    for (auto const& buffer : buffers) {
        if (!IsDataURI(buffer->uri)) {
            result.push_back(buffer->uri);
        }
    }

#if defined(GLTF2_EXTENSION_HW_XR_EXT)
    // All thumbnails glTF (in case there are non-embedded thumbnail uris).
    for (auto const& thumbnail : thumbnails) {
        if (!IsDataURI(thumbnail.uri)) {
            result.push_back(thumbnail.uri);
        }
    }
#endif

    return result;
}

size_t Data::GetDefaultSceneIndex() const
{
    if (defaultScene != nullptr) {
        for (size_t i = 0u; i < scenes.size(); ++i) {
            if (scenes[i].get() == defaultScene) {
                return i;
            }
        }
    }

    return CORE_GLTF_INVALID_INDEX;
}

size_t Data::GetSceneCount() const
{
    return scenes.size();
}

size_t Data::GetThumbnailImageCount() const
{
#if defined(GLTF2_EXTENSION_HW_XR_EXT)
    return thumbnails.size();
#else
    return 0;
#endif
}

IGLTFData::ThumbnailImage Data::GetThumbnailImage(size_t thumbnailIndex)
{
    IGLTFData::ThumbnailImage result;

#if defined(GLTF2_EXTENSION_HW_XR_EXT)
    if (thumbnailIndex >= thumbnails.size()) {
        return result;
    }

    auto& thumbnail = thumbnails[thumbnailIndex];
    if (thumbnail.data.empty()) {
        // Load thumbnail data.
        string_view extension;
        if (LoadUri(thumbnail.uri, "image", filepath, fileManager_, extension, thumbnail.data) != URI_LOAD_SUCCESS) {
            thumbnails[thumbnailIndex].data.clear();
            thumbnails[thumbnailIndex].extension = "";
        } else {
            thumbnail.extension = extension;
        }
    }

    result.data = thumbnail.data;
    result.extension = thumbnail.extension;
#endif

    return result;
}

void Data::Destroy()
{
    delete this;
}

// class SceneData from GLTFData.h
SceneData::SceneData(unique_ptr<GLTF2::Data> data) : data_(BASE_NS::move(data)) {}

const GLTF2::Data* SceneData::GetData() const
{
    return data_.get();
}

size_t SceneData::GetDefaultSceneIndex() const
{
    return data_->GetDefaultSceneIndex();
}

size_t SceneData::GetSceneCount() const
{
    return data_->GetSceneCount();
}

const IInterface* SceneData::GetInterface(const BASE_NS::Uid& uid) const
{
    if (uid == ISceneData::UID) {
        return static_cast<const ISceneData*>(this);
    }
    if (uid == SceneData::UID) {
        return static_cast<const SceneData*>(this);
    }
    if (uid == IInterface::UID) {
        return static_cast<const IInterface*>(this);
    }
    return nullptr;
}

IInterface* SceneData::GetInterface(const BASE_NS::Uid& uid)
{
    if (uid == ISceneData::UID) {
        return static_cast<ISceneData*>(this);
    }
    if (uid == SceneData::UID) {
        return static_cast<SceneData*>(this);
    }
    if (uid == IInterface::UID) {
        return static_cast<IInterface*>(this);
    }
    return nullptr;
}

void SceneData::Ref()
{
    ++refcnt_;
}

void SceneData::Unref()
{
    if (--refcnt_ == 0) {
        delete this;
    }
}
} // namespace GLTF2
CORE3D_END_NAMESPACE()
