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

#include "gltf/gltf2_loader.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>

#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/perf/cpu_perf_scope.h>
#include <core/perf/intf_performance_data_manager.h>

#include "gltf/data.h"
#include "gltf/gltf2_util.h"
#include "util/json_util.h"

namespace {
#include <3d/shaders/common/3d_dm_structures_common.h>
}

inline void SetError(CORE3D_NS::GLTF2::LoadResult& loadResult, const BASE_NS::string_view message)
{
    loadResult.error += message + '\n';
    loadResult.success = false;
}

#define RETURN_WITH_ERROR(loadResult, message) \
    {                                          \
        SetError((loadResult), (message));     \
        return false;                          \
    }

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;

namespace GLTF2 {
namespace {
const string_view SUPPORTED_EXTENSIONS[] = {
#if defined(GLTF2_EXTENSION_IGFX_COMPRESSED)
    "IGFX_compressed",
#endif
#if defined(GLTF2_EXTENSION_KHR_LIGHTS)
    "KHR_lights_punctual",
#endif
#if defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
    "KHR_lights_pbr",
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_CLEARCOAT)
    "KHR_materials_clearcoat",
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_EMISSIVE_STRENGTH)
    "KHR_materials_emissive_strength",
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_IOR)
    "KHR_materials_ior",
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_PBRSPECULARGLOSSINESS)
    "KHR_materials_pbrSpecularGlossiness",
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SHEEN)
    "KHR_materials_sheen",
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SPECULAR)
    "KHR_materials_specular",
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_TRANSMISSION)
    "KHR_materials_transmission",
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_UNLIT)
    "KHR_materials_unlit",
#endif
#if defined(GLTF2_EXTENSION_KHR_MESH_QUANTIZATION)
    "KHR_mesh_quantization",
#endif
#if defined(GLTF2_EXTENSION_KHR_TEXTURE_BASISU)
    "KHR_texture_basisu",
#endif
#if defined(GLTF2_EXTENSION_KHR_TEXTURE_TRANSFORM)
    "KHR_texture_transform",
#endif
#if defined(GLTF2_EXTENSION_HW_XR_EXT)
    "HW_XR_EXT",
#endif
#if defined(GLTF2_EXTENSION_EXT_LIGHTS_IMAGE_BASED)
    "EXT_lights_image_based",
#endif
    "MSFT_texture_dds",
    // legacy stuff found potentially in animoji models
    "IGFX_lights",
    "IGFX_environment",
};

// Replace percent-encoded characters from the URI.
// NOTE: glTF spec says "Reserved characters must be percent-encoded, per RFC 3986, Section 2.2.". The RFC says that
// for consistency, percent encoded unreserved characters (alphanum, '-', '.', '_', '~') should be also decoded. The RFC
// doesn't list space as reserved, but it' s still somehow expected.
// -> Not limiting to reserved and unreserved characters, but converts everything in the range of [%00, %FF].
void DecodeUri(string& uri)
{
    string::size_type pos = 0;
    while ((pos = uri.find('%', pos)) != string::npos) {
        // there should be at least two characters after '%'
        if ((pos + 2) < uri.size()) {
            // start converting after '%'
            const auto begin = uri.data() + (pos + 1);
            // convert up to two characters
            const auto end = begin + 2;
            uint32_t val;
            if (const auto result = std::from_chars(begin, end, val, 16);
                (result.ec == std::errc()) && result.ptr == end) {
                // replace '%' with the hex value converted to char
                *(begin - 1) = static_cast<char>(val);
                // remove the encoding
                uri.erase(pos + 1, 2);
            }
        }
        pos++;
    }
}

template<typename EnumType, typename InputType>
bool RangedEnumCast(LoadResult& loadResult, EnumType& out, InputType input)
{
    if (input >= static_cast<int>(EnumType::BEGIN) && input < static_cast<int>(EnumType::COUNT)) {
        out = static_cast<EnumType>(input);
        return true;
    }

    RETURN_WITH_ERROR(loadResult, "Invalid enum cast");
}

template<typename Parser>
bool ParseObject(LoadResult& loadResult, const json::value& jsonObject, Parser parser)
{
    if (!jsonObject.is_object()) {
        RETURN_WITH_ERROR(loadResult, "Parsing GLTF failed: expected JSON object");
    }

    return parser(loadResult, jsonObject);
}

template<typename Parser>
bool ParseObject(LoadResult& loadResult, const json::value& jsonObject, const string_view name, Parser parser)
{
    if (auto it = jsonObject.find(name); it) {
        return ParseObject(loadResult, *it, parser);
    }

    return true;
}

template<typename Parser>
bool ForEachInArray(LoadResult& loadResult, const json::value& jsonArray, Parser parser)
{
    if (!jsonArray.is_array()) {
        RETURN_WITH_ERROR(loadResult, "Parsing GLTF failed: expected JSON array");
    }

    for (const auto& item : jsonArray.array_) {
        if (!parser(loadResult, item)) {
            return false;
        }
    }

    return true;
}

template<typename Parser>
bool ForEachInArray(LoadResult& loadResult, const json::value& jsonObject, const string_view name, Parser parser)
{
    if (auto it = jsonObject.find(name); it) {
        return ForEachInArray(loadResult, *it, parser);
    }

    return true;
}

template<typename Parser>
bool ForEachObjectInArray(LoadResult& loadResult, const json::value& jsonObject, Parser parser)
{
    return ForEachInArray(loadResult, jsonObject, [parser](LoadResult& loadResult, const json::value& item) -> bool {
        return ParseObject(loadResult, item,
            [parser](LoadResult& loadResult, const json::value& item) -> bool { return parser(loadResult, item); });
    });
}

template<typename Parser>
bool ForEachObjectInArray(LoadResult& loadResult, const json::value& jsonObject, const string_view name, Parser parser)
{
    return ForEachInArray(
        loadResult, jsonObject, name, [parser](LoadResult& loadResult, const json::value& item) -> bool {
            return ParseObject(loadResult, item,
                [parser](LoadResult& loadResult, const json::value& item) -> bool { return parser(loadResult, item); });
        });
}

bool ParseOptionalString(LoadResult& loadResult, string& out, const json::value& jsonObject, const string_view name,
    const string_view defaultValue)
{
    if (const auto value = jsonObject.find(name); value) {
        if (!value->is_string()) {
            RETURN_WITH_ERROR(loadResult, "expected string");
        }
        if (value->string_.find('\\') != string::npos) {
            out = json::unescape(value->string_);
        } else {
            out = value->string_;
        }

        return true;
    }

    out = defaultValue;
    return true;
}

template<typename Number>
void ConvertStringToValue(const string_view str, Number& value)
{
#if defined(__OHOS_PLATFORM__) || defined(__linux__) || defined(__APPLE__)
    if constexpr (std::is_integral_v<Number>) {
        std::from_chars(str.data(), str.data() + str.size(), value);
    } else {
        value = strtof(str.data(), nullptr);
    }
#else
    std::from_chars(str.data(), str.data() + str.size(), value);
#endif
}

template<typename T>
bool ParseOptionalNumber(
    LoadResult& loadResult, T& out, const json::value& jsonObject, const string_view name, T defaultValue)
{
    if (const auto it = jsonObject.find(name); it) {
        if (it->is_number()) {
            out = it->as_number<T>();
            return true;
        } else if (it->is_string()) {
            // Some glTF files have strings in place of numbers, so we try to perform auto conversion here.
            ConvertStringToValue(it->string_, out);
            CORE_LOG_W("Expected number when parsing but found string (performing auto conversion to number type).");
            return true;
        } else {
            out = defaultValue;
            RETURN_WITH_ERROR(loadResult, "expected number");
        }
    }
    out = defaultValue;
    return true;
}

bool ParseOptionalBoolean(
    LoadResult& loadResult, bool& out, const json::value& jsonObject, const string_view name, bool defaultValue)
{
    if (const auto value = jsonObject.find(name); value) {
        if (value->is_boolean()) {
            out = value->boolean_;
            return true;
        } else {
            RETURN_WITH_ERROR(loadResult, "expected boolean");
        }
    }

    out = defaultValue;
    return true;
}

template<typename T>
bool ParseOptionalNumberArray(LoadResult& loadResult, vector<T>& out, const json::value& jsonObject,
    const string_view name, vector<T> defaultValue, uint32_t minSize = 0,
    uint32_t maxSize = std::numeric_limits<int>::max())
{
    if (auto it = jsonObject.find(name); it) {
        auto& values = *it;
        if (values.is_array()) {
            out.reserve(values.array_.size());
            for (const auto& item : values.array_) {
                if (item.is_number()) {
                    out.push_back(item.as_number<T>());

                    if (out.size() > maxSize) {
                        return true;
                    }
                } else {
                    RETURN_WITH_ERROR(loadResult, "expected array of numbers");
                }
            }

            if (out.size() >= minSize) {
                return true;
            }
        } else {
            RETURN_WITH_ERROR(loadResult, "expected array");
        }
    }

    out = defaultValue;
    return true;
}

/** Tries to parse a Core::Math object (e.g. Vec4, Quat, and Mat4X4) from json. */
template<typename T>
bool ParseOptionalMath(
    LoadResult& loadResult, T& out, const json::value& jsonObject, const string_view name, T defaultValue)
{
    if (auto it = jsonObject.find(name); it) {
        if (it->is_array() && (it->array_.size() == countof(out.data))) {
            auto& array = it->array_;
            std::transform(array.begin(), array.end(), out.data, [](const json::value& item) {
                if (item.is_number()) {
                    return item.as_number<float>();
                } else if (item.is_string()) {
                    float value;
                    ConvertStringToValue(item.string_, value);
                    return value;
                } else {
                    return 0.f;
                }
            });
            return true;
        } else {
            RETURN_WITH_ERROR(loadResult, "expected array of size when parsing");
        }
    } else {
        out = defaultValue;
        return true;
    }
}

std::optional<std::pair<uint32_t, uint32_t>> ParseVersion(string_view version)
{
    uint32_t min = 0;
    uint32_t maj = 0;
    if (const auto delim = version.find('.'); delim == string::npos) {
        return {};
    } else if (auto result = std::from_chars(version.data(), version.data() + delim, maj, 10);
               result.ec != std::errc() || *(result.ptr) != '.') {
        return {};
    } else if (auto result2 = std::from_chars(result.ptr + 1, version.data() + version.size(), min, 10);
               result2.ec != std::errc() || *(result2.ptr) != '\0') {
        return {};
    }
    return std::make_pair(maj, min);
}

bool ParseBuffer(LoadResult& loadResult, const json::value& jsonData)
{
    bool result = true;

    int32_t length = 0;
    if (!SafeGetJsonValue(jsonData, "byteLength", loadResult.error, length)) {
        SetError(loadResult, "Failed to read buffer.byteLength");
        result = false;
    }

    if (length < 1) {
        SetError(loadResult, "buffer.byteLength was smaller than 1 byte");
        result = false;
    }

    string uri;
    if (!ParseOptionalString(loadResult, uri, jsonData, "uri", string())) {
        result = false;
    }

    auto buffer = make_unique<Buffer>();
    if (result) {
        DecodeUri(uri);
        buffer->uri = move(uri);
        buffer->byteLength = size_t(length);
    }

    loadResult.data->buffers.push_back(move(buffer));

    return result;
}

std::optional<Buffer*> BufferViewBuffer(LoadResult& loadResult, const json::value& jsonData)
{
    int index = 0;
    if (!SafeGetJsonValue(jsonData, "buffer", loadResult.error, index)) {
        SetError(loadResult, "Failed to read bufferView.buffer");
        return std::nullopt;
    } else if (index < 0 || size_t(index) >= loadResult.data->buffers.size()) {
        SetError(loadResult, "bufferView.buffer isn't valid index");
        return std::nullopt;
    } else {
        return loadResult.data->buffers[(unsigned int)index].get();
    }
}

std::optional<int> BufferViewByteLength(LoadResult& loadResult, const json::value& jsonData)
{
    int length = 0;
    if (!SafeGetJsonValue(jsonData, "byteLength", loadResult.error, length)) {
        SetError(loadResult, "Failed to read bufferView.byteLength");
        return std::nullopt;
    } else if (length < 1) {
        SetError(loadResult, "bufferView.byteLength was smaller than 1 byte");
        return std::nullopt;
    }
    return length;
}

