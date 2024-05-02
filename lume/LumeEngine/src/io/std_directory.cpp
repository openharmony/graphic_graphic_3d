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

#include "io/std_directory.h"

#include <algorithm>

#ifdef __has_include
#if __has_include(<filesystem>)
#include <filesystem>
#include <chrono>
#include <system_error>
#endif
#endif
#if !defined(HAS_FILESYSTEM)
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <cstddef>
#include <cstdint>

#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/io/intf_directory.h>
#include <core/log.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::make_unique;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::vector;

#if defined(HAS_FILESYSTEM)

// Hiding the directory implementation from the header.
struct DirImpl {
    explicit DirImpl(string_view path) : path_(path) {}
    string path_;
};

namespace {
uint64_t GetTimeStamp(const std::filesystem::directory_entry& entry)
{
    return static_cast<uint64_t>(entry.last_write_time().time_since_epoch().count());
}

IDirectory::Entry::Type GetEntryType(const std::filesystem::directory_entry& entry)
{
    if (entry.is_directory()) {
        return IDirectory::Entry::DIRECTORY;
    }
    if (entry.is_regular_file()) {
        return IDirectory::Entry::FILE;
    }

    return IDirectory::Entry::UNKNOWN;
}

std::filesystem::path U8Path(string_view str)
{
    return std::filesystem::u8path(str.begin().ptr(), str.end().ptr());
}
} // namespace

StdDirectory::StdDirectory(BASE_NS::unique_ptr<DirImpl> dir) : dir_(BASE_NS::move(dir)) {}

StdDirectory::~StdDirectory()
{
    Close();
}

void StdDirectory::Close()
{
    if (dir_) {
        dir_.reset();
    }
}

IDirectory::Ptr StdDirectory::Create(BASE_NS::string_view path)
{
    std::error_code ec;
    if (std::filesystem::create_directory(U8Path(path), ec)) {
        // Directory creation successful.
        return IDirectory::Ptr { BASE_NS::make_unique<StdDirectory>(make_unique<DirImpl>(path)).release() };
    }
    return {};
}

IDirectory::Ptr StdDirectory::Open(const string_view path)
{
    std::error_code ec;
    if (std::filesystem::is_directory(U8Path(path), ec)) {
        return IDirectory::Ptr { BASE_NS::make_unique<StdDirectory>(make_unique<DirImpl>(path)).release() };
    }
    const auto message = ec.message();
    CORE_LOG_E("'%.*s ec %d '%s'", static_cast<int>(path.size()), path.data(), ec.value(), message.data());
    return {};
}

vector<IDirectory::Entry> StdDirectory::GetEntries() const
{
    CORE_ASSERT_MSG(dir_, "Dir not open");
    vector<IDirectory::Entry> result;
    if (dir_) {
        std::error_code ec;
        for (auto& iter : std::filesystem::directory_iterator(U8Path(dir_->path_), ec)) {
            const auto filename = iter.path().filename().u8string();
            auto str = string(filename.c_str(), filename.length());
            result.push_back(IDirectory::Entry { GetEntryType(iter), move(str), GetTimeStamp(iter) });
        }
    }

    return result;
}

#else // HAS_FILESYSTEM not defined

// Hiding the directory implementation from the header.
struct DirImpl {
    DirImpl(const string_view path, DIR* aDir) : path_(path), dir_(aDir) {}
    string path_;
    DIR* dir_;
};

namespace {
IDirectory::Entry CreateEntry(const string_view path, const dirent& entry)
{
    const auto fullPath = path + string(entry.d_name);
    struct stat statBuf {};
    const auto statResult = (stat(fullPath.c_str(), &statBuf) == 0);

    IDirectory::Entry::Type type = IDirectory::Entry::UNKNOWN;
#ifdef _DIRENT_HAVE_D_TYPE
    switch (entry.d_type) {
        case DT_DIR:
            type = IDirectory::Entry::DIRECTORY;
            break;
        case DT_REG:
            type = IDirectory::Entry::FILE;
            break;
        default:
            type = IDirectory::Entry::UNKNOWN;
            break;
    }
#else
    if (statResult) {
        if (S_ISDIR(statBuf.st_mode)) {
            type = IDirectory::Entry::DIRECTORY;
        } else if (S_ISREG(statBuf.st_mode)) {
            type = IDirectory::Entry::FILE;
        }
    }
#endif

    // Get the timestamp for the directory entry.
    uint64_t timestamp = 0;
    if (statResult) {
        timestamp = static_cast<uint64_t>(statBuf.st_mtime);
    }

    return IDirectory::Entry { type, entry.d_name, timestamp };
}
} // namespace

