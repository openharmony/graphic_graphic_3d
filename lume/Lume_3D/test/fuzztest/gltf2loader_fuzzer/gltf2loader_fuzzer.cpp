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

/* Fuzz test for IGltf2::LoadGLTF() via <3d/gltf/gltf.h>. */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <securec.h>

#include <3d/gltf/gltf.h>
#include <3d/namespace.h>
#include <base/containers/array_view.h>
#include <base/containers/refcnt_ptr.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_file_system.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>

#include "fuzz_consumer.h"
#include "gltf/gltf2.h"
#include "gltf2loader_fuzzer.h"

using namespace BASE_NS;

namespace {

// In-memory IFile. Destroy() is no-op; lifetime managed by FuzzFileManager.
// Intentionally no bounds check on Read: OOB reads trigger ASan detection.
class MemoryFile : public CORE_NS::IFile {
public:
    MemoryFile() = default;
    ~MemoryFile()
    {
        ::free(buf_);
    }
    MemoryFile(const MemoryFile&) = delete;
    MemoryFile& operator=(const MemoryFile&) = delete;

    Mode GetMode() const override
    {
        return Mode::READ_WRITE;
    }
    void Close() override
    {
        pos_ = 0;
    }

    uint64_t Read(void* buffer, uint64_t count) override
    {
        if (buffer == nullptr || count == 0) {
            return 0;
        }
        if (memcpy_s(buffer, static_cast<size_t>(count), buf_ + pos_, static_cast<size_t>(count)) != EOK) {
            return 0;
        }
        pos_ += static_cast<size_t>(count);
        return count;
    }

    uint64_t Write(const void* buffer, uint64_t count) override
    {
        if (buffer == nullptr || count == 0) {
            return 0;
        }
        size_t need = pos_ + static_cast<size_t>(count);
        if (need < pos_) {
            return 0;  // overflow
        }
        if (need > cap_) {
            // Allocate exactly needed bytes so ASan red zone is right after data.
            void* newBuf = ::malloc(need);
            if (!newBuf) {
                return 0;
            }
            if (buf_ != nullptr && size_ > 0) {
                if (memcpy_s(newBuf, need, buf_, size_) != EOK) {
                    ::free(newBuf);
                    return 0;
                }
            }
            ::free(buf_);
            buf_ = static_cast<uint8_t*>(newBuf);
            cap_ = need;
        }
        if (memcpy_s(buf_ + pos_, cap_ - pos_, buffer, static_cast<size_t>(count)) != EOK) {
            return 0;
        }
        pos_ += static_cast<size_t>(count);
        if (pos_ > size_) {
            size_ = pos_;
        }
        return count;
    }

    uint64_t Append(const void* buffer, uint64_t count, uint64_t) override
    {
        pos_ = size_;
        return Write(buffer, count);
    }

    uint64_t GetLength() const override
    {
        return size_;
    }

    bool Seek(uint64_t offset) override
    {
        if (offset > size_) {
            return false;
        }
        pos_ = static_cast<size_t>(offset);
        return true;
    }

    uint64_t GetPosition() const override
    {
        return pos_;
    }

private:
    uint8_t* buf_ = nullptr;
    size_t size_ = 0;
    size_t cap_ = 0;
    size_t pos_ = 0;
    void Destroy() override
    {
        // no-op; FuzzFileManager::Clear() handles cleanup
    }
};

// In-memory IFileManager. allFiles_ owns all MemoryFile instances;
// DeleteFile only removes from lookup map to avoid UAF while Ptr still held.
class FuzzFileManager : public CORE_NS::IFileManager {
public:
    ~FuzzFileManager()
    {
        Clear();
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto* f : allFiles_) {
            delete f;
        }
        allFiles_.clear();
        files_.clear();
    }

    CORE_NS::IFile::Ptr CreateFile(string_view uri) override
    {
        auto* file = new MemoryFile();
        std::lock_guard<std::mutex> lock(mutex_);
        files_[string(uri)] = file;
        allFiles_.push_back(file);
        return CORE_NS::IFile::Ptr(file);
    }

    CORE_NS::IFile::Ptr OpenFile(string_view uri) override
    {
        return OpenFile(uri, CORE_NS::IFile::Mode::READ_ONLY);
    }

    CORE_NS::IFile::Ptr OpenFile(string_view uri, CORE_NS::IFile::Mode) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = files_.find(string(uri));
        if (it != files_.end()) {
            it->second->Seek(0);
            return CORE_NS::IFile::Ptr(it->second);
        }
        return CORE_NS::IFile::Ptr();
    }

    bool DeleteFile(string_view uri) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = files_.find(string(uri));
        if (it != files_.end()) {
            files_.erase(it);  // lookup-only removal, allFiles_ still owns it
            return true;
        }
        return false;
    }

    bool FileExists(string_view uri) const override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return files_.find(string(uri)) != files_.end();
    }

    CORE_NS::IDirectory::Entry GetEntry(string_view) override
    {
        return {CORE_NS::IDirectory::Entry::UNKNOWN, "", 0};
    }
    CORE_NS::IDirectory::Ptr OpenDirectory(string_view) override
    {
        return {};
    }
    CORE_NS::IDirectory::Ptr CreateDirectory(string_view) override
    {
        return {};
    }
    bool DeleteDirectory(string_view) override
    {
        return false;
    }
    bool DirectoryExists(string_view) const override
    {
        return false;
    }
    bool Rename(string_view, string_view) override
    {
        return false;
    }
    bool RegisterFilesystem(string_view, CORE_NS::IFilesystem::Ptr) override
    {
        return false;
    }
    void UnregisterFilesystem(string_view) override
    {}
    CORE_NS::IFilesystem* GetFilesystem(string_view) const override
    {
        return nullptr;
    }
    void RegisterAssetPath(string_view) override
    {}
    void UnregisterAssetPath(string_view) override
    {}
    bool CheckExistence(string_view) override
    {
        return false;
    }
    bool RegisterPath(string_view, string_view, bool) override
    {
        return false;
    }
    void UnregisterPath(string_view, string_view) override
    {}
    CORE_NS::IFilesystem::Ptr CreateROFilesystem(const void* const, uint64_t) override
    {
        return {};
    }

    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override
    {
        if (uid == CORE_NS::IFileManager::UID || uid == CORE_NS::IInterface::UID) {
            return this;
        }
        return nullptr;
    }
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override
    {
        if (uid == CORE_NS::IFileManager::UID || uid == CORE_NS::IInterface::UID) {
            return this;
        }
        return nullptr;
    }
    void Ref() override
    {}
    void Unref() override
    {}