std::optional<int> BufferViewByteOffset(
    LoadResult& loadResult, const json::value& jsonData, std::optional<Buffer*> buffer, std::optional<int> byteLength)
{
    int offset;
    if (!ParseOptionalNumber<int>(loadResult, offset, jsonData, "byteOffset", 0)) { // "default": 0
        return std::nullopt;
    } else if (offset < 0) {
        SetError(loadResult, "bufferView.byteOffset isn't valid offset");
        return std::nullopt;
    } else if (!buffer || !(*buffer) || !byteLength || ((*buffer)->byteLength < (size_t)(offset + *byteLength))) {
        SetError(loadResult, "bufferView.byteLength is larger than buffer.byteLength");
        return std::nullopt;
    }
    return offset;
}

std::optional<int> BufferViewByteStride(LoadResult& loadResult, const json::value& jsonData)
{
    // "default": 0 "minimum": 4, "maximum": 252, "multipleOf":
    int stride;
    if (!ParseOptionalNumber<int>(loadResult, stride, jsonData, "byteStride", 0)) {
        return std::nullopt;
    } else if ((stride < 4 && stride != 0) || stride > 252 || (stride % 4)) {
        SetError(loadResult, "bufferView.byteStride isn't valid stride");
        return std::nullopt;
    }
    return stride;
}

std::optional<BufferTarget> BufferViewTarget(LoadResult& loadResult, const json::value& jsonData)
{
    // "default": NOT_DEFINED if set then ARRAY_BUFFER or ELEMENT_ARRAY_BUFFER
    int target;
    if (!ParseOptionalNumber<int>(
            loadResult, target, jsonData, "target", static_cast<int>(BufferTarget::NOT_DEFINED))) {
        return std::nullopt;
    } else if (target != static_cast<int>(BufferTarget::NOT_DEFINED) &&
               target != static_cast<int>(BufferTarget::ARRAY_BUFFER) &&
               target != static_cast<int>(BufferTarget::ELEMENT_ARRAY_BUFFER)) {
        SetError(loadResult, "bufferView.target isn't valid target");
        return std::nullopt;
    }
    return static_cast<BufferTarget>(target);
}

bool ParseBufferView(LoadResult& loadResult, const json::value& jsonData)
{
    const auto buffer = BufferViewBuffer(loadResult, jsonData);
    const auto byteLength = BufferViewByteLength(loadResult, jsonData);
    const auto offset = BufferViewByteOffset(loadResult, jsonData, buffer, byteLength);
    const auto stride = BufferViewByteStride(loadResult, jsonData);
    const auto target = BufferViewTarget(loadResult, jsonData);

    const auto result = byteLength && buffer && offset && stride && target;

    auto view = make_unique<BufferView>();
    if (result) {
        view->buffer = *buffer;
        view->byteLength = size_t(*byteLength);
        view->byteOffset = size_t(*offset);
        view->byteStride = size_t(*stride);
        view->target = *target;
    }

    loadResult.data->bufferViews.push_back(move(view));

    return result;
}

std::optional<ComponentType> AccessorComponentType(LoadResult& loadResult, const json::value& jsonData)
{
    int componentType = 0;
    if (!SafeGetJsonValue(jsonData, "componentType", loadResult.error, componentType)) {
        SetError(loadResult, "Failed to read accessor.componentType");
        return std::nullopt;
    }
    return static_cast<ComponentType>(componentType);
}

std::optional<uint32_t> AccessorCount(LoadResult& loadResult, const json::value& jsonData)
{
    int count = 0;
    if (!SafeGetJsonValue(jsonData, "count", loadResult.error, count)) {
        SetError(loadResult, "Failed to read accessor.count");
        return std::nullopt;
    } else if (count < 1) {
        SetError(loadResult, "Accessor.count is invalid");
        return std::nullopt;
    }
    return count;
}

std::optional<DataType> AccessorType(LoadResult& loadResult, const json::value& jsonData)
{
    string_view type;
    if (!SafeGetJsonValue(jsonData, "type", loadResult.error, type)) {
        SetError(loadResult, "Failed to read accessor.type");
        return std::nullopt;
    }

    DataType datatype;
    if (!GetDataType(type, datatype)) {
        SetError(loadResult, "Invalid or unsupported data type");
        return std::nullopt;
    }
    return datatype;
}

std::optional<BufferView*> AccessorBufferView(LoadResult& loadResult, const json::value& jsonData)
{
    BufferView* bufferView = nullptr;

    size_t bufferIndex;
    if (!ParseOptionalNumber<size_t>(loadResult, bufferIndex, jsonData, "bufferView", GLTF_INVALID_INDEX)) {
        return std::nullopt;
    } else if (bufferIndex != GLTF_INVALID_INDEX) {
        if (bufferIndex < loadResult.data->bufferViews.size()) {
            bufferView = loadResult.data->bufferViews[bufferIndex].get();
        } else {
            SetError(loadResult, "Accessor.bufferView is invalid");
            return std::nullopt;
        }
    }
    return bufferView;
}

std::optional<uint32_t> AccessorByteOffset(
    LoadResult& loadResult, const json::value& jsonData, std::optional<BufferView*> bufferView)
{
    uint32_t byteOffset;
    if (!ParseOptionalNumber<uint32_t>(loadResult, byteOffset, jsonData, "byteOffset", 0)) {
        return false;
    } else if (!bufferView || (!(*bufferView)) || (*bufferView)->byteLength <= byteOffset) {
        SetError(loadResult, "Accessor.byteOffset isn't valid offset");
        return std::nullopt;
    }
    return byteOffset;
}

std::optional<bool> AccessorNormalized(LoadResult& loadResult, const json::value& jsonData)
{
    bool normalized = false;
    if (!ParseOptionalBoolean(loadResult, normalized, jsonData, "normalized", false)) {
        return std::nullopt;
    }
    return normalized;
}

std::optional<vector<float>> AccessorMin(LoadResult& loadResult, const json::value& jsonData)
{
    vector<float> min;
    if (!ParseOptionalNumberArray(loadResult, min, jsonData, "min", vector<float>(), 1U, 16U)) {
        return std::nullopt;
    }
    return move(min);
}

std::optional<vector<float>> AccessorMax(LoadResult& loadResult, const json::value& jsonData)
{
    vector<float> max;
    if (!ParseOptionalNumberArray(loadResult, max, jsonData, "max", vector<float>(), 1U, 16U)) {
        return std::nullopt;
    }
    return move(max);
}

std::optional<SparseIndices> AccessorSparseIndices(LoadResult& loadResult, const json::value& jsonData)
{
    SparseIndices indices;
    size_t bufferViewIndex;
    if (!SafeGetJsonValue(jsonData, "bufferView", loadResult.error, bufferViewIndex)) {
        SetError(loadResult, "Failed to read Sparse.indices.bufferView");
        return std::nullopt;
    }

    if (bufferViewIndex < loadResult.data->bufferViews.size()) {
        indices.bufferView = loadResult.data->bufferViews[bufferViewIndex].get();
    } else {
        SetError(loadResult, "Sparse.indices.bufferView is invalid");
        return std::nullopt;
    }

    int32_t componentType;
    if (!SafeGetJsonValue(jsonData, "componentType", loadResult.error, componentType)) {
        SetError(loadResult, "Failed to read Sparse.indices.componentType");
        return std::nullopt;
    }

    indices.componentType = static_cast<ComponentType>(componentType);

    if (!ParseOptionalNumber<uint32_t>(loadResult, indices.byteOffset, jsonData, "bufferOffset", 0)) {
        return std::nullopt;
    }

    return indices;
}

std::optional<SparseValues> AccessorSparseValues(LoadResult& loadResult, const json::value& jsonData)
{
    SparseValues values;
    size_t bufferViewIndex;
    if (!SafeGetJsonValue(jsonData, "bufferView", loadResult.error, bufferViewIndex)) {
        SetError(loadResult, "Failed to read Sparse.values.bufferView");
        return std::nullopt;
    } else if (bufferViewIndex < loadResult.data->bufferViews.size()) {
        values.bufferView = loadResult.data->bufferViews[bufferViewIndex].get();
    } else {
        SetError(loadResult, "Sparse.values.bufferView is invalid");
        return std::nullopt;
    }

    if (!ParseOptionalNumber<uint32_t>(loadResult, values.byteOffset, jsonData, "bufferOffset", 0)) {
        return std::nullopt;
    }

    return values;
}

std::optional<Sparse> AccessorSparse(LoadResult& loadResult, const json::value& jsonData)
{
    Sparse sparse;
    if (auto sparseJson = jsonData.find("sparse"); sparseJson) {
        if (!SafeGetJsonValue(*sparseJson, "count", loadResult.error, sparse.count)) {
            SetError(loadResult, "Failed to read sparse.count");
            return std::nullopt;
        }

        if (auto it = sparseJson->find("indices"); it) {
            if (auto ret = AccessorSparseIndices(loadResult, *it); ret) {
                sparse.indices = *ret;
            } else {
                return std::nullopt;
            }
        } else {
            SetError(loadResult, "Failed to read sparse.indices");
            return std::nullopt;
        }

        if (auto it = sparseJson->find("values"); it) {
            if (auto ret = AccessorSparseValues(loadResult, *it); ret) {
                sparse.values = *ret;
            } else {
                return std::nullopt;
            }
        } else {
            SetError(loadResult, "Failed to read sparse.values");
            return std::nullopt;
        }
    }
    return sparse;
}

bool ValidateAccessor(LoadResult& loadResult, ComponentType componentType, uint32_t count, DataType dataType,
    const BufferView* bufferView, uint32_t byteOffset, const vector<float>& min, const vector<float>& max)
{
    if (const size_t elementSize = GetComponentsCount(dataType) * GetComponentByteSize(componentType); elementSize) {
        if (bufferView) {
            // check count against buffer size
            const auto bufferSize = bufferView->byteLength - byteOffset;
            const auto elementCount = bufferSize / elementSize;

            if (count > elementCount) {
                SetError(loadResult, "Accessor.count is invalid");
                return false;
            }
        }
    } else {
        SetError(loadResult, "Accessor.type or componentType is invalid");
        return false;
    }
    const auto minVecSize = min.size();
    const auto maxVecSize = max.size();
    if (minVecSize == maxVecSize) {
        if (minVecSize && minVecSize != GetComponentsCount(dataType)) {
            SetError(loadResult, "Accessor.min and max vector size doesn't match component count");
            return false;
        }
    } else {
        SetError(loadResult, "Accessor.min and max vectors have different size");
        return false;
    }
    return true;
}

bool ParseAccessor(LoadResult& loadResult, const json::value& jsonData)
{
    const auto componentType = AccessorComponentType(loadResult, jsonData);
    const auto count = AccessorCount(loadResult, jsonData);
    const auto datatype = AccessorType(loadResult, jsonData);

    const auto bufferView = AccessorBufferView(loadResult, jsonData);
    const auto byteOffset = AccessorByteOffset(loadResult, jsonData, bufferView);
    const auto normalized = AccessorNormalized(loadResult, jsonData);
    auto max = AccessorMax(loadResult, jsonData);
    auto min = AccessorMin(loadResult, jsonData);
    auto sparse = AccessorSparse(loadResult, jsonData);

    bool result = true;
    if (!ValidateAccessor(loadResult, componentType.value_or(ComponentType::INVALID), count.value_or(0U),
            datatype.value_or(DataType::INVALID), bufferView.value_or(nullptr), byteOffset.value_or(0U),
            min.value_or(vector<float> {}), max.value_or(vector<float> {}))) {
        result = false;
    }

    auto accessor = make_unique<Accessor>();
    result = result && componentType && count && datatype && bufferView && byteOffset && max && min && sparse;
    if (result) {
        accessor->componentType = static_cast<ComponentType>(*componentType);
        accessor->count = *count;
        accessor->type = *datatype;
        accessor->bufferView = *bufferView;
        accessor->byteOffset = *byteOffset;
        accessor->max = move(*max);
        accessor->min = move(*min);
        accessor->normalized = *normalized;
        accessor->sparse = move(*sparse);
    }
    loadResult.data->accessors.push_back(move(accessor));

    return result;
}

