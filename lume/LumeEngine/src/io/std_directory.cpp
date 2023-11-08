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

#include "io/std_directory.h"

#include <algorithm>

// MINGW has filesystem but it's buggy: https://sourceforge.net/p/mingw-w64/bugs/737/
// Not using it for now.
#ifdef _MSC_VER
#include <filesystem>
#define HAS_FILESYSTEM
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <core/log.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::make_unique;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::vector;

#ifdef HAS_FILESYSTEM

// Hiding the directory implementation from the header.
struct DirImpl {
    DirImpl(const string_view path) : path_(path) {}
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
    } else if (entry.is_regular_file()) {
        return IDirectory::Entry::FILE;
    }

    return IDirectory::Entry::UNKNOWN;
}
} // namespace

StdDirectory::StdDirectory() = default;

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

bool StdDirectory::Open(const string_view path)
{
    CORE_ASSERT_MSG((dir_ == nullptr), "Dir already opened.");
    std::error_code ec;
    if (std::filesystem::is_directory({ path.begin().ptr(), path.end().ptr() }, ec)) {
        dir_ = make_unique<DirImpl>(path);
        return true;
    }
    return false;
}

vector<IDirectory::Entry> StdDirectory::GetEntries() const
{
    CORE_ASSERT_MSG(dir_, "Dir not open");
    vector<IDirectory::Entry> result;
    if (dir_) {
        std::error_code ec;
        for (auto& iter :
            std::filesystem::directory_iterator({ dir_->path_.begin().ptr(), dir_->path_.end().ptr() }, ec)) {
            const auto filename = iter.path().filename().u8string();
            auto str = string(filename.c_str(), filename.length());
            result.emplace_back(IDirectory::Entry { GetEntryType(iter), move(str), GetTimeStamp(iter) });
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

    IDirectory::Entry::Type type;
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
    type = IDirectory::Entry::UNKNOWN;
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
        timestamp = statBuf.st_mtime;
    }

    return IDirectory::Entry { type, entry.d_name, timestamp };
}
} // namespace

StdDirectory::StdDirectory() : dir_() {}

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

bool StdDirectory::Open(const string_view path)
{
    CORE_ASSERT_MSG((dir_ == nullptr), "Dir already opened.");
    DIR* dir = opendir(string(path).c_str());
    if (dir) {
        dir_ = make_unique<DirImpl>(path, dir);
        return true;
    }
    return false;
}

vector<IDirectory::Entry> StdDirectory::GetEntries() const
{
    CORE_ASSERT_MSG(dir_, "Dir not open");
    vector<IDirectory::Entry> result;
    if (dir_) {
        // Go back to start.
        rewinddir(dir_->dir_);

        // Iterate all entries and write to result.
        struct dirent* directoryEntry;
        while ((directoryEntry = readdir(dir_->dir_)) != nullptr) {
            if (!strcmp(directoryEntry->d_name, ".") || !strcmp(directoryEntry->d_name, "..")) {
                continue;
            }
            result.emplace_back(CreateEntry(dir_->path_, *directoryEntry));
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
    std::filesystem::path fsPath = std::filesystem::canonical({ path.begin().ptr(), path.end().ptr() }, ec);
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
    } else {
        path.resize(separatorPos);
        return path;
    }
}

string StdDirectory::GetBaseName(const string_view fullPath)
{
    auto path = string(fullPath);
    StdDirectory::FormatPath(path, false);

    const size_t separatorPos = path.find_last_of('/');
    if (separatorPos == string::npos) {
        return path;
    } else {
        path.erase(0, separatorPos + 1);
        return path;
    }
}
CORE_END_NAMESPACE()
