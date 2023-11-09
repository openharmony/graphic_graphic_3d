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

#include "std_filesystem.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <core/namespace.h>

#include "io/path_tools.h"
#include "std_directory.h"
#include "std_file.h"
#include "util/string_util.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::make_unique;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::vector;

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
        auto file = make_unique<StdFile>();
        if (file->Open(path, IFile::Mode::READ_ONLY)) {
            return IFile::Ptr { file.release() };
        }
    }
    return IFile::Ptr();
}

IFile::Ptr StdFilesystem::CreateFile(const string_view pathIn)
{
    auto path = ValidatePath(pathIn);
    if (!path.empty()) {
        auto file = make_unique<StdFile>();
        if (file->Create(path, IFile::Mode::READ_WRITE)) {
            return IFile::Ptr { file.release() };
        }
    }

    return IFile::Ptr();
}

bool StdFilesystem::DeleteFile(const string_view pathIn)
{
    auto path = ValidatePath(pathIn);
    if (path.empty()) {
        return false;
    }
    return remove(path.c_str()) == 0;
}

IDirectory::Ptr StdFilesystem::OpenDirectory(const string_view pathIn)
{
    auto path = ValidatePath(pathIn);
    if (!path.empty()) {
        auto directory = make_unique<StdDirectory>();
        if (directory->Open(path)) {
            return IDirectory::Ptr { directory.release() };
        }
    }

    return IDirectory::Ptr();
}

IDirectory::Ptr StdFilesystem::CreateDirectory(const string_view pathIn)
{
    auto path = ValidatePath(pathIn);
    if (!path.empty()) {
#ifdef _WIN32
        int result = _mkdir(string(path).c_str());
#else
        int result = mkdir(string(path).c_str(), S_IRWXU | S_IRWXO);
#endif
        if (result == 0) {
            // Directory creation successful.
            return OpenDirectory(path);
        }
    }

    return IDirectory::Ptr();
}

bool StdFilesystem::DeleteDirectory(const string_view pathIn)
{
    auto path = ValidatePath(pathIn);
    if (path.empty()) {
        return false;
    }
#ifdef _WIN32
    return _rmdir(string(path).c_str()) == 0;
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

    return rename(pathFrom.c_str(), pathTo.c_str()) == 0;
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
#if defined(_WIN32)
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#define CORE_MAX_PATH MAX_PATH
#else
#include <climits>
#define CORE_MAX_PATH PATH_MAX
#endif

CORE_BEGIN_NAMESPACE()
IDirectory::Entry StdFilesystem::GetEntry(const string_view uriIn)
{
    auto uri = ValidatePath(uriIn);
    if (!uri.empty()) {
        struct stat ds;
        auto path = string(uri);
        char canonicalPath[CORE_MAX_PATH] = { 0 };

#if defined(_WIN32)
        if (!PathCanonicalize(canonicalPath, path.c_str())) {
            return {};
        }
#else
        if (realpath(path.c_str(), canonicalPath) == NULL) {
            return {};
        }
#endif

        if (stat(canonicalPath, &ds) != 0) {
            return {};
        }

        if ((ds.st_mode & S_IFDIR)) {
            return { IDirectory::Entry::DIRECTORY, canonicalPath, static_cast<uint64_t>(ds.st_mtime) };
        }
        if ((ds.st_mode & S_IFREG)) {
            return { IDirectory::Entry::FILE, canonicalPath, static_cast<uint64_t>(ds.st_mtime) };
        }
    }
    return {};
}

CORE_END_NAMESPACE()