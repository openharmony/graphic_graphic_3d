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

#include "std_filesystem.h"

#if defined(__has_include)
#if __has_include(<filesystem>)
#include <filesystem>
#endif
#endif // defined(__has_include)

#if !defined(HAS_FILESYSTEM)
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <cstdint>
#include <cstdio>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/log.h>
#include <core/namespace.h>

#include "io/path_tools.h"
#include "std_directory.h"
#include "std_file.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::make_unique;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::vector;

namespace {
#if defined(HAS_FILESYSTEM)
std::filesystem::path U8Path(string_view str)
{
    return std::filesystem::u8path(str.begin().ptr(), str.end().ptr());
}
#endif
} // namespace

string StdFilesystem::ValidatePath(const string_view pathIn) const
{
    auto path = NormalizePath(pathIn);
    if (!path.empty()) {
        if (!basePath_.empty()) {
            // If basePath_ is set we are in a sandbox. so all paths are relative to basePath_ (after normalization)
            path = basePath_ + path;
        }
        // path must be absolute.
        if (path[0] != '/') {
            CORE_LOG_V("Corrupted path in StdFilesystem::ValidatePath. not absolute");
            return "";
        }
#ifdef _WIN32
        // path must have drive letter, otherwise it is NOT absolute. ie. must conform to "/C:/" style
        if ((path.length() < 4) || (path[2] != ':') || (path[3] != '/')) { // 4: size limit; 2 3: index of ':' '/'
            CORE_LOG_V("Corrupted path in StdFilesystem::ValidatePath. missing drive letter, or incorrect root");
            return "";
        }
        // remove the '/' slash, which is not used in windows.
        return string(path.substr(1));
#endif
    }
    return path;
}

IFile::Ptr StdFilesystem::OpenFile(const string_view pathIn)
{
    auto path = ValidatePath(pathIn);
    if (!path.empty()) {
        return StdFile::Open(path, IFile::Mode::READ_ONLY);
    }
    return IFile::Ptr();
}

IFile::Ptr StdFilesystem::CreateFile(const string_view pathIn)
{
    auto path = ValidatePath(pathIn);
    if (!path.empty()) {
        return StdFile::Create(path, IFile::Mode::READ_WRITE);
    }

    return IFile::Ptr();
}

bool StdFilesystem::DeleteFile(const string_view pathIn)
{
    auto path = ValidatePath(pathIn);
    if (path.empty()) {
        return false;
    }
#if defined(HAS_FILESYSTEM)
    std::error_code ec;
    return std::filesystem::remove(U8Path(path), ec) && !ec;
#else
    return std::remove(path.c_str()) == 0;
#endif
}

IDirectory::Ptr StdFilesystem::OpenDirectory(const string_view pathIn)
{
    auto path = ValidatePath(pathIn);
    if (!path.empty()) {
        return IDirectory::Ptr { StdDirectory::Open(path).release() };
    }

    return IDirectory::Ptr();
}

IDirectory::Ptr StdFilesystem::CreateDirectory(const string_view pathIn)
{
    auto path = ValidatePath(pathIn);
    if (!path.empty()) {
        return IDirectory::Ptr { StdDirectory::Create(path).release() };
    }

    return IDirectory::Ptr();
}

bool StdFilesystem::DeleteDirectory(const string_view pathIn)
{
    auto path = ValidatePath(pathIn);
    if (path.empty()) {
        return false;
    }
#if defined(HAS_FILESYSTEM)
    std::error_code ec;
    return std::filesystem::remove(U8Path(path), ec) && !ec;
#else
    return rmdir(string(path).c_str()) == 0;
#endif
}

bool StdFilesystem::Rename(const string_view fromPath, const string_view toPath)
{
    auto pathFrom = ValidatePath(fromPath);
    auto pathTo = ValidatePath(toPath);
    if (pathFrom.empty() || pathTo.empty()) {
        return false;
    }

#if defined(HAS_FILESYSTEM)
    std::error_code ec;
    std::filesystem::rename(U8Path(pathFrom), U8Path(pathTo), ec);
    return !ec;
#else
    return std::rename(pathFrom.c_str(), pathTo.c_str()) == 0;
#endif
}

vector<string> StdFilesystem::GetUriPaths(const string_view) const
{
    return {};
}

StdFilesystem::StdFilesystem(string_view basePath) : basePath_(basePath)
{
    // remove the extraneous slash
    if (basePath_.back() == '/') {
        basePath_.resize(basePath_.size() - 1);
    }
}

CORE_END_NAMESPACE()

// the rest is here, due to shlwapi leaking windows CreateFile macro, and breaking build.
#if !defined(HAS_FILESYSTEM)
#include <climits>
#define CORE_MAX_PATH PATH_MAX
#endif

CORE_BEGIN_NAMESPACE()
IDirectory::Entry StdFilesystem::GetEntry(const string_view uriIn)
{
    auto uri = ValidatePath(uriIn);
    if (!uri.empty()) {
#if defined(HAS_FILESYSTEM)
        std::error_code ec;
        auto canonicalPath = std::filesystem::canonical(U8Path(uri), ec);
        if (ec) {
            return {};
        }
        auto status = std::filesystem::status(canonicalPath, ec);
        if (ec) {
            return {};
        }
        auto time = std::filesystem::last_write_time(canonicalPath, ec);
        if (ec) {
            return {};
        }

        auto asString = canonicalPath.u8string();
        if (std::filesystem::is_directory(status)) {
            return { IDirectory::Entry::DIRECTORY, string { asString.data(), asString.size() },
                static_cast<uint64_t>(time.time_since_epoch().count()) };
        }
        if (std::filesystem::is_regular_file(status)) {
            return { IDirectory::Entry::FILE, string { asString.data(), asString.size() },
                static_cast<uint64_t>(time.time_since_epoch().count()) };
        }
#else
        auto path = string(uri);
        char canonicalPath[CORE_MAX_PATH] = { 0 };

        if (realpath(path.c_str(), canonicalPath) == nullptr) {
            return {};
        }
        struct stat ds {};
        if (stat(canonicalPath, &ds) != 0) {
            return {};
        }

        if ((ds.st_mode & S_IFDIR)) {
            return { IDirectory::Entry::DIRECTORY, canonicalPath, static_cast<uint64_t>(ds.st_mtime) };
        }
        if ((ds.st_mode & S_IFREG)) {
            return { IDirectory::Entry::FILE, canonicalPath, static_cast<uint64_t>(ds.st_mtime) };
        }
#endif
    }
    return {};
}

CORE_END_NAMESPACE()
