/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include <limits.h>
#include <sys/stat.h>

namespace ige {

namespace {

void formatPath(std::string& aPath, bool isDirectory)
{
    size_t length = aPath.length();

    // Make directory separators consistent.
    std::replace(aPath.begin(), aPath.end(), '\\', '/');

    // Ensure there is last separator in place.
    if (aPath.length() > 0 && isDirectory) {
        if (aPath[length - 1] != '/')
            aPath += '/';
    }
}

std::string resolveAbsolutePath(const std::string& aPath, bool isDirectory)
{
    std::string absolutePath;

#ifdef HAS_FILESYSTEM
    std::error_code ec;
    std::filesystem::path path = std::filesystem::canonical(aPath, ec);
    if (ec.value() == 0) {
        absolutePath = path.string();
    }
#elif defined(USE_WIN32)
    char resolvedPath[_MAX_PATH] = {};
    auto ret = GetFullPathNameA(aPath.c_str(), sizeof(resolvedPath), resolvedPath, nullptr);
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
    if (_fullpath(resolvedPath, aPath.c_str(), _MAX_PATH) != nullptr) {
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
    if (realpath(aPath.c_str(), resolvedPath) != nullptr) {
        absolutePath = resolvedPath;
    }
#endif

    formatPath(absolutePath, isDirectory);

    return absolutePath;
}

void recursivelyCollectAllFiles(const std::string& aPath, std::vector<std::string>& aFiles)
{
    std::vector<std::string> files;
    std::vector<std::string> directories;
#if defined(USE_WIN32)
    WIN32_FIND_DATAA data;
    HANDLE handle = INVALID_HANDLE_VALUE;
    handle = FindFirstFileA((aPath + "*.*").c_str(), &data);
    if (handle == INVALID_HANDLE_VALUE) {
        // Unable to open directory.
        return;
    }
    do {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) {
            // skip devices.
        } else if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            directories.push_back(data.cFileName);
        } else {
            files.push_back(data.cFileName);
        }
    } while (FindNextFileA(handle, &data));

    FindClose(handle);
#else
    DIR* handle = opendir(aPath.c_str());
    if (!handle) {
        // Unable to open directory.
        return;
    }

    struct dirent* directory;
    while ((directory = readdir(handle)) != nullptr) {
        switch (directory->d_type) {
            case DT_DIR:
                directories.push_back(directory->d_name);
                break;

            case DT_REG:
                files.push_back(directory->d_name);
                break;

            default:
                break;
        }
    }

    closedir(handle);
#endif

    // Collect files.
    for (const auto& filename : files) {
        std::string absoluteFilename = resolveAbsolutePath(aPath + filename, false);
        if (!absoluteFilename.empty()) {
            aFiles.push_back(absoluteFilename);
        }
    }

    // Process recursively.
    for (const auto& directoryName : directories) {
        if (directoryName != "." && directoryName != "..") {
            std::string absoluteDirectory = resolveAbsolutePath(aPath + directoryName, true);
            if (!absoluteDirectory.empty()) {
                recursivelyCollectAllFiles(absoluteDirectory, aFiles);
            }
        }
    }
}

} // namespace

FileMonitor::FileMonitor() {}

FileMonitor::~FileMonitor() {}

bool FileMonitor::addPath(const std::string& aPath)
{
    std::string absolutePath = resolveAbsolutePath(aPath, true);

    if (absolutePath.empty() || isWatchingDirectory(absolutePath) || isWatchingSubDirectory(absolutePath)) {
        // Already exists or unable to resolve.
        return false;
    }

    std::vector<std::string> files;
    recursivelyCollectAllFiles(absolutePath, files);

    // Add all files to watch list.
    for (const auto& ref : files) {
        addFile(ref);
    }

    // Store directory to watch list.
    mDirectories.push_back(absolutePath);

    return true;
}

