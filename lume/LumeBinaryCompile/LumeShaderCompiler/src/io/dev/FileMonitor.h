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

#ifndef IGE_FILEMONITOR_H
#define IGE_FILEMONITOR_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ige {
class FileMonitor {
public:
    /** Adds path to watch list, the monitor will recursively monitor all files in this directory and it's subtree.
        @param path Path to directory that is being monitored, such as 'x:/images/' or './images'.
        @return True if path is succesfully added to watch list, otherwise false.
    */
    bool AddPath(std::string_view path);

    /** Removes path from watch list, the monitor will no longer watch files in this directory or it's subtree.
        @param path Path to directory to be no longer monitored.
        @return True if the watch is successfully removed, otherwise false.
    */
    bool RemovePath(std::string_view path);

    /** Scans for file modifications since last call to this function.
        @param aAdded List of files that were added.
        @param aRemoved List of files that were removed.
        @param aModified List of files that were modified.
    */
    void ScanModifications(
        std::vector<std::string>& added, std::vector<std::string>& removed, std::vector<std::string>& modified);

    std::vector<std::string> GetMonitoredFiles() const;

private:
    struct FileInfo {
        time_t timestamp;
    };

    bool AddFile(std::string_view path);
    bool RemoveFile(const std::string& path);

    bool IsWatchingDirectory(std::string_view path);
    bool IsWatchingSubDirectory(std::string_view path);

    std::vector<std::string> mDirectories;
    std::unordered_map<std::string, FileInfo> mFiles;
};

} // namespace ige

#endif // IGE_FILEMONITOR_H