bool ParseTextureInfo(LoadResult& loadResult, TextureInfo& info, const json::value& jsonData)
{
    if (!ParseOptionalNumber<uint32_t>(loadResult, info.index, jsonData, "index", GLTF_INVALID_INDEX)) {
        return false;
    }

    if (info.index != GLTF_INVALID_INDEX && info.index < loadResult.data->textures.size()) {
        info.texture = loadResult.data->textures[info.index].get();
    }

    if (!ParseOptionalNumber<uint32_t>(loadResult, info.texCoordIndex, jsonData, "texCoord", GLTF_INVALID_INDEX)) {
        return false;
    }
#if defined(GLTF2_EXTENSION_KHR_TEXTURE_TRANSFORM)
    const auto textureTransformParser = [&info](LoadResult& loadResult, const json::value& extensions) -> bool {
        return ParseObject(loadResult, extensions, "KHR_texture_transform",
            [&info](LoadResult& loadResult, const json::value& textureTransform) {
                if (!ParseOptionalMath(loadResult, info.transform.offset, textureTransform, "offset", { 0.0f, 0.0f })) {
                    return false;
                }

                if (!ParseOptionalNumber(loadResult, info.transform.rotation, textureTransform, "rotation", 0.0f)) {
                    return false;
                }

                if (!ParseOptionalMath(loadResult, info.transform.scale, textureTransform, "scale", { 1.0f, 1.0f })) {
                    return false;
                }

                if (!ParseOptionalNumber(
                        loadResult, info.transform.texCoordIndex, textureTransform, "texCoord", GLTF_INVALID_INDEX)) {
                    return false;
                }
                return true;
            });
    };
    if (!ParseObject(loadResult, jsonData, "extensions", textureTransformParser)) {
        return false;
    }
#endif
    return true;
}

bool ParseMetallicRoughness(LoadResult& loadResult, const json::value& jsonData, MetallicRoughness& metallicRoughness)
{
    if (auto roughnessJson = jsonData.find("pbrMetallicRoughness"); roughnessJson) {
        if (!ParseOptionalMath(loadResult, metallicRoughness.baseColorFactor, *roughnessJson, "baseColorFactor",
                metallicRoughness.baseColorFactor)) {
            return false;
        }

        if (!ParseObject(loadResult, *roughnessJson, "baseColorTexture",
                [&metallicRoughness](LoadResult& loadResult, const json::value& baseJson) -> bool {
                    return ParseTextureInfo(loadResult, metallicRoughness.baseColorTexture, baseJson);
                })) {
            return false;
        }

        if (!ParseObject(loadResult, *roughnessJson, "metallicRoughnessTexture",
                [&metallicRoughness](LoadResult& loadResult, const json::value& baseJson) -> bool {
                    return ParseTextureInfo(loadResult, metallicRoughness.metallicRoughnessTexture, baseJson);
                })) {
            return false;
        }

        if (!ParseOptionalNumber<float>(loadResult, metallicRoughness.metallicFactor, *roughnessJson, "metallicFactor",
                metallicRoughness.metallicFactor)) {
            return false;
        }

        if (!ParseOptionalNumber<float>(loadResult, metallicRoughness.roughnessFactor, *roughnessJson,
                "roughnessFactor", metallicRoughness.roughnessFactor)) {
            return false;
        }
    }
    return true;
}

std::optional<Sampler*> TextureSampler(LoadResult& loadResult, const json::value& jsonData)
{
    size_t samplerIndex;
    if (!ParseOptionalNumber<size_t>(loadResult, samplerIndex, jsonData, "sampler", GLTF_INVALID_INDEX)) {
        return std::nullopt;
    }

    Sampler* sampler = loadResult.data->defaultSampler.get();
    if (samplerIndex != GLTF_INVALID_INDEX) {
        if (samplerIndex < loadResult.data->samplers.size()) {
            sampler = loadResult.data->samplers[samplerIndex].get();
        } else {
            SetError(loadResult, "Invalid sampler index");
            return std::nullopt;
        }
    }
    return sampler;
}

std::optional<Image*> TextureSource(LoadResult& loadResult, const json::value& jsonData)
{
    size_t imageIndex;
    if (!ParseOptionalNumber<size_t>(loadResult, imageIndex, jsonData, "source", GLTF_INVALID_INDEX)) {
        return std::nullopt;
    }

    Image* image = nullptr;
    if (imageIndex != GLTF_INVALID_INDEX) {
        if (imageIndex < loadResult.data->images.size()) {
            image = loadResult.data->images[imageIndex].get();
        } else {
            SetError(loadResult, "Invalid image index");
            return std::nullopt;
        }
    }
    return image;
}

bool ParseTexture(LoadResult& loadResult, const json::value& jsonData)
{
    auto sampler = TextureSampler(loadResult, jsonData);
    auto image = TextureSource(loadResult, jsonData);
    if (auto jsonExtensions = jsonData.find("extensions"); jsonExtensions) {
        if (auto jsonDds = jsonExtensions->find("MSFT_texture_dds"); jsonDds) {
            image = TextureSource(loadResult, *jsonDds);
        }
#if defined(GLTF2_EXTENSION_KHR_TEXTURE_BASISU)
        if (auto jsonBasisu = jsonExtensions->find("KHR_texture_basisu"); jsonBasisu) {
            image = TextureSource(loadResult, *jsonBasisu);
        }
#endif
    }
    const auto result = (sampler && image);
    auto texture = make_unique<Texture>();
    if (result) {
        texture->sampler = *sampler;
        texture->image = *image;
    }
    loadResult.data->textures.push_back(move(texture));

    return result;
}

bool ParseImage(LoadResult& loadResult, const json::value& jsonData)
{
    auto image = make_unique<Image>();
    if (!ParseOptionalString(loadResult, image->uri, jsonData, "uri", string())) {
        return false;
    }

    DecodeUri(image->uri);

    size_t bufferIndex;
    if (!ParseOptionalNumber<size_t>(loadResult, bufferIndex, jsonData, "bufferView", GLTF_INVALID_INDEX)) {
        return false;
    }

    if (bufferIndex != GLTF_INVALID_INDEX && bufferIndex < loadResult.data->bufferViews.size()) {
        image->bufferView = loadResult.data->bufferViews[bufferIndex].get();

        string imageType;
        if (!ParseOptionalString(loadResult, imageType, jsonData, "mimeType", "")) {
            return false;
        }

        if (!GetMimeType(imageType, image->type)) {
            RETURN_WITH_ERROR(loadResult, "Invalid mime type.");
        }
    }

    loadResult.data->images.push_back(move(image));

    return true;
}

bool ParseFilterMode(LoadResult& loadResult, FilterMode& out, const json::value& jsonData, const string& filterName)
{
    int value = 0;
    if (!ParseOptionalNumber<int>(loadResult, value, jsonData, filterName, static_cast<int>(FilterMode::LINEAR))) {
        return false;
    }

    out = (FilterMode)value;

    switch (out) {
        case FilterMode::NEAREST:
        case FilterMode::LINEAR:
        case FilterMode::NEAREST_MIPMAP_NEAREST:
        case FilterMode::LINEAR_MIPMAP_NEAREST:
        case FilterMode::NEAREST_MIPMAP_LINEAR:
        case FilterMode::LINEAR_MIPMAP_LINEAR:
            return true;
        default:
            break;
    }

    RETURN_WITH_ERROR(loadResult, "Invalid filter mode");
}

bool ParseWrapMode(LoadResult& loadResult, WrapMode& out, const json::value& jsonData, const string& wrapModeName)
{
    int value = 0;
    if (!ParseOptionalNumber(loadResult, value, jsonData, wrapModeName, static_cast<int>(WrapMode::REPEAT))) {
        return false;
    }

    out = (WrapMode)value;

    switch (out) {
        case WrapMode::CLAMP_TO_EDGE:
        case WrapMode::MIRRORED_REPEAT:
        case WrapMode::REPEAT:
            return true;
        default:
            break;
    }

    RETURN_WITH_ERROR(loadResult, "Invalid wrap mode");
}

bool ParseSampler(LoadResult& loadResult, const json::value& jsonData)
{
    bool result = true;

    FilterMode magFilter;
    if (!ParseFilterMode(loadResult, magFilter, jsonData, "magFilter")) {
        result = false;
    }

    FilterMode minFilter;
    if (!ParseFilterMode(loadResult, minFilter, jsonData, "minFilter")) {
        result = false;
    }

    WrapMode wrapS;
    if (!ParseWrapMode(loadResult, wrapS, jsonData, "wrapS")) {
        result = false;
    }

    WrapMode wrapT;
    if (!ParseWrapMode(loadResult, wrapT, jsonData, "wrapT")) {
        result = false;
    }

    auto sampler = make_unique<Sampler>();
    if (result) {
        sampler->magFilter = magFilter;
        sampler->minFilter = minFilter;
        sampler->wrapS = wrapS;
        sampler->wrapT = wrapT;
    }

    loadResult.data->samplers.push_back(move(sampler));

    return result;
}

bool ParseNormalTexture(LoadResult& loadResult, const json::value& jsonData, NormalTexture& normalTexture)
{
    if (auto normalJson = jsonData.find("normalTexture"); normalJson) {
        if (!ParseTextureInfo(loadResult, normalTexture.textureInfo, *normalJson)) {
            return false;
        }

        if (!ParseOptionalNumber<float>(loadResult, normalTexture.scale, *normalJson, "scale", normalTexture.scale)) {
            return false;
        }
    }

    return true;
}

bool ParseOcclusionTexture(LoadResult& loadResult, const json::value& jsonData, OcclusionTexture& occlusionTexture)
{
    if (auto occlusionJson = jsonData.find("occlusionTexture"); occlusionJson) {
        if (!ParseTextureInfo(loadResult, occlusionTexture.textureInfo, *occlusionJson)) {
            return false;
        }

        if (!ParseOptionalNumber<float>(
                loadResult, occlusionTexture.strength, *occlusionJson, "strength", occlusionTexture.strength)) {
            return false;
        }
    }

    return true;
}

bool ParseEmissiveTexture(LoadResult& loadResult, const json::value& jsonData, TextureInfo& emissiveTexture)
{
    if (auto emissiveJson = jsonData.find("emissiveTexture"); emissiveJson) {
        if (!ParseTextureInfo(loadResult, emissiveTexture, *emissiveJson)) {
            return false;
        }
    }

    return true;
}