bool FileMonitor::removePath(const std::string& aPath)
{
    std::string absolutePath = resolveAbsolutePath(aPath, true);

    std::vector<std::string>::iterator iterator = std::find(mDirectories.begin(), mDirectories.end(), absolutePath);
    if (iterator != mDirectories.end()) {
        // Collect all tracked files within this directory.
        std::vector<std::string> files;
        recursivelyCollectAllFiles(absolutePath, files);

        // Stop tracking of removed files.
        for (const auto& ref : files) {
            // Remove from tracked list.
            removeFile(ref);
        }

        // Remove directory from watch list.
        mDirectories.erase(iterator);
        return true;
    }

    return false;
}

bool FileMonitor::addFile(const std::string& aPath)
{
    std::string absolutePath = resolveAbsolutePath(aPath, false);
    if (absolutePath.empty() || mFiles.find(absolutePath) != mFiles.end()) {
        // Already exists or unable to resolve.
        return false;
    }

    struct stat ds;
    if (stat(absolutePath.c_str(), &ds) != 0) {
        // Unable to get file stats.
        return false;
    }

    // Collect file data.
    FileInfo info;
    info.timestamp = ds.st_mtime;
    mFiles[absolutePath] = info;

    return true;
}

bool FileMonitor::removeFile(const std::string& aPath)
{
    std::map<std::string, FileInfo>::iterator iterator = mFiles.find(aPath);
    if (iterator != mFiles.end()) {
        mFiles.erase(iterator);
        return true;
    }

    return false;
}

bool FileMonitor::isWatchingDirectory(const std::string& aPath)
{
    for (const auto& ref : mDirectories) {
        if (aPath.find(ref) != std::string::npos) {
            // Already watching this directory or it's parent.
            return true;
        }
    }

    return false;
}

bool FileMonitor::isWatchingSubDirectory(const std::string& aPath)
{
    for (const auto& ref : mDirectories) {
        if (ref.find(aPath) != std::string::npos) {
            // Already watching subdirectory of given directory.
            return true;
        }
    }

    return false;
}

void FileMonitor::scanModifications(
    std::vector<std::string>& aAdded, std::vector<std::string>& aRemoved, std::vector<std::string>& aModified)
{
    // Collect all files that are under monitoring.
    std::vector<std::string> files;
    for (const auto& ref : mDirectories) {
        recursivelyCollectAllFiles(ref, files);
    }

    // See which of the files are modified.
    for (std::vector<std::string>::iterator it = files.begin(); it != files.end(); ++it) {
        std::map<std::string, FileInfo>::iterator iterator = mFiles.find(*it);
        if (iterator != mFiles.end()) {
            // File being watched, see if it is modified.
            struct stat fs;
            if (stat((*it).c_str(), &fs) == 0) {
                if (fs.st_mtime != iterator->second.timestamp) {
                    // This file is modified.
                    aModified.push_back(*it);

                    // Store new time.
                    iterator->second.timestamp = fs.st_mtime;
                }
            }
        } else {
            // This is a new file.
            aAdded.push_back(*it);
        }
    }

    // See which of the files are removed.
    for (const auto& ref : mFiles) {
        if (std::find(files.begin(), files.end(), ref.first) == files.end()) {
            // This file no longer exists.
            aRemoved.push_back(ref.first);
        }
    }

    // Stop tracking of removed files.
    for (const auto& ref : aRemoved) {
        // Remove from tracked list.
        removeFile(ref);
    }

    // Start tracking of new files.
    for (const auto& ref : aAdded) {
        // Add to tracking list.
        addFile(ref);
    }
}

std::vector<std::string> FileMonitor::getMonitoredFiles() const
{
    std::vector<std::string> files;
    files.reserve(mFiles.size());

    for (std::map<std::string, FileInfo>::const_iterator it = mFiles.begin(), end = mFiles.end(); it != end; ++it) {
        files.push_back(it->first);
    }
    return files;
}

} // namespace ige
