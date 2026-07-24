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

/*
 * Fuzz the loader with well-formed GLB framing assembled from fuzz parameters. The fuzz input
 * drives a header + JSON chunk + BIN chunk wrapper around buffer/bufferView/accessor numeric
 * fields, so this target exercises GLB chunk parsing and accessor/buffer bounds arithmetic
 * (rather than raw JSON parsing, which the json-loader target covers).
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <securec.h>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/vector.h>

#include "fuzz_consumer.h"
#include "gltf_fuzz_common.h"

using namespace BASE_NS;

namespace {

constexpr uint32_t MAX_JSON_CHUNK_LEN = 8192;
constexpr uint32_t MAX_BIN_CHUNK_LEN = 8192;
constexpr uint32_t MAX_BUFFER_BYTE_LENGTH = 1024;
constexpr uint32_t MAX_BUFFER_VIEW_BYTE_LENGTH = 1024;
constexpr uint32_t MAX_BUFFER_VIEW_BYTE_STRIDE = 256;
constexpr uint32_t MAX_ACCESSOR_COUNT = 256;

constexpr uint32_t GLB_HEADER_SIZE = 12;
constexpr uint32_t GLB_CHUNK_HEADER_SIZE = 8;
constexpr uint32_t GLB_ALIGNMENT = 4;
constexpr uint32_t GLB_MAGIC = 0x46546C67;
constexpr uint32_t GLB_VERSION = 2;
constexpr uint32_t GLB_CHUNK_TYPE_JSON = 0x4E4F534A;
constexpr uint32_t GLB_CHUNK_TYPE_BIN = 0x004E4942;
constexpr uint32_t GLTF_COMPONENT_TYPE_FLOAT = 5126;

constexpr size_t GLB_FUZZ_HEADER_SIZE = 6;
constexpr size_t GLB_HEADER_PAYLOAD_OFFSET = 5;
constexpr size_t FUZZ_FIELD_SIZE = sizeof(uint32_t);
constexpr size_t FUZZ_FOURTH_FIELD_OFFSET = FUZZ_FIELD_SIZE * 4;
constexpr uint8_t FUZZ_RAW_MODE_BIT = 0x01;

void AppendU32(vector<uint8_t>& out, uint32_t val)
{
    size_t oldSize = out.size();
    out.resize(oldSize + sizeof(uint32_t));
    if (memcpy_s(out.data() + oldSize, sizeof(uint32_t), &val, sizeof(uint32_t)) != EOK) {
        out.resize(oldSize);
    }
}

string UintToString(uint32_t value)
{
    if (value == 0) {
        return string("0");
    }
    char buf[16];
    constexpr int decimalBase = 10;
    int pos = 0;
    while (value > 0) {
        buf[pos++] = '0' + static_cast<char>(value % decimalBase);
        value /= decimalBase;
    }
    std::reverse(buf, buf + pos);
    buf[pos] = '\0';
    return string(buf);
}

// rawMode: use the fuzz bytes as the JSON chunk. structured: buffers/bufferViews/accessors from
// fuzz fields (Field 0: buffer.byteLength, 1: bv.byteLength, 2: bv.byteStride, 3: accessor.count).
string BuildGltfJson(const uint8_t* fuzzPayload, size_t fuzzRemaining, uint32_t jsonChunkLen, bool rawMode)
{
    string jsonContent;
    if (rawMode) {
        size_t jsonLen = std::min(static_cast<size_t>(jsonChunkLen), fuzzRemaining);
        jsonContent.assign(reinterpret_cast<const char*>(fuzzPayload), jsonLen);
        return jsonContent;
    }

    jsonContent = "{\"asset\":{\"generator\":\"fuzz\",\"version\":\"2.0\"},"
                  "\"scenes\":[{\"nodes\":[]}],\"nodes\":[{}],\"meshes\":[]";
    FuzzConsumer fc(fuzzPayload, fuzzRemaining);
    size_t payloadLen = std::min(static_cast<size_t>(jsonChunkLen), fuzzRemaining);
    if (payloadLen > FUZZ_FIELD_SIZE) {
        uint32_t bufLen = 0;
        if (!fc.Consume(bufLen)) {
            jsonContent.push_back('}');
            return jsonContent;
        }
        jsonContent += ",\"buffers\":[{\"byteLength\":";
        jsonContent += UintToString(bufLen % MAX_BUFFER_BYTE_LENGTH);
        jsonContent += "}]";
    }
    if (payloadLen > FUZZ_FOURTH_FIELD_OFFSET) {
        uint32_t bvLen = 0;
        uint32_t bvStride = 0;
        uint32_t accCount = 0;
        if (!fc.Consume(bvLen) || !fc.Consume(bvStride) || !fc.Consume(accCount)) {
            jsonContent.push_back('}');
            return jsonContent;
        }
        jsonContent += ",\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":";
        jsonContent += UintToString(bvLen % MAX_BUFFER_VIEW_BYTE_LENGTH);
        jsonContent += ",\"byteStride\":";
        jsonContent += UintToString(bvStride % MAX_BUFFER_VIEW_BYTE_STRIDE);
        jsonContent += "}]";
        jsonContent += ",\"accessors\":[{\"componentType\":";
        jsonContent += UintToString(GLTF_COMPONENT_TYPE_FLOAT);
        jsonContent += ",\"count\":";
        jsonContent += UintToString(accCount % MAX_ACCESSOR_COUNT);
        jsonContent += ",\"type\":\"VEC3\",\"bufferView\":0,\"byteOffset\":0}]";
    }
    jsonContent += "}";
    return jsonContent;
}

void AssembleGLB(
    vector<uint8_t>& outGlb, const string& jsonContent, uint32_t binChunkLen, const uint8_t* binData, size_t binDataLen)
{
    string paddedJson = jsonContent;
    size_t pad = (GLB_ALIGNMENT - paddedJson.size() % GLB_ALIGNMENT) % GLB_ALIGNMENT;
    paddedJson.append(pad, ' ');

    uint32_t jsonActualLen = static_cast<uint32_t>(paddedJson.size());
    uint32_t totalLength =
        GLB_HEADER_SIZE + GLB_CHUNK_HEADER_SIZE + jsonActualLen + GLB_CHUNK_HEADER_SIZE + binChunkLen;

    outGlb.clear();
    outGlb.reserve(totalLength);

    AppendU32(outGlb, GLB_MAGIC);
    AppendU32(outGlb, GLB_VERSION);
    AppendU32(outGlb, totalLength);

    AppendU32(outGlb, jsonActualLen);
    AppendU32(outGlb, GLB_CHUNK_TYPE_JSON);
    {
        size_t jsonOld = outGlb.size();
        outGlb.resize(jsonOld + paddedJson.size());
        if (memcpy_s(outGlb.data() + jsonOld, paddedJson.size(), paddedJson.data(), paddedJson.size()) != EOK) {
            outGlb.clear();
            return;
        }
    }

    AppendU32(outGlb, binChunkLen);
    AppendU32(outGlb, GLB_CHUNK_TYPE_BIN);
    size_t fill = std::min(static_cast<size_t>(binChunkLen), binDataLen);
    if (fill > 0) {
        size_t binOld = outGlb.size();
        outGlb.resize(binOld + fill);
        if (memcpy_s(outGlb.data() + binOld, fill, binData, fill) != EOK) {
            outGlb.clear();
            return;
        }
    }
    outGlb.resize(outGlb.size() + binChunkLen - fill, 0);
}

bool BuildGLB(const uint8_t* data, size_t size, vector<uint8_t>& outGlb)
{
    if (size < GLB_FUZZ_HEADER_SIZE) {
        return false;
    }
    FuzzConsumer fc(data, size);
    uint8_t flags = 0;
    uint16_t jsonChunkLen16 = 0;
    uint16_t binChunkLen16 = 0;
    if (!fc.Consume(flags) || !fc.Consume(jsonChunkLen16) || !fc.Consume(binChunkLen16)) {
        return false;
    }
    uint32_t jsonChunkLen = static_cast<uint32_t>(jsonChunkLen16) % MAX_JSON_CHUNK_LEN;
    uint32_t binChunkLen = static_cast<uint32_t>(binChunkLen16) % MAX_BIN_CHUNK_LEN;

    const uint8_t* fuzzPayload = data + GLB_HEADER_PAYLOAD_OFFSET;
    size_t fuzzRemaining = fc.Remaining();

    string jsonContent = BuildGltfJson(fuzzPayload, fuzzRemaining, jsonChunkLen, (flags & FUZZ_RAW_MODE_BIT) != 0);

    size_t jsonConsumed = std::min(static_cast<size_t>(jsonChunkLen), fuzzRemaining);
    const uint8_t* binData = fuzzPayload + jsonConsumed;
    size_t binDataAvail = (jsonConsumed < fuzzRemaining) ? (fuzzRemaining - jsonConsumed) : 0;

    AssembleGLB(outGlb, jsonContent, binChunkLen, binData, binDataAvail);
    return true;
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    vector<uint8_t> glb;
    if (!BuildGLB(data, size, glb) || glb.empty()) {
        return 0;
    }
    GltfFuzz::RunLoadGltf(array_view<uint8_t const>(glb.data(), glb.size()));
    return 0;
}