bool ParseMaterialExtras(LoadResult& loadResult, const json::value& jsonData, Material& material)
{
    if (auto extrasJson = jsonData.find("extras"); extrasJson) {
#if defined(GLTF2_EXTRAS_CLEAR_COAT_MATERIAL)
        const auto parseClearCoat = [&material](LoadResult& loadResult, const json::value& materialJson) -> bool {
            if (!ParseOptionalNumber<float>(
                    loadResult, material.clearcoat.factor, materialJson, "factor", material.clearcoat.factor)) {
                return false;
            }

            if (!ParseOptionalNumber<float>(loadResult, material.clearcoat.roughness, materialJson, "roughness",
                    material.clearcoat.roughness)) {
                return false;
            }

            return true;
        };
        if (!ParseObject(loadResult, *extrasJson, "clearCoat", parseClearCoat)) {
            return false;
        }
#endif
    }
    return true;
}

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_CLEARCOAT)
bool ParseKhrMaterialsClearcoat(LoadResult& loadResult, const json::value& jsonData, Material::Clearcoat& clearcoat)
{
    if (auto clearcoatJson = jsonData.find("KHR_materials_clearcoat"); clearcoatJson) {
        // clearcoatFactor
        if (!ParseOptionalNumber(loadResult, clearcoat.factor, *clearcoatJson, "clearcoatFactor", 0.f)) {
            return false;
        }

        // clearcoatTexture
        const auto parseClearcoatTexture = [&textureInfo = clearcoat.texture](
                                               LoadResult& loadResult, const json::value& clearcoat) -> bool {
            return ParseTextureInfo(loadResult, textureInfo, clearcoat);
        };
        if (!ParseObject(loadResult, *clearcoatJson, "clearcoatTexture", parseClearcoatTexture)) {
            return false;
        }

        // clearcoaRoughnessFactor
        if (!ParseOptionalNumber(loadResult, clearcoat.roughness, *clearcoatJson, "clearcoatRoughnessFactor", 0.f)) {
            return false;
        }

        // clearcoatRougnessTexture
        const auto parseClearcoatRoughnessTexture = [&textureInfo = clearcoat.roughnessTexture](
                                                        LoadResult& loadResult, const json::value& clearcoat) -> bool {
            return ParseTextureInfo(loadResult, textureInfo, clearcoat);
        };
        if (!ParseObject(loadResult, *clearcoatJson, "clearcoatRoughnessTexture", parseClearcoatRoughnessTexture)) {
            return false;
        }

        // clearcoatNormalTexture
        const auto parseClearcoatNormalTexture = [&normalTexture = clearcoat.normalTexture](
                                                     LoadResult& loadResult, const json::value& clearcoat) -> bool {
            if (!ParseTextureInfo(loadResult, normalTexture.textureInfo, clearcoat)) {
                return false;
            }

            if (!ParseOptionalNumber(loadResult, normalTexture.scale, clearcoat, "scale", normalTexture.scale)) {
                return false;
            }
            return true;
        };
        if (!ParseObject(loadResult, *clearcoatJson, "clearcoatNormalTexture", parseClearcoatNormalTexture)) {
            return false;
        }
        return true;
    }
    return true;
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_EMISSIVE_STRENGTH)
bool ParseKhrMaterialsEmissiveStrength(LoadResult& loadResult, const json::value& jsonData, Material& material)
{
    if (auto emissiveStrengthJson = jsonData.find("KHR_materials_emissive_strength"); emissiveStrengthJson) {
        return ParseOptionalNumber(
            loadResult, material.emissiveFactor.w, *emissiveStrengthJson, "emissiveStrength", 1.f);
    }
    return true;
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_IOR)
bool ParseKhrMaterialsIor(LoadResult& loadResult, const json::value& jsonData, Material::Ior& ior)
{
    if (auto iorJson = jsonData.find("KHR_materials_ior"); iorJson) {
        return ParseOptionalNumber(loadResult, ior.ior, *iorJson, "ior", ior.ior);
    }
    return true;
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_PBRSPECULARGLOSSINESS)
bool ParseKhrMaterialsPbrSpecularGlossiness(LoadResult& loadResult, const json::value& jsonData, Material& material)
{
    if (auto specGlossJson = jsonData.find("KHR_materials_pbrSpecularGlossiness"); specGlossJson) {
        material.type = Material::Type::SpecularGlossiness;

        if (!ParseOptionalMath(loadResult, material.specularGlossiness.diffuseFactor, *specGlossJson, "diffuseFactor",
                material.specularGlossiness.diffuseFactor)) {
            return false;
        }

        const auto parseDiffuseTexture = [&material](LoadResult& loadResult, const json::value& jsonData) -> bool {
            return ParseTextureInfo(loadResult, material.specularGlossiness.diffuseTexture, jsonData);
        };
        if (!ParseObject(loadResult, *specGlossJson, "diffuseTexture", parseDiffuseTexture)) {
            return false;
        }

        if (!ParseOptionalMath(loadResult, material.specularGlossiness.specularFactor, *specGlossJson, "specularFactor",
                material.specularGlossiness.specularFactor)) {
            return false;
        }

        if (!ParseOptionalNumber<float>(
                loadResult, material.specularGlossiness.glossinessFactor, *specGlossJson, "glossinessFactor", 1.0f)) {
            return false;
        }

        const auto parseSpecularGlossinessTexture = [&material](
                                                        LoadResult& loadResult, const json::value& jsonData) -> bool {
            return ParseTextureInfo(loadResult, material.specularGlossiness.specularGlossinessTexture, jsonData);
        };
        if (!ParseObject(loadResult, *specGlossJson, "specularGlossinessTexture", parseSpecularGlossinessTexture)) {
            return false;
        }
    }
    return true;
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SHEEN)
bool ParseKhrMaterialsSheen(LoadResult& loadResult, const json::value& jsonData, Material::Sheen& sheen)
{
    if (auto sheenJson = jsonData.find("KHR_materials_sheen"); sheenJson) {
        // sheenColorFactor
        if (!ParseOptionalMath(loadResult, sheen.factor, *sheenJson, "sheenColorFactor", {})) {
            return false;
        }

        // sheenColorTexture
        const auto parseSheenTexture = [&textureInfo = sheen.texture](
                                           LoadResult& loadResult, const json::value& sheen) -> bool {
            return ParseTextureInfo(loadResult, textureInfo, sheen);
        };
        if (!ParseObject(loadResult, *sheenJson, "sheenColorTexture", parseSheenTexture)) {
            return false;
        }

        // sheenRoughnessFactor
        if (!ParseOptionalNumber(loadResult, sheen.roughness, *sheenJson, "sheenRoughnessFactor", 0.f)) {
            return false;
        }

        // sheenRougnessTexture
        const auto parseSheenRoughnessTexture = [&textureInfo = sheen.roughnessTexture](
                                                    LoadResult& loadResult, const json::value& sheen) -> bool {
            return ParseTextureInfo(loadResult, textureInfo, sheen);
        };
        if (!ParseObject(loadResult, *sheenJson, "sheenRoughnessTexture", parseSheenRoughnessTexture)) {
            return false;
        }
    }
    return true;
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SPECULAR)
bool ParseKhrMaterialsSpecular(LoadResult& loadResult, const json::value& jsonData, Material::Specular& specular)
{
    if (auto specularJson = jsonData.find("KHR_materials_specular"); specularJson) {
        // specularFactor
        if (!ParseOptionalNumber(loadResult, specular.factor, *specularJson, "specularFactor", 1.f)) {
            return false;
        }

        // specularTexture
        const auto parseSpecularTexture = [&textureInfo = specular.texture](
                                              LoadResult& loadResult, const json::value& specular) -> bool {
            return ParseTextureInfo(loadResult, textureInfo, specular);
        };
        if (!ParseObject(loadResult, *specularJson, "specularTexture", parseSpecularTexture)) {
            return false;
        }

        // specularColorFactor
        if (!ParseOptionalMath(loadResult, specular.color, *specularJson, "specularColorFactor", specular.color)) {
            return false;
        }

        // specularColorTexture
        const auto parseSpecularColorTexture = [&textureInfo = specular.colorTexture](
                                                   LoadResult& loadResult, const json::value& specular) -> bool {
            return ParseTextureInfo(loadResult, textureInfo, specular);
        };
        if (!ParseObject(loadResult, *specularJson, "specularColorTexture", parseSpecularColorTexture)) {
            return false;
        }
    }
    return true;
}
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_TRANSMISSION)
bool ParseKhrMaterialsTransmission(
    LoadResult& loadResult, const json::value& jsonData, Material::Transmission& transmission)
{
    if (auto transmissionJson = jsonData.find("KHR_materials_transmission"); transmissionJson) {
        // transmissionFactor
        if (!ParseOptionalNumber(loadResult, transmission.factor, *transmissionJson, "transmissionFactor", 0.f)) {
            return false;
        }

        // transmissionTexture
        const auto parseTransmissionTexture = [&textureInfo = transmission.texture](
                                                  LoadResult& loadResult, const json::value& transmission) -> bool {
            return ParseTextureInfo(loadResult, textureInfo, transmission);
        };
        if (!ParseObject(loadResult, *transmissionJson, "transmissionTexture", parseTransmissionTexture)) {
            return false;
        }
    }
    return true;
}
#endif

bool ParseMaterialExtensions(LoadResult& loadResult, const json::value& jsonData, Material& material)
{
    if (auto extensionsJson = jsonData.find("extensions"); extensionsJson) {
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_CLEARCOAT)
        if (!ParseKhrMaterialsClearcoat(loadResult, *extensionsJson, material.clearcoat)) {
            return false;
        }
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_EMISSIVE_STRENGTH)
        if (!ParseKhrMaterialsEmissiveStrength(loadResult, *extensionsJson, material)) {
            return false;
        }
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_IOR)
        if (!ParseKhrMaterialsIor(loadResult, *extensionsJson, material.ior)) {
            return false;
        }
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_PBRSPECULARGLOSSINESS)
        if (!ParseKhrMaterialsPbrSpecularGlossiness(loadResult, *extensionsJson, material)) {
            return false;
        }
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SHEEN)
        if (!ParseKhrMaterialsSheen(loadResult, *extensionsJson, material.sheen)) {
            return false;
        }
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SPECULAR)
        if (!ParseKhrMaterialsSpecular(loadResult, *extensionsJson, material.specular)) {
            return false;
        }
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_TRANSMISSION)
        if (!ParseKhrMaterialsTransmission(loadResult, *extensionsJson, material.transmission)) {
            return false;
        }
#endif
#if defined(GLTF2_EXTENSION_KHR_MATERIALS_UNLIT)
        const auto parseUnlitExtension = [&material](LoadResult& loadResult, const json::value& materialJson) -> bool {
            material.type = Material::Type::Unlit;
            return true;
        };

        if (!ParseObject(loadResult, *extensionsJson, "KHR_materials_unlit", parseUnlitExtension)) {
            // Parsing of materials_unlit failed.
            return false;
        }
#endif
        return true;
    }
    return true;
}

bool ParseMaterial(LoadResult& loadResult, const json::value& jsonData)
{
    bool result = true;

    auto material = make_unique<Material>();
    if (!ParseMetallicRoughness(loadResult, jsonData, material->metallicRoughness)) {
        result = false;
    }

    if (!ParseNormalTexture(loadResult, jsonData, material->normalTexture)) {
        result = false;
    }

    if (!ParseOcclusionTexture(loadResult, jsonData, material->occlusionTexture)) {
        result = false;
    }

    if (!ParseEmissiveTexture(loadResult, jsonData, material->emissiveTexture)) {
        result = false;
    }

    if (!ParseOptionalString(
            loadResult, material->name, jsonData, "name", "material_" + to_string(loadResult.data->materials.size()))) {
        result = false;
    }

    if (Math::Vec3 emissive; !ParseOptionalMath(loadResult, emissive, jsonData, "emissiveFactor", emissive)) {
        result = false;
    } else {
        material->emissiveFactor.x = emissive.x;
        material->emissiveFactor.y = emissive.y;
        material->emissiveFactor.z = emissive.z;
    }

    string alphaMode;
    if (!ParseOptionalString(loadResult, alphaMode, jsonData, "alphaMode", "OPAQUE")) {
        result = false;
    }

    if (!GetAlphaMode(alphaMode, material->alphaMode)) {
        SetError(loadResult, "Invalid alpha mode.");
        result = false;
    }

    if (!ParseOptionalNumber<float>(
            loadResult, material->alphaCutoff, jsonData, "alphaCutoff", material->alphaCutoff)) {
        result = false;
    }

    if (!ParseOptionalBoolean(loadResult, material->doubleSided, jsonData, "doubleSided", false)) {
        result = false;
    }

    if (!ParseMaterialExtras(loadResult, jsonData, *material)) {
        result = false;
    }

    if (!ParseMaterialExtensions(loadResult, jsonData, *material)) {
        result = false;
    }

    loadResult.data->materials.push_back(move(material));

    return result;
}

