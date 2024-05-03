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

#include "io/std_file.h"

#include <cstdint>

#ifdef __has_include
#if __has_include(<filesystem>)
#include <filesystem>
#endif
#endif

#ifndef HAS_FILESYSTEM
#include <cerrno>
#include <dirent.h>

#ifndef _DIRENT_HAVE_D_TYPE
#include <sys/stat.h>
#include <sys/types.h>

#endif
#include <climits>
#define CORE_MAX_PATH PATH_MAX
#endif

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/io/intf_file.h>
#include <core/log.h>
#include <core/namespace.h>

#include "io/std_directory.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;

namespace {
std::ios_base::openmode OpenFileAccessMode(IFile::Mode mode)
{
    switch (mode) {
        case IFile::Mode::INVALID:
            CORE_ASSERT_MSG(false, "Invalid file access mode.");
            return {};
        case IFile::Mode::READ_ONLY:
            return std::ios_base::binary | std::ios_base::in;
        case IFile::Mode::READ_WRITE:
            return std::ios_base::binary | std::ios_base::out | std::ios_base::in;
        default:
            return {};
    }
}

std::ios_base::openmode CreateFileAccessMode(IFile::Mode mode)
{
    switch (mode) {
        case IFile::Mode::INVALID:
        case IFile::Mode::READ_ONLY:
            CORE_ASSERT_MSG(false, "Invalid create file access mode.");
            return {};
        case IFile::Mode::READ_WRITE:
            return std::ios_base::binary | std::ios_base::out | std::ios_base::in | std::ios_base::trunc;
        default:
            return {};
    }
}

#if defined(HAS_FILESYSTEM)
std::filesystem::path U8Path(string_view str)
{
    return std::filesystem::u8path(str.begin().ptr(), str.end().ptr());
}
#endif
} // namespace

StdFile::StdFile(Mode mode, std::fstream&& stream) : mode_(mode), file_(BASE_NS::move(stream)) {}

StdFile::~StdFile()
{
    Close();
}

IFile::Mode StdFile::GetMode() const
{
    return mode_;
}

bool StdFile::IsValidPath(const string_view /* path */)
{
    // Path's should always be valid here.
    return true;
}

IFile::Ptr StdFile::Open(const string_view path, Mode mode)
{
#if defined(HAS_FILESYSTEM)
    std::error_code ec;
    auto canonicalPath = std::filesystem::canonical(U8Path(path), ec);
    if (ec) {
        return {};
    }

    if (std::filesystem::is_directory(canonicalPath)) {
        return {};
    }

    if (auto stream = std::fstream(canonicalPath, OpenFileAccessMode(mode)); stream) {
        return IFile::Ptr { BASE_NS::make_unique<StdFile>(mode, BASE_NS::move(stream)).release() };
    }
#else
    char canonicalPath[CORE_MAX_PATH] = { 0 };
    if (realpath(string(path).c_str(), canonicalPath) == nullptr) {
        return {};
    }

    if (auto stream = std::fstream(canonicalPath, OpenFileAccessMode(mode)); stream) {
        return IFile::Ptr { BASE_NS::make_unique<StdFile>(mode, BASE_NS::move(stream)).release() };
    }

#endif
    return {};
}

IFile::Ptr StdFile::Create(const string_view path, Mode mode)
{
    if (path.empty()) {
        return {};
    }

#if defined(HAS_FILESYSTEM)
    std::error_code ec;
    auto canonicalPath = std::filesystem::weakly_canonical(U8Path(path), ec);
    if (ec) {
        return {};
    }
    // Create the file.
    if (auto stream = std::fstream(canonicalPath, CreateFileAccessMode(mode)); stream) {
        return IFile::Ptr { BASE_NS::make_unique<StdFile>(mode, BASE_NS::move(stream)).release() };
    }
#else
    // NOTE: As realpath requires that the path exists, we check that the
    // parent dir is valid instead of the full path of the file being created.
    const string parentDir = StdDirectory::GetDirName(path);
    char canonicalParentPath[CORE_MAX_PATH] = { 0 };
    if (realpath(parentDir.c_str(), canonicalParentPath) == nullptr) {
        return {};
    }
    const string filename = StdDirectory::GetBaseName(path);
    const string fullPath = string_view(canonicalParentPath) + '/' + filename;

    // Create the file.
    if (auto stream = std::fstream(fullPath.data(), CreateFileAccessMode(mode)); stream) {
        return IFile::Ptr { BASE_NS::make_unique<StdFile>(mode, BASE_NS::move(stream)).release() };
    }
#endif
    return {};
}

void StdFile::Close()
{
    if (file_.is_open()) {
        file_ = {};
        mode_ = Mode::INVALID;
    }
}

uint64_t StdFile::Read(void* buffer, uint64_t count)
{
    file_.read(static_cast<char*>(buffer), static_cast<std::streamsize>(count));
    if (file_.eof() && file_.fail()) {
        file_.clear();
    }
    return static_cast<uint64_t>(file_.gcount());
}

uint64_t StdFile::Write(const void* buffer, uint64_t count)
{
    const auto pos = file_.tellp();
    file_.write(static_cast<const char*>(buffer), static_cast<std::streamsize>(count));
    return static_cast<uint64_t>(file_.tellp() - pos);
}

uint64_t StdFile::GetLength() const
{
    const auto offset = file_.tellg();
    if (offset == decltype(file_)::pos_type(-1)) {
        return {};
    }

    const auto end = file_.seekg(0, std::ios_base::end).tellg();
    if (!file_.good()) {
        return {};
    }
    const auto beg = file_.seekg(0, std::ios_base::beg).tellg();
    file_.seekg(offset);
    return static_cast<uint64_t>(end - beg);
}

bool StdFile::Seek(uint64_t offset)
{
    file_.seekg(static_cast<decltype(file_)::off_type>(offset), std::ios_base::beg);
    return file_.good();
}

uint64_t StdFile::GetPosition() const
{
    return static_cast<uint64_t>(file_.tellg());
}
CORE_END_NAMESPACE()
