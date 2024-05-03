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

#include "io/dev/file_monitor.h"

#include <cstddef>

#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file_manager.h>
#include <core/namespace.h>
#include <core/perf/cpu_perf_scope.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::vector;

void FileMonitor::RecursivelyCollectAllFiles(string& path)
{
    auto dir = fileManager_.OpenDirectory(path);
    if (dir == nullptr) {
        // path does not exist.
        return;
    }
    const auto& entries = dir->GetEntries();
    const size_t oldLength = path.length();
    for (const IDirectory::Entry& entry : entries) {
        if (entry.name == "." || entry.name == "..") {
            continue;
        }

        path.reserve(oldLength + entry.name.length() + 1);
        path += entry.name;

        if (entry.type == IDirectory::Entry::DIRECTORY) {
            path += '/';
        }

        auto iterator = files_.find(path);
        if (iterator != files_.end()) {
            // File or directory being tracked, see if it is modified.
            if (entry.timestamp == iterator->second.timestamp) {
                iterator->second.state = FileInfo::NOCHANGE;
            } else {
                iterator->second.timestamp = entry.timestamp;
                iterator->second.state = FileInfo::MODIFIED;
            }
        } else {
            // This is a new file or directory, start tracking it.
            files_.insert({ path, { entry.timestamp, FileInfo::ADDED } });
        }
        if (entry.type == IDirectory::Entry::DIRECTORY) {
            RecursivelyCollectAllFiles(path);
        }
        path.resize(oldLength);
    }
}
FileMonitor::FileMonitor(IFileManager& manager) : fileManager_(manager) {}
void FileMonitor::CleanPath(const string_view inPath, string& path)
{
    // cleanup slashes. (partial sanitation, path needs to end with '/' and slashes must be '/' (not '\\')
    if ((inPath.back() != '/') && (inPath.back() != '\\')) {
        path.reserve(inPath.size() + 1);
        path = inPath;
        path += '/';
    } else {
        path = inPath;
    }
    for (auto& c : path) {
        if (c == '\\') {
            c = '/';
        }
    }
}

bool FileMonitor::AddPath(const string_view inPath)
{
    string path;
    CleanPath(inPath, path);
    if (IsWatchingDirectory(path) || IsWatchingSubDirectory(path)) {
        // Already exists or unable to resolve.
        return false;
    }
    // Collect information for files in path.
    RecursivelyCollectAllFiles(path);
    if (path.capacity() > pathTmp_.capacity()) {
        pathTmp_.reserve(path.capacity());
    }
    // Update state.
    for (auto& ref : files_) {
        ref.second.state = FileInfo::REMOVED;
    }
    // Store directory to watch list.
    directories_.push_back(path);
    return true;
}

bool FileMonitor::RemovePath(const string_view inPath)
{
    string path;
    CleanPath(inPath, path);
    auto iterator = directories_.cbegin();
    for (; iterator != directories_.cend(); ++iterator) {
        if (*iterator == path) {
            // scan through tracked files, and remove the ones that start with "path"
            for (auto it = files_.begin(); it != files_.end();) {
                const auto pos = it->first.find(path);
                if (pos == 0) {
                    it = files_.erase(it);
                } else {
                    ++it;
                }
            }
            // Remove directory from watch list.
            directories_.erase(iterator);
            return true;
        }
    }
    return false;
}

bool FileMonitor::IsWatchingDirectory(const string_view inPath)
{
    string path;
    CleanPath(inPath, path);
    for (const auto& ref : directories_) {
        if (path.find(ref) != string_view::npos) {
            // Already watching this directory or it's parent.
            return true;
        }
    }
    return false;
}

bool FileMonitor::IsWatchingSubDirectory(const string_view inPath)
{
    string path;
    CleanPath(inPath, path);
    for (const auto& ref : directories_) {
        if (ref.find(path) != string_view::npos) {
            // Already watching subdirectory of given directory.
            return true;
        }
    }
    return false;
}

void FileMonitor::ScanModifications(vector<string>& added, vector<string>& removed, vector<string>& modified)
{
    CORE_CPU_PERF_SCOPE("Other", "FileMonitor", "ScanModifications()");
    // Collect all files that are under monitoring.
    for (const auto& ref : directories_) {
        pathTmp_ = ref;
        RecursivelyCollectAllFiles(pathTmp_);
    }

    // See which of the files are removed.
    for (auto it = files_.begin(); it != files_.end();) {
        if (it->second.state == FileInfo::REMOVED) {
            removed.push_back(it->first);
        } else if (it->second.state == FileInfo::MODIFIED) {
            modified.push_back(it->first);
        } else if (it->second.state == FileInfo::ADDED) {
            added.push_back(it->first);
        }
        if (it->second.state != FileInfo::REMOVED) {
            // default state is removed.
            it->second.state = FileInfo::REMOVED;
            ++it;
        } else {
            it = files_.erase(it);
        }
    }
}

vector<string> FileMonitor::GetMonitoredFiles() const
{
    vector<string> filesRes;
    filesRes.reserve(files_.size());
    for (auto& f : files_) {
        filesRes.push_back(f.first);
    }
    return filesRes;
}

CORE_END_NAMESPACE()
