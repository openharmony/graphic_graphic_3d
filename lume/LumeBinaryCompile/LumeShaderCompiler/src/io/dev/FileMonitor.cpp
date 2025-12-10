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

#include "io/dev/FileMonitor.h"

#include <algorithm>
#include <iostream>
#include <optional>

#ifdef _WIN32
#if __has_include(<filesystem>)
#include <filesystem>
#define HAS_FILESYSTEM 1
#endif
#define USE_WIN32
#include <Windows.h>
#else
#include <dirent.h>
#endif

#include <climits>
#include <sys/stat.h>

namespace ige {
namespace {
void FormatPath(std::string& path, const bool isDirectory)
{
    // Make directory separators consistent.
    std::replace(path.begin(), path.end(), '\\', '/');

    // Ensure there is last separator in place.
    if (!path.empty() && isDirectory) {
        if (path[path.length() - 1] != '/') {
            path += '/';
        }
    }
}

std::string ResolveAbsolutePath(std::string_view path, bool isDirectory)
{
    std::string absolutePath;

#ifdef HAS_FILESYSTEM
    std::error_code error;
    if (auto resolvedPath = std::filesystem::canonical(std::filesystem::u8path(path), error); !error) {
        absolutePath = resolvedPath.u8string();
    }
#elif defined(USE_WIN32)
    char resolvedPath[_MAX_PATH] = {};
    auto ret = GetFullPathNameA(path.data(), sizeof(resolvedPath), resolvedPath, nullptr);
    if (ret < sizeof(resolvedPath)) {
        WIN32_FIND_DATAA data;
        HANDLE handle = FindFirstFileA(resolvedPath, &data);
        if (handle != INVALID_HANDLE_VALUE) {
            absolutePath = resolvedPath;
            FindClose(handle);
        }
    }
#elif defined(__MINGW32__) || defined(__MINGW64__)
    char resolvedPath[_MAX_PATH] = {};
    if (_fullpath(resolvedPath, path.data(), _MAX_PATH) != nullptr) {
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
#else
    char resolvedPath[PATH_MAX];
    if (realpath(path.data(), resolvedPath) != nullptr) {
        absolutePath = resolvedPath;
    }
#endif

    FormatPath(absolutePath, isDirectory);

    return absolutePath;
}

struct Collection {
    std::vector<std::string> files;
    std::vector<std::string> directories;
};

std::optional<Collection> GatherFilesAndDirectories(std::string_view path)
{
    Collection collection;
#ifdef HAS_FILESYSTEM
    std::error_code error;
    for (auto const& entry : std::filesystem::directory_iterator{ std::filesystem::u8path(path), error }) {
        if (entry.is_directory()) {
            collection.directories.push_back(entry.path().filename().u8string());
        } else if (entry.is_regular_file()) {
            collection.files.push_back(entry.path().filename().u8string());
        }
    }
    if (error) {
        return std::nullopt;
    }
#elif defined(USE_WIN32)
    WIN32_FIND_DATAA data;
    HANDLE handle = INVALID_HANDLE_VALUE;
    handle = FindFirstFileA((std::string(path) + "*.*").c_str(), &data);
    if (handle == INVALID_HANDLE_VALUE) {
        // Unable to open directory.
        return std::nullopt;
    }
    do {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) {
            // skip devices.
        } else if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            collection.directories.push_back(data.cFileName);
        } else {
            collection.files.push_back(data.cFileName);
        }
    } while (FindNextFileA(handle, &data));

    FindClose(handle);
#else
    DIR* handle = opendir(path.data());
    if (!handle) {
        // Unable to open directory.
        return std::nullopt;
    }

    struct dirent* directory;
    while ((directory = readdir(handle)) != nullptr) {
        switch (directory->d_type) {
            case DT_DIR:
                collection.directories.push_back(directory->d_name);
                break;

            case DT_REG:
                collection.files.push_back(directory->d_name);
                break;

            default:
                break;
        }
    }

    closedir(handle);
#endif
    return collection;
}

void RecursivelyCollectAllFiles(const std::string_view path, std::vector<std::string>& aFiles) // NOLINT(*-no-recursion)
{
    const auto filesAndDirs = GatherFilesAndDirectories(path);
    if (!filesAndDirs) {
        return;
    }
    // Collect files.
    std::string originalPath;
    for (const auto& filename : filesAndDirs->files) {
        originalPath = path;
        originalPath += filename;
        std::string absoluteFilename = ResolveAbsolutePath(originalPath, false);
        if (!absoluteFilename.empty()) {
            aFiles.push_back(absoluteFilename);
        }
    }

    // Process recursively.
    for (const auto& directoryName : filesAndDirs->directories) {
        if (directoryName != "." && directoryName != "..") {
            originalPath = path;
            originalPath += directoryName;
            std::string absoluteDirectory = ResolveAbsolutePath(originalPath, true);
            if (!absoluteDirectory.empty()) {
                RecursivelyCollectAllFiles(absoluteDirectory, aFiles);
            }
        }
    }
}
} // namespace