bool PrimitiveAttributes(LoadResult& loadResult, const json::value& jsonData, MeshPrimitive& meshPrimitive)
{
    if (const auto* attirbutesJson = jsonData.find("attributes"); attirbutesJson && attirbutesJson->is_object()) {
        for (const auto& it : attirbutesJson->object_) {
            if (it.value.is_number()) {
                Attribute attribute;

                if (!GetAttributeType(it.key, attribute.attribute)) {
                    RETURN_WITH_ERROR(loadResult, "Invalid attribute type.");
                }

                const uint32_t accessor = it.value.as_number<uint32_t>();
                if (accessor < loadResult.data->accessors.size()) {
                    attribute.accessor = loadResult.data->accessors[accessor].get();

                    auto const validationResult = ValidatePrimitiveAttribute(
                        attribute.attribute.type, attribute.accessor->type, attribute.accessor->componentType);
                    if (!validationResult.empty()) {
#if defined(GLTF2_EXTENSION_KHR_MESH_QUANTIZATION)
                        if (loadResult.data->quantization) {
                            auto const extendedValidationResult = ValidatePrimitiveAttributeQuatization(
                                attribute.attribute.type, attribute.accessor->type, attribute.accessor->componentType);
                            if (!extendedValidationResult.empty()) {
                                RETURN_WITH_ERROR(loadResult, extendedValidationResult);
                            }
                        } else {
#else
                        {
#endif
                            RETURN_WITH_ERROR(loadResult, validationResult);
                        }
                    }

                    meshPrimitive.attributes.push_back(attribute);
                }
            }
        }
        if (std::none_of(meshPrimitive.attributes.begin(), meshPrimitive.attributes.end(),
                [](const Attribute& attr) { return attr.attribute.type == AttributeType::POSITION; })) {
            RETURN_WITH_ERROR(loadResult, "Primitive must have POSITION attribute.");
        }
    } else {
        RETURN_WITH_ERROR(loadResult, "Missing primitive.attributes.");
    }
    return true;
}

bool PrimitiveTargets(
    LoadResult& loadResult, const json::value& jsonData, MeshPrimitive& meshPrimitive, bool compressed)
{
    return ForEachInArray(loadResult, jsonData, "targets",
        [&meshPrimitive, compressed](LoadResult& loadResult, const json::value& target) -> bool {
            MorphTarget mTarget;
#ifdef GLTF2_EXTENSION_IGFX_COMPRESSED
            mTarget.iGfxCompressed = compressed;
#endif
            for (const auto& it : target.object_) {
                if (it.value.is_number()) {
                    Attribute attribute;

                    if (!GetAttributeType(it.key, attribute.attribute)) {
                        RETURN_WITH_ERROR(loadResult, "Invalid attribute type.");
                    }

                    const uint32_t accessor = it.value.as_number<uint32_t>();

                    if (accessor < loadResult.data->accessors.size()) {
                        attribute.accessor = loadResult.data->accessors[accessor].get();

                        auto const validationResult = ValidateMorphTargetAttribute(
                            attribute.attribute.type, attribute.accessor->type, attribute.accessor->componentType);
                        if (!validationResult.empty()) {
#if defined(GLTF2_EXTENSION_KHR_MESH_QUANTIZATION)
                            if (loadResult.data->quantization) {
                                auto const extendedValidationResult =
                                    ValidateMorphTargetAttributeQuantization(attribute.attribute.type,
                                        attribute.accessor->type, attribute.accessor->componentType);
                                if (!extendedValidationResult.empty()) {
                                    RETURN_WITH_ERROR(loadResult, extendedValidationResult);
                                }
                            } else {
#else
                                {
#endif
                                RETURN_WITH_ERROR(loadResult, validationResult);
                            }
                        }

                        mTarget.target.push_back(attribute);
                    }
                }
            }

            meshPrimitive.targets.push_back(move(mTarget));

            return true;
        });
}

bool ParsePrimitive(LoadResult& loadResult, vector<MeshPrimitive>& primitives, const json::value& jsonData)
{
    MeshPrimitive meshPrimitive;

    if (!PrimitiveAttributes(loadResult, jsonData, meshPrimitive)) {
        return false;
    }

    size_t indices;
    if (!ParseOptionalNumber<size_t>(loadResult, indices, jsonData, "indices", GLTF_INVALID_INDEX)) {
        return false;
    }
    if (indices != GLTF_INVALID_INDEX && indices < loadResult.data->accessors.size()) {
        meshPrimitive.indices = loadResult.data->accessors[indices].get();
    }

    if (!ParseOptionalNumber<uint32_t>(
            loadResult, meshPrimitive.materialIndex, jsonData, "material", GLTF_INVALID_INDEX)) {
        return false;
    }
    if (meshPrimitive.materialIndex != GLTF_INVALID_INDEX &&
        meshPrimitive.materialIndex < loadResult.data->materials.size()) {
        meshPrimitive.material = loadResult.data->materials[meshPrimitive.materialIndex].get();
    } else {
        meshPrimitive.material = loadResult.data->defaultMaterial.get();
    }

    int mode;
    if (!ParseOptionalNumber<int>(loadResult, mode, jsonData, "mode", static_cast<int>(RenderMode::TRIANGLES))) {
        return false;
    }

    if (!RangedEnumCast<RenderMode>(loadResult, meshPrimitive.mode, mode)) {
        return false;
    }

    bool compressed = false;

    if (const auto& extensionsJson = jsonData.find("extensions"); extensionsJson) {
#ifdef GLTF2_EXTENSION_IGFX_COMPRESSED
        if (const auto& compressedJson = extensionsJson->find("IGFX_compressed"); compressedJson) {
            compressed = true;
            if (!PrimitiveTargets(loadResult, *compressedJson, meshPrimitive, compressed)) {
                return false;
            }
        }
#endif
    }

    if (!compressed) {
        if (!PrimitiveTargets(loadResult, jsonData, meshPrimitive, compressed)) {
            return false;
        }
    }

    primitives.push_back(move(meshPrimitive));

    return true;
}

bool MeshExtras(LoadResult& loadResult, const json::value& jsonData, array_view<MeshPrimitive> primitives)
{
    size_t index = 0;
    return ForEachInArray(loadResult, jsonData, "targetNames",
        [&primitives, &index](LoadResult& loadResult, const json::value& targetName) -> bool {
            if (!targetName.is_string()) {
                RETURN_WITH_ERROR(loadResult, "mesh.extras.targetNames should be an array of strings");
            }

            auto name = targetName.string_;

            for (auto& primitive : primitives) {
                if (index < primitive.targets.size()) {
                    primitive.targets[index].name = name;
                }
            }

            index++;

            return true;
        });
}

bool ParseMesh(LoadResult& loadResult, const json::value& jsonData)
{
    bool result = true;

    string name;
    if (!ParseOptionalString(loadResult, name, jsonData, "name", "")) {
        return false;
    }

    vector<MeshPrimitive> primitives;
    if (auto const primitivesJson = jsonData.find("primitives"); primitivesJson) {
        if (!ForEachInArray(
                loadResult, *primitivesJson, [&primitives](LoadResult& loadResult, const json::value& item) -> bool {
                    return ParsePrimitive(loadResult, primitives, item);
                })) {
            return false;
        }
    }

    vector<float> weights;
    const auto parseWeights = [&weights](LoadResult& loadResult, const json::value& weight) -> bool {
        if (weight.is_number()) {
            weights.push_back(weight.as_number<float>());
        }
        return true;
    };

    if (!ForEachInArray(loadResult, jsonData, "weights", parseWeights)) {
        return false;
    }

    // validate morph target counts
    for (size_t i = 1; i < primitives.size(); i++) {
        if (primitives[i].targets.size() != primitives[0].targets.size()) {
            SetError(loadResult,
                "Morph target count mismatch: each primitive of a mesh should have same amount of morph targets");
            result = false;
        }
    }

    const auto parseExtras = [&primitives](LoadResult& loadResult, const json::value& extras) -> bool {
        return MeshExtras(loadResult, extras, primitives);
    };

    if (!ParseObject(loadResult, jsonData, "extras", parseExtras)) {
        return false;
    }

    auto mesh = make_unique<Mesh>();
    if (result) {
        mesh->name = move(name);
        mesh->weights = move(weights);
        mesh->primitives = move(primitives);
    }

    loadResult.data->meshes.push_back(move(mesh));

    return true;
}

bool CameraPerspective(
    LoadResult& loadResult, const json::value& jsonData, Camera::Attributes::Perspective& perspective)
{
    if (!ParseOptionalNumber<float>(loadResult, perspective.aspect, jsonData, "aspectRatio", -1.f)) {
        return false;
    }
    if (!ParseOptionalNumber<float>(loadResult, perspective.yfov, jsonData, "yfov", -1.f)) { // required
        return false;
    }
    if (!ParseOptionalNumber<float>(loadResult, perspective.zfar, jsonData, "zfar", -1.f)) {
        return false;
    }
    if (!ParseOptionalNumber<float>(loadResult, perspective.znear, jsonData, "znear", -1.f)) { // required
        return false;
    }
    if (perspective.yfov < 0 || perspective.znear < 0) {
        RETURN_WITH_ERROR(loadResult, "Invalid camera properties (perspective)");
    }

    return true;
}

bool CameraOrthographic(LoadResult& loadResult, const json::value& jsonData, Camera::Attributes::Ortho& ortho)
{
    if (!ParseOptionalNumber<float>(loadResult, ortho.xmag, jsonData, "xmag", 0)) { // required
        return false;
    }
    if (!ParseOptionalNumber<float>(loadResult, ortho.ymag, jsonData, "ymag", 0)) { // required
        return false;
    }
    if (!ParseOptionalNumber<float>(loadResult, ortho.zfar, jsonData, "zfar", -1.f)) { // required
        return false;
    }
    if (!ParseOptionalNumber<float>(loadResult, ortho.znear, jsonData, "znear", -1.f)) { // required
        return false;
    }
    if (ortho.zfar < 0 || ortho.znear < 0 || ortho.xmag == 0 || ortho.ymag == 0) {
        RETURN_WITH_ERROR(loadResult, "Invalid camera properties (ortho)");
    }
    return true;
}

bool ParseCamera(LoadResult& loadResult, const json::value& jsonData)
{
    bool result = true;

    auto camera = make_unique<Camera>();
    if (!ParseOptionalString(loadResult, camera->name, jsonData, "name", "")) {
        result = false;
    }
    string cameraType;
    if (!ParseOptionalString(loadResult, cameraType, jsonData, "type", "")) {
        result = false;
    }
    if (!GetCameraType(cameraType, camera->type)) {
        SetError(loadResult, "Invalid camera type");
        result = false;
    }

    switch (camera->type) {
        case CameraType::PERSPECTIVE: {
            const auto parser = [&camera](LoadResult& loadResult, const json::value& perspective) -> bool {
                return CameraPerspective(loadResult, perspective, camera->attributes.perspective);
            };

            if (!ParseObject(loadResult, jsonData, "perspective", parser)) {
                result = false;
            }
            break;
        }

        case CameraType::ORTHOGRAPHIC: {
            const auto parser = [&camera](LoadResult& loadResult, const json::value& orthographic) -> bool {
                return CameraOrthographic(loadResult, orthographic, camera->attributes.ortho);
            };

            if (!ParseObject(loadResult, jsonData, "orthographic", parser)) {
                result = false;
            }
            break;
        }

        default:
        case CameraType::INVALID: {
            SetError(loadResult, "Invalid camera type");
            result = false;
        }
    }

    loadResult.data->cameras.push_back(move(camera));
    return result;
}

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
bool LightSpot(LoadResult& loadResult, const json::value& jsonData, decltype(KHRLight::positional.spot)& spot)
{
    return ParseObject(
        loadResult, jsonData, "spot", [&spot](LoadResult& loadResult, const json::value& jsonData) -> bool {
            if (!ParseOptionalNumber<float>(loadResult, spot.innerAngle, jsonData, "innerConeAngle", 0.f)) {
                return false;
            }

            if (!ParseOptionalNumber<float>(
                    loadResult, spot.outerAngle, jsonData, "outerConeAngle", 0.785398163397448f)) {
                return false;
            }

            return true;
        });
}

bool LightShadow(LoadResult& loadResult, const json::value& jsonData, decltype(KHRLight::shadow)& shadow)
{
    return ParseObject(
        loadResult, jsonData, "shadow", [&shadow](LoadResult& loadResult, const json::value& jsonData) -> bool {
            if (!ParseOptionalBoolean(loadResult, shadow.shadowCaster, jsonData, "caster", false)) {
                return false;
            }

            if (!ParseOptionalNumber<float>(
                    loadResult, shadow.nearClipDistance, jsonData, "znear", shadow.nearClipDistance)) {
                return false;
            }

            if (!ParseOptionalNumber<float>(
                    loadResult, shadow.farClipDistance, jsonData, "zfar", shadow.farClipDistance)) {
                return false;
            }
            return true;
        });
}

bool ParseKHRLight(LoadResult& loadResult, const json::value& jsonData)
{
    bool result = true;

    auto light = make_unique<KHRLight>();

    if (!ParseOptionalString(loadResult, light->name, jsonData, "name", "")) {
        result = false;
    }

    string lightType;
    if (!ParseOptionalString(loadResult, lightType, jsonData, "type", "")) {
        result = false;
    }

    if (!GetLightType(lightType, light->type)) {
        SetError(loadResult, "Invalid light type.");
        result = false;
    }

    const auto parsePositionalInfo = [&light](LoadResult& loadResult, const json::value& positional) -> bool {
        return ParseOptionalNumber<float>(loadResult, light->positional.range, positional, "range", 0.0f);
    };

    if (!ParseObject(loadResult, jsonData, "positional", parsePositionalInfo)) {
        result = false;
    }

    if (!ParseOptionalNumber<float>(loadResult, light->positional.range, jsonData, "range", 0.0f)) {
        return false;
    }

    if (!LightSpot(loadResult, jsonData, light->positional.spot)) {
        result = false;
    }

    if (!ParseOptionalMath(loadResult, light->color, jsonData, "color", light->color)) {
        result = false;
    }

    // blender uses strength
    if (!ParseOptionalNumber<float>(loadResult, light->intensity, jsonData, "strength", 1.0f)) {
        result = false;
    }

    // khronos uses intensity
    if (!ParseOptionalNumber<float>(loadResult, light->intensity, jsonData, "intensity", light->intensity)) {
        result = false;
    }
    if (!LightShadow(loadResult, jsonData, light->shadow)) {
        result = false;
    }

    loadResult.data->lights.push_back(move(light));

    return result;
}
#endif

#if defined(GLTF2_EXTENSION_EXT_LIGHTS_IMAGE_BASED)
bool ImageBasedLightIrradianceCoefficients(
    LoadResult& loadResult, const json::value& jsonData, vector<ImageBasedLight::LightingCoeff>& irradianceCoefficients)
{
    const auto parseIrradianceCoefficients = [&irradianceCoefficients](
                                                 LoadResult& loadResult, const json::value& mipLevelJson) -> bool {
        ImageBasedLight::LightingCoeff coeff;
        if (mipLevelJson.is_array()) {
            coeff.reserve(mipLevelJson.array_.size());
            std::transform(mipLevelJson.array_.begin(), mipLevelJson.array_.end(), std::back_inserter(coeff),
                [](const json::value& item) { return item.is_number() ? item.as_number<float>() : 0.f; });
        }

        if (coeff.size() != 3) {
            return false;
        }

        irradianceCoefficients.push_back(move(coeff));
        return true;
    };

    return ForEachInArray(loadResult, jsonData, "irradianceCoefficients", parseIrradianceCoefficients);
}

bool ImageBasedLightSpecularImages(
    LoadResult& loadResult, const json::value& jsonData, vector<ImageBasedLight::CubemapMipLevel>& specularImages)
{
    const auto parseCubeMipLevel = [&specularImages](LoadResult& loadResult, const json::value& mipLevelJson) -> bool {
        ImageBasedLight::CubemapMipLevel mipLevel;
        if (mipLevelJson.is_array()) {
            mipLevel.reserve(mipLevelJson.array_.size());
            std::transform(mipLevelJson.array_.begin(), mipLevelJson.array_.end(), std::back_inserter(mipLevel),
                [](const json::value& item) {
                    return item.is_number() ? item.as_number<size_t>() : GLTF_INVALID_INDEX;
                });
        }

        if (mipLevel.size() != 6) {
            return false;
        }

        specularImages.push_back(move(mipLevel));
        return true;
    };

    return ForEachInArray(loadResult, jsonData, "specularImages", parseCubeMipLevel);
}

bool ParseImageBasedLight(LoadResult& loadResult, const json::value& jsonData)
{
    bool result = true;

    auto light = make_unique<ImageBasedLight>();

    if (!ParseOptionalString(loadResult, light->name, jsonData, "name", "")) {
        result = false;
    }

    if (!ParseOptionalMath(loadResult, light->rotation, jsonData, "rotation", light->rotation)) {
        result = false;
    }

    if (!ParseOptionalNumber<float>(loadResult, light->intensity, jsonData, "intensity", light->intensity)) {
        result = false;
    }

    if (!ImageBasedLightIrradianceCoefficients(loadResult, jsonData, light->irradianceCoefficients)) {
        result = false;
    }

    if (!ImageBasedLightSpecularImages(loadResult, jsonData, light->specularImages)) {
        result = false;
    }

    if (!ParseOptionalNumber<uint32_t>(
            loadResult, light->specularImageSize, jsonData, "specularImageSize", light->specularImageSize)) {
        result = false;
    }

    const auto parseExtras = [&light](LoadResult& loadResult, const json::value& e) -> bool {
        if (!ParseOptionalNumber(loadResult, light->skymapImage, e, "skymapImage", light->skymapImage)) {
            return false;
        }

        if (!ParseOptionalNumber(
                loadResult, light->skymapImageLodLevel, e, "skymapImageLodLevel", light->skymapImageLodLevel)) {
            return false;
        }

        if (!ParseOptionalNumber(
                loadResult, light->specularCubeImage, e, "specularCubeImage", light->specularCubeImage)) {
            return false;
        }

        return true;
    };

    if (!ParseObject(loadResult, jsonData, "extras", parseExtras)) {
        result = false;
    }

    loadResult.data->imageBasedLights.push_back(move(light));

    return result;
}
#endif

bool NodeName(LoadResult& loadResult, const json::value& jsonData, string& name)
{
    if (!ParseOptionalString(loadResult, name, jsonData, "name", "")) {
        return false;
    }

    if (name.empty()) {
        name = "node_" + to_string(loadResult.data->nodes.size());
    }
    return true;
}

bool NodeMesh(LoadResult& loadResult, const json::value& jsonData, Mesh*& mesh)
{
    size_t meshIndex;
    if (!ParseOptionalNumber<size_t>(loadResult, meshIndex, jsonData, "mesh", GLTF_INVALID_INDEX)) {
        return false;
    }

    if (meshIndex != GLTF_INVALID_INDEX) {
        if (meshIndex < loadResult.data->meshes.size()) {
            mesh = loadResult.data->meshes[meshIndex].get();
        } else {
            SetError(loadResult, "Node refers to invalid mesh index");
            return false;
        }
    }
    return true;
}

bool NodeCamera(LoadResult& loadResult, const json::value& jsonData, Camera*& camera)
{
    size_t cameraIndex;
    if (!ParseOptionalNumber<size_t>(loadResult, cameraIndex, jsonData, "camera", GLTF_INVALID_INDEX)) {
        return false;
    }

    if (cameraIndex != GLTF_INVALID_INDEX) {
        if (size_t(cameraIndex) < loadResult.data->cameras.size()) {
            camera = loadResult.data->cameras[cameraIndex].get();
        } else {
            SetError(loadResult, "Node refers to invalid camera index");
            return false;
        }
    }
    return true;
}

struct ExtensionData {
    size_t lightIndex;
    bool compressed;
};

std::optional<ExtensionData> NodeExtensions(LoadResult& loadResult, const json::value& jsonData, Node& node)
{
    ExtensionData data { GLTF_INVALID_INDEX, false };

    const auto parseExtensions = [&data, &node](LoadResult& loadResult, const json::value& extensions) -> bool {
#if defined(GLTF2_EXTENSION_KHR_LIGHTS)
        const auto parseLight = [&data](LoadResult& loadResult, const json::value& light) -> bool {
            if (!ParseOptionalNumber<size_t>(loadResult, data.lightIndex, light, "light", GLTF_INVALID_INDEX)) {
                return false;
            }

            return true;
        };

        if (!ParseObject(loadResult, extensions, "KHR_lights_punctual", parseLight)) {
            return false;
        }
#endif

#if defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
        if (data.lightIndex == GLTF_INVALID_INDEX) {
            const auto parseLightPbr = [&data](LoadResult& loadResult, const json::value& light) -> bool {
                if (!ParseOptionalNumber<size_t>(loadResult, data.lightIndex, light, "light", GLTF_INVALID_INDEX)) {
                    return false;
                }

                if (data.lightIndex != GLTF_INVALID_INDEX) {
                    data.lightIndex += loadResult.data->pbrLightOffset;
                }

                return true;
            };

            if (!ParseObject(loadResult, extensions, "KHR_lights_pbr", parseLightPbr)) {
                return false;
            }
        }
#endif

#ifdef GLTF2_EXTENSION_IGFX_COMPRESSED
        const auto parseCompressed = [&data, &weights = node.weights](
                                         LoadResult& loadResult, const json::value& compressedJson) {
            data.compressed = true;
            return ParseOptionalNumberArray(loadResult, weights, compressedJson, "weights", vector<float>());
        };
        if (!ParseObject(loadResult, extensions, "IGFX_compressed", parseCompressed)) {
            return false;
        }
#endif
        return true;
    };

    if (!ParseObject(loadResult, jsonData, "extensions", parseExtensions)) {
        return std::nullopt;
    }
    return data;
}

bool NodeChildren(LoadResult& loadResult, const json::value& jsonData, Node& node)
{
    return ForEachInArray(
        loadResult, jsonData, "children", [&node](LoadResult& loadResult, const json::value& item) -> bool {
            if (item.is_number()) {
                // indices will be later resolved to pointers when all nodes have been parsed
                // this is required since children may come later than parents
                node.tmpChildren.push_back(item.as_number<size_t>());
                return true;
            } else {
                node.tmpChildren.push_back(GLTF_INVALID_INDEX);
                SetError(loadResult, "Node children index was expected to be number");
                return false;
            }
        });
}

bool NodeTransform(LoadResult& loadResult, const json::value& jsonData, Node& node)
{
    bool result = true;
    if (auto const pos = jsonData.find("matrix"); pos) {
        if (ParseOptionalMath(loadResult, node.matrix, jsonData, "matrix", node.matrix)) {
            node.usesTRS = false;
        }
    } else {
        if (!ParseOptionalMath(loadResult, node.translation, jsonData, "translation", node.translation)) {
            result = false;
        }

        // order is x,y,z,w as defined in gltf
        if (!ParseOptionalMath(loadResult, node.rotation, jsonData, "rotation", node.rotation)) {
            result = false;
        }

        if (!ParseOptionalMath(loadResult, node.scale, jsonData, "scale", node.scale)) {
            result = false;
        }
    }
    return result;
}

bool NodeExtras(LoadResult& loadResult, const json::value& jsonData, Node& node)
{
#if defined(GLTF2_EXTRAS_RSDZ)
    const auto parseExtras = [&node](LoadResult& loadResult, const json::value& extras) -> bool {
        ParseOptionalString(loadResult, node.modelIdRSDZ, extras, "modelId", "");
        return true;
    };
    if (!ParseObject(loadResult, jsonData, "extras", parseExtras)) {
        return false;
    }
#endif
    return true;
}

bool ParseNode(LoadResult& loadResult, const json::value& jsonData)
{
    auto node = make_unique<Node>();

    bool result = NodeName(loadResult, jsonData, node->name);

    if (!NodeMesh(loadResult, jsonData, node->mesh)) {
        result = false;
    }

    if (!NodeCamera(loadResult, jsonData, node->camera)) {
        result = false;
    }

    if (auto extensionData = NodeExtensions(loadResult, jsonData, *node); extensionData) {
        if (!extensionData->compressed) {
            if (!ParseOptionalNumberArray(loadResult, node->weights, jsonData, "weights", vector<float>())) {
                result = false;
            }
        }
        if (!node->mesh && node->weights.size() > 0) {
            SetError(loadResult, "No mesh defined for node using morph target weights");
            result = false;
        }
#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
        if (extensionData->lightIndex != GLTF_INVALID_INDEX) {
            if (extensionData->lightIndex < loadResult.data->lights.size()) {
                node->light = loadResult.data->lights[extensionData->lightIndex].get();
            } else {
                SetError(loadResult, "Node refers to invalid light index");
                result = false;
            }
        }
#endif
    } else {
        result = false;
    }

    if (!ParseOptionalNumber(loadResult, node->tmpSkin, jsonData, "skin", GLTF_INVALID_INDEX)) {
        result = false;
    }
    if (!NodeChildren(loadResult, jsonData, *node)) {
        result = false;
    }

    if (!NodeTransform(loadResult, jsonData, *node)) {
        result = false;
    }

    if (!NodeExtras(loadResult, jsonData, *node)) {
        result = false;
    }

    loadResult.data->nodes.push_back(move(node));
    return result;
}

bool FinalizeNodes(LoadResult& loadResult)
{
    bool result = true;
    // resolve indices to direct pointers
    auto& nodes = loadResult.data->nodes;

    for (const auto& node : nodes) {
        for (auto index : node->tmpChildren) {
            if (index < nodes.size()) {
                auto childNode = nodes[index].get();
                assert(!childNode->parent &&
                       "Currently only single parent supported (GLTF spec reserves option to have multiple)");
                childNode->parent = node.get(); // since parent owns childs, and we don't want ref-loops, pass a
                                                // raw pointer instead
                node->children.push_back(childNode);
            } else {
                SetError(loadResult, "Invalid node index");
                result = false;
            }
        }

        if (node->tmpSkin != GLTF_INVALID_INDEX && node->tmpSkin < loadResult.data->skins.size()) {
            node->skin = loadResult.data->skins[node->tmpSkin].get();
        }
    }

    return result;
}

bool SceneExtensions(LoadResult& loadResult, const json::value& jsonData, Scene& scene)
{
    const auto parseExtensions = [&scene](LoadResult& loadResult, const json::value& extensions) -> bool {
        size_t lightIndex = GLTF_INVALID_INDEX;
#if defined(GLTF2_EXTENSION_KHR_LIGHTS)
        if (!ParseObject(loadResult, extensions, "KHR_lights",
                [&lightIndex](LoadResult& loadResult, const json::value& light) -> bool {
                    return ParseOptionalNumber<size_t>(loadResult, lightIndex, light, "light", GLTF_INVALID_INDEX);
                })) {
            return false;
        }
#endif
#if defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
        if (lightIndex == GLTF_INVALID_INDEX) {
            if (!ParseObject(loadResult, extensions, "KHR_lights_pbr",
                    [&lightIndex](LoadResult& loadResult, const json::value& light) -> bool {
                        if (!ParseOptionalNumber<size_t>(loadResult, lightIndex, light, "light", GLTF_INVALID_INDEX)) {
                            return false;
                        } else if (lightIndex != GLTF_INVALID_INDEX) {
                            lightIndex += loadResult.data->pbrLightOffset;
                        }
                        return true;
                    })) {
                return false;
            }
        }
#endif

#if defined(GLTF2_EXTENSION_EXT_LIGHTS_IMAGE_BASED)
        if (!ParseObject(loadResult, extensions, "EXT_lights_image_based",
                [&scene](LoadResult& loadResult, const json::value& light) -> bool {
                    return ParseOptionalNumber<size_t>(
                        loadResult, scene.imageBasedLightIndex, light, "light", GLTF_INVALID_INDEX);
                })) {
            return false;
        }
#endif
#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
        if (lightIndex != GLTF_INVALID_INDEX) {
            if (lightIndex < loadResult.data->lights.size()) {
                // Only Ambient light could be set for scene
                scene.light = loadResult.data->lights[lightIndex].get();
            } else {
                SetError(loadResult, "Scene refers to invalid light index");
                return false;
            }
        }
#endif
        return true;
    };

    return ParseObject(loadResult, jsonData, "extensions", parseExtensions);
}

bool ParseScene(LoadResult& loadResult, const json::value& jsonData)
{
    bool result = true;

    auto scene = make_unique<Scene>();
    if (!ParseOptionalString(loadResult, scene->name, jsonData, "name", "")) {
        return false;
    }

    const auto parseNodes = [&scene](LoadResult& loadResult, const json::value& nodeIndex) -> bool {
        if (!nodeIndex.is_number()) {
            RETURN_WITH_ERROR(loadResult, "Invalid node index (not a number)");
        }

        const size_t index = nodeIndex.as_number<size_t>();
        if (index >= loadResult.data->nodes.size()) {
            RETURN_WITH_ERROR(loadResult, "Invalid node index");
        }

        auto const equalsNodeToAdd = [nodeToAdd = loadResult.data->nodes[index].get()](
                                         auto const& node) { return node == nodeToAdd; };

        if (std::any_of(scene->nodes.begin(), scene->nodes.end(), equalsNodeToAdd)) {
            RETURN_WITH_ERROR(loadResult, "Non-unique node index");
        }

        scene->nodes.push_back(loadResult.data->nodes[index].get());

        return true;
    };

    if (!ForEachInArray(loadResult, jsonData, "nodes", parseNodes)) {
        result = false;
    }

    if (!SceneExtensions(loadResult, jsonData, *scene)) {
        result = false;
    }

    loadResult.data->scenes.push_back(move(scene));

    return result;
}

bool SceneContainsNode(Scene const& scene, Node const& node)
{
    return std::any_of(
        scene.nodes.begin(), scene.nodes.end(), [nodePtr = &node](auto const node) { return nodePtr == node; });
}

Scene* SceneContainingNode(vector<unique_ptr<Scene>> const& scenes, Node const& node)
{
    auto const pos = std::find_if(scenes.begin(), scenes.end(),
        [nodePtr = &node](auto const& scene) { return SceneContainsNode(*scene, *nodePtr); });
    if (pos != scenes.end()) {
        return pos->get();
    }
    return nullptr;
}

bool JointsInSameScene(Skin const& skin, LoadResult& loadResult)
{
    Scene const* scene = nullptr;
    return std::all_of(skin.joints.begin(), skin.joints.end(), [&loadResult, &scene](auto const joint) {
        Node const* hierarchyRoot = joint;
        while (hierarchyRoot->parent) {
            hierarchyRoot = hierarchyRoot->parent;
        }

        if (!scene) {
            scene = SceneContainingNode(loadResult.data->scenes, *hierarchyRoot);
            if (!scene) {
                RETURN_WITH_ERROR(loadResult, "Joint must belong to a scene");
            }
        } else if (!SceneContainsNode(*scene, *hierarchyRoot)) {
            RETURN_WITH_ERROR(loadResult, "Skin joints must belong to the same scene");
        }
        return true;
    });
}

bool ParseSkin(LoadResult& loadResult, const json::value& jsonData)
{
    auto skin = make_unique<Skin>();

    size_t matrices;
    if (!ParseOptionalNumber<size_t>(loadResult, matrices, jsonData, "inverseBindMatrices", GLTF_INVALID_INDEX)) {
        return false;
    }

    if (matrices != GLTF_INVALID_INDEX && matrices < loadResult.data->accessors.size()) {
        skin->inverseBindMatrices = loadResult.data->accessors[matrices].get();
    }

    size_t skeleton;
    if (!ParseOptionalNumber<size_t>(loadResult, skeleton, jsonData, "skeleton", GLTF_INVALID_INDEX)) {
        return false;
    }

    if (skeleton != GLTF_INVALID_INDEX && skeleton < loadResult.data->nodes.size()) {
        skin->skeleton = loadResult.data->nodes[skeleton].get();
    }

    vector<size_t> joints;
    if (!ParseOptionalNumberArray(loadResult, joints, jsonData, "joints", vector<size_t>())) {
        return false;
    }

    if (joints.size() > CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT) {
        CORE_LOG_W("Number of joints (%zu) more than current limit (%u)", joints.size(),
            CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT);
    }

    skin->joints.resize(joints.size());

    for (size_t i = 0; i < joints.size(); i++) {
        if (joints[i] >= loadResult.data->nodes.size()) {
            RETURN_WITH_ERROR(loadResult, "Invalid node index");
        }
        auto joint = loadResult.data->nodes[joints[i]].get();
        joint->isJoint = true;
        skin->joints[i] = joint;
    }

    loadResult.data->skins.push_back(move(skin));

    return true;
}

void FinalizeGltfContent(LoadResult& loadResult)
{
    using ImageContainer = vector<unique_ptr<Image>>;
    using TextureContainer = vector<unique_ptr<Texture>>;

    // See if there are duplicate images with the same uri.
    for (size_t imageIndex = 0; imageIndex < loadResult.data->images.size(); ++imageIndex) {
        if (loadResult.data->images[imageIndex]->uri.empty()) {
            continue;
        }

        bool hasDuplicate = false;
        for (size_t lookupImageIndex = imageIndex + 1; lookupImageIndex < loadResult.data->images.size();) {
            // Two images are the same?
            if (loadResult.data->images[imageIndex]->uri == loadResult.data->images[lookupImageIndex]->uri) {
                hasDuplicate = true;

                // Fix all textures to reference the first image.
                for (TextureContainer::iterator textureIt = loadResult.data->textures.begin();
                     textureIt != loadResult.data->textures.end(); ++textureIt) {
                    if ((*textureIt)->image == loadResult.data->images[lookupImageIndex].get()) {
                        (*textureIt)->image = loadResult.data->images[imageIndex].get();
                    }
                }

                // Two images are the same and the other one can be removed.
                const auto indexOffset = static_cast<typename ImageContainer::difference_type>(lookupImageIndex);
                loadResult.data->images.erase(loadResult.data->images.begin() + indexOffset);
            } else {
                ++lookupImageIndex;
            }
        }

        if (hasDuplicate) {
            CORE_LOG_D("Optimizing out duplicate image from glTF: %s/images/%zu",
                loadResult.data->defaultResources.c_str(), imageIndex);
        }
    }
}

bool AnimationSamplers(LoadResult& loadResult, const json::value& jsonData, Animation& animation)
{
    const auto parseSamplers = [&animation](LoadResult& loadResult, const json::value& samplerJson) -> bool {
        auto sampler = make_unique<AnimationSampler>();

        // parse sampler
        size_t accessor;
        if (!ParseOptionalNumber<size_t>(loadResult, accessor, samplerJson, "input", GLTF_INVALID_INDEX)) {
            return false;
        }

        if (accessor != GLTF_INVALID_INDEX && accessor < loadResult.data->accessors.size()) {
            sampler->input = loadResult.data->accessors[accessor].get();
        }

        if (!ParseOptionalNumber<size_t>(loadResult, accessor, samplerJson, "output", GLTF_INVALID_INDEX)) {
            return false;
        }

        if (accessor != GLTF_INVALID_INDEX && accessor < loadResult.data->accessors.size()) {
            sampler->output = loadResult.data->accessors[accessor].get();
        }

        string interpolation;
        if (!ParseOptionalString(loadResult, interpolation, samplerJson, "interpolation", string())) {
            return false;
        }

        // This attribute is not required, defaults to linear.
        GetAnimationInterpolation(interpolation, sampler->interpolation);

        animation.samplers.push_back(move(sampler));

        return true;
    };

    if (!ForEachInArray(loadResult, jsonData, "samplers", parseSamplers)) {
        return false;
    }
    return true;
}

bool AnimationChannels(LoadResult& loadResult, const json::value& jsonData, Animation& animation)
{
    const auto channelsParser = [&animation](LoadResult& loadResult, const json::value& channelJson) -> bool {
        AnimationTrack animationTrack;

        // parse sampler
        size_t sampler;
        if (!ParseOptionalNumber<size_t>(loadResult, sampler, channelJson, "sampler", GLTF_INVALID_INDEX)) {
            return false;
        }

        if (sampler != GLTF_INVALID_INDEX && sampler < animation.samplers.size()) {
            animationTrack.sampler = animation.samplers[sampler].get();
        }

        const auto targetParser = [&animationTrack](LoadResult& loadResult, const json::value& targetJson) -> bool {
            {
                string path;
                if (!ParseOptionalString(loadResult, path, targetJson, "path", string())) {
                    return false;
                }

                if (path.empty()) {
                    RETURN_WITH_ERROR(loadResult, "Path is required");
                }

                if (!GetAnimationPath(path, animationTrack.channel.path)) {
                    CORE_LOG_W("Skipping unsupported animation path: %s", path.c_str());
                    return false;
                }
            }

            size_t node;
            if (!ParseOptionalNumber<size_t>(loadResult, node, targetJson, "node", GLTF_INVALID_INDEX)) {
                return false;
            }

            if (node != GLTF_INVALID_INDEX && node < loadResult.data->nodes.size()) {
                animationTrack.channel.node = loadResult.data->nodes[node].get();
            } else {
                // this channel will be ignored
            }

            return true;
        };

        if (!ParseObject(loadResult, channelJson, "target", targetParser)) {
            return false;
        }

        animation.tracks.push_back(move(animationTrack));
        return true;
    };

    if (!ForEachInArray(loadResult, jsonData, "channels", channelsParser)) {
        return false;
    }
    return true;
}

bool ParseAnimation(LoadResult& loadResult, const json::value& jsonData)
{
    auto animation = make_unique<Animation>();
    if (!ParseOptionalString(loadResult, animation->name, jsonData, "name",
            "animation_" + to_string(loadResult.data->animations.size()))) {
        return false;
    }

    if (!AnimationSamplers(loadResult, jsonData, *animation)) {
        return false;
    }

    if (!AnimationChannels(loadResult, jsonData, *animation)) {
        return false;
    }

    if (!animation->tracks.empty() && !animation->samplers.empty()) {
        loadResult.data->animations.push_back(move(animation));
    } else {
        // RsdzExporter produces empty animations so just adding an error message.
        loadResult.error += "Skipped empty animation. Animation should have at least one channel and sampler.\n";
    }
    return true;
}

bool GltfAsset(LoadResult& loadResult, const json::value& jsonData)
{
    if (auto const& assetJson = jsonData.find("asset"); assetJson) {
        // Client implementations should first check whether a minVersion property is specified and ensure both
        // major and minor versions can be supported.
        string version;
        ParseOptionalString(loadResult, version, *assetJson, "minVersion", "");
        if (!version.empty()) {
            if (const auto minVersion = ParseVersion(version); minVersion) {
                if ((minVersion->first > 2u) || (minVersion->second > 0u)) {
                    RETURN_WITH_ERROR(loadResult, "Required glTF minVersion not supported");
                }
            } else {
                RETURN_WITH_ERROR(loadResult, "Invalid minVersion");
            }
        } else {
            // If no minVersion is specified, then clients should check the version property and ensure the major
            // version is supported.
            ParseOptionalString(loadResult, version, *assetJson, "version", "");
            if (const auto minVersion = ParseVersion(version); minVersion) {
                if ((minVersion->first > 2u)) {
                    RETURN_WITH_ERROR(loadResult, "Required glTF version not supported");
                }
            } else {
                RETURN_WITH_ERROR(loadResult, "Invalid version");
            }
        }
        return true;
    } else {
        RETURN_WITH_ERROR(loadResult, "Missing asset metadata");
    }
}

bool GltfRequiredExtension(LoadResult& loadResult, const json::value& jsonData)
{
    const auto parseRequiredExtensions = [](LoadResult& loadResult, const json::value& extension) {
        if (extension.is_string()) {
            const auto& val = extension.string_;
            if (std::find(std::begin(SUPPORTED_EXTENSIONS), std::end(SUPPORTED_EXTENSIONS), val) ==
                std::end(SUPPORTED_EXTENSIONS)) {
                SetError(loadResult, "glTF requires unsupported extension: " + val);
                return false;
            }
#if defined(GLTF2_EXTENSION_KHR_MESH_QUANTIZATION)
            if (val == "KHR_mesh_quantization") {
                loadResult.data->quantization = true;
            }
#endif
        }

        return true;
    };
    return ForEachInArray(loadResult, jsonData, "extensionsRequired", parseRequiredExtensions);
}

bool GltfUsedExtension(LoadResult& loadResult, const json::value& jsonData)
{
    const auto parseUsedExtensions = [](LoadResult& loadResult, const json::value& extension) {
        if (extension.is_string()) {
            const auto& val = extension.string_;
            if (std::find(std::begin(SUPPORTED_EXTENSIONS), std::end(SUPPORTED_EXTENSIONS), val) ==
                std::end(SUPPORTED_EXTENSIONS)) {
                CORE_LOG_W("glTF uses unsupported extension: %s", string(val).c_str());
            }
        }

        return true;
    };
    return ForEachInArray(loadResult, jsonData, "extensionsUsed", parseUsedExtensions);
}

bool GltfExtension(LoadResult& loadResult, const json::value& jsonData)
{
    return ParseObject(loadResult, jsonData, "extensions", [](LoadResult& loadResult, const json::value& extensions) {
        bool result = true;

#if defined(GLTF2_EXTENSION_KHR_LIGHTS)
        if (!ParseObject(loadResult, extensions, "KHR_lights_punctual",
                [](LoadResult& loadResult, const json::value& khrLights) {
                    return ForEachObjectInArray(loadResult, khrLights, "lights", ParseKHRLight);
                })) {
            result = false;
        }
#endif
#if defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
        loadResult.data->pbrLightOffset = static_cast<uint32_t>(loadResult.data->lights.size());

        if (!ParseObject(loadResult, extensions, "KHR_lights_pbr",
                [](LoadResult& loadResult, const json::value& khrLights) -> bool {
                    return ForEachObjectInArray(loadResult, khrLights, "lights", ParseKHRLight);
                })) {
            result = false;
        }
#endif

#if defined(GLTF2_EXTENSION_HW_XR_EXT)
        if (!ParseObject(
                loadResult, extensions, "HW_XR_EXT", [](LoadResult& loadResult, const json::value& jsonData) -> bool {
                    string thumbnailUri;
                    if (ParseOptionalString(loadResult, thumbnailUri, jsonData, "sceneThumbnail", "")) {
                        DecodeUri(thumbnailUri);
                        loadResult.data->thumbnails.push_back(Assets::Thumbnail { move(thumbnailUri), {}, {} });
                    } else {
                        return false;
                    }
                    return true;
                })) {
            result = false;
        }
#endif

#if defined(GLTF2_EXTENSION_EXT_LIGHTS_IMAGE_BASED)
        if (!ParseObject(loadResult, extensions, "EXT_lights_image_based",
                [](LoadResult& loadResult, const json::value& imageBasedLights) -> bool {
                    return ForEachObjectInArray(loadResult, imageBasedLights, "lights", ParseImageBasedLight);
                })) {
            result = false;
        }
#endif
        return result;
    });
}

bool GltfExtras(LoadResult& loadResult, const json::value& jsonData)
{
#if defined(GLTF2_EXTRAS_RSDZ)
    const auto parseExtras = [](LoadResult& loadResult, const json::value& extras) -> bool {
        return ForEachObjectInArray(loadResult, extras, "rsdzAnimations", ParseAnimation);
    };
    return ParseObject(loadResult, jsonData, "extras", parseExtras);
#else
    return true;
#endif
}

bool ParseGLTF(LoadResult& loadResult, const json::value& jsonData)
{
    if (!GltfAsset(loadResult, jsonData) || !GltfRequiredExtension(loadResult, jsonData)) {
        return false;
    }

    bool result = true;
    if (!GltfUsedExtension(loadResult, jsonData)) {
        result = false;
    }

    if (!ForEachObjectInArray(loadResult, jsonData, "buffers", ParseBuffer)) {
        result = false;
    }

    if (!ForEachObjectInArray(loadResult, jsonData, "bufferViews", ParseBufferView)) {
        result = false;
    }

    if (!ForEachObjectInArray(loadResult, jsonData, "accessors", ParseAccessor)) {
        result = false;
    }

    if (!ForEachObjectInArray(loadResult, jsonData, "images", ParseImage)) {
        result = false;
    }

    if (!ForEachObjectInArray(loadResult, jsonData, "samplers", ParseSampler)) {
        result = false;
    }

    if (!ForEachObjectInArray(loadResult, jsonData, "textures", ParseTexture)) {
        result = false;
    }

    if (!ForEachObjectInArray(loadResult, jsonData, "materials", ParseMaterial)) {
        result = false;
    }

    if (!ForEachObjectInArray(loadResult, jsonData, "meshes", ParseMesh)) {
        result = false;
    }

    if (!ForEachObjectInArray(loadResult, jsonData, "cameras", ParseCamera)) {
        result = false;
    }

    if (!GltfExtension(loadResult, jsonData)) {
        result = false;
    }

    if (!ForEachObjectInArray(loadResult, jsonData, "nodes", ParseNode)) {
        result = false;
    }

    if (!ForEachObjectInArray(loadResult, jsonData, "skins", ParseSkin)) {
        result = false;
    }

    if (!ForEachObjectInArray(loadResult, jsonData, "animations", ParseAnimation)) {
        result = false;
    }

    if (!GltfExtras(loadResult, jsonData)) {
        result = false;
    }

    if (!FinalizeNodes(loadResult)) {
        result = false;
    }

    if (!ForEachObjectInArray(loadResult, jsonData, "scenes", ParseScene)) {
        result = false;
    }

    if (!std::all_of(loadResult.data->skins.begin(), loadResult.data->skins.end(),
            [&loadResult](auto const& skin) { return JointsInSameScene(*skin, loadResult); })) {
        return false;
    }

    int defaultSceneIndex;
    if (!ParseOptionalNumber<int>(loadResult, defaultSceneIndex, jsonData, "scene", -1)) {
        result = false;
    } else if (defaultSceneIndex != -1) {
        if (defaultSceneIndex < 0 || size_t(defaultSceneIndex) >= loadResult.data->scenes.size()) {
            loadResult.error += "Invalid default scene index\n";
            loadResult.success = false;
        } else {
            loadResult.data->defaultScene = loadResult.data->scenes[static_cast<size_t>(defaultSceneIndex)].get();
        }
    }

    FinalizeGltfContent(loadResult);

    return result;
}

void LoadGLTF(LoadResult& loadResult, IFile& file)
{
    const uint64_t byteLength = file.GetLength();

    string raw;
    raw.resize(static_cast<size_t>(byteLength));

    if (file.Read(raw.data(), byteLength) != byteLength) {
        return;
    }
    CORE_CPU_PERF_BEGIN(jkson, "glTF", "Load", "jkatteluson::parse");
    json::value jsonObject = json::parse(raw.data());
    CORE_CPU_PERF_END(jkson);
    if (!jsonObject) {
        SetError(loadResult, "Parsing GLTF failed: invalid JSON");
        return;
    }

    ParseGLTF(loadResult, jsonObject);
}

bool LoadGLB(LoadResult& loadResult, IFile& file)
{
    GLBHeader header;
    uint64_t bytes = file.Read(&header, sizeof(GLBHeader));

    if (bytes < sizeof(GLBHeader)) {
        // cannot read header
        RETURN_WITH_ERROR(loadResult, "Parsing GLTF failed: expected GLB object");
    }

    if (header.magic != GLTF_MAGIC) {
        // 0x46546C67 >> "glTF"
        RETURN_WITH_ERROR(loadResult, "Parsing GLTF failed: expected GLB header");
    }

    if (header.length > loadResult.data->size) {
        RETURN_WITH_ERROR(loadResult, "Parsing GLTF failed: GLB header definition for size is larger than file size");
    } else {
        loadResult.data->size = header.length;
    }

    if (header.version != 2) {
        RETURN_WITH_ERROR(loadResult, "Parsing GLTF failed: expected GLB version 2");
    }

    GLBChunk chunkJson;
    bytes = file.Read(&chunkJson, sizeof(GLBChunk));

    if (bytes < sizeof(GLBChunk)) {
        // cannot read chunk data
        RETURN_WITH_ERROR(loadResult, "Parsing GLTF failed: expected GLB chunk");
    }

    if (chunkJson.chunkType != static_cast<uint32_t>(ChunkType::JSON) || chunkJson.chunkLength == 0 ||
        (chunkJson.chunkLength % 4) || chunkJson.chunkLength > (header.length - sizeof(header) - sizeof(chunkJson))) {
        // first chunk have to be JSON
        RETURN_WITH_ERROR(loadResult, "Parsing GLTF failed: expected JSON chunk");
    }

    const size_t dataOffset = chunkJson.chunkLength + sizeof(GLBHeader) + 2 * sizeof(GLBChunk);

    if (dataOffset > loadResult.data->size) {
        RETURN_WITH_ERROR(loadResult, "Parsing GLTF failed: data part offset is out of file");
    }

    loadResult.data->defaultResourcesOffset = static_cast<int32_t>(dataOffset);

    string jsonString;
    jsonString.resize(chunkJson.chunkLength);

    if (jsonString.size() != chunkJson.chunkLength) {
        RETURN_WITH_ERROR(loadResult, "Parsing GLTF failed: allocation for JSON data failed");
    }

    bytes = file.Read(reinterpret_cast<void*>(jsonString.data()), chunkJson.chunkLength);

    if (bytes < chunkJson.chunkLength) {
        // cannot read chunk data
        RETURN_WITH_ERROR(loadResult, "Parsing GLTF failed: JSON chunk size not match");
    }

    json::value o = json::parse(jsonString.data());
    if (!o) {
        RETURN_WITH_ERROR(loadResult, "Parsing GLTF failed: invalid JSON");
    }

    return ParseGLTF(loadResult, o);
}
} // namespace

// Internal loading function.
LoadResult LoadGLTF(IFileManager& fileManager, const string_view uri)
{
    LoadResult result;

    CORE_CPU_PERF_SCOPE("glTF", "LoadGLTF()", uri);

    IFile::Ptr file = fileManager.OpenFile(uri);
    if (!file) {
        CORE_LOG_D("Error loading '%s'", string(uri).data());
        return LoadResult("Failed to open file.");
    }

    const uint64_t fileLength = file->GetLength();
    if (fileLength > SIZE_MAX) {
        CORE_LOG_D("Error loading '%s'", string(uri).data());
        return LoadResult("Failed to open file, file size larger than SIZE_MAX");
    }

    string_view baseName;
    string_view path;
    SplitFilename(uri, baseName, path);

    result.data = make_unique<Data>(fileManager);
    result.data->filepath = path;
    result.data->defaultResources = baseName;
    result.data->size = static_cast<size_t>(fileLength);

    string_view baseNameNoExt;
    string_view extensionView;
    SplitBaseFilename(baseName, baseNameNoExt, extensionView);

    string extension(extensionView.size(), '\0');
    std::transform(extensionView.begin(), extensionView.end(), extension.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (extension == "gltf" || extension == "glt") {
        LoadGLTF(result, *file);
    } else if (extension == "glb") {
        LoadGLB(result, *file);
    } else {
        LoadGLB(result, *file);
    }
    return result;
}

LoadResult LoadGLTF(IFileManager& fileManager, array_view<uint8_t const> data)
{
    LoadResult result;

    // if the buffer starts with a GLB header assume GLB, otherwise glTF with embedded data.
    char const* ext = ".gltf";
    if (data.size() >= (sizeof(GLBHeader) + sizeof(GLBChunk))) {
        GLBHeader const& header = *reinterpret_cast<GLBHeader const*>(data.data());
        if (header.magic == GLTF_MAGIC) {
            ext = ".glb";
        }
    }

    // wrap the buffer in a temporary file
    auto const tmpFileName = "memory://" + to_string((uintptr_t)data.data()) + ext;
    {
        auto tmpFile = fileManager.CreateFile(tmpFileName);
        // NOTE: not ideal as this actually copies the data
        // alternative would be to cast to MemoryFile and give the array_view to the file
        tmpFile->Write(data.data(), data.size());

        result = GLTF2::LoadGLTF(fileManager, tmpFileName);
        if (result.success) {
            // File is stored here so it can be deleted from the file manager. loadBuffers is the only thing at
            // the moment which will need the file after parsing and that will check whether to use the
            // mMemoryFile or the URI related to the buffer.
            (*result.data).memoryFile_ = move(tmpFile);
        }
    }
    fileManager.DeleteFile(tmpFileName);
    return result;
}
} // namespace GLTF2
CORE3D_END_NAMESPACE()