StdDirectory::StdDirectory(BASE_NS::unique_ptr<DirImpl> dir) : dir_(BASE_NS::move(dir)) {}

StdDirectory::~StdDirectory()
{
    Close();
}

void StdDirectory::Close()
{
    if (dir_) {
        closedir(dir_->dir_);
        dir_.reset();
    }
}

IDirectory::Ptr StdDirectory::Create(BASE_NS::string_view path)
{
    int result = mkdir(string(path).c_str(), S_IRWXU | S_IRWXO);
    if (result == 0) {
        // Directory creation successful.
        return Open(path);
    }
    return {};
}

IDirectory::Ptr StdDirectory::Open(const string_view path)
{
    DIR* dir = opendir(string(path).c_str());
    if (dir) {
        return IDirectory::Ptr { make_unique<StdDirectory>(make_unique<DirImpl>(path, dir)).release() };
    }
    return {};
}

vector<IDirectory::Entry> StdDirectory::GetEntries() const
{
    CORE_ASSERT_MSG(dir_, "Dir not open");
    vector<IDirectory::Entry> result;
    if (dir_) {
        // Go back to start.
        rewinddir(dir_->dir_);

        // Iterate all entries and write to result.
        struct dirent* directoryEntry = nullptr;
        while ((directoryEntry = readdir(dir_->dir_)) != nullptr) {
            if (!strcmp(directoryEntry->d_name, ".") || !strcmp(directoryEntry->d_name, "..")) {
                continue;
            }
            result.push_back(CreateEntry(dir_->path_, *directoryEntry));
        }
    }

    return result;
}

#endif // HAS_FILESYSTEM

string StdDirectory::ResolveAbsolutePath(const string_view pathIn, bool isDirectory)
{
    string absolutePath;
    auto path = pathIn;
#ifdef _WIN32
    // remove the '/' slash..
    path = path.substr(1);
#endif

#ifdef HAS_FILESYSTEM
    std::error_code ec;
    std::filesystem::path fsPath = std::filesystem::canonical(U8Path(path), ec);
    if (ec.value() == 0) {
        const auto pathStr = fsPath.string();
        absolutePath.assign(pathStr.data(), pathStr.size());
    }
#elif defined(_WIN32)
    char resolvedPath[_MAX_PATH] = {};
    if (_fullpath(resolvedPath, string(path).c_str(), _MAX_PATH) != nullptr) {
        if (isDirectory) {
            auto handle = opendir(resolvedPath);
            if (handle) {
                closedir(handle);
                absolutePath = resolvedPath;
            }
        } else {
            auto handle = fopen(resolvedPath, "r");
            if (handle) {
                fclose(handle);
                absolutePath = resolvedPath;
            }
        }
    }
#elif defined(__linux__)
    char resolvedPath[PATH_MAX];
    if (realpath(string(path).c_str(), resolvedPath) != nullptr) {
        absolutePath = resolvedPath;
    }
#endif

    FormatPath(absolutePath, isDirectory);

    return absolutePath;
}

void StdDirectory::FormatPath(string& path, bool isDirectory)
{
    const size_t length = path.length();

    // Make directory separators consistent.
    std::replace(path.begin(), path.end(), '\\', '/');

    // Ensure there is last separator in place.
    if (path.length() > 0 && isDirectory) {
        if (path[length - 1] != '/') {
            path += '/';
        }
    }
}

string StdDirectory::GetDirName(const string_view fullPath)
{
    auto path = string(fullPath);
    StdDirectory::FormatPath(path, false);

    const size_t separatorPos = path.find_last_of('/');
    if (separatorPos == string::npos) {
        return ".";
    }
    path.resize(separatorPos);
    return path;
}

string StdDirectory::GetBaseName(const string_view fullPath)
{
    auto path = string(fullPath);
    StdDirectory::FormatPath(path, false);

    const size_t separatorPos = path.find_last_of('/');
    if (separatorPos == string::npos) {
        return path;
    }
    path.erase(0, separatorPos + 1);
    return path;
}
CORE_END_NAMESPACE()