bool FileMonitor::AddPath(const std::string_view path)
{
    std::string absolutePath = ResolveAbsolutePath(path, true);
    if (absolutePath.empty() || IsWatchingDirectory(absolutePath) || IsWatchingSubDirectory(absolutePath)) {
        // Already exists or unable to resolve.
        return false;
    }

    std::vector<std::string> files;
    RecursivelyCollectAllFiles(absolutePath, files);

    // Add all files to watch list.
    for (const auto& ref : files) {
        AddFile(ref);
    }

    // Store directory to watch list.
    mDirectories.push_back(std::move(absolutePath));

    return true;
}

bool FileMonitor::RemovePath(const std::string_view path)
{
    const std::string absolutePath = ResolveAbsolutePath(path, true);

    const auto iterator = std::find(mDirectories.begin(), mDirectories.end(), absolutePath);
    if (iterator == mDirectories.end()) {
        return false;
    }

    // Collect all tracked files within this directory.
    std::vector<std::string> files;
    RecursivelyCollectAllFiles(absolutePath, files);

    // Stop tracking of removed files.
    for (const auto& ref : files) {
        // Remove from tracked list.
        RemoveFile(ref);
    }

    // Remove directory from watch list.
    mDirectories.erase(iterator);
    return true;
}

bool FileMonitor::AddFile(std::string_view path)
{
    std::string absolutePath = ResolveAbsolutePath(path, false);
    if (absolutePath.empty() || mFiles.find(absolutePath) != mFiles.end()) {
        // Already exists or unable to resolve.
        return false;
    }
#if HAS_FILESYSTEM
    std::error_code error;
    const auto time = std::filesystem::last_write_time(std::filesystem::u8path(absolutePath), error);
    if (error) {
        return false;
    }
    // Collect file data.
    mFiles[absolutePath].timestamp = time.time_since_epoch().count();
#else
    struct stat ds;
    if (stat(absolutePath.c_str(), &ds) != 0) {
        // Unable to get file stats.
        return false;
    }

    // Collect file data.
    FileInfo info;
    info.timestamp = ds.st_mtime;
    mFiles[absolutePath] = info;
#endif
    return true;
}

bool FileMonitor::RemoveFile(const std::string& path)
{
    const auto iterator = mFiles.find(path);
    if (iterator != mFiles.end()) {
        mFiles.erase(iterator);
        return true;
    }

    return false;
}

bool FileMonitor::IsWatchingDirectory(std::string_view path) const
{
    return std::any_of(mDirectories.begin(), mDirectories.end(), [&path](const auto& ref) {
        return path.find(ref) != std::string::npos; // Already watching this directory or it's parent.
    });
}

bool FileMonitor::IsWatchingSubDirectory(std::string_view path)
{
    return std::any_of(mDirectories.begin(), mDirectories.end(), [&path](const auto& ref) {
        return ref.find(path) != std::string::npos; // Already watching subdirectory of given directory.
    });
}

void FileMonitor::ScanModifications(
    std::vector<std::string>& added, std::vector<std::string>& removed, std::vector<std::string>& modified)
{
    // Collect all files that are under monitoring.
    std::vector<std::string> files;
    for (const auto& ref : mDirectories) {
        RecursivelyCollectAllFiles(ref, files);
    }

    // See which of the files are modified.
    for (const auto& file : files) {
        auto iterator = mFiles.find(file);
        if (iterator != mFiles.end()) {
#if HAS_FILESYSTEM
            const auto u8path = std::filesystem::u8path(file);
            std::error_code error;
            const auto time = std::filesystem::last_write_time(u8path, error).time_since_epoch().count();
            if ((!error) && (time != iterator->second.timestamp)) {
                // This file is modified.
                modified.push_back(file);

                // Store new time.
                iterator->second.timestamp = time;
            }
#else
            // File being watched, see if it is modified.
            struct stat fs;
            if ((stat(file.c_str(), &fs) == 0) && (fs.st_mtime != iterator->second.timestamp)) {
                // This file is modified.
                modified.push_back(file);

                // Store new time.
                iterator->second.timestamp = fs.st_mtime;
            }
#endif
        } else {
            // This is a new file.
            added.push_back(file);
        }
    }

    // See which of the files are removed.
    for (const auto& ref : mFiles) {
        if (std::find(files.begin(), files.end(), ref.first) == files.end()) {
            // This file no longer exists.
            removed.push_back(ref.first);
        }
    }

    // Stop tracking of removed files.
    for (const auto& ref : removed) {
        // Remove from tracked list.
        RemoveFile(ref);
    }

    // Start tracking of new files.
    for (const auto& ref : added) {
        // Add to tracking list.
        AddFile(ref);
    }
}

std::vector<std::string> FileMonitor::GetMonitoredFiles() const
{
    std::vector<std::string> files;
    files.reserve(mFiles.size());

    for (const auto& mFile : mFiles) {
        files.push_back(mFile.first);
    }
    return files;
}
} // namespace ige
