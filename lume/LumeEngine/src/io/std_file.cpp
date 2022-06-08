/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "io/std_file.h"

#include <io/std_directory.h>

#include <core/log.h>
#include <core/namespace.h>

#if defined(_WIN32)
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#define CORE_MAX_PATH MAX_PATH
#else
#include <cerrno>
#include <dirent.h>

#ifndef _DIRENT_HAVE_D_TYPE
#include <sys/stat.h>
#include <sys/types.h>

#endif
#include <climits>
#define CORE_MAX_PATH PATH_MAX
#endif

CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;

namespace {
const char* OpenFileAccessMode(IFile::Mode mode)
{
    switch (mode) {
        case IFile::Mode::INVALID:
            CORE_ASSERT_MSG(false, "Invalid file access mode.");
            return "";
        case IFile::Mode::READ_ONLY:
            return "rb";
        case IFile::Mode::READ_WRITE:
            return "rb+";
        default:
            return "";
    }
}

const char* CreateFileAccessMode(IFile::Mode mode)
{
    switch (mode) {
        case IFile::Mode::INVALID:
        case IFile::Mode::READ_ONLY:
            CORE_ASSERT_MSG(false, "Invalid create file access mode.");
            return "";
        case IFile::Mode::READ_WRITE:
            return "wb+";
        default:
            return "";
    }
}
} // namespace

StdFile::~StdFile()
{
    Close();
}

IFile::Mode StdFile::GetMode() const
{
    return mode_;
}

bool StdFile::IsValidPath(const string_view path)
{
    // Path's should always be valid here.
    return true;
}

bool StdFile::Open(const string_view path, Mode mode)
{
    char canonicalPath[CORE_MAX_PATH] = { 0 };

#if defined(_WIN32)
    if (!PathCanonicalize(canonicalPath, string(path).c_str())) {
        return false;
    }
#else
    if (realpath(string(path).c_str(), canonicalPath) == NULL) {
        return false;
    }
#endif
    if (!IsValidPath(canonicalPath)) {
        return false;
    }
    file_ = fopen(canonicalPath, OpenFileAccessMode(mode));
    if (file_) {
        mode_ = mode;
        return true;
    }

    return false;
}

bool StdFile::Create(const string_view path, Mode mode)
{
    if (path.empty()) {
        return false;
    }

    // NOTE: As realpath requires that the path exists, we check that the
    // parent dir is valid instead of the full path of the file being created.
    const string parentDir = StdDirectory::GetDirName(path);
    char canonicalParentPath[CORE_MAX_PATH] = { 0 };

#if defined(_WIN32)
    if (!PathCanonicalize(canonicalParentPath, parentDir.c_str())) {
        return false;
    }
#else
    if (realpath(parentDir.c_str(), canonicalParentPath) == NULL) {
        return false;
    }
#endif

    const string filename = StdDirectory::GetBaseName(path);
    const string fullPath = string_view(canonicalParentPath) + '/' + filename;
    if (!IsValidPath(fullPath.c_str())) {
        return false;
    }

    // Create the file.
    file_ = fopen(fullPath.c_str(), CreateFileAccessMode(mode));
    if (file_) {
        mode_ = mode;
        return true;
    }

    return false;
}

void StdFile::Close()
{
    if (file_) {
        if (fclose(file_) != 0) {
            CORE_LOG_E("Failed to close file.");
        }

        mode_ = Mode::INVALID;
        file_ = nullptr;
    }
}

uint64_t StdFile::Read(void* buffer, uint64_t count)
{
    return fread(buffer, 1, static_cast<size_t>(count), file_);
}

uint64_t StdFile::Write(const void* buffer, uint64_t count)
{
    return fwrite(buffer, 1, static_cast<size_t>(count), file_);
}

uint64_t StdFile::GetLength() const
{
    const long offset = ftell(file_);
    if (offset == EOF) {
        CORE_ASSERT_MSG(false, "ftell failed, unable to evaluate file length.");
        return {};
    }

    if (fseek(file_, 0, SEEK_END) != 0) {
        // Library implementations are allowed to not meaningfully support SEEK_END.
        CORE_ASSERT_MSG(false, "SEEK_END is not supported, unable to evaluate file length.");
    }

    uint64_t length = 0;
    if (auto const endPosition = ftell(file_); endPosition != EOF) {
        length = static_cast<uint64_t>(endPosition);
    }
    fseek(file_, offset, SEEK_SET);

    return length;
}

bool StdFile::Seek(uint64_t offset)
{
    return fseek(file_, static_cast<long>(offset), SEEK_SET) == 0;
}

uint64_t StdFile::GetPosition() const
{
    return static_cast<uint64_t>(ftell(file_));
}

CORE_END_NAMESPACE()
