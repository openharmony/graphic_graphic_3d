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

#include "rofs_filesystem.h"

#include <algorithm>
#include <cstdint>

#include <base/containers/allocator.h>
#include <base/containers/array_view.h>
#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/log.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
namespace {
using BASE_NS::array_view;
using BASE_NS::CloneData;
using BASE_NS::move;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::vector;

struct FsEntry {
    const char fname[256];
    const uint64_t offset;
    const uint64_t size;
};

/** Read-only memory file. */
class ROFSMemoryFile final : public IFile {
public:
    ~ROFSMemoryFile() override = default;
    ROFSMemoryFile(const uint8_t* const data, const size_t size) : data_(data), size_(size) {}
    ROFSMemoryFile(const ROFSMemoryFile&) = delete;
    ROFSMemoryFile(ROFSMemoryFile&&) = delete;
    ROFSMemoryFile& operator=(const ROFSMemoryFile&) = delete;
    ROFSMemoryFile& operator=(ROFSMemoryFile&&) = delete;

    Mode GetMode() const override
    {
        return IFile::Mode::READ_ONLY;
    }

    void Close() override {}

    uint64_t Read(void* buffer, uint64_t count) override
    {
        uint64_t toRead = count;
        if ((index_ + toRead) > size_) {
            toRead = size_ - index_;
        }

        if (toRead > 0) {
            if (toRead <= SIZE_MAX) {
                if (CloneData(buffer, static_cast<size_t>(count), data_ + index_, static_cast<size_t>(toRead))) {
                    index_ += toRead;
                }
            } else {
                CORE_ASSERT_MSG(false, "Unable to read chunks bigger than (SIZE_MAX) bytes.");
                toRead = 0;
            }
        }

        return toRead;
    }

    uint64_t Write(const void* /* buffer */, uint64_t /* count */) override
    {
        return 0;
    }

    uint64_t GetLength() const override
    {
        return size_;
    }

    bool Seek(uint64_t offset) override
    {
        if (offset < size_) {
            index_ = offset;
            return true;
        }

        return false;
    }

    uint64_t GetPosition() const override
    {
        return index_;
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    uint64_t index_ { 0 };
    const uint8_t* const data_;
    const size_t size_;
};

class ROFSMemoryDirectory final : public IDirectory {
public:
    ~ROFSMemoryDirectory() override = default;

    explicit ROFSMemoryDirectory(const vector<IDirectory::Entry>& contents) : contents_(contents) {}

    ROFSMemoryDirectory(const ROFSMemoryDirectory&) = delete;
    ROFSMemoryDirectory(ROFSMemoryDirectory&&) = delete;
    ROFSMemoryDirectory& operator=(const ROFSMemoryDirectory&) = delete;
    ROFSMemoryDirectory& operator=(ROFSMemoryDirectory&&) = delete;

    void Close() override {}

    vector<Entry> GetEntries() const override
    {
        return contents_;
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    const vector<IDirectory::Entry>& contents_;
};

string_view Trim(string_view path)
{
    // remove leading and trailing slash..
    if (!path.empty()) {
        if (path.back() == '/') {
            path.remove_suffix(1U);
        }
    }
    if (!path.empty()) {
        if (path.front() == '/') {
            path.remove_prefix(1);
        }
    }
    return path;
}
} // namespace

RoFileSystem::RoFileSystem(const void* const blob, size_t blobSize)
{
    for (size_t i = 0; i < blobSize; i++) {
        IDirectory::Entry entry;
        const auto& romEntry = (reinterpret_cast<const FsEntry*>(blob))[i];
        if (romEntry.fname[0] == 0) {
            break;
        }
        const string_view tmp = romEntry.fname;
        size_t t = 0;
        string path;
        for (;;) {
            const size_t t2 = tmp.find_first_of('/', t);
            if (t2 == string::npos) {
                break;
            }
            t = t2;
            const auto pathLength = path.length();
            entry.name = tmp.substr(pathLength, t - pathLength);
            path.reserve(pathLength + entry.name.length() + 1);
            path += entry.name;
            path += '/';
            if (directories_.find(path) == directories_.end()) {
                // new directory seen
                entry.type = IDirectory::Entry::DIRECTORY;
                const auto& parentDir = Trim(path.substr(0, pathLength));
                if (const auto pos = directories_.find(parentDir); pos != directories_.cend()) {
                    // add each subdirectory only once
                    if (std::none_of(
                        pos->second.cbegin(), pos->second.cend(), [&entry](const IDirectory::Entry& child) {
                            return child.type == entry.type && child.name == entry.name;
                        })) {
                        pos->second.push_back(move(entry));
                    }
                } else {
                    directories_[string(parentDir)].push_back(move(entry));
                }
                directories_[path.substr(0, path.length() - 1)].reserve(1);
            }
            t++;
        }
        // add the file entry..
        entry.name = tmp.substr(t);
        entry.type = IDirectory::Entry::FILE;
        const auto pathLength = path.length();
        path.reserve(pathLength + entry.name.length());
        path += entry.name;
        directories_[Trim(path.substr(0, pathLength))].push_back(move(entry));
        auto* data = reinterpret_cast<const uint8_t*>(blob) + romEntry.offset;
        files_[move(path)] = array_view(data, static_cast<size_t>(romEntry.size));
    }
}

IDirectory::Entry RoFileSystem::GetEntry(const string_view uri)
{
    const string_view t = Trim(uri);
    // check if it's a file first...
    const auto it = files_.find(t);
    if (it != files_.end()) {
        return { IDirectory::Entry::FILE, string(uri), 0 };
    }
    // is it a directory then
    const auto it2 = directories_.find(t);
    if (it2 != directories_.end()) {
        return { IDirectory::Entry::DIRECTORY, string(uri), 0 };
    }
    // nope. does not exist.
    return {};
}

IFile::Ptr RoFileSystem::OpenFile(const string_view path)
{
    auto it = files_.find(Trim(path));
    if (it != files_.end()) {
        return IFile::Ptr { new ROFSMemoryFile(it->second.data(), it->second.size()) };
    }
    return IFile::Ptr();
}

IFile::Ptr RoFileSystem::CreateFile(const string_view /* path */)
{
    return IFile::Ptr();
}

bool RoFileSystem::DeleteFile(const string_view /* path */)
{
    return false;
}

IDirectory::Ptr RoFileSystem::OpenDirectory(const string_view path)
{
    const string_view t = Trim(path);
    auto it = directories_.find(t);
    if (it != directories_.end()) {
        return IDirectory::Ptr { new ROFSMemoryDirectory(it->second) };
    }
    return IDirectory::Ptr();
}

IDirectory::Ptr RoFileSystem::CreateDirectory(const string_view /* path */)
{
    return IDirectory::Ptr();
}

bool RoFileSystem::DeleteDirectory(const string_view /* path */)
{
    return false;
}

bool RoFileSystem::Rename(const string_view /* fromPath */, const string_view /* toPath */)
{
    return false;
}

vector<string> RoFileSystem::GetUriPaths(const string_view) const
{
    return {};
}
CORE_END_NAMESPACE()
