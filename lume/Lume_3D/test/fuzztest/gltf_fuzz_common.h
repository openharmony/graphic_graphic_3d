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

#ifndef FUZZTEST_GLTF_FUZZ_COMMON_H
#define FUZZTEST_GLTF_FUZZ_COMMON_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <securec.h>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_file_system.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>

#include "gltf/data.h"
#include "gltf/gltf2_loader.h"

// Shared, minimal-dependency fuzz harness for the glTF/GLB loader. Provides an in-memory
// IFileManager/IFile so the parse path links NO engine filesystem code, and a helper that
// drives the free GLTF2::LoadGLTF function (not the Gltf2 class, which pulls in render/ECS).

namespace GltfFuzz {

// In-memory IFile. Read is intentionally NOT bounds-checked so loader OOB reads trip ASan.
class MemoryFile : public CORE_NS::IFile {
public:
    MemoryFile() = default;
    ~MemoryFile() override
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
            void* newBuf = ::malloc(need);  // exact size: ASan red zone sits right after the data
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
        // no-op; FuzzFileManager owns and frees the MemoryFile instances.
    }
};

// In-memory IFileManager. allFiles_ owns every MemoryFile; DeleteFile only drops the lookup
// entry (no UAF while an IFile::Ptr is still held). Keyed on the full URI string, so the
// loader's "memory://N.gltf" temp-file round-trip works without any registered filesystem.
class FuzzFileManager : public CORE_NS::IFileManager {
public:
    ~FuzzFileManager() override
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
    CORE_NS::IFile::Ptr CreateFile(BASE_NS::string_view uri) override
    {
        auto* file = new MemoryFile();
        std::lock_guard<std::mutex> lock(mutex_);
        files_[BASE_NS::string(uri)] = file;
        allFiles_.push_back(file);
        return CORE_NS::IFile::Ptr(file);
    }
    CORE_NS::IFile::Ptr OpenFile(BASE_NS::string_view uri) override
    {
        return OpenFile(uri, CORE_NS::IFile::Mode::READ_ONLY);
    }
    CORE_NS::IFile::Ptr OpenFile(BASE_NS::string_view uri, CORE_NS::IFile::Mode) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = files_.find(BASE_NS::string(uri));
        if (it != files_.end()) {
            it->second->Seek(0);
            return CORE_NS::IFile::Ptr(it->second);
        }
        return CORE_NS::IFile::Ptr();
    }
    bool DeleteFile(BASE_NS::string_view uri) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = files_.find(BASE_NS::string(uri));
        if (it != files_.end()) {
            files_.erase(it);  // lookup-only removal; allFiles_ still owns it
            return true;
        }
        return false;
    }
    bool FileExists(BASE_NS::string_view uri) const override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return files_.find(BASE_NS::string(uri)) != files_.end();
    }
    CORE_NS::IDirectory::Entry GetEntry(BASE_NS::string_view) override
    {
        return {CORE_NS::IDirectory::Entry::UNKNOWN, "", 0};
    }
    CORE_NS::IDirectory::Ptr OpenDirectory(BASE_NS::string_view) override
    {
        return {};
    }
    CORE_NS::IDirectory::Ptr CreateDirectory(BASE_NS::string_view) override
    {
        return {};
    }
    bool DeleteDirectory(BASE_NS::string_view) override
    {
        return false;
    }
    bool DirectoryExists(BASE_NS::string_view) const override
    {
        return false;
    }
    bool Rename(BASE_NS::string_view, BASE_NS::string_view) override
    {
        return false;
    }
    bool RegisterFilesystem(BASE_NS::string_view, CORE_NS::IFilesystem::Ptr) override
    {
        return false;
    }
    void UnregisterFilesystem(BASE_NS::string_view) override
    {}
    CORE_NS::IFilesystem* GetFilesystem(BASE_NS::string_view) const override
    {
        return nullptr;
    }
    void RegisterAssetPath(BASE_NS::string_view) override
    {}
    void UnregisterAssetPath(BASE_NS::string_view) override
    {}
    bool CheckExistence(BASE_NS::string_view) override
    {
        return false;
    }
    bool RegisterPath(BASE_NS::string_view, BASE_NS::string_view, bool) override
    {
        return false;
    }
    void UnregisterPath(BASE_NS::string_view, BASE_NS::string_view) override
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
    BASE_NS::unordered_map<BASE_NS::string, MemoryFile*> files_;
    BASE_NS::vector<MemoryFile*> allFiles_;
};

// Drive the loader over one buffer. The LoadResult is scoped to die BEFORE the file manager:
// it holds a unique_ptr<IFile> (the temp memoryFile_), and the manager frees the backing
// MemoryFile in its destructor/Clear, so the result must be released first.
inline void RunLoadGltf(BASE_NS::array_view<uint8_t const> data)
{
    FuzzFileManager fileManager;
    {
        CORE3D_NS::GLTF2::LoadResult res = CORE3D_NS::GLTF2::LoadGLTF(fileManager, data);
        if (res.success && res.data) {
            res.data->GetSceneCount();
            res.data->GetDefaultSceneIndex();
            res.data->GetExternalFileUris();
            res.data->GetThumbnailImageCount();
        }
    }
}

}  // namespace GltfFuzz

#endif  // FUZZTEST_GLTF_FUZZ_COMMON_H