private:
    mutable std::mutex mutex_;
    unordered_map<string, MemoryFile*> files_;
    vector<MemoryFile*> allFiles_;
};

FuzzFileManager g_fileManager;
static CORE3D_NS::Gltf2 g_gltf2(g_fileManager);

// GLB constants
static constexpr size_t GLB_FUZZ_HEADER_SIZE = 6;
static constexpr size_t GLB_HEADER_PAYLOAD_OFFSET = 5;
static constexpr uint32_t MAX_JSON_CHUNK_LEN = 8192;
static constexpr uint32_t MAX_BIN_CHUNK_LEN = 8192;
static constexpr uint32_t MAX_BUFFER_BYTE_LENGTH = 1024;
static constexpr uint32_t MAX_BUFFER_VIEW_BYTE_LENGTH = 1024;
static constexpr uint32_t MAX_BUFFER_VIEW_BYTE_STRIDE = 256;
static constexpr uint32_t MAX_ACCESSOR_COUNT = 256;

static constexpr uint32_t GLB_HEADER_SIZE = 12;
static constexpr uint32_t GLB_CHUNK_HEADER_SIZE = 8;
static constexpr uint32_t GLB_ALIGNMENT = 4;

static constexpr uint32_t GLB_MAGIC = 0x46546C67;
static constexpr uint32_t GLB_VERSION = 2;
static constexpr uint32_t GLB_CHUNK_TYPE_JSON = 0x4E4F534A;
static constexpr uint32_t GLB_CHUNK_TYPE_BIN = 0x004E4942;

static constexpr uint32_t GLTF_COMPONENT_TYPE_FLOAT = 5126;
static constexpr uint32_t GLTF_COMPONENT_TYPE_UNSIGNED_INT = 5125;

static constexpr size_t FUZZ_FIELD_SIZE = sizeof(uint32_t);

static constexpr size_t FUZZ_MIN_INPUT_SIZE = 2;
static constexpr size_t FUZZ_SELECTOR_OFFSET = 0;
static constexpr size_t FUZZ_PAYLOAD_OFFSET = 1;
static constexpr uint8_t FUZZ_RAW_MODE_BIT = 0x01;
static constexpr uint8_t FUZZ_CASE_COUNT = 2;
static constexpr size_t FUZZ_FOURTH_FIELD_OFFSET = FUZZ_FIELD_SIZE * 4;

void AppendU32(vector<uint8_t>& out, uint32_t val)
{
    size_t oldSize = out.size();
    out.resize(oldSize + sizeof(uint32_t));
    if (memcpy_s(out.data() + oldSize, sizeof(uint32_t), &val, sizeof(uint32_t)) != EOK) {
        out.resize(oldSize);
        return;
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

// rawMode: fuzz bytes as JSON. structured: buffers/bufferViews/accessors from fuzz fields.
//   Field 0: buffer.byteLength, Field 1: bv.byteLength, Field 2: bv.byteStride, Field 3: accessor.count
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
    // Field 0: buffer.byteLength
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

    // Fields 1-3: bufferView + accessor
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

// Assemble GLB binary from JSON content and BIN data.
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

    // GLB header
    AppendU32(outGlb, GLB_MAGIC);
    AppendU32(outGlb, GLB_VERSION);
    AppendU32(outGlb, totalLength);

    // JSON chunk
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

    // BIN chunk header + data
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

void ExerciseGLTFData(CORE3D_NS::IGLTFData& gltfData)
{
    gltfData.GetSceneCount();
    gltfData.GetDefaultSceneIndex();
    gltfData.GetExternalFileUris();
    gltfData.GetThumbnailImageCount();
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < FUZZ_MIN_INPUT_SIZE) {
        return 0;
    }

    uint8_t selector = data[FUZZ_SELECTOR_OFFSET];
    const uint8_t* payload = data + FUZZ_PAYLOAD_OFFSET;
    size_t payloadSize = size - FUZZ_PAYLOAD_OFFSET;

    switch (selector % FUZZ_CASE_COUNT) {
        case 0: {
            // Raw fuzz bytes fed directly to LoadGLTF
            auto result = g_gltf2.LoadGLTF(array_view<uint8_t const>(payload, payloadSize));
            if (result.success && result.data) {
                ExerciseGLTFData(*result.data);
            }
            break;
        }
        case 1: {
            // Construct well-formed GLB from fuzz parameters
            vector<uint8_t> glbData;
            if (BuildGLB(payload, payloadSize, glbData)) {
                auto result = g_gltf2.LoadGLTF(array_view<uint8_t const>(glbData.data(), glbData.size()));
                if (result.success && result.data) {
                    ExerciseGLTFData(*result.data);
                }
            }
            break;
        }
        default: {
            break;
        }
    }

    g_fileManager.Clear();
    return 0;
}
