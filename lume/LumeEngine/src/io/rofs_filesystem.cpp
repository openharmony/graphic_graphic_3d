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

#include "rofs_filesystem.h"

#include <algorithm>

#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/log.h>

CORE_BEGIN_NAMESPACE()
namespace {
using BASE_NS::array_view;
using BASE_NS::CloneData;
using BASE_NS::move;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::vector;

struct fs_entry {
    const char fname[256];
    const uint64_t offset;
    const uint64_t size;
};

/** Read-only memory file. */
class ROFSMemoryFile final : public IFile {
public:
    ROFSMemoryFile(const uint8_t* const data, const size_t size) : data_(data), size_(size) {}

    ~ROFSMemoryFile() override = default;

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

    uint64_t Write(const void* buffer, uint64_t count) override
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
string_view trim(const string_view path)
{
    // remove leading and trailing slash..
    string_view t = path;
    if (!t.empty()) {
        if (*t.rbegin() == '/') {
            t = t.substr(0, t.length() - 1);
        }
    }
    if (!t.empty()) {
        if (*t.begin() == '/') {
            t = t.substr(1);
        }
    }
    return t;
}
} // namespace

RoFileSystem::RoFileSystem(const void* const blob, size_t blobSize)
{
    for (size_t i = 0; i < blobSize; i++) {
        IDirectory::Entry entry;
        const auto& romEntry = (reinterpret_cast<const fs_entry*>(blob))[i];
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
                const auto& parentDir = trim(path.substr(0, pathLength));
                if (const auto pos = directories_.find(parentDir); pos != directories_.cend()) {
                    // add each subdirectory only once
                    if (std::none_of(
                            pos->second.cbegin(), pos->second.cend(), [&entry](const IDirectory::Entry& child) {
                                return child.type == entry.type && child.name == entry.name;
                            })) {
                        pos->second.emplace_back(move(entry));
                    }
                } else {
                    directories_[string(parentDir)].emplace_back(move(entry));
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
        directories_[trim(path.substr(0, pathLength))].emplace_back(move(entry));
        const uint8_t* data = (const uint8_t*)(romEntry.offset + (uintptr_t)blob);
        files_[move(path)] = array_view(data, (size_t)romEntry.size);
    }
}

IDirectory::Entry RoFileSystem::GetEntry(const string_view uri)
{
    const string_view t = trim(uri);
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
    auto it = files_.find(path);
    if (it != files_.end()) {
        return IFile::Ptr { new ROFSMemoryFile(it->second.data(), it->second.size()) };
    }
    return IFile::Ptr();
}

IFile::Ptr RoFileSystem::CreateFile(const string_view path)
{
    return IFile::Ptr();
}

bool RoFileSystem::DeleteFile(const string_view path)
{
    return false;
}

IDirectory::Ptr RoFileSystem::OpenDirectory(const string_view path)
{
    const string_view t = trim(path);
    auto it = directories_.find(t);
    if (it != directories_.end()) {
        return IDirectory::Ptr { new ROFSMemoryDirectory(it->second) };
    }
    return IDirectory::Ptr();
}

IDirectory::Ptr RoFileSystem::CreateDirectory(const string_view path)
{
    return IDirectory::Ptr();
}

bool RoFileSystem::DeleteDirectory(const string_view path)
{
    return false;
}

bool RoFileSystem::Rename(const string_view fromPath, const string_view toPath)
{
    return false;
}

vector<string> RoFileSystem::GetUriPaths(const string_view) const
{
    return {};
}
CORE_END_NAMESPACE()